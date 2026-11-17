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
