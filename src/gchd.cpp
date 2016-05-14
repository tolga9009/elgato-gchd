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

#include <gchd.hpp>

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

// constants
#define INTERFACE_NUM		0x00
#define CONFIGURATION_VALUE	0x01

int GCHD::init() {
	// initialize device handler
	if (openDevice()) {
		return 1;
	}

	// check for firmware files
	if (checkFirmware()) {
		return 1;
	}

	// detach kernel driver and claim interface
	if (getInterface()) {
		return 1;
	}

	return 0;
}

void GCHD::stream(unsigned char *data, int length) {
	// this function requires an initialized device
	if (!isInitialized_) {
		initializeDevice();
	}

	int transfer;

	libusb_bulk_transfer(devh_, 0x81, data, length, &transfer, TIMEOUT);
}

int GCHD::checkFirmware() {
	// TODO check in /usr/lib/firmware, /usr/local/lib/firmware and ./
	if (deviceType_ == DeviceType::GameCaptureHD) {
		std::vector<std::string> list = {"/usr/lib/firmware/gchd/", "/usr/local/lib/firmware/gchd/", "./"};

		for (auto it : list) {
			std::string idle = it + FW_MB86H57_H58_IDLE;
			std::string enc = it + FW_MB86H57_H58_ENC;

			if (!access(idle.c_str(), F_OK)
					&& !access(enc.c_str(), F_OK)) {
				firmwareIdle_ = idle;
				firmwareEnc_ = enc;
				return 0;
			}
		}
	}

	std::cerr << "Firmware files missing." << std::endl;

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
		std::cerr << "This revision of the Elgato Game Capture HD is currently not supported." << std::endl;
		return 1;
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

	return 1;
}

int GCHD::getInterface() {
	if (libusb_kernel_driver_active(devh_, INTERFACE_NUM)) {
		libusb_detach_kernel_driver(devh_, INTERFACE_NUM);
	}

	if (libusb_set_configuration(devh_, CONFIGURATION_VALUE)) {
		std::cerr << "Could not set configuration." << std::endl;
		return 1;
	}

	if (libusb_claim_interface(devh_, INTERFACE_NUM)) {
		std::cerr << "Failed to claim interface." << std::endl;
		return 1;
	}

	return 0;
}

void GCHD::initializeDevice() {
	// set device configuration
	std::cerr << "Initializing device." << std::endl;
	isInitialized_ = true;

	switch (settings_->getInputSource()) {
		case InputSource::Composite: configure_dev_576i(); break;
		case InputSource::Component:
			switch (settings_->getResolution()) {
				case Resolution::Standard: configure_dev_component_576p(); break;
				case Resolution::HD720: configure_dev_component_720p(); break;
				case Resolution::HD1080: configure_dev_component_1080p(); break;
				default: return;
			}
			break;
		case InputSource::HDMI:
			switch (settings_->getResolution()) {
				case Resolution::HD720:
					switch (settings_->getColorSpace()) {
						case ColorSpace::YUV: configure_dev_720p(); break;
						case ColorSpace::RGB: configure_dev_720p_rgb(); break;
						default: return;
					}
					break;
				case Resolution::HD1080:
					switch (settings_->getColorSpace()) {
						case ColorSpace::YUV: configure_dev_1080p(); break;
						case ColorSpace::RGB: configure_dev_1080p_rgb(); break;
						default: return;
					}
					break;
				default: return;
			}
			break;
		default: return;
	}
}

void GCHD::closeDevice() {
	if (devh_) {
		if (isInitialized_) {
			std::cerr << "Your device is going to be reset. Please wait and do not interrupt or unplug your device." << std::endl;
			uninitDevice();
			std::cerr << "Device has been reset." << std::endl;
		}

		libusb_release_interface(devh_, INTERFACE_NUM);
		libusb_close(devh_);
	}
}


GCHD::GCHD(Settings *settings) {
	devh_ = nullptr;
	libusb_ = 1;
	isInitialized_ = false;
	deviceType_ = DeviceType::Unknown;
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
