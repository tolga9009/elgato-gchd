/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "gchd.hpp"

// USB VID & PIDs
#define VENDOR_ELGATO		0x0fd9
#define GAME_CAPTURE_HD_0	0x0044
#define GAME_CAPTURE_HD_1	0x004e
#define GAME_CAPTURE_HD_2	0x0051
#define GAME_CAPTURE_HD_3	0x005d // new revision GCHD (HDNew)
#define GAME_CAPTURE_HD60	0x005c // Game Capture HD60 - unsupported
#define GAME_CAPTURE_HD60_S	0x004f // Game Capture HD60 S - unsupported

// firmware
const char * FW_MB86H57_H58_IDLE[] =
{"MB86H57_H58_IDLE",
 "mb86h57_h58_idle.bin" };
const char * FW_MB86H57_H58_ENC[] =
{ "MB86H57_H58_ENC_H",
  "mb86h57_h58_enc_h.bin" };

const char * FW_MB86M01_ASSP_NSEC_IDLE[] =
{"MB86M01_ASSP_NSEC_IDLE",
 "mb86m01_assp_nsec_idle.bin" };
const char * FW_MB86M01_ASSP_NSEC_ENC[] =
{"MB86M01_ASSP_NSEC_ENC_H",
 "mb86m01_assp_nsec_enc_h.bin" };

// constants
#define INTERFACE_NUM		0x00
#define CONFIGURATION_VALUE	0x01

int GCHD::checkDevice() {
	// initialize device handler
	if (openDevice()) {
		return 1;
	}

	// check for firmware files
	if (checkFirmware()) {
		return 1;
	}
	return 0;
}


int GCHD::init() {
	// detach kernel driver and claim interface
	if (getInterface()) {
		return 1;
	}

	// configure device
	setupConfiguration();

	return 0;
}

void GCHD::checkUsbOperation() {
    if (deviceGone_) throw usb_error("USB device disconnected (LIBUSB_ERROR_NO_DEVICE)");
    if (shuttingDown_) {
        if (std::chrono::steady_clock::now() >= shutdownDeadline_) throw usb_error("Device shutdown deadline expired");
    } else if (!Process::isActive()) throw usb_error("USB operation interrupted");
}

void GCHD::usbFailure(int code, const std::string &operation) {
    if (code == LIBUSB_ERROR_NO_DEVICE) deviceGone_ = true;
    throw usb_error(std::string(operation) + ": " + libusb_error_name(code));
}

int GCHD::controlTransfer(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, unsigned char *data, uint16_t length) {
    checkUsbOperation();
    const int result=libusb_control_transfer(devh_, requestType, request, value, index, data, length, 1000);
    if (result < 0) {
        std::ostringstream operation;
        operation << "USB control transfer (type=0x" << std::hex << unsigned(requestType)
                  << ", request=0x" << unsigned(request) << ", value=0x" << value
                  << ", index=0x" << index << std::dec << ", length=" << length << ')';
        usbFailure(result, operation.str());
    }
    return result;
}

