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

## Phase 8 — hardware testing

A cold-start CLI recording is now verified on the original `0fd9:0044` card,
revision `V11050900`, Ubuntu 24.04 / WSL 2 kernel 6.18.58.1 and usbipd 5.3.0.
The native trace exposed a revision-formatting bug: the final digit overwrote
offset 4 instead of offset 8. This is fixed with tests for both register layouts.
The original SD source was detected as HDMI 720×480 progressive, YUV, 4:3.

| Test | Observed result |
| --- | --- |
| Earlier firmware upload / warm handoff attempts | Upload timeouts, invalid state or USB STALL; nonzero exit, zero stream bytes |
| Initialization SIGINT | Prompt interrupted-error exit without abort |
| Native Windows USBPcap baseline | Valid H.264 720×480 and stereo 48 kHz MP2; encoder hash matches approved firmware |
| Native driver rebind trace | Idle and two encoder uploads match approved hashes; no video payload in that trace |
| Warm forced WSL attach | State register `0xffff`; bounded wait failed after 4.728 s, exit 1 |
| USB reset on warm forced attachment | Reset returned 0 in 0.268 s; next initialization stalled, exit 1 |
| Power cycle while already force-bound, then WSL attach | Initial state `0x0000`; both firmware uploads succeeded without re-enumeration |
| 30-second CLI capture after cold start | Exit 0 in 36.750 s including setup/shutdown; 96,468,992 bytes; valid video and audio |
| Capture stats | 25.7094 Mbps, 0 dropped buffers, 0 USB overflows, 1 timeout |
| udev access | Regular-user read/write access after attachment; no root pipeline needed |

The confirmed workaround and remaining uncertainty are documented as a
[known WSL/USB-IP transfer/handoff issue](WSL2.md#known-wslusb-ip-transferhandoff-issue).
The failing handoff and working cold start differ in device startup state;
the underlying component responsible has not been isolated. Per the user's
request, further root-cause tracing is deferred. No speculative firmware or
async streaming change was made. Recovery through USB reset is not established.

The requested 30-second streaming interval produced a TS container duration of
32.733 s. The reason for this timing difference has not been established.
ffprobe found H.264
720×480 video and stereo MP2 audio at 48 kHz. The first 30 seconds also decode without diagnostics and no TS continuity,
sync or transport-error indicators were found. The raw file ends with a partial
176-byte TS packet; full-file decoding reports one H.264 warning beyond the
clean first 30 seconds. Zero reported queue drops alone does not establish
stream integrity. Ctrl+C/restart, live unplug recovery,
wrapper-on-hardware and 30+ minute recording remained to be tested at that point;
subsequent results follow below.

USBPcap 1.5.4.0 was installed with explicit approval; the user rebooted Windows.
Native traces include proprietary firmware and remain in ignored artifacts/.
The tested summary exporter refuses missing VID/PID descriptors and incomplete
PCAP files and omits bulk/interrupt data, serial strings and unrelated devices.
The first capture utility did not stop on its quit key; only its owned process
was terminated after the Elgato app shut down cleanly. Packet records were
complete. Normal Windows bindings were restored after the completed WSL tests.

Seven CTest suites pass under GCC strict warnings and Clang ASan/UBSan.
Sanitized evidence: [November 16 results](hardware-results-2026-11-16.json).
[Earlier failures](hardware-results-2026-11-11.json) remain for historical context.
Full logs, captures and firmware remain local and excluded from Git. The other
agent's shared-workspace evidence was preserved.

### Short acceptance checks and color override

The user replaced the planned 31-minute test with a one-minute recording.
Long-duration reliability remains unverified. On the same WSL attachment:

| Test | Observed result |
| --- | --- |
| Ctrl+C during active streaming | Exit 0; shutdown in 2.870 s; valid H.264/MP2 output |
| Restart without replugging | MP4 wrapper completed, exit 0; H.264 video and AAC stereo audio |
| One-minute wrapper/MKV recording | Exit 0; 185,626,603-byte file; H.264/MP2; 0 reported drops or USB overflows |
| Explicit RGB/YUV comparison with stable cables | RGB matched the official Windows color bars; YUV reproduced pink whites |
| RGB wrapper override | Five-second MP4 completed with H.264/AAC; user confirmed normal HDMI pass-through colors |
| USB/IP detach during active streaming | LIBUSB_ERROR_NO_DEVICE; exit 1 in 0.621 s after detach request; no abort or forced signal |
| Cleanup | Card detached, unbound and returned to normal shared Windows binding |

Automatic color detection chose YUV for this RGB source. The existing `-c rgb`
corrected capture and pass-through colors; automatic detection remains unchanged.
The wrapper now accepts validated `GCHD_COLOR_SPACE=rgb|yuv|auto` and preserves
existing defaults when unset. Forwarding and invalid-value rejection are tested.

The user changed HDMI cables during the one-minute recording. Its successful
transport/mux result does not establish uninterrupted source video. Representative
frames still contained the expected color bars. The container duration was
64.965 s for the requested 60-second streaming interval; timestamp/trim precision
has not been validated. ffmpeg also reported a final truncated PES at EOF while
finalizing the short MP4 and MKV outputs, with exit 0 and readable containers.

Physical USB unplug during streaming and long-duration recording were not tested. A real USB/IP
disconnect was tested, which covers the observed userspace removal/error path
without claiming a physical power-loss test. Further WSL handoff root-cause
investigation remains deferred at the user's request.

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
