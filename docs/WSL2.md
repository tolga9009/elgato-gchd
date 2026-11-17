# WSL 2 + usbipd-win recording

This targets the original Game Capture HD (`0fd9:0044`, `004e`, `0051`, `005d`).
USB enumeration, non-root access, both firmware uploads and a 30-second CLI
recording were verified on Ubuntu 24.04 / WSL 2 with an original `0fd9:0044`
card. ffprobe found H.264 720×480 video and stereo 48 kHz MP2 audio. The capture
contained 96,468,992 bytes with no reported dropped buffers or USB overflows.
Other card PIDs and input modes are unverified. See [hardware results](hardware-results-2026-11-16.json).

## Known WSL/USB-IP transfer/handoff issue

On the tested system, transferring a previously initialized card from the
Windows driver to WSL could leave the device in an unusable startup state:
the state register read `0xffff`, and initialization timed out or a system
command stalled. Earlier attempts also timed out on the first firmware write.
A software USB reset returned success but did not recover capture.

This is an **observed WSL/USB-IP handoff issue**, not a confirmed defect in a
specific Windows, Linux, usbipd or device-firmware component. Its internal root
cause has not been isolated. The same firmware and driver successfully recorded
after the card was power-cycled while already force-bound to USB/IP; its initial
state was then `0x0000`. No firmware chunk-size or streaming-queue change was
needed for that successful test.

Tested workaround for the original card (replace BUSID with its current ID):

1. Close the official Elgato app. In **administrator PowerShell**, run:

   ```powershell
   usbipd list
   usbipd bind --force --busid=BUSID
   ```

2. Leave that forced binding in place. Unplug only the card's USB power for
   about 10 seconds, then reconnect to the same port. If HDMI keeps its lights
   on, disconnect HDMI during the power-off interval too, then reconnect both.
   Keep the Windows Elgato app closed.

3. In normal PowerShell, attach the cold card:

   ```powershell
   usbipd attach --wsl --busid=BUSID
   ```

4. Verify permissions and record in WSL using the commands below. Both firmware
   images loaded through the same attachment during the successful test.

Forced binding changes this card's Windows driver association and prevents the
official app from using it until unbound. This test had USBPcap installed for
diagnostics; usbipd 5.3.0 explicitly required forced binding because of that
filter. USBPcap is **not** a recording dependency. The workaround was verified
with that configuration; behavior without the filter is not inferred from it.

To return the card to Windows, detach in normal PowerShell and unbind in an
administrator PowerShell:

```powershell
usbipd detach --busid=BUSID
usbipd unbind --busid=BUSID
# Optional: restore normal persistent sharing for a future standard attach.
usbipd bind --busid=BUSID
```

Keep the attachment in place between Linux recording sessions. Returning the
card to Windows and handing it back may require the cold-start procedure again.
Automatic recovery from the unusable state is not established.

## Windows: once per installation/device

These are commands for you to run in Windows; Windows-side changes require your approval. Use an administrator PowerShell for installation
and binding. Replace `BUSID` with the current ID from `usbipd list` (the tested ID was `9-4`, which may change):

```powershell
winget install --interactive --exact dorssel.usbipd-win
usbipd list
usbipd bind --busid=BUSID
```

