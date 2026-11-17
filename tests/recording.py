import ast, json, os, pathlib, signal, subprocess, sys, tempfile, time
wrapper=pathlib.Path(sys.argv[1]).resolve()
# Deliver a first wrapper signal synchronously from inside a pending send to
# cover handler reentrancy without depending on scheduler timing.
embedded=wrapper.read_text().split("<<'PYRECORD'\n",1)[1].rsplit('\nPYRECORD',1)[0]
helpers=ast.Module(body=[node for node in ast.parse(embedded).body
                        if isinstance(node,ast.FunctionDef) and node.name in ('stop_driver','handle_signal')],
                   type_ignores=[])
context=dict(signal=signal,stop_signal=0,stop_sent=False,mux=None)
class ReentrantCapture:
    def __init__(self): self.signals=[]
    def poll(self): return None
    def send_signal(self,sig):
        self.signals.append(sig)
        if len(self.signals)==1: context['handle_signal'](signal.SIGTERM,None)
reentrant=ReentrantCapture(); context['capture']=reentrant
exec(compile(helpers,str(wrapper),'exec'),context)
context['stop_driver']()
assert reentrant.signals==[signal.SIGINT], 'reentrant first signal duplicated driver SIGINT'
with tempfile.TemporaryDirectory(prefix="gchd-record-test-") as directory:
    work=pathlib.Path(directory)
    fixture=work/"fixture.ts"
    subprocess.run(["ffmpeg","-v","error","-f","lavfi","-i","testsrc=size=64x64:rate=10",
                    "-f","lavfi","-i","sine=frequency=440:sample_rate=48000","-t","1",
                    "-c:v","libx264","-pix_fmt","yuv420p","-c:a","mp2","-f","mpegts",str(fixture)],check=True,timeout=15)
    fake=work/"fake-gchd"
    fake.write_text("""#!/usr/bin/env python3
import json, os, pathlib, signal, sys, time
pathlib.Path(os.environ['GCHD_TEST_ARGS']).write_text(json.dumps(sys.argv[1:]))
marker=pathlib.Path(os.environ['GCHD_TEST_MARKER'])
stop_count=0
def stop(sig, frame):
    global stop_count
    stop_count+=1
    if os.environ.get('GCHD_TEST_SIGNAL_COUNT'):
        pathlib.Path(os.environ['GCHD_TEST_SIGNAL_COUNT']).write_text(str(stop_count))
    if stop_count>1: sys.exit(99)
    if os.environ.get('GCHD_TEST_CLEANUP_DELAY'): time.sleep(0.5)
    marker.write_text('SIGINT' if sig==signal.SIGINT else str(sig))
    sys.exit(0)
signal.signal(signal.SIGINT,signal.SIG_IGN if os.environ.get('GCHD_TEST_IGNORE_SIGINT') else stop)
if os.environ.get('GCHD_TEST_PID'):
    pathlib.Path(os.environ['GCHD_TEST_PID']).write_text(str(os.getpid()))
sys.stdout.buffer.write(pathlib.Path(os.environ['GCHD_TEST_TS']).read_bytes())
sys.stdout.buffer.flush()
marker.write_text('ready')
if os.environ.get('GCHD_TEST_WAIT'):
    while True: time.sleep(0.1)
""")
    fake.chmod(0o755)
    env=os.environ.copy(); env.update(GCHD_BIN=str(fake),GCHD_TEST_TS=str(fixture),GCHD_TEST_MARKER=str(work/"marker"),GCHD_TEST_ARGS=str(work/"args.json"))
    def codecs(path):
        result=subprocess.run(["ffprobe","-v","error","-show_streams","-of","json",str(path)],capture_output=True,check=True,timeout=5)
        return {s['codec_name'] for s in json.loads(result.stdout)['streams']}
    for suffix, audio in [('mkv','mp2'),('mp4','aac')]:
        output=work/('capture.'+suffix)
        if suffix=='mkv': env['GCHD_COLOR_SPACE']='rgb'
        else: env.pop('GCHD_COLOR_SPACE',None)
        result=subprocess.run([str(wrapper),str(output),'1s'],env=env,capture_output=True,timeout=20)
        assert result.returncode==0, result.stderr.decode()
        expected=['--duration','1s']+(['-c','rgb'] if suffix=='mkv' else [])+['-']
        assert json.loads((work/'args.json').read_text())==expected
        assert {'h264',audio} <= codecs(output)
        original=output.read_bytes()
        refused=subprocess.run([str(wrapper),str(output)],env=env,capture_output=True,timeout=3)
        assert refused.returncode!=0 and output.read_bytes()==original
    (work/'marker').unlink(); (work/'args.json').unlink()
    invalid_env=dict(env,GCHD_COLOR_SPACE='invalid')
    invalid=subprocess.run([str(wrapper),str(work/'invalid-color.mkv')],env=invalid_env,capture_output=True,timeout=3)
    assert invalid.returncode!=0 and b'GCHD_COLOR_SPACE' in invalid.stderr
    assert not (work/'args.json').exists() and not (work/'invalid-color.mkv').exists()
    env['GCHD_TEST_WAIT']='1'
    marker=pathlib.Path(env['GCHD_TEST_MARKER']); marker.unlink(missing_ok=True)
    interrupted=work/'interrupted.mkv'
    recording=subprocess.Popen([str(wrapper),str(interrupted)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    deadline=time.monotonic()+5
    while not marker.exists() and time.monotonic()<deadline: time.sleep(0.02)
    assert marker.exists(), 'fake driver did not start'
    recording.send_signal(signal.SIGTERM)
    _,errors=recording.communicate(timeout=20)
    assert recording.returncode==128+signal.SIGTERM, errors.decode()
    assert marker.read_text()=='SIGINT'
    assert {'h264','mp2'} <= codecs(interrupted)
    marker.unlink()
    # A terminal Ctrl+C targets the wrapper process group. Forward it exactly
    # once so a driver's second-signal emergency exit cannot bypass cleanup.
    terminal_env=dict(env,GCHD_TEST_CLEANUP_DELAY='1',GCHD_TEST_PID=str(work/'terminal-driver.pid'),
                      GCHD_TEST_SIGNAL_COUNT=str(work/'signal-count'))
    terminal_output=work/'terminal.mkv'
    driver_pid=None
    terminal=subprocess.Popen([str(wrapper),str(terminal_output)],env=terminal_env,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:
        deadline=time.monotonic()+5
        while not marker.exists() and time.monotonic()<deadline: time.sleep(0.02)
        assert marker.exists(), 'terminal-signal fake driver did not start'
        driver_pid=int((work/'terminal-driver.pid').read_text())
        os.killpg(terminal.pid,signal.SIGINT)
        _,errors=terminal.communicate(timeout=10)
        assert terminal.returncode==128+signal.SIGINT, errors.decode()
        assert (work/'signal-count').read_text()=='1', 'terminal SIGINT was forwarded twice'
        assert marker.read_text()=='SIGINT', 'terminal SIGINT bypassed driver cleanup'
        assert {'h264','mp2'} <= codecs(terminal_output)
    finally:
        if terminal.poll() is None:
            terminal.send_signal(signal.SIGTERM)
            try: terminal.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                terminal.kill(); terminal.communicate(timeout=5)
        if driver_pid is not None:
            try: os.kill(driver_pid,signal.SIGKILL)
            except ProcessLookupError: pass
    marker.unlink()
    # A consumer failure can already be stopping the driver when the user's
    # first signal arrives. Normal shutdown paths must share the same SIGINT.
    early_bin=work/'early-exit-bin'; early_bin.mkdir()
    early_ffmpeg=early_bin/'ffmpeg'
    early_ffmpeg.write_text("#!/usr/bin/env python3\nimport os,pathlib,sys\nsys.stdin.buffer.read(pathlib.Path(os.environ['GCHD_TEST_TS']).stat().st_size)\nsys.exit(1)\n")
    early_ffmpeg.chmod(0o755)
    early_env=dict(env,GCHD_TEST_CLEANUP_DELAY='1',GCHD_TEST_PID=str(work/'early-driver.pid'),
                   GCHD_TEST_SIGNAL_COUNT=str(work/'early-signal-count'),
                   PATH=str(early_bin)+os.pathsep+env['PATH'])
    driver_pid=None
    early=subprocess.Popen([str(wrapper),str(work/'early.mkv')],env=early_env,
                           stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:
        deadline=time.monotonic()+5
        count_file=work/'early-signal-count'
        while not count_file.exists() and time.monotonic()<deadline: time.sleep(0.02)
        assert count_file.exists(), 'consumer failure did not request driver cleanup'
        driver_pid=int((work/'early-driver.pid').read_text())
        early.send_signal(signal.SIGTERM)
        _,errors=early.communicate(timeout=5)
        assert early.returncode==128+signal.SIGTERM, errors.decode()
        assert count_file.read_text()=='1', 'normal cleanup paths forwarded SIGINT twice'
        assert marker.read_text()=='SIGINT', 'consumer failure bypassed driver cleanup'
    finally:
        if early.poll() is None:
            early.send_signal(signal.SIGTERM)
            try: early.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                early.kill(); early.communicate(timeout=5)
        if driver_pid is not None:
            try: os.kill(driver_pid,signal.SIGKILL)
            except ProcessLookupError: pass
    marker.unlink()
    # A single stop request must bound shutdown even when the producer ignores SIGINT.
    stubborn_env=dict(env,GCHD_TEST_IGNORE_SIGINT='1',GCHD_TEST_PID=str(work/'driver.pid'))
    stubborn_output=work/'stubborn.mkv'
    driver_pid=None
    stubborn=subprocess.Popen([str(wrapper),str(stubborn_output)],env=stubborn_env,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:
        deadline=time.monotonic()+5
        while not marker.exists() and time.monotonic()<deadline: time.sleep(0.02)
        assert marker.exists(), 'unresponsive fake driver did not start'
        driver_pid=int((work/'driver.pid').read_text())
        stubborn.send_signal(signal.SIGTERM)
        _,errors=stubborn.communicate(timeout=20)
        assert stubborn.returncode==128+signal.SIGTERM, errors.decode()
        assert b'Driver did not shut down within 12 seconds' in errors
        assert marker.read_text()=='ready', 'fake driver unexpectedly handled SIGINT'
        try: os.kill(driver_pid,0)
        except ProcessLookupError: pass
        else: raise AssertionError('unresponsive driver was not killed and reaped')
        assert {'h264','mp2'} <= codecs(stubborn_output)
    finally:
        if stubborn.poll() is None:
            stubborn.send_signal(signal.SIGTERM)
            try: stubborn.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                if driver_pid is not None:
                    try: os.kill(driver_pid,signal.SIGKILL)
                    except ProcessLookupError: pass
                stubborn.kill(); stubborn.communicate(timeout=5)
        if driver_pid is not None:
            try: os.kill(driver_pid,signal.SIGKILL)
            except ProcessLookupError: pass
    marker.unlink()
    # Isolation must preserve a second wrapper signal's immediate escalation.
    driver_pid=None
    escalating=subprocess.Popen([str(wrapper),str(work/'escalated.mkv')],env=stubborn_env,
                                stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:
        deadline=time.monotonic()+5
        while not marker.exists() and time.monotonic()<deadline: time.sleep(0.02)
        assert marker.exists(), 'escalation fake driver did not start'
        driver_pid=int((work/'driver.pid').read_text())
        escalating.send_signal(signal.SIGTERM)
        time.sleep(0.1)
        start=time.monotonic()
        escalating.send_signal(signal.SIGTERM)
        _,errors=escalating.communicate(timeout=5)
        assert escalating.returncode==128+signal.SIGTERM, errors.decode()
        assert time.monotonic()-start<5
        try: os.kill(driver_pid,0)
        except ProcessLookupError: pass
        else: raise AssertionError('second signal did not kill and reap the driver')
    finally:
        if escalating.poll() is None:
            escalating.send_signal(signal.SIGTERM)
            try: escalating.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                escalating.kill(); escalating.communicate(timeout=5)
        if driver_pid is not None:
            try: os.kill(driver_pid,signal.SIGKILL)
            except ProcessLookupError: pass
    marker.unlink()
    # Force a consumer failure before probing, with a producer waiting for SIGINT.
    tiny=work/'tiny.ts'; tiny.write_bytes(b'x'); env['GCHD_TEST_TS']=str(tiny)
    bin_dir=work/'bin'; bin_dir.mkdir()
    failing_ffmpeg=bin_dir/'ffmpeg'; failing_ffmpeg.write_text('#!/bin/sh\nsleep 0.2\nexit 1\n'); failing_ffmpeg.chmod(0o755)
    env['PATH']=str(bin_dir)+os.pathsep+env['PATH']
    bad=subprocess.run([str(wrapper),str(work/'bad.unsupported')],env=env,capture_output=True,timeout=20)
    assert bad.returncode!=0 and marker.read_text()=='SIGINT', bad.stderr.decode()
print('Synthetic recording, container finalization, no-overwrite, SIGINT forwarding and bounded shutdown passed')
