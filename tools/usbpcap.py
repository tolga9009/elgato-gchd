#!/usr/bin/env python3
"""Summarize a card's classic USBPcap trace without exporting firmware or video.

Format: https://desowin.org/usbpcap/captureformat.html
Raw USB payloads stay local; only short, known vendor controls may be exported.
"""
import argparse
import collections
import hashlib
import json
import struct


def records(path):
    with open(path, 'rb') as source:
        header = source.read(24)
        formats = {b'\xd4\xc3\xb2\xa1': ('<', 1e6), b'\xa1\xb2\xc3\xd4': ('>', 1e6),
                   b'\x4d\x3c\xb2\xa1': ('<', 1e9), b'\xa1\xb2\x3c\x4d': ('>', 1e9)}
        if len(header) != 24 or header[:4] not in formats:
            raise ValueError('Expected classic PCAP, not PCAPNG or an incomplete file')
        endian, scale = formats[header[:4]]
        major, minor, _, _, snaplen, link = struct.unpack(endian + 'HHIIII', header[4:])
        if (major, minor) != (2, 4) or link != 249 or not 0 < snaplen <= 16 * 1024 * 1024:
            raise ValueError('Unsupported USBPcap header or snapshot size')
        while True:
            header = source.read(16)
            if not header:
                break
            if len(header) != 16:
                raise ValueError('Incomplete PCAP packet header')
            seconds, fraction, size, original = struct.unpack(endian + 'IIII', header)
            if size > snaplen or size != original:
                raise ValueError('Truncated snapshot or invalid packet size')
            packet = source.read(size)
            if len(packet) != size or size < 27:
                raise ValueError('Incomplete USBPcap packet')
            length, irp, status, function, info, bus, device, endpoint, kind, data_size = struct.unpack_from('<HQIHBHHBBI', packet)
            if not 27 <= length <= size or (kind == 2 and length < 28):
                raise ValueError('Invalid USBPcap transfer header')
            yield dict(time=seconds + fraction / scale, key=(bus, device), irp=irp,
                       status=status, function=function, complete=bool(info & 1),
                       endpoint=endpoint, kind=kind, stage=packet[27] if kind == 2 else None,
                       data=packet[length:], data_size=data_size)


def control_pairs(path):
    pending = {}
    for packet in records(path):
        if packet['kind'] != 2:
            continue
        key = (packet['key'], packet['irp'])
        if packet['stage'] == 0 and len(packet['data']) >= 8:
            pending[key] = packet
        elif packet['stage'] == 3:
            request = pending.pop(key, None)
            if request is not None:
                yield request, packet


def selected_devices(path, vid, pid):
    identities = collections.defaultdict(set)
    for request, response in control_pairs(path):
        setup = struct.unpack_from('<BBHHH', request['data'])
        data = response['data']
        if (setup[:3] == (0x80, 6, 0x100) and response['status'] == 0
                and len(data) >= 18 and data[:2] == b'\x12\x01'):
            identities[request['key']].add(struct.unpack_from('<HH', data, 8))
    devices = {key for key, observed in identities.items() if (vid, pid) in observed}
    if not devices:
        raise ValueError('Selected VID/PID descriptor not found; refusing an unfiltered export')
    # Addresses can be reused after reconnecting; never mix different devices.
    if any(len(identities[key]) != 1 for key in devices):
        raise ValueError('Selected USB address has conflicting VID/PID descriptors; refusing export')
    return devices


def summarize(path, vid=0x0fd9, pid=0x0044, include_controls=False):
    devices = selected_devices(path, vid, pid)
    counts = collections.Counter()
    statuses = collections.Counter()
    uploads = {}
    packet_count = 0
    first = None
    for packet in records(path):
        if packet['key'] not in devices:
            continue
        packet_count += 1
        if first is None:
            first = packet['time']
        counts[packet['kind']] += 1
        if packet['complete']:
            statuses[packet['status']] += 1
        if packet['kind'] == 3 and packet['endpoint'] == 2 and not packet['complete']:
            upload = uploads.setdefault(packet['key'], [0, hashlib.sha256()])
            upload[0] += len(packet['data'])
            upload[1].update(packet['data'])
    controls, commands, initial_reads = [], [], []
    control_count = 0
    for request, response in control_pairs(path):
        if request['key'] not in devices:
            continue
        control_count += 1
        typ, code, value, index, length = struct.unpack_from('<BBHHH', request['data'])
        row = dict(time_seconds=round(request['time'] - first, 6), request_type=typ,
                   request=code, value=value, index=index, length=length,
                   usb_status=response['status'])
        payload = response['data'] if typ & 0x80 else request['data'][8:]
        # No standard descriptor strings, bulk firmware, interrupt or video bytes.
        limit = 6 if code == 0xb8 else 256
        if (typ in (0x40, 0xc0) and code in (0xb8, 0xbc, 0xbd)
                and len(payload) == length and length <= limit
                and (code != 0xb8 or (typ == 0x40 and value == index == 0 and length == 6))):
            row['data_hex'] = payload.hex()
        if include_controls:
            controls.append(row)
        if code == 0xb8 and 'data_hex' in row:
            commands.append(row)
        if typ == 0xc0 and code == 0xbc and len(initial_reads) < 12:
            initial_reads.append(row)
    result = dict(vid=vid, pid=pid, addresses=[dict(bus=b, address=d) for b, d in sorted(devices)],
                  packet_count=packet_count, control_count=control_count,
                  packets_by_transfer_type=dict(counts),
                  completion_status_counts={hex(k): v for k, v in statuses.items()},
                  encoder_endpoint_uploads=[dict(bus=k[0], address=k[1], bytes=v[0], sha256=v[1].hexdigest())
                                            for k, v in sorted(uploads.items())],
                  initial_register_reads=initial_reads, system_commands=commands)
    if include_controls:
        result['controls'] = controls
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace')
    parser.add_argument('--vid', type=lambda s: int(s, 0), default=0x0fd9)
    parser.add_argument('--pid', type=lambda s: int(s, 0), default=0x0044)
    parser.add_argument('--controls', action='store_true', help='Include control metadata and allowlisted short vendor data')
    args = parser.parse_args()
    try:
        result = summarize(args.trace, args.vid, args.pid, args.controls)
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, str(error) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
