#include "utility.hpp"
#include "firmware.hpp"
#include "gchd/settings.hpp"
#include "gchd/psi_pat.hpp"
#include "gchd/psi_pmt.hpp"
#include "gchd/psi_sit.hpp"
#include <iostream>
#include <stdexcept>
static void check(bool ok) { if (!ok) throw std::runtime_error("logic regression"); }
int main() {
    FILE *empty=tmpfile(); check(empty!=nullptr);
    bool rejected=false;
    try { Firmware::validate(empty,"MB86H57_H58_IDLE"); } catch (const std::runtime_error &) { rejected=true; }
    fclose(empty); check(rejected);
    check(TranscoderSettings::unsignedH264Level(4.1f)==41);
    double seconds=0;
    check(Utility::parseDuration(seconds,"10m") && seconds==600);
    check(Utility::parseDuration(seconds,"90s") && seconds==90);
    check(Utility::parseDuration(seconds,"0.1s") && seconds==0.1);
    for (const auto &bad: {"", "0", "-1s", "nan", "inf", "10ms", "1x", "999999999999h"}) check(!Utility::parseDuration(seconds,bad));
    unsigned long x=0,y=0;
    check(Utility::parseNumericResolution(x,y," 1920x1080 ") && x==1920 && y==1080);
    for (const auto &bad: {"", "x", "1920x", "x1080", "-1x480", "0x480", "1x2x3", "99999999999999999999999999x480"})
        check(!Utility::parseNumericResolution(x,y,bad));
    std::string a,p;
    check(Utility::splitIPAddressAndPort(a,p,"[::1]:123") && a=="::1" && p=="123");
    check(Utility::splitIPAddressAndPort(a,p,"127.0.0.1:42") && a=="127.0.0.1" && p=="42");
    check(Utility::splitIPAddressAndPort(a,p,"::1") && a=="::1" && p.empty());
    check(Utility::splitIPAddressAndPort(a,p,"") && a.empty() && p.empty());
    check(!Utility::splitIPAddressAndPort(a,p,"[::1]junk:3"));
    check(!Utility::splitIPAddressAndPort(a,p,"[::1"));
    std::vector<uint8_t> bytes;
    PAT pat(1); pat.addEntry(PAT_Entry(1,0x100)); pat.bytes(bytes);
    check(bytes == std::vector<uint8_t>({0,0xb0,0x0d,0,1,0xc1,0,0,0,1,0xe1,0}));
    PMT pmt(1,0x101); pmt.addMapEntry(PMT_Mapping(0x101)); pmt.bytes(bytes);
    check(bytes.size()==17 && bytes[0]==2 && bytes[8]==0xe1 && bytes[9]==1 && bytes[12]==0x1b);
    SIT sit; sit.bytes(bytes); check(bytes.size()==10 && bytes[0]==0x7f);
    PSI_AVC_VideoDescriptor descriptor; descriptor.bytes(bytes);
    check(bytes.size()==6 && bytes[2]==0 && bytes[3]==0 && bytes[4]==0);
    PSI_Syntax syntax(1,0,false); syntax.bytes(bytes); check((bytes[2]&1)==0);
    std::cerr << "Logic checks passed\n";
}
