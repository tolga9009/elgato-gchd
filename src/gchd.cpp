/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <iostream>
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

void GCHD::stream(std::vector<unsigned char> *buffer, unsigned timeout) {
	buffer->resize(DATA_BUF); //Make sure full size.

	if (!isInitialized_) {
		return;
	}

	int transfer;

	libusb_bulk_transfer(devh_, 0x81, buffer->data(), static_cast<int>(buffer->size()), &transfer, timeout);

	buffer->resize(transfer); //Resize to transferred bytes.
	//libusb most certainly will partially fill a buffer than timeout
	//with this operation broken up into multiple transfers.
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
	 "./",
	 "/Applications/Game Capture HD.app/Contents/Resources/Firmware/Beddo/"};

	for (auto it : locationList) {
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
	libusb_ = libusb_init(nullptr);

	if (libusb_) {
		std::cerr << "Error initializing libusb." << std::endl;
		return 1;
	}

	// uncomment for verbose debugging
	//libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_DEBUG);

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD_0);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD_1);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD_2);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD_3);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHDNew;
		return 0;
	}

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD60);
	if (devh_) {
		deviceType_ = DeviceType::GameCaptureHD60;
		std::cerr << "The Elgato Game Capture HD60 is currently not supported." << std::endl;
		return 1;
	}

	devh_ = libusb_open_device_with_vid_pid(nullptr, VENDOR_ELGATO, GAME_CAPTURE_HD60_S);
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
	if (libusb_kernel_driver_active(devh_, INTERFACE_NUM)) {
		libusb_detach_kernel_driver(devh_, INTERFACE_NUM);
	}

	if (libusb_set_configuration(devh_, CONFIGURATION_VALUE)) {
		if (libusb_set_configuration(devh_, CONFIGURATION_VALUE)) {
			std::cerr << "Could not set configuration." << std::endl;
			return 1;
		}
	}

	if (libusb_claim_interface(devh_, INTERFACE_NUM)) {
		std::cerr << "Failed to claim interface." << std::endl;
		return 1;
	}

	return 0;
}

void GCHD::setupConfiguration() {
	// set device configuration
	std::cerr << "Initializing device." << std::endl;
	isInitialized_ = true;
	configureDevice();
}

void GCHD::closeDevice() {
	if (devh_) {
		if (isInitialized_) {
			uninitDevice();
		}

		libusb_release_interface(devh_, INTERFACE_NUM);
		libusb_close(devh_);
	}
}


GCHD::GCHD(Process *process, InputSettings inputSettings, TranscoderSettings transcoderSettings) {
	devh_ = nullptr;
	libusb_ = 1;
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

GCHD::~GCHD() {
	// uninit and close device
	closeDevice();

	// deinitialize libusb
	if (!libusb_) {
		libusb_exit(nullptr);
	}
}
