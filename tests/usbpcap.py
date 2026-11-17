import importlib.util
import json
import pathlib
import struct
import sys
import tempfile
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('usbpcap', pathlib.Path(__file__).parents[1] / 'tools/usbpcap.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def packet(irp, device, kind, endpoint, complete, data, stage=None):
    length = 28 if kind == 2 else 27
    header = struct.pack('<HQIHBHHBBI', length, irp, 0, 0, complete, 1, device, endpoint, kind, len(data))
    body = header + (bytes([stage]) if stage is not None else b'') + data
    return struct.pack('<IIII', 1, 0, len(body), len(body)) + body

def control(irp, device, setup, data):
    typ = setup[0]
    return (packet(irp, device, 2, 0, 0, struct.pack('<BBHHH', *setup) + (data if not typ & 0x80 else b''), 0)
            + packet(irp, device, 2, 0x80 if typ & 0x80 else 0, 1, data if typ & 0x80 else b'', 3))

with tempfile.TemporaryDirectory() as directory:
    p = pathlib.Path(directory) / 'trace.pcap'
    header = struct.pack('<IHHIIII', 0xa1b2c3d4, 2, 4, 0, 0, 1048576, 249)
    descriptor = bytes.fromhex('1201000200000040d90f4400000201020301')
    body = control(1, 3, (0x80, 6, 0x100, 0, 18), descriptor)
    body += control(2, 3, (0x40, 0xb8, 0, 0, 6), bytes.fromhex('000002000000'))
    body += control(3, 9, (0x40, 0xb8, 0, 0, 6), b'OTHER!')
    body += control(4, 3, (0x80, 6, 0x302, 0, 6), b'SERIAL')
    body += packet(5, 3, 3, 2, 0, b'PROPRIETARY FIRMWARE')
    body += packet(6, 3, 3, 0x81, 1, b'PRIVATE VIDEO')
    p.write_bytes(header + body)
    result = module.summarize(p, include_controls=True)
    text = json.dumps(result)
    assert result['packet_count'] == 8 and result['control_count'] == 3
    assert result['system_commands'][0]['data_hex'] == '000002000000'
    assert result['encoder_endpoint_uploads'][0]['bytes'] == len(b'PROPRIETARY FIRMWARE')
    for secret in (b'OTHER!', b'SERIAL', b'PROPRIETARY FIRMWARE', b'PRIVATE VIDEO'):
        assert secret.decode() not in text and secret.hex() not in text
    for malformed in (header + body[:-1], header + body + b'\0', header[:20]):
        p.write_bytes(malformed)
        try: module.summarize(p)
        except ValueError: pass
        else: raise AssertionError('Incomplete capture accepted')
    p.write_bytes(header + control(2, 3, (0x40, 0xb8, 0, 0, 6), b'123456'))
    try: module.summarize(p)
    except ValueError: pass
    else: raise AssertionError('Missing descriptor permitted unfiltered export')
print('USBPcap selection, payload exclusion and truncated-file checks passed')