Binding persists across reboots. Keep a WSL terminal open for the next step.
See Microsoft's [USB connection guide](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)
and the [usbipd WSL guide](https://github.com/dorssel/usbipd-win/wiki/WSL-support).

## Windows: after reboot, replug, or WSL restart

Normal (non-administrator) PowerShell:

```powershell
usbipd list
usbipd attach --wsl --busid=BUSID
usbipd list
```

With supported versions, check `usbipd attach --help` and optionally use:

```powershell
usbipd attach --wsl --busid=BUSID --auto-attach
```

Auto-attach runs a foreground retry loop; keep its terminal open. It is not a
permanent service setting. Current usbipd implements an
[attach loop](https://github.com/dorssel/usbipd-win/blob/master/Usbipd/Wsl.cs).
While attached, Windows capture software cannot use the device. When done:

```powershell
usbipd detach --busid=BUSID
```

## WSL: verify USB and permissions

```sh
uname -r
lsusb -d 0fd9:
```

Use the [README](../README.md) to build, install firmware and configure udev.
WSL may have no running udev daemon or desktop seat, so `TAG+="uaccess"` alone
may not grant access. Reload rules before attaching if udev is available. If
reload reports that no daemon is running, your distro may support
`sudo service udev restart`; do not assume it is active.

For a narrow temporary fallback, find the actual bus/device number in lsusb:

```sh
# Example only: replace 001/002 with the numbers printed by lsusb.
sudo chgrp "$(id -gn)" /dev/bus/usb/001/002
sudo chmod 0660 /dev/bus/usb/001/002
```

This applies only to that device node and must be repeated after reattach or
re-enumeration. Avoid broad permissions on all USB nodes. Keep ffmpeg unprivileged.

## Color handling on the tested source

The RGB 480p color-bar source was misidentified as YUV by automatic detection.
The recordings then had pink whites and incorrect colors; forcing `-c rgb`
matched the earlier official-Windows-app reference. A stable-cable comparison
reproduced the issue with explicit YUV and corrected it with explicit RGB.
This is a separate input-color limitation, not evidence of USB packet loss.

For this source, use:

```sh
build/src/gchd -i hdmi -c rgb --duration 60s capture.ts
GCHD_COLOR_SPACE=rgb scripts/gchd-record.sh capture.mp4 60s
```

`GCHD_COLOR_SPACE` accepts rgb, yuv or auto. Leave it unset for the driver's
existing default. This override is source-specific; do not force RGB for every
HDMI device. Automatic color detection remains unchanged. The user changed
cables during the one-minute acceptance recording, so that run does not validate
uninterrupted HDMI input or automatic recovery after a source change.

## WSL: record and inspect

Prefer the Linux filesystem for builds and initial recording:

```sh
export GCHD_FIRMWARE_DIR="$HOME/.local/share/gchd/firmware"
build/src/gchd --duration 30s "$HOME/capture.ts"
ffprobe -v error -show_streams "$HOME/capture.ts"
scripts/gchd-record.sh "$HOME/capture.mp4" 10m
```

Or pipe directly, with shell pipeline failure propagation:

```sh
set -o pipefail
build/src/gchd --duration 90s - | ffmpeg -i - -c copy "$HOME/capture.mkv"
```

Stop with Ctrl+C once and wait. Disconnects should exit nonzero and cleanup
cannot throw; these paths are covered by USB fault mocks. Actual initialization
timeout and SIGINT tests exited cleanly; the timed recording also exited 0 after
shutdown. Ctrl+C during real streaming and restart on the same attachment
succeeded. A USB/IP detach during streaming produced LIBUSB_ERROR_NO_DEVICE
and exit 1 promptly; physical USB unplug remains unverified.
Stats appear every five seconds on stderr. Nonzero drop/overflow counts mean
loss; persistent timeouts with zero bytes do not establish a working stream.
If protocol readiness fails, capture stderr and try a physical power cycle;
do not repeatedly upload after an unresolved failure.

Normal firmware upload uses the already-open device and does not request a USB
reset; one attach sufficed for both firmware images in the successful short
test, without re-enumeration. `gchd --reset` explicitly calls libusb_reset_device
and may require a new Windows attach. It did not recover the observed handoff
failure; use the tested cold-start sequence for that condition.

## Hardware acceptance checklist

Verified: both firmware uploads, non-root access, timed capture, Ctrl+C/restart,
MP4/MKV wrapper output and a USB/IP disconnect during streaming. A one-minute
recording completed with zero reported dropped buffers/overflows, but the user
changed HDMI cables during it, so uninterrupted input was not validated.
The planned 31-minute run was replaced with this short check at the user's
request. Physical USB unplug during streaming and long-duration reliability remain unverified.
The requested 60-second streaming interval produced a 64.965-second container;
precise timestamp/trim behavior is unverified. EOF can leave a partial TS/PES;
ffmpeg finalized the tested containers successfully despite a final PES warning.
If measurements implicate USB/IP round-trip latency, evaluate 4–8 async bulk-IN
transfers against FF-5's MIT-licensed experimental branch. Its existence is not
evidence of working capture. V4L2 and analog/mode-detection changes are deferred.
