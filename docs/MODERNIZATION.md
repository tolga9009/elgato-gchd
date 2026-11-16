# Modernization report

Branch: `modernize`, based on upstream `e5bc6b9`.

## Phase 0 — setup

Isolated Linux checkout: `/home/shane/elgato-gchd-modernize`. The shared
Windows drive rejects Git chmod operations. Origin is ShaneKent/elgato-gchd;
upstream pushes are disabled. Reference remotes: ff5, eddyv73, upstream/dev.
Ubuntu 24.04, WSL 2 kernel 6.18.58.1, GCC 13.3, Clang 18.1, CMake 3.28,
libusb 1.0.27. Original Elgato 0fd9:0044 enumerated through lsusb.
Firmware binaries and downloaded installers are excluded from Git.

Hardware enumeration alone does not establish working firmware or capture.

## Phase 1 — build and CI

GCC 13 and Clang 18 strict builds pass, including Clang ASan/UBSan.
Regression suites cover CLI errors, resolution and address parsing, PAT/PMT/SIT
and descriptor generation. Constructor order, missing cstdint (credit PR #55),
ptr_fun, uninitialized state/descriptor fields, polymorphic destructors fixed.
CMake uses imported libusb, target options, install rules, and CTest.
Compared eddyv73 CMake (MIT); no fork code copied. CI gates correctness and
portability cppcheck diagnostics; performance suggestions are outside this gate.

## Phase 2 — USB robustness

Defects 1–4 fixed: initialized and checked streaming lengths/status, partial
timeout data retained, overflow data discarded, fatal errors propagated.
Explicit libusb context, auto detach/reattach, interface ownership tracked.
Already-active USB configurations are preserved.
Only successful configuration sets initialized; partial configuration does not
attempt the full shutdown protocol. Cleanup never throws, skips removed devices,
and has a five-second budget plus at most one in-flight transfer.
Control, interrupt and firmware transfers use 1000 ms timeouts; polling loops
check cancellation with 15-second/2000-attempt limits. Firmware I/O and every
upload chunk are checked (no spurious zero-length final transfer).
Additive --reset performs a USB reset and exits; WSL may need reattach.
Mock fault tests cover timeout, overflow, I/O, disconnect, short/missing firmware,
interruption and non-throwing destruction. Actual upload-timeout and initialization
interruption behavior were subsequently tested; see Phase 8.

## Phase 3 — firmware

Added --firmware-dir (highest priority), GCHD_FIRMWARE_DIR, then system/Mac
paths. Explicit overrides are exclusive, and implicit current-directory search
is removed. Runtime verifies SHA-256 and exact length before each upload, on
the same open file. OpenSSL Crypto is now a build dependency.

Both approved official installers were downloaded over HTTPS and extracted
with 7z successfully. Eight versioned size/hash records are in
[firmware-hashes.json](firmware-hashes.json); H57/H58 files match across both
installers, M01 files differ. get-firmware.sh pins installer hashes, accepts
only the approved URLs or their exact local installers, verifies all four
files before installation, and was tested end-to-end for DMG and MSI.
The optional dmg2img fallback is untested. No firmware/installer is in Git.

## Phase 4 — output safety

Defects 5–11 fixed. Explicit existing FIFO/file/symlink paths are preserved on
creation failure; only the created inode is unlinked. Default FIFO uses a
validated private XDG_RUNTIME_DIR or /tmp/gchd-UID directory with mode 0600,
O_NOFOLLOW and an advisory lock. Safe stale default FIFOs can be reused.
Writes handle partial writes/EINTR/EAGAIN; EPIPE, disk-full and UDP errors
propagate. UDP datagrams are never split on short writes. Socket connection
tries all addresses, frees addrinfo, and defaults to loopback as a destination.
Signal handlers only set sig_atomic_t; SIGHUP included, second signal _exit.
SIGPIPE disposition and masks are initialized. PID fd sentinel corrected.

A 256-buffer queue (at most 4 MiB plus in-flight buffers) and writer thread
separate synchronous USB reads from output latency. Saturation drops newest
buffers and counts losses; shutdown allows two seconds to drain before
cancelling nonblocking pipe/socket writes. A blocked kernel write to a regular
file on an unresponsive filesystem cannot be forcibly cancelled by this design.
Tests verify preserved files/symlinks, FIFO modes/cleanup, 1 MiB partial pipe
writes, EPIPE, /dev/full, bounded queue saturation/failure and SIGHUP.

## Phase 5 — CLI recording

Added gchd -, -o -, and -of stdout; only the selected sink is written.
Stdout is binary-only, rejects terminals, and restores pipe flags on close.
--duration accepts positive seconds or s/m/h suffixes and counts time after
initialization. Five-second stderr stats and final stats report USB/output
bytes, bitrate, dropped buffers/bytes, USB overflows and timeouts.
Help reflects runtime FIFO paths, hides disabled PID/frame-rate flags, and
adds input-no-stretch while preserving no-input-stretch. Numeric parsing
rejects non-finite/overflow values before unsafe casts. A related H.264 level
conversion swapped modf outputs; level 4.1 now converts to 41 correctly.
Tests include stdout byte fidelity and timed mocked streaming.

## Phase 6 — non-root use

Defect 12 fixed: udev appends elgato_gchd and uaccess tags, retaining the video
group/mode 0660 fallback. CMake installs the rule in an architecture-independent
lib/udev/rules.d directory; GCHD_UDEV_RULES_DIR can select /etc/udev/rules.d.
The video-group rule was installed and triggered by the user on WSL, and
non-root access persisted across subsequent reattachments. Desktop uaccess
seat ACLs remain unverified. A manual permission fallback is documented for
WSL environments without udev.

## Phase 7 — documentation and recording wrapper

README and WSL2.md document build, official firmware extraction, non-root use,
new FIFO location, stdout/ffmpeg recording, reset and attach behavior. Windows
commands are separated into one-time admin bind and recurring non-admin attach.
Windows-side diagnostics and attachment changes were subsequently executed with
the user's explicit approval; see Phase 8 and USB-TRACING.md.

gchd-record.sh forwards stop signals as SIGINT to the driver, waits for EOF and
ffmpeg container finalization, refuses existing output, and checks both child
exit statuses. TS/MKV use stream copy; MP4/MOV preserve video and encode AAC audio.
Synthetic ffmpeg H.264/MPEG-audio fixtures verified MKV stream copy, MP4/AAC
output, stop forwarding and finalization, overwrite refusal, and cleanup after
a forced ffmpeg error. This is not a hardware capture test.

## Phase 8 — hardware testing: recording not yet working

Original card: 0fd9:0044, revision V1100090. User reports HDMI around 480p and
confirmed video/audio preview in the official Windows app. Firmware from the
approved Mac 2.0.3 installer passed size/hash validation before each upload.
Tests ran as the regular Linux user after granting video-group access.

| Test | Observed result |
| --- | --- |
| First guarded capture | Firmware upload timed out; exit 1 in 1.03 s, zero stream bytes |
| Warm retry | Device-state wait expired; exit 1 in 9.13 s, zero bytes |
| gchd --reset | Returned 0 in 0.28 s; device remained attached |
| Capture after reset | State wait still failed; reset did not restore firmware readiness |
| Physical power cycle, diagnostic capture | First idle-firmware chunk: offset 0, requested 16384, transferred 0; timeout and exit 1 in 1.14 s |
| SIGINT during real initialization | Clean interrupted-error exit 1, approximately 4 ms after SIGINT; no abort |
| Official Windows preview then close/reattach | First idle-firmware chunk still timed out; zero stream bytes |
| Preserve already-active USB configuration | Mock tests pass; real warm-start comparison still failed at first firmware chunk |
| Persistent udev rule | root:video 0660 and regular-user USB access observed after reattachments |
| Graceful Windows handoff, later test | First B8 command returned LIBUSB_ERROR_PIPE; exit 1, zero stream bytes |
| Native versus WSL ETW headers | Matching initial control headers; native B8 success versus WSL USB STALL |
| Broader native ETW startup | 9,751 successful control completions; command/register payloads were not emitted |

No successful firmware upload, valid 30-second recording, or real stream has
been observed. Streaming Ctrl+C/restart, unplug during streaming, loss counts
and 30+ minute USB/IP recording remain unverified. Partial upload errors can
leave the card waiting for a state transition; USB reset did not recover that
condition in this test. A reliable recovery guarantee has not been achieved.

Added focused diagnostics for firmware offsets/lengths and named hardware
waits. USB setup now preserves an already-active configuration, avoiding the
lightweight reset documented by libusb; this did not solve the observed upload
failure. Mock tests cover configuration selection and deadline/attempt bounds.

Sanitized results: [hardware-results-2026-11-11.json](hardware-results-2026-11-11.json).
Full logs remain locally under artifacts/ and are excluded from Git. The other
agent's upstream-only logs in the shared workspace were preserved unchanged.

The failure is before HDMI detection or bulk-IN streaming. There is no evidence
that input resolution or async receive queues would fix it. The current evidence
does not distinguish an unofficial-driver boot-protocol mismatch from USB/IP
transport behavior. Native Windows and WSL startup headers have now been
compared using built-in ETW, with explicit approval for Windows commands, UAC, app startup/shutdown and
attachment changes. The first nine control headers match. The first B8 command
succeeds natively but returns USB STALL under WSL after a graceful handoff.
A later native warm-start trace again shows successful B8 commands. This does
not establish whether command contents, device state or USB/IP is responsible.

After the user's request to collect broader startup information, control data
logging was enabled across UCX and the host-controller provider. The final
20-second native trace yielded 20,010 selected-card transfer events, including
9,751 successful control completions, but no payload-data events. The exact
six-byte payload filter was removed for this broader check. Thus the missing
payloads cannot be attributed solely to that narrow filter. Register values
and command contents remain unavailable from these ETW traces.

During the ETW experiments no USBPcap driver was installed and Windows was
not restarted. Repeating the
same ETW capture is not a useful next step. A payload-capable native capture or
an independent native-Linux hardware comparison is needed before changing the
boot protocol based on assumptions. See [USB-TRACING.md](USB-TRACING.md) for the
trace settings, verified limits and next-capture scope.

On 2026-11-16 the user approved USBPcap installation. Official version 1.5.4.0
installed with exit 0; installer and driver signatures are valid. Capture
interfaces were initially unavailable. At the user's request, starting the
USBPcap service without reboot succeeded, but elevated interface enumeration
still returned no interfaces. No Windows, USB hub or controller restart has
been initiated. usbipd-win now reports its known USBPcap filter incompatibility;
no forced binding has been attempted. The next step is the full native Windows
trace after restart, followed by a separately reviewed resolution of the filter
conflict before the WSL comparison. This is diagnostic setup, not a successful
capture or a driver protocol fix.

## Defect and verification summary

| Review defects | Result | Evidence |
| --- | --- | --- |
| 1–4 USB reads, cleanup, infinite waits, firmware I/O | Fixed in software | USB fault mocks, ASan/UBSan, static review |
| 5–6 FIFO ownership/default path | Fixed | Existing-file/symlink preservation, private mode/cleanup tests |
| 7–8 writes/backpressure | Fixed | Partial writes, EPIPE, /dev/full, queue saturation and bounded cancellation tests |
| 9 socket connection loop | Fixed | Reviewed address iteration and RAII cleanup; real network streaming unverified |
| 10–11 signals | Fixed | Initialized dispositions, SIGHUP and second-signal subprocess tests |
| 12 udev tags | Fixed | udevadm verify; real video-group access after reattachments; seat ACLs unverified |

Additional fixes: explicit libusb context and automatic kernel-driver reattach;
firmware override/hash checks and no implicit cwd search; resolution/numeric
validation; matching CLI help and alias; PID descriptor sentinel.

Deferred/out of scope: V4L2/v4l2loopback, multi-card support, upstream dev merge,
analog fixes, HD60/HD60 S, mode detection, source changes on the fly, and
sub-480i inputs. The large protocol configuration class remains for later work.
