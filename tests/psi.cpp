#include "gchd/psi_pat.hpp"
#include "gchd/psi_pmt.hpp"
#include "gchd/psi_sit.hpp"
#include "gchd/psi_exceptions.hpp"
#include <iostream>
#include <stdexcept>

static void check(bool ok) {
    if (!ok) throw std::runtime_error("PSI regression");
}

template<class T> static void roundtrip(T original, T parsed) {
    std::vector<uint8_t> bytes, repacked;
    original.bytes(bytes);
    auto cursor=bytes.cbegin();
    parsed.unpack(bytes,cursor,bytes.size());
    check(cursor==bytes.cend());
    parsed.bytes(repacked);
    check(repacked==bytes);
}

int main() {
    // Buffers sent to the device exclude the CRC that the hardware appends.
    roundtrip(PAT(1),PAT(0));
    PAT pat(1);
    pat.addEntry(PAT_Entry(1,0x100));
    pat.addEntry(PAT_Entry(2,0x200));
    roundtrip(pat,PAT(0));
    PMT pmt(1,0x101);
    pmt.addMapEntry(PMT_Mapping(0x101));
    roundtrip(pmt,PMT(0,0));
    roundtrip(SIT(),SIT());
    roundtrip(PSI_Syntax(0x1234,3,false),PSI_Syntax());

    // Descriptor payloads must survive parsing and advance to the next entry.
    for (size_t size: {size_t(0),size_t(1),size_t(3),size_t(255)}) {
        std::vector<uint8_t> payload(size,0xa5);
        roundtrip(PSI_ProgramDescriptor(payload),PSI_ProgramDescriptor());
        PSI_UnknownDescriptor unknown(0xfe);
        unknown.data=payload;
        std::vector<uint8_t> bytes;
        unknown.bytes(bytes);
        check(bytes.size()==size+2 && bytes[1]==size);
        check(std::vector<uint8_t>(bytes.begin()+2,bytes.end())==payload);
        roundtrip(unknown,PSI_UnknownDescriptor(0));
    }
    roundtrip(PSI_AVC_VideoDescriptor(100,41,true,false,true),PSI_AVC_VideoDescriptor());
    for (bool timing: {false,true}) for (bool clock90: {false,true}) {
        PSI_AVC_TimingAndHRDDescriptor hrd(true,true,false,true);
        hrd.picture_and_timing_info_present_=timing;
        hrd.kHz90_flag_=clock90;
        hrd.N_=1001; hrd.K_=60000; hrd.num_units_in_tick_=2;
        roundtrip(hrd,PSI_AVC_TimingAndHRDDescriptor());
    }
    auto program=std::make_shared<PSI_HDMV_ShortDescriptor>();
    auto unknown=std::make_shared<PSI_UnknownDescriptor>(0xfe);
    unknown->data={0x12,0x34,0x56};
    PMT_Mapping mapping(0x101);
    mapping.addDescriptor(std::make_shared<PSI_AVC_VideoDescriptor>(100,41));
    mapping.addDescriptor(program);
    mapping.addDescriptor(unknown);
    roundtrip(mapping,PMT_Mapping());
    pmt.addProgramInfo(program);
    pmt.addProgramInfo(unknown);
    pmt.addMapEntry(mapping);
    roundtrip(pmt,PMT(0,0));
    SIT sit;
    sit.data_.transmissionParameters_={program,unknown};
    roundtrip(sit,SIT());

    for (uint8_t length=0; length<4; ++length) {
        const std::vector<uint8_t> bytes{0,0xb0,length};
        auto cursor=bytes.cbegin();
        PSI_TableHeader header;
        bool rejected=false;
        try { header.unpack(bytes,cursor,bytes.size()); }
        catch (const PSI_FormatException &) { rejected=true; }
        check(rejected);
    }
    PSI_TableHeader oversized(0);
    oversized.innerLength_=65535;
    bool rejected=false;
    try { std::vector<uint8_t> bytes; oversized.bytes(bytes); }
    catch (const PSI_ValueException &) { rejected=true; }
    check(rejected);
    std::cerr << "PSI checks passed\n";
}
