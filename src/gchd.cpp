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
#define GAME_CAPTURE_HD_3	0x005d // new revision GCHD - unsupported
#define GAME_CAPTURE_HD60	0x005c // Game Capture HD60 - unsupported
#define GAME_CAPTURE_HD60_S	0x004f // Game Capture HD60 S - unsupported

// firmware
constexpr auto FW_MB86H57_H58_IDLE = "mb86h57_h58_idle.bin";
constexpr auto FW_MB86H57_H58_ENC = "mb86h57_h58_enc_h.bin";

constexpr auto FW_MB86M01_ASSP_NSEC_IDLE = "MB86M01_ASSP_NSEC_IDLE";
constexpr auto FW_MB86M01_ASSP_NSEC_ENC = "MB86M01_ASSP_NSEC_ENC_H";

// constants
#define INTERFACE_NUM		0x00
#define CONFIGURATION_VALUE	0x01

int GCHD::checkDevice() {
	// initialize device handler
	if (openDevice()) {
		return 1;
	}
	libusb_reset_device(devh_);

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

	// activate process and configure device
	process_->setActive(true);
	setupConfiguration();

	return 0;
}

void GCHD::stream(std::array<unsigned char, DATA_BUF> *buffer) {
	if (!isInitialized_) {
		return;
	}

	int transfer;

	libusb_bulk_transfer(devh_, 0x81, buffer->data(), static_cast<int>(buffer->size()), &transfer, TIMEOUT);
}

int GCHD::checkFirmware() {
	std::string idleName;
	std::string encName;

	if (deviceType_ == DeviceType::GameCaptureHD) {
		idleName = FW_MB86H57_H58_IDLE;
		encName = FW_MB86H57_H58_ENC;
	}
	else if (deviceType_ == DeviceType::GameCaptureHDNew) {
		idleName = FW_MB86M01_ASSP_NSEC_IDLE;
		encName = FW_MB86M01_ASSP_NSEC_ENC;
	}
	else
	{
		throw std::logic_error( "Unsupported device.");
	}

	std::vector<std::string> locationList =
	{"/usr/lib/firmware/gchd/",
	 "/usr/local/lib/firmware/gchd/",
	 "./",
	 "/Applications/Game Capture HD.app/Contents/Resources/Firmware/Beddo/"};

	for (auto it : locationList) {
		std::string idle = it + idleName;
		std::string enc = it + encName;

		if (!access(idle.c_str(), F_OK)
				&& !access(enc.c_str(), F_OK)) {
			firmwareIdle_ = idle;
			firmwareEnc_ = enc;
			return 0;
		}
	}
	std::cerr << "Firmware files missing." << std::endl;
	std::cerr << "Need: " << idleName << std::endl;
	std::cerr << "Need: " << encName << std::endl;

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

	switch (settings_->getOutputResolution()) {
		case Resolution::NTSC:
			horizontalOutputResolution_=720;
			verticalOutputResolution_=480;
			break;

		case Resolution::PAL:
			horizontalOutputResolution_=720;
			verticalOutputResolution_=480;
			break;

		case Resolution::HD720:
			horizontalOutputResolution_=1280;
			verticalOutputResolution_=720;
			break;

		case Resolution::HD1080:
			horizontalOutputResolution_=1920;
			verticalOutputResolution_=1080;
			break;

		default:
			throw runtime_error("Unsupported configuration.");
			break;
	}
	configureDevice();
}

void GCHD::closeDevice() {
	if (devh_) {
		if (isInitialized_) {
			std::cerr << "Resetting device - this may take a while." << std::endl;
			uninitDevice();
			std::cerr << "Device has been reset." << std::endl;
		}

		libusb_release_interface(devh_, INTERFACE_NUM);
		libusb_close(devh_);
	}
}


GCHD::GCHD(Process *process, Settings *settings) {
	devh_ = nullptr;
	libusb_ = 1;
	isInitialized_ = false;
	deviceType_ = DeviceType::Unknown;
	process_ = process;
	settings_ = settings;
}

GCHD::~GCHD() {
	// uninit and close device
	closeDevice();

	// deinitialize libusb
	if (!libusb_) {
		libusb_exit(nullptr);
	}
}
