# Windows USB diagnostics

## Built-in tracing without a capture-driver installation

Windows ETW/Logman was used successfully to capture the original 0fd9:0044
card's startup headers on 2026-11-11. No USBPcap installation or Windows restart
was needed. The provider settings follow [Microsoft's USB tracing guide](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/how-to-capture-a-usb-event-trace).

Run scripts/trace-windows-usb.ps1 from an administrator PowerShell, using a
fresh output directory accessible to that elevated account. Copy the script to
a local drive first if an elevated session cannot see a mapped network drive.
The default duration is 20 seconds, with a 32 MiB circular-file limit.

```powershell
.\trace-windows-usb.ps1 -OutputDirectory "$env:TEMP\gchd-usb-new" -DurationSeconds 20
```

Open the official Elgato app while the trace is running. The helper stops its
own unique session in finally and exports XML with tracerpt. It enables only
Default, HeadersBusTrace and Rundown; no packet payload keywords are enabled.
The raw trace contains controller-wide metadata and device descriptors. Keep
it private and outside Git. Output files are refused if already present.

Export only the selected card's transfer metadata in Linux:

```sh
python3 tools/usb_etw.py /path/to/usb-headers.xml > /tmp/elgato-headers.json
```

The parser requires a matching VID/PID descriptor, never falls back to exporting
all devices, omits OS/device pointers, and never exports bulk/interrupt payload.
For a separate trace without rundown, use --descriptors with a header trace
from the same unchanged device connection. Raw ETL/XML files are local evidence;
only reviewed, sanitized findings belong in the repository.

The observed native warm startup submitted 649648 encoder bytes to endpoint
0x02 successfully (19 x 32768 plus 27056 bytes). No idle-image upload was present
in that warm trace. Its first nine control request headers matched the WSL
startup. The first B8 system command succeeded natively and stalled under WSL.
Header-only tracing cannot compare the six data bytes in that command. This
difference is evidence to investigate, not a confirmed USB/IP root cause.

## Broader capture result and next capture

The subsequent 20-second native startup trace enabled UCX Default,
HeadersBusTrace, PartialDataBusTrace and FullDataBusTrace (0x1c1), with
USBXHCI also using those keywords plus Rundown (0x81c1), and USBHUB3 using
headers/default/rundown (0x8041). UCX event-ID filtering included control
submissions/completions/data (20–25), bulk/interrupt headers (26–27) and
standard/class/vendor request headers (34–62), excluding bulk/interrupt
payload events. No payload-value predicates were applied in this final run.

The trace contained 20,010 selected-card transfer events: 9,751 vendor control
submissions, 9,751 successful control completions, 274 bulk/interrupt
submissions and 234 completions. Outstanding reads at the end of the window
are not evidence of dropped video. UCX emitted completion event 21 version 1;
no control-data event 22 or 25 and no TransferData field was present anywhere
in the exported trace. The user's working official-app preview is the hardware
baseline; these counts alone do not establish a valid Linux recording.

Microsoft documents payload tracing, but enabling its keywords did not produce
payloads on this host. The reason is unconfirmed. Do not treat absent bytes as
zero-valued data, infer SCMD contents from headers, or keep repeating narrow ETW
captures. Use contemporaneous device descriptors: device handles changed after
handoff, so an older rundown can select zero events from a later trace.

For a payload-capable capture, collect one complete sequence for this card:
app startup, working preview, app shutdown, handoff to WSL and the failing Linux
startup. Keep all target-card control bytes, transfer statuses and timing;
include endpoint/firmware-upload metadata and the device's descriptors. This
covers both initialization and shutdown without guessing which six bytes will
matter later. Keep raw captures private and outside Git; firmware payloads must
never be published. Do not capture unrelated USB devices when the tool supports
a device filter.

An independent native-Linux test is another way to isolate the USB/IP
transport, but does not replace the missing official Windows command payloads.

## Approved USBPcap installation, 2026-11-16

After explicit user approval, the official USBPcap 1.5.4.0 installer was
installed successfully (exit code 0). The installer and installed driver both
have valid Windows Authenticode signatures from Tomasz Mon. The installer came
from the [official release](https://github.com/desowin/usbpcap/releases/tag/1.5.4.0)
and has SHA-256
`87a7edf9bbbcf07b5f4373d9a192a6770d2ff3add7aa1e276e82e38582ccb622`.
The registered version and installed driver version are both 1.5.4.0.

The [official installation instructions](https://desowin.org/usbpcap/) require
a Windows restart. No restart was initiated by the agent. Before restart,
the driver service was stopped and noninteractive --extcap-interfaces
returned no capture interfaces (exit 0). No USBPcap payload capture has yet
been performed. Installation evidence stays locally under artifacts/.

After installation, usbipd-win 5.3.0 list reported that USBPcap is an
incompatible USB filter and bind --force would be required. This warning
matches its [filter compatibility check](https://github.com/dorssel/usbipd-win/blob/v5.3.0/Usbipd/ConsoleTools.cs).
The card was still present in Windows as 0fd9:0044, shared at bus 9-4.
No forced binding or controller restart was attempted. Collect the native
Windows startup/preview/shutdown trace first after restart; resolve the filter
conflict before resuming WSL tests. A forced binding changes the Windows driver
association and must be reviewed separately rather than folded into the usual
attach command. An alternative is to remove USBPcap after collecting the native
trace, then follow its restart requirements before returning to normal WSL use.

The user subsequently requested a no-Windows-restart attempt. Starting only the
USBPcap service succeeded: its state became Running, but another elevated,
noninteractive interface enumeration returned exit 0 with empty output.
No usable capture interface was observed. The [matching driver source](https://github.com/desowin/usbpcap/blob/1.5.4.0/USBPcapDriver/USBPcapFilterManager.c) attaches
to root hubs in AddDevice and creates its capture control device there; merely
loading the driver does not attach it to an already-running root-hub stack.
A USB root-hub restart is a possible next attempt, not a tested recovery step.

The card's root hub also has Bluetooth, UPS USB monitoring and two downstream
hub trees. Restarting this hub would affect more than the capture card. No USB
hub or controller restart was performed during the service-only attempt;
separate approval was requested for the targeted root-hub restart. Do not use
USBPcap's interactive restart-all-controllers option for this comparison.

## Targeted root-hub restart attempt, 2026-11-16

The user explicitly approved a restart of only the card's root hub. Its live
parent chain was resolved again before invoking PnPUtil /restart-device with
that single root-hub instance ID. No wildcard, device-class, parent-controller
or /reboot option was used. Windows returned 3010 and reported that a system
reboot was needed to complete configuration operations.

After this attempt the USBPcap service was Running, but repeated elevated
--extcap-interfaces calls still produced no interfaces (exit 0). The Elgato
card remained present with PnP status OK. No USBPcap payload trace was obtained,
and no Windows restart was initiated. This targeted attempt did not avoid the
restart requirement on the tested host. The user plans to restart later;
no broader controller restart was attempted.
