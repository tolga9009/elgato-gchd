#include "utility.hpp"
#include "firmware.hpp"
#include "gchd/settings.hpp"
#include "gchd/psi_pat.hpp"
#include "gchd/psi_pmt.hpp"
#include "gchd/psi_sit.hpp"
#include <iostream>
#include <stdexcept>
static void check(bool ok) { if (!ok) throw std::runtime_error("logic regression"); }
static TranscoderSettings settingsAtMode(Resolution resolution, ScanMode scanMode,
                                         double refreshRate, double requestedRate,
                                         float requestedLevel=0) {
    InputSettings input;
    input.setSource(InputSource::HDMI);
    input.setResolution(resolution);
    input.setScanMode(scanMode);
    input.setRefreshRate(refreshRate);
    TranscoderSettings requested, resolved;
    requested.setConstantBitRateMbps(5);
    requested.setFrameRate(requestedRate);
    requested.setH264Level(requestedLevel);
    resolved.mergeAutodetect(requested,input);
    return resolved;
}
int main() {
    FILE *empty=tmpfile(); check(empty!=nullptr);
    bool rejected=false;
    try { Firmware::validate(empty,"MB86H57_H58_IDLE"); } catch (const std::runtime_error &) { rejected=true; }
    fclose(empty); check(rejected);
    check(TranscoderSettings::unsignedH264Level(4.1f)==41);
    // The auto frame-rate sentinel must select the same level as its resolved rate.
    auto settingsAtRate = [](double refreshRate, double requestedRate) {
        return settingsAtMode(Resolution::HD720,ScanMode::Progressive,refreshRate,requestedRate);
    };
    for (double refreshRate: {60.0,30.0,50.0,25.0}) {
        auto automatic=settingsAtRate(refreshRate,0);
        auto explicitRate=settingsAtRate(refreshRate,refreshRate);
        check(automatic.getFrameRate()==0);
        check(automatic.getEffectiveFrameRate()==explicitRate.getEffectiveFrameRate());
        check(automatic.getH264Level()==explicitRate.getH264Level());
        check(TranscoderSettings::unsignedH264Level(automatic.getH264Level())==(refreshRate>30 ? 32u : 31u));
        auto halfRate=settingsAtRate(refreshRate,refreshRate/2);
        check(halfRate.getEffectiveFrameRate()==refreshRate/2);
        check(TranscoderSettings::unsignedH264Level(halfRate.getH264Level())==31);
    }
    for (double fieldRate: {60.0,50.0}) {
        auto automatic=settingsAtMode(Resolution::HD1080,ScanMode::Interlaced,fieldRate,0);
        auto explicitRate=settingsAtMode(Resolution::HD1080,ScanMode::Interlaced,fieldRate,fieldRate);
        auto frameEquivalent=settingsAtMode(Resolution::HD1080,ScanMode::Progressive,fieldRate/2,fieldRate/2);
        check(automatic.getFrameRate()==0 && automatic.getEffectiveFrameRate()==fieldRate);
        check(explicitRate.getFrameRate()==fieldRate && explicitRate.getEffectiveFrameRate()==fieldRate);
        check(automatic.getH264Level()==explicitRate.getH264Level());
        check(automatic.getH264Level()==frameEquivalent.getH264Level());
        check(TranscoderSettings::unsignedH264Level(automatic.getH264Level())==40);
        auto halfRate=settingsAtMode(Resolution::HD1080,ScanMode::Interlaced,fieldRate,fieldRate/2);
        check(halfRate.getEffectiveFrameRate()==fieldRate/2);
        check(TranscoderSettings::unsignedH264Level(halfRate.getH264Level())==40);
        auto explicitLevel=settingsAtMode(Resolution::HD1080,ScanMode::Interlaced,fieldRate,0,4.1f);
        check(TranscoderSettings::unsignedH264Level(explicitLevel.getH264Level())==41);
    }
    // Genuine progressive 1080p50/60 needs level 4.2; never silently cap it to
    // the supported 4.1 maximum. A requested 25/30 fps remains supported.
    for (double refreshRate: {60.0,50.0}) {
        for (double requestedRate: {0.0,refreshRate}) {
            bool unsupported=false;
            try { settingsAtMode(Resolution::HD1080,ScanMode::Progressive,refreshRate,requestedRate); }
            catch (const setting_error &error) { unsupported=std::string(error.what()).find("4.1")!=std::string::npos; }
            check(unsupported);
        }
        auto reduced=settingsAtMode(Resolution::HD1080,ScanMode::Progressive,refreshRate,refreshRate/2);
        check(reduced.getEffectiveFrameRate()==refreshRate/2);
        check(TranscoderSettings::unsignedH264Level(reduced.getH264Level())==40);
    }
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
