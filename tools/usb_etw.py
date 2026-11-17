#!/usr/bin/env python3
"""Export only a selected USB device's ETW transfer metadata from tracerpt XML."""
import argparse
import collections
import json
import xml.etree.ElementTree as ET

NS = {"e": "http://schemas.microsoft.com/win/2004/08/events/event"}

def number(value):
    try:
        return int(value, 0) if value.lower().startswith("0x") else int(value)
    except (ValueError, AttributeError):
        return None

def events(path):
    for _, event in ET.iterparse(path, events=["end"]):
        if event.tag.rsplit("}", 1)[-1] != "Event":
            continue
        system = event.find("e:System", NS)
        data = event.find("e:EventData", NS)
        if system is not None and data is not None:
            fields = collections.defaultdict(list)
            for leaf in data.iter():
                if leaf.get("Name") and not list(leaf) and leaf.text:
                    fields[leaf.get("Name")].append(leaf.text.strip())
            provider = system.find("e:Provider", NS)
            created = system.find("e:TimeCreated", NS)
            yield {
                "provider": provider.get("Name", "") if provider is not None else "",
                "id": number(system.findtext("e:EventID", "0", NS)),
                "time": created.get("SystemTime", "") if created is not None else "",
                "fields": dict(fields),
            }
        event.clear()

def first(fields, key):
    values = fields.get(key, [])
    return values[0] if values else None

def discover(paths, vid, pid):
    handles = set()
    for path in paths:
        for event in events(path):
            f = event["fields"]
            if number(first(f, "fid_idVendor")) == vid and number(first(f, "fid_idProduct")) == pid:
                handle = first(f, "fid_UsbDevice")
                if handle:
                    handles.add(handle)
    if not handles:
        raise ValueError("Selected device descriptor not found; no unfiltered export is allowed")
    return handles

def selected(path, handles):
    for event in events(path):
        f = event["fields"]
        if first(f, "fid_UsbDevice") not in handles:
            continue
        if event["provider"] != "Microsoft-Windows-USB-UCX":
            continue
        row = {"time": event["time"], "event_id": event["id"]}
        setup = {name: number(first(f, "fid_URB_Setup_" + name))
                 for name in ("bmRequestType", "bRequest", "wValue", "wIndex", "wLength")}
        if setup["bRequest"] is not None:
            row.update(kind="control", setup=setup)
            # Export only six-byte SCMD data; never firmware or arbitrary USB payloads.
            payload = [number(v) for v in f.get("fid_URB_TransferData", [])]
            if (setup == {"bmRequestType": 0x40, "bRequest": 0xb8,
                          "wValue": 0, "wIndex": 0, "wLength": 6}
                    and len(payload) == 6 and all(v is not None and 0 <= v <= 255 for v in payload)):
                row["scmd_hex"] = bytes(payload).hex()
        elif event["id"] in (26, 27):
            row["kind"] = "bulk_or_interrupt"
        else:
            continue
        for name, field in (("length", "fid_URB_TransferBufferLength"),
                            ("flags", "fid_URB_TransferFlags"),
                            ("usb_status", "fid_URB_Hdr_Status"),
                            ("nt_status", "fid_IRP_NtStatus")):
            value = number(first(f, field))
            if value is not None:
                row[name] = value
        yield row

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace")
    parser.add_argument("--descriptors", action="append", default=[], help="Header trace containing device rundown descriptors")
    parser.add_argument("--vid", type=lambda v: int(v, 0), default=0x0fd9)
    parser.add_argument("--pid", type=lambda v: int(v, 0), default=0x0044)
    args = parser.parse_args()
    handles = discover(args.descriptors or [args.trace], args.vid, args.pid)
    print(json.dumps({"vid": args.vid, "pid": args.pid,
                      "events": list(selected(args.trace, handles))}, indent=2))

if __name__ == "__main__":
    main()
