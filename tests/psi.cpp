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

template<class T> static void invalidRanges(T prototype) {
    const std::vector<uint8_t> bytes{0,0};
    for (int size: {-1,3,1000000}) {
        auto parsed=prototype;
        auto cursor=bytes.cbegin();
        bool rejected=false;
        try { parsed.unpack(bytes,cursor,size); }
        catch (const PSI_FormatException &) { rejected=true; }
        check(rejected);
    }
}

template<class T> static void malformedCorpus(T prototype) {
    std::vector<uint8_t> valid;
    prototype.bytes(valid);
    auto parse=[&](const std::vector<uint8_t> &bytes) {
        auto parsed=prototype;
        auto cursor=bytes.cbegin();
        try { parsed.unpack(bytes,cursor,bytes.size()); }
        catch (const PSI_FormatException &) { return; }
        check(cursor==bytes.cend());
    };
    // Exercise all truncations and every single-byte mutation of generated
    // input. Sanitizers check the rejection paths as well as valid round trips.
    for (size_t length=0; length<valid.size(); ++length)
        parse(std::vector<uint8_t>(valid.begin(),valid.begin()+length));
    for (size_t offset=0; offset<valid.size(); ++offset) {
        auto bytes=valid;
        for (unsigned value=0; value<256; ++value) {
            bytes[offset]=value;
            parse(bytes);
        }
    }
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

    invalidRanges(PAT(0)); invalidRanges(PAT_Data()); invalidRanges(PAT_Entry());
    invalidRanges(PMT(0,0)); invalidRanges(PMT_Data()); invalidRanges(PMT_Mapping());
    invalidRanges(SIT()); invalidRanges(SIT_Data());
    invalidRanges(PSI_TableHeader()); invalidRanges(PSI_Syntax());
    invalidRanges(PSI_ParseDescriptor());
    malformedCorpus(pat); malformedCorpus(pmt); malformedCorpus(sit);

    // An impossible nested length must be rejected before an end iterator is formed.
    for (const auto &bytes: {std::vector<uint8_t>{0x1b,0xe1,1,0xf3,0xff},
                            std::vector<uint8_t>{0x1b,0xe1,1,0xf0,6,0x28,4}}) {
        PMT_Mapping parsed;
        auto cursor=bytes.cbegin();
        bool rejected=false;
        try { parsed.unpack(bytes,cursor,bytes.size()); }
        catch (const PSI_FormatException &) { rejected=true; }
        check(rejected);
    }
    PSI_ProgramDescriptor tooLarge(std::vector<uint8_t>(256));
    bool tooLargeRejected=false;
    try { std::vector<uint8_t> bytes; tooLarge.bytes(bytes); }
    catch (const PSI_ValueException &) { tooLargeRejected=true; }
    check(tooLargeRejected);
    PAT tooManyEntries(1);
    for (int i=0; i<16384; ++i) tooManyEntries.addEntry(PAT_Entry(1,0x100));
    tooLargeRejected=false;
    try { std::vector<uint8_t> bytes; tooManyEntries.bytes(bytes); }
    catch (const PSI_ValueException &) { tooLargeRejected=true; }
    check(tooLargeRejected);

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
