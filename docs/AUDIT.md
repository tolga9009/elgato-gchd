# Follow-up software audit — 2026-11-16

The modernization branch received a further review of USB lifecycle, output
ownership and concurrency, firmware/CLI/wrapper logic, trace exporters, and PSI
serialization. Findings below were checked against the implementation and
reproduced before fixing. Independent review checked the resulting changes.
No new Windows commands or physical-card tests were run for this audit.

## Confirmed defects and fixes

| Area | Failure and resulting behavior | Regression evidence |
| --- | --- | --- |
| USB initialization cleanup | START could succeed or time out ambiguously, then confirmation could fail before `isInitialized_` became true. Teardown skipped stopping that connected device. A separate flag now records the START attempt before transmission; bounded cleanup runs after late failure, and still skips confirmed disconnects and failures before START. | Mock timeout, I/O, cancellation, ambiguous START transfer, disconnect, and cleanup failure. Restoring the original cleanup guard fails the regression. |
| Empty USB reads | Immediate repeated OVERFLOW responses retried without pacing and consumed a CPU. Empty recoverable errors now wait 5 ms in cancellation-aware steps; partial timeout bytes and successful payloads pass through immediately. | Original mock exceeded 80,000 calls in 100 ms. Regression caps calls at 32 per 100 ms and checks cancellation and partial bytes. |
| Signal concurrency | `volatile sig_atomic_t` was accessed by handlers on the writer thread and by the main thread without synchronization. A guaranteed lock-free atomic exchange now requests shutdown and makes the second signal force exit. | ThreadSanitizer reproduced the original race; the fixed reproducer completed without a report. Child-process tests deliver signals on different threads and verify exit status. |
| Disk finalization | Delayed `close()` errors were discarded after successful writes, allowing success despite storage failure. Explicit finalization now reports them after the writer joins. Destruction remains nonthrowing and never retries a released descriptor. | Injected ENOSPC, EIO, and EINTR after a real close; immediate descriptor reuse proves finalization/destruction cannot close an unrelated file. |
| Recording wrapper shutdown | After forwarding a stop signal, the polling loop could wait forever for a driver ignoring SIGINT and never reach its existing timeout. Stop requests now enter the bounded wait, kill/reap a stuck driver, and allow ffmpeg to finalize. | Synthetic unresponsive producer, real ffmpeg, bounded termination, exit 143 on SIGTERM, reaped child, and readable MKV video/audio. |
| Wrapper terminal signals | Terminal Ctrl+C reached the driver directly and again through forwarding, triggering its second-signal emergency exit before cleanup. Driver and ffmpeg now run in separate sessions; the wrapper owns forwarding and sends only one graceful stop per child. | Terminal process-group SIGINT, delayed driver cleanup, exactly one forwarded SIGINT, finalized video/audio, and explicit second-wrapper-signal escalation. |
| Automatic H.264 level | Level selection used the unresolved zero frame-rate sentinel. At 720p60/5 Mbps it chose level 3.1 instead of 3.2. Selection now uses the resolved rate, with interlaced field rate converted to full-frame rate for level calculation. Automatic selection rejects levels outside the supported range. | Automatic/explicit equivalence at 720p60/50/30/25, 1080i50/60 and half rates; 1080p30 accepted and unsupported 1080p50/60 levels rejected. New test fails with the old implementation. |
| Trace identity filtering | An address with a matching descriptor was trusted for the whole trace, even after another VID/PID reused it. Exporters now refuse ambiguous address/handle identities. ETW checks both the actual trace and supplemental descriptor files. | Synthetic PCAP reuse leaked unrelated control bytes before the fix. Both reuse orders, default/detailed PCAP export, ETW supplemental traces, and empty stdout on refusal are covered. Actual ETW pointer reuse was not observed. |
| PSI tables and descriptors | Generated CRC-free tables failed their own parser; PMT descriptors dereferenced a null pointer; program/unknown descriptor cursors did not advance; unknown payloads were lost on serialization. Header/syntax decoding, parser allocation, payload packing, and cursor movement are corrected. Declared spans and nested lengths are checked before iterator construction; oversized serialized lengths are rejected. | PAT/PMT/SIT and descriptor round trips, original golden bytes, all table truncations and single-byte mutations, invalid caller spans, oversized descriptors/tables, ASan/UBSan. The original PMT reproducer reported a null dereference. |

PSI parsers currently validate internally generated tables; they do not parse
live USB input. Their bounds fixes protect that API and its validation tools;
this report does not claim an observed malformed-device-input vulnerability.
The generated bytes for existing supported table configurations are unchanged.

## Validation

- GCC 13 and Clang 18 with `-Wall -Wextra -Werror`.
- All eight CTest suites, including real ffmpeg synthetic recording tests.
- Full Clang AddressSanitizer/UndefinedBehaviorSanitizer run with leak checking.
- cppcheck warning/portability checks and staged CMake installation.
- Focused negative controls for cleanup, retry pacing, H.264 level selection,
  and trace filtering; the old behavior fails the new tests.

ThreadSanitizer is supplemental to CI. It sometimes failed during startup on
this WSL host because of address mappings; a non-PIE original run reported the
signal race and a fixed run completed cleanly. Some sandboxed scratch ASan runs
needed leak checking disabled because of ptrace; the full escalated Clang test
run passed with default leak checking enabled.

## Remaining limits and improvements

The physical-card results and WSL cold-start workaround in
[MODERNIZATION.md](MODERNIZATION.md) and [WSL2.md](WSL2.md) still apply. No claim
is made that these software-only regressions establish new hardware behavior.
Physical unplug during streaming, other card revisions/input modes, long-term
reliability, and precise timestamp/EOF trimming remain unverified. Automatic
color detection still needs an explicit RGB override for the tested source.

A blocked kernel write to an unresponsive regular-file filesystem cannot be
cancelled by the output queue. Close error reporting does not guarantee data
has reached stable storage; no new durability promise is made. Trace filtering
rejects observed identity conflicts; a trace omitting identity changes cannot
prove that an address or handle was never reused.

Future maintenance can make resource-owning classes explicitly noncopyable and
split protocol/state management out of the large GCHD class. Async bulk reads
remain a possible response to measured USB/IP drops, rather than a change
justified by the successful short captures. Multi-card support, analog/mode
improvements, HD60 support, V4L2, and merging upstream `dev` remain outside this
work's scope.
