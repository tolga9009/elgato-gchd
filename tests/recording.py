import json, os, pathlib, signal, subprocess, sys, tempfile, time
wrapper=pathlib.Path(sys.argv[1]).resolve()
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
def stop(sig, frame):
    marker.write_text('SIGINT' if sig==signal.SIGINT else str(sig))
    sys.exit(0)
signal.signal(signal.SIGINT,stop)
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
    # Force a consumer failure before probing, with a producer waiting for SIGINT.
    tiny=work/'tiny.ts'; tiny.write_bytes(b'x'); env['GCHD_TEST_TS']=str(tiny)
    bin_dir=work/'bin'; bin_dir.mkdir()
    failing_ffmpeg=bin_dir/'ffmpeg'; failing_ffmpeg.write_text('#!/bin/sh\nsleep 0.2\nexit 1\n'); failing_ffmpeg.chmod(0o755)
    env['PATH']=str(bin_dir)+os.pathsep+env['PATH']
    bad=subprocess.run([str(wrapper),str(work/'bad.unsupported')],env=env,capture_output=True,timeout=20)
    assert bad.returncode!=0 and marker.read_text()=='SIGINT', bad.stderr.decode()
print('Synthetic recording, container finalization, no-overwrite, and SIGINT forwarding passed')
