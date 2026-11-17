#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
exec python3 - "$script_dir" "$@" <<'PYRECORD'
import os, pathlib, shutil, signal, subprocess, sys, time
script_dir=pathlib.Path(sys.argv[1])
if len(sys.argv) not in (3,4):
    raise SystemExit("Usage: gchd-record.sh <output-file> [duration]")
output=sys.argv[2]
if os.path.lexists(output): raise SystemExit("Refusing to overwrite existing output: "+output)
local=script_dir.parent/"build/src/gchd"
driver=os.environ.get("GCHD_BIN") or (str(local) if local.is_file() else shutil.which("gchd"))
if not driver: raise SystemExit("gchd not found; build/install it or set GCHD_BIN")
ffmpeg=shutil.which("ffmpeg")
if not ffmpeg: raise SystemExit("ffmpeg is required")
args=[driver]
if len(sys.argv)==4: args += ["--duration",sys.argv[3]]
color_space=os.environ.get("GCHD_COLOR_SPACE")
if color_space is not None:
    if color_space not in ("auto","rgb","yuv"):
        raise SystemExit("GCHD_COLOR_SPACE must be auto, rgb, or yuv")
    args += ["-c",color_space]
args += ["-"]
capture=None
mux=None
stop_signal=0

def stop_driver():
    if capture is not None and capture.poll() is None:
        try: capture.send_signal(signal.SIGINT)
        except ProcessLookupError: pass

def handle_signal(sig, frame):
    global stop_signal
    if stop_signal:
        if capture is not None and capture.poll() is None: capture.kill()
        if mux is not None and mux.poll() is None: mux.terminate()
    else:
        stop_signal=sig
        stop_driver()

for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP): signal.signal(sig,handle_signal)
try:
    capture=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE)
    if stop_signal: stop_driver()
    codecs=["-c","copy"]
    if pathlib.Path(output).suffix.lower() in (".mp4",".mov",".m4v"):
        # Original firmware commonly emits MPEG audio; AAC is widely compatible in MP4.
        codecs=["-c:v","copy","-c:a","aac","-b:a","192k"]
    mux=subprocess.Popen([ffmpeg,"-nostdin","-hide_banner","-n","-i","pipe:0","-map","0",*codecs,output],
                         stdin=capture.stdout)
    capture.stdout.close()
    while capture.poll() is None and mux.poll() is None: time.sleep(0.05)
    if mux.poll() is not None: stop_driver()
    try: driver_status=capture.wait(timeout=12)
    except subprocess.TimeoutExpired:
        capture.kill(); capture.wait(); raise RuntimeError("Driver did not shut down within 12 seconds")
    try: mux_status=mux.wait(timeout=12)
    except subprocess.TimeoutExpired:
        mux.terminate(); mux.wait(timeout=3); raise RuntimeError("ffmpeg did not finish after driver exit")
    if stop_signal: sys.exit(128+stop_signal)
    if driver_status or mux_status:
        raise SystemExit("Recording failed: driver=%s ffmpeg=%s" % (driver_status,mux_status))
except (OSError,RuntimeError) as error:
    raise SystemExit(str(error))
finally:
    stop_driver()
    if capture is not None and capture.poll() is None:
        try: capture.wait(timeout=12)
        except subprocess.TimeoutExpired: capture.kill(); capture.wait()
    if mux is not None and mux.poll() is None:
        mux.terminate()
        try: mux.wait(timeout=3)
        except subprocess.TimeoutExpired: mux.kill(); mux.wait()
PYRECORD
