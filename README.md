# Game Capture HD userspace driver

This fork modernizes the unofficial MIT-licensed libusb driver for the **original
Elgato Game Capture HD** (VID `0fd9`, PIDs `0044`, `004e`, `0051`, `005d`).
HD60, HD60 S, and HD60 Pro are unsupported. A 30-second CLI recording was
verified on the original `0fd9:0044` card under WSL 2: H.264 720×480 video,
stereo 48 kHz MP2 audio, and zero reported dropped buffers or USB overflows.
**A known WSL/USB-IP handoff issue requires a cold start while force-bound** on
the tested setup; see the [tested startup procedure](docs/WSL2.md#known-wslusb-ip-transferhandoff-issue).
The protocol remains experimental. Ctrl+C/restart, wrapper recording and
USB/IP-disconnect handling were tested. Physical unplug during streaming and long-duration
reliability remain unverified; the user requested short recording checks.
See the [phase report](docs/MODERNIZATION.md) and [follow-up software audit](docs/AUDIT.md).

## Build and install

On Ubuntu 24.04 (use the Linux filesystem inside WSL for Git and builds):

```sh
sudo apt update
sudo apt install build-essential cmake pkg-config libusb-1.0-0-dev libssl-dev python3 p7zip-full curl ffmpeg
git clone --branch modernize https://github.com/ShaneKent/elgato-gchd.git
cd elgato-gchd
cmake -S . -B build -DGCHD_WERROR=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Binary: `build/src/gchd`. GCC and Clang CI builds use `-Wall -Wextra -Werror`,
with a Clang ASan/UBSan job and cppcheck correctness/portability analysis.
Use `-DGCHD_SANITIZE=ON` for local sanitizers. The unfinished Qt GUI is not built.
Optional system installation defaults to `/usr/local`:

```sh
sudo cmake --install build
```

This installs the binary, recording wrapper, docs, and udev rule. Override
`CMAKE_INSTALL_PREFIX` and/or `GCHD_UDEV_RULES_DIR` at configure time if needed.

## Firmware from official Elgato installers

Firmware is proprietary and is neither distributed nor committed here. The
extractor accepts these official versions (or exact local copies), verifies
installer SHA-256, extracts with **7z**, then checks all four firmware lengths
and SHA-256 hashes before installing. No installer is executed.

- [Mac 2.0.3 DMG](https://edge.elgato.com/egc/macos/egcm/2.0.3/final/gchdm_203_970.dmg)
- [Windows 3.20 MSI](https://edge.elgato.com/egc/windows/egcw/3.20/final/GameCaptureSetup_3.20.33.1533_x64.msi)

For a local, non-root installation:

```sh
scripts/get-firmware.sh https://edge.elgato.com/egc/macos/egcm/2.0.3/final/gchdm_203_970.dmg "$HOME/.local/share/gchd/firmware"
export GCHD_FIRMWARE_DIR="$HOME/.local/share/gchd/firmware"
```

Or use an already-downloaded approved `.dmg` or `.msi` as the first argument.
The optional second argument is the install directory; the default is
`/usr/local/lib/firmware/gchd` (requires write access). Both installers have been
tested end-to-end using 7z. The optional dmg2img fallback is unverified; some
DMGs fail in dmg2img despite being valid, so prefer 7z.

Search priority: `--firmware-dir DIR`, then `GCHD_FIRMWARE_DIR`, then
`/usr/lib/firmware/gchd`, `/usr/local/lib/firmware/gchd`, and the macOS app path.
An override is exclusive. The implicit current-directory search was removed;
use `--firmware-dir .` if intentional. Uppercase Windows and lowercase Mac
filenames work. Unknown firmware versions fail validation; update verified
records from an official installer instead of bypassing the check. The eight
versioned records are in [firmware-hashes.json](docs/firmware-hashes.json).

## Record from the command line

```sh
# Native transport stream file; duration starts after device initialization.
build/src/gchd --duration 90s capture.ts

# Pipe to ffmpeg; choose a container compatible with the source codecs.
set -o pipefail
build/src/gchd --duration 10m - | ffmpeg -i - -c copy capture.mkv

# MP4: preserve H.264 video and convert MPEG audio to AAC if needed.
build/src/gchd --duration 10m -o - | ffmpeg -i - -c:v copy -c:a aac capture.mp4

# Wrapper: handles child lifecycle, signal forwarding and container finalization.
scripts/gchd-record.sh capture.mp4 10m
```

The wrapper refuses existing destinations, copies streams for TS/MKV, and uses
AAC audio for MP4/MOV. It requires Python 3 and ffmpeg. `GCHD_BIN` can select a
different binary. `GCHD_COLOR_SPACE=rgb`, `yuv`, or `auto` selects an explicit
input color space for the wrapper; an unset variable preserves driver defaults.
SIGINT/SIGTERM/SIGHUP are forwarded to the driver as SIGINT;
ffmpeg receives EOF and finishes its container. An interrupted wrapper returns
`128 + signal`. A driver or ffmpeg failure returns nonzero. Direct disk mode
retains the historical behavior of truncating its destination.

For the tested 480p RGB source, automatic color detection selected YUV and
produced pink whites. Explicit RGB matched the official Windows reference:

```sh
build/src/gchd -i hdmi -c rgb --duration 60s capture.ts
GCHD_COLOR_SPACE=rgb scripts/gchd-record.sh capture.mp4 60s
```

The color setting programs the card's input processing. Keep cables stable
when comparing colors; a successful USB stream does not establish correct colors
or an uninterrupted HDMI source. Use YUV when that is the source's actual format.

Existing single-dash flags remain accepted. Use `gchd --help` or `--full-help`:
`-i hdmi|component|composite|auto`, `-c yuv|rgb|auto`, `-or 1280x720`, `-br 10`,
`-abr 320`, `-hp high`, `-hl 4.1`. Frame-rate and PID-file flags remain disabled
and are hidden in help. Input detection/mode changes are unchanged from upstream.

`--duration` accepts positive seconds or `s`, `m`, `h` suffixes, including
fractions (maximum one year). Binary stdout is selected by `-`, `-o -`, or
`-of stdout`; all logs and five-second/final statistics go to stderr. Statistics
include USB/output bytes, bitrate, queue drops, USB overflows and timeouts.

With no destination the driver creates a FIFO at `$XDG_RUNTIME_DIR/gchd.ts`
when that directory is private and owned by you; otherwise it uses
`/tmp/gchd-UID/gchd.ts` in a private directory. This replaces upstream's
`/tmp/gchd.ts`. The chosen path is printed. For a fixed path, explicitly use
`-of fifo /path/to/stream.ts`; an existing explicit path is refused and preserved.
FIFOs are mode `0600`, protected by a sibling advisory `.lock` file. Safe stale
default FIFOs can be reused after a crash. The `.lock` file intentionally remains.

UDP output is `-of socket 127.0.0.1:57384` (IPv6: `[::1]:57384`). It connects to
a destination rather than binding a server. It remains experimental and keeps
one UDP datagram per USB buffer, which may fragment at the network MTU.

The writer queue is bounded at 256 USB buffers (4 MiB). Slow consumers cause
counted dropped buffers rather than blocking USB reads. Drops/overflows can
produce corrupt or missing frames; zero drops does not prove valid video. At
shutdown, queued output gets two seconds to drain, then pipe/socket writes are
cancelled. A regular-file write stuck inside the kernel remains an OS limitation.

## Run without root

The installed udev rule appends `uaccess` for a logged-in seat and retains a
`video` group fallback with mode `0660`. If your distribution does not read the
installed rule directory, copy it to `/etc/udev/rules.d`:

```sh
sudo install -m 0644 udev-rules/55-elgato-game-capture.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=usb --attr-match=idVendor=0fd9
```

A new login may be needed after joining `video`. Desktop seat ACLs may be absent
in WSL; use the [WSL permission instructions](docs/WSL2.md). Do not run a pipeline's
ffmpeg as root just to access USB.

## Stop and recover

Press Ctrl+C once and wait for cleanup. A second stop signal forces exit and
skips remaining cleanup. Disconnects and fatal USB errors return nonzero with a
libusb error name. Control/interrupt/firmware timeouts are one second; hardware
polls have finite budgets. Cleanup is best-effort, non-throwing, and bounded.
A failed partial initialization does not run the complete shutdown sequence.

`gchd --reset` requests a USB reset and exits, without requiring firmware. It may
re-enumerate the device and require a new usbipd attach in WSL. Recovery from a
locked device via this option is **not established**: the reset call succeeded
in testing but did not restore firmware readiness. For the observed WSL handoff
issue, follow the [cold-start procedure](docs/WSL2.md#known-wslusb-ip-transferhandoff-issue)
instead of repeatedly retrying USB reset or firmware upload.

## License and future work

[MIT](LICENSE); original notices and [credits](CREDITS.md) retained. No firmware
license is implied. This is unofficial and unsupported by Elgato.

Deferred: V4L2/v4l2loopback, multi-card support, merging upstream `dev`, analog
fixes, HD60/HD60 S, mode detection improvements, and async USB bulk-IN queues.
Async 4–8-transfer streaming should follow measured USB/IP loss and real-device
validation; it is not enabled speculatively.
