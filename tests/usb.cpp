#include "gchd.hpp"
#include "streamer.hpp"
#include <cstdio>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
struct UsbTestAccess {
    static void arm(GCHD &g) { g.interfaceClaimed_=true; g.devh_=reinterpret_cast<libusb_device_handle *>(1); }
    static std::string revision(GCHD &g) { std::vector<unsigned char> bytes; g.readVersion(bytes); return std::string(bytes.begin(),bytes.end()-1); }
    static int claim(GCHD &g) { g.devh_=reinterpret_cast<libusb_device_handle *>(1); return g.getInterface(); }
    static void initialized(GCHD &g) { g.isInitialized_=true; }
    static bool isInitialized(const GCHD &g) { return g.isInitialized_; }
    static void startAndConfirm(GCHD &g) {
        g.deviceType_=DeviceType::GameCaptureHD;
        g.scmd(SCMD_STATE_CHANGE,0,SCMD_STATE_START);
        g.completeStateChange(SCMD_STATE_STOP,SCMD_STATE_START);
    }
    static void firmware(GCHD &g, const char *path) { FILE *file=fopen(path,"rb");
        if (!file) { g.dlfirm(path); return; }
        try { g.uploadFirmware(file,16); } catch (...) { fclose(file); throw; }
        fclose(file); }
};
static int code=0, length=0, calls=0;
static int versionLayout=0;
static int controlCalls=0, closes=0, releases=0;
static int startFailure=0, stopAttempts=0, cleanupReads=0;
static bool startFixture=false, failStartWrite=false, cancelAfterStart=false, cleanupFixture=false;
static int configuration=1, configurationResult=0, setConfigurationCalls=0;
extern "C" int __wrap_libusb_set_auto_detach_kernel_driver(libusb_device_handle *, int) { return 0; }
extern "C" int __wrap_libusb_get_configuration(libusb_device_handle *, int *current) { *current=configuration; return configurationResult; }
extern "C" int __wrap_libusb_set_configuration(libusb_device_handle *, int next) { ++setConfigurationCalls; configuration=next; return 0; }
extern "C" int __wrap_libusb_claim_interface(libusb_device_handle *, int) { return 0; }
extern "C" int __wrap_libusb_bulk_transfer(libusb_device_handle *, unsigned char, unsigned char *, int, int *transferred, unsigned timeout) {
    if (!timeout) throw std::runtime_error("infinite timeout");
    ++calls;
    if (code != LIBUSB_ERROR_NO_DEVICE) *transferred=length;
    return code;
}
extern "C" int __wrap_libusb_control_transfer(libusb_device_handle *, uint8_t type, uint8_t request, uint16_t value, uint16_t index, unsigned char *data, uint16_t size, unsigned timeout) {
    if (!timeout) throw std::runtime_error("infinite timeout");
    ++controlCalls;
    if (startFixture) {
        if (cleanupFixture) {
            if (type==0xc0 && request==0xbc && value==0x0800 && index==0x2008 && size==2) {
                ++cleanupReads;
                data[0]=0; data[1]=SCMD_STATE_START;
                return size;
            }
            if (type==0x40 && request==0xb8 && size==6 && data[2]==SCMD_STATE_CHANGE && data[5]==SCMD_STATE_NULL) {
                ++stopAttempts;
                // Cleanup itself can fail and must remain bounded and noexcept.
                return LIBUSB_ERROR_IO;
            }
            throw std::runtime_error("Unexpected cleanup operation");
        }
        if (type==0x40 && request==0xb8 && size==6 && data[2]==SCMD_STATE_CHANGE && data[5]==SCMD_STATE_START) {
            if (cancelAfterStart) Process::setActive(false);
            return failStartWrite ? startFailure : size;
        }
        return startFailure;
    }
    if (versionLayout && type==0xc0 && request==0xbc && value==0x0800 && size==4) {
        if (versionLayout==1 && (index==0x94 || index==0x98)) { std::memset(data,0,4); return 4; }
        const uint16_t base=versionLayout==1 ? 0x10 : 0x94;
        const unsigned char chunks[3][4]={{'V','1','1','0'},{'5','0','9','0'},{'0',0x1a,0,0}};
        if (index>=base && index<=base+8 && (index-base)%4==0) { std::memcpy(data,chunks[(index-base)/4],4); return 4; }
    }
    return LIBUSB_ERROR_NO_DEVICE;
}
extern "C" void __wrap_libusb_close(libusb_device_handle *) { ++closes; }
extern "C" int __wrap_libusb_release_interface(libusb_device_handle *, int) { ++releases; return 0; }
static void check(bool ok) { if (!ok) throw std::runtime_error("USB regression"); }
template<class F> void throws(F f) { bool caught=false; try { f(); } catch (const usb_error &) { caught=true; } check(caught); }
int main() {
    Process process; Process::setActive(true);
    // Native trace fragments: "V110", "5090", "0". The third register must not overwrite digit five.
    for (versionLayout=1; versionLayout<=2; ++versionLayout) {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
        check(UsbTestAccess::revision(g)=="V11050900");
    }
    versionLayout=0;
    { GCHD g(&process,InputSettings(),TranscoderSettings()); check(UsbTestAccess::claim(g)==0); check(setConfigurationCalls==0); }
    configuration=0;
    { GCHD g(&process,InputSettings(),TranscoderSettings()); check(UsbTestAccess::claim(g)==0); check(setConfigurationCalls==1 && configuration==1); }
    configurationResult=LIBUSB_ERROR_NO_DEVICE;
    { GCHD g(&process,InputSettings(),TranscoderSettings()); throws([&] { UsbTestAccess::claim(g); }); check(setConfigurationCalls==1); }
    configurationResult=0;
    // Initialization has sent START but has not yet marked itself successful.
    // A confirmation failure or ambiguous failed START still needs best-effort
    // stop; a confirmed disconnect must never issue more device commands.
    startFixture=true;
    struct StartFailureScenario { int result; bool failedWrite, cancelled; };
    const StartFailureScenario scenarios[]={
        {LIBUSB_ERROR_TIMEOUT,false,false},
        {LIBUSB_ERROR_IO,false,false},
        {LIBUSB_ERROR_TIMEOUT,false,true},
        {LIBUSB_ERROR_NO_DEVICE,false,false},
        {LIBUSB_ERROR_TIMEOUT,true,false},
        {LIBUSB_ERROR_NO_DEVICE,true,false}
    };
    for (const auto &scenario: scenarios) {
        startFailure=scenario.result;
        cancelAfterStart=scenario.cancelled;
        failStartWrite=scenario.failedWrite;
        cleanupFixture=false; stopAttempts=0; cleanupReads=0;
        const int beforeCloses=closes, beforeReleases=releases;
        const auto start=std::chrono::steady_clock::now();
        {
            GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
            throws([&] { UsbTestAccess::startAndConfirm(g); });
            check(!UsbTestAccess::isInitialized(g));
            cleanupFixture=true;
        }
        check(std::chrono::steady_clock::now()-start<std::chrono::seconds(1));
        const bool disconnected=startFailure==LIBUSB_ERROR_NO_DEVICE;
        check(cleanupReads==(disconnected ? 0 : 1));
        check(stopAttempts==(disconnected ? 0 : 1));
        check(closes==beforeCloses+1 && releases==beforeReleases+1);
    }
    startFixture=false;
    // Failures before START (for example, firmware upload) must not attempt
    // protocol teardown on a device which never reached streaming setup.
    {
        const int beforeControls=controlCalls;
        {
            GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
            throws([&] { UsbTestAccess::firmware(g,"/nonexistent/gchd-firmware"); });
        }
        check(controlCalls==beforeControls);
    }
    UsbPollBudget attempts("test completion",std::chrono::seconds(1),1);
    attempts.check(false);
    try { attempts.check(false); check(false); }
    catch(const usb_error &e) { check(std::string(e.what()).find("test completion")!=std::string::npos); }
    UsbPollBudget expired("test deadline",std::chrono::milliseconds(0));
    throws([&] { expired.check(false); });
    Process::setActive(false); UsbPollBudget cancelled;
    throws([&] { cancelled.check(false); });
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
        std::vector<unsigned char> b(10,0xab);
        code=LIBUSB_ERROR_TIMEOUT; length=188; g.stream(&b); check(b.size()==188);
        length=0; g.stream(&b); check(b.empty());
        code=LIBUSB_ERROR_OVERFLOW; length=16384; g.stream(&b); check(b.empty());
        code=LIBUSB_ERROR_IO; throws([&] { g.stream(&b); }); check(b.empty());
        code=LIBUSB_ERROR_NO_DEVICE; throws([&] { g.stream(&b); }); check(b.empty());
        const int before=calls; throws([&] { g.stream(&b); }); check(calls==before);
    }
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g); UsbTestAccess::initialized(g);
        // Destructor encounters a disconnected device during control transfer and must not throw.
    }
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
        throws([&] { UsbTestAccess::firmware(g,"/nonexistent/gchd-firmware"); });
        char path[]="/tmp/gchd-test-XXXXXX"; int fd=mkstemp(path); check(fd>=0);
        char data[16]{}; check(write(fd,data,sizeof data)==sizeof data); close(fd);
        code=0; length=1; throws([&] { UsbTestAccess::firmware(g,path); });
        code=LIBUSB_ERROR_TIMEOUT;
        try { UsbTestAccess::firmware(g,path); check(false); }
        catch (const usb_error &e) {
            const std::string message=e.what();
            check(message.find("offset 0")!=std::string::npos && message.find("transferred=1")!=std::string::npos);
        }
        code=0; length=16; const int before=calls; UsbTestAccess::firmware(g,path); check(calls==before+1);
        unlink(path);
        code=LIBUSB_ERROR_TIMEOUT; length=0;
        Streamer streamer(&g,&process,StreamOutput::Disk); check(streamer.disk.enable("/dev/null")==0);
        const auto start=std::chrono::steady_clock::now(); streamer.loop(0.02);
        check(std::chrono::steady_clock::now()-start<std::chrono::seconds(1));
        Process::setActive(false); throws([&] { std::vector<unsigned char> b; g.stream(&b); });
    }
}