void GCHD::stream(std::vector<unsigned char> *buffer, unsigned timeout) {
    buffer->clear();
    if (!interfaceClaimed_) return;
    checkUsbOperation();
    buffer->resize(DATA_BUF);
    int transferred = 0;
    const int result=libusb_bulk_transfer(devh_, 0x81, buffer->data(), DATA_BUF, &transferred, timeout ? timeout : TIMEOUT);
    if (result == LIBUSB_ERROR_OVERFLOW) { ++overflows_; buffer->clear(); return; }
    if (result == LIBUSB_ERROR_TIMEOUT) ++timeouts_;
    if (result != 0 && result != LIBUSB_ERROR_TIMEOUT) {
        buffer->clear(); usbFailure(result, "USB streaming read");
    }
    if (transferred < 0 || transferred > DATA_BUF) { buffer->clear(); throw usb_error("Invalid USB read length"); }
    buffer->resize(static_cast<size_t>(transferred));
    if (buffer->empty() && result == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void GCHD::resetDevice() {
    if (openDevice()) throw usb_error("Cannot open device for reset");
    checkUsbOperation();
    const int result=libusb_reset_device(devh_);
    if (result < 0) usbFailure(result, "USB reset (WSL may require usbipd reattach)");
    std::cerr << "USB reset requested. If the device re-enumerates, reattach it with usbipd before capture.\n";
}

int GCHD::checkFirmware() {
	const char **idleNames;
	const char **encNames;
	unsigned nameCount;

	if (deviceType_ == DeviceType::GameCaptureHD) {
		idleNames = FW_MB86H57_H58_IDLE;
		encNames = FW_MB86H57_H58_ENC;
		nameCount = sizeof(FW_MB86H57_H58_IDLE)/sizeof(const char *);
	}
	else if (deviceType_ == DeviceType::GameCaptureHDNew) {
		idleNames = FW_MB86M01_ASSP_NSEC_IDLE;
		encNames = FW_MB86M01_ASSP_NSEC_ENC;
		nameCount = sizeof(FW_MB86M01_ASSP_NSEC_IDLE)/sizeof(const char *);
	}
	else {
		throw std::logic_error( "Unsupported device.");
	}

	std::vector<std::string> locationList =
	{"/usr/lib/firmware/gchd/",
	 "/usr/local/lib/firmware/gchd/",
	 "/Applications/Game Capture HD.app/Contents/Resources/Firmware/Beddo/"};

    const char *env=std::getenv("GCHD_FIRMWARE_DIR");
    const std::string overrideDirectory=!firmwareDirectory_.empty() ? firmwareDirectory_ : (env ? env : "");
    if (!overrideDirectory.empty()) locationList={overrideDirectory + "/"};
    for (const auto &it : locationList) {
		for ( unsigned i=0 ; i<nameCount; ++i ) {
			std::string idle = it + idleNames[i];
			std::string enc = it + encNames[i];

			if (!access(idle.c_str(), F_OK)
					&& !access(enc.c_str(), F_OK)) {
				firmwareIdle_ = idle;
				firmwareEnc_ = enc;
				return 0;
			}
		}
	}
	std::cerr << "Firmware files missing." << std::endl;
	for ( unsigned i=0 ; i<nameCount; ++i ) {
		std::cerr << "Need: " << idleNames[i] << std::endl;
		std::cerr << "Need: " << encNames[i] << std::endl;
		if( i < (nameCount-1)) {
			std::cerr << "-------OR-------" << std::endl;
		}
	}
	return 1;
}

int GCHD::openDevice() {
	const int result = libusb_init(&context_);

	if (result) {
		std::cerr << "Error initializing libusb." << std::endl;
		return 1;
	}

	// uncomment for verbose debugging
	//libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_DEBUG);

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD_0);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD_1);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD_2);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD_3);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHDNew;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD60);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD60;
		std::cerr << "The Elgato Game Capture HD60 is currently not supported." << std::endl;
		return 1;
	}

	devh_ = libusb_open_device_with_vid_pid(context_, VENDOR_ELGATO, GAME_CAPTURE_HD60_S);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD60S;
		std::cerr << "The Elgato Game Capture HD60 S is currently not supported." << std::endl;
		return 1;
	}

	std::cerr << "Unable to find a supported device." << std::endl;
	if( geteuid() != 0 ) {
		std::cerr << "This may be because you are not running this as root, or have not configured UDEV properly to run this without root privilege." << std::endl;
	}

	return 1;
}

int GCHD::getInterface() {
    const int detach=libusb_set_auto_detach_kernel_driver(devh_, 1);
    if (detach < 0 && detach != LIBUSB_ERROR_NOT_SUPPORTED) usbFailure(detach, "Enable automatic kernel driver detach");

    // Reselecting an active configuration resets endpoint state; preserve it.
    int configuration=0;
    int result=libusb_get_configuration(devh_,&configuration);
    if (result < 0) usbFailure(result,"Read USB configuration");
    if (configuration != CONFIGURATION_VALUE) {
        result=libusb_set_configuration(devh_,CONFIGURATION_VALUE);
        if (result < 0) usbFailure(result,"Set USB configuration");
    }
    result=libusb_claim_interface(devh_,INTERFACE_NUM);
    if (result < 0) usbFailure(result,"Claim USB interface");

	interfaceClaimed_ = true;
	return 0;
}

void GCHD::setupConfiguration() {
	// set device configuration
	std::cerr << "Initializing device." << std::endl;
	configureDevice();
	isInitialized_ = true;
}

void GCHD::closeDevice() noexcept {
	shuttingDown_ = true;
	shutdownDeadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	if (devh_) {
		if (isInitialized_ && !deviceGone_) {
			uninitDevice();
		}

		if (interfaceClaimed_) libusb_release_interface(devh_, INTERFACE_NUM);
		libusb_close(devh_);
		devh_ = nullptr;
		interfaceClaimed_ = false;
		isInitialized_ = false;
	}
}


GCHD::GCHD(Process *process, InputSettings inputSettings, TranscoderSettings transcoderSettings) {
	devh_ = nullptr;
	savedEnableRegister_ = 0;
	savedEnableStateRegister_ = 0;
	specialDetectMask_ = 0xffff;
	isInitialized_ = false;
	deviceType_ = DeviceType::Unknown;
	process_ = process;

	//Yes, these are copies.
	passedInputSettings_ = inputSettings;
	currentInputSettings_ = inputSettings;

	passedTranscoderSettings_ = transcoderSettings;
	currentTranscoderSettings_ = transcoderSettings;

	// activate process
	process->setActive(true);
}

GCHD::~GCHD() noexcept {
	// uninit and close device
	closeDevice();

	// deinitialize libusb
	if (context_) {
		libusb_exit(context_);
	}
}
