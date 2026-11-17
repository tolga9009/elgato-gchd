import importlib.util, pathlib, tempfile
spec=importlib.util.spec_from_file_location("usb_etw",pathlib.Path(__file__).parents[1]/"tools/usb_etw.py")
etw=importlib.util.module_from_spec(spec); spec.loader.exec_module(etw)
def event(fields):
    return '<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event"><System><Provider Name="Microsoft-Windows-USB-UCX"/><EventID>22</EventID></System><EventData>'+fields+'</EventData></Event>'
def data(key,value): return '<Data Name="'+key+'">'+value+'</Data>'
with tempfile.TemporaryDirectory() as directory:
    p=pathlib.Path(directory)/"trace.xml"
    descriptor=data("fid_idVendor","0xfd9")+data("fid_idProduct","0x44")+data("fid_UsbDevice","card")
    setup=''.join(data("fid_URB_Setup_"+k,v) for k,v in {'bmRequestType':'0x40','bRequest':'0xb8','wValue':'0','wIndex':'0','wLength':'6'}.items())
    payload=''.join(data('fid_URB_TransferData',v) for v in ['0','0','2','0','0','0'])
    p.write_text('<Events>'+event(descriptor)+event(data('fid_UsbDevice','card')+setup+payload)+event(data('fid_UsbDevice','other')+setup+payload)+'</Events>')
    handles=etw.discover([p],0xfd9,0x44); rows=list(etw.selected(p,handles))
    assert len(rows)==1 and rows[0]['scmd_hex']=='000002000000'
    assert 'card' not in str(rows) and 'other' not in str(rows)
    p.write_text('<Events>'+event(data('fid_UsbDevice','other')+setup+payload)+'</Events>')
    try: etw.discover([p],0xfd9,0x44)
    except ValueError: pass
    else: raise AssertionError('missing descriptor permitted unfiltered output')
print('ETW device isolation and six-byte-only export checks passed')
