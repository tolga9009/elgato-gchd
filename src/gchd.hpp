/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef GCHD_H
#define GCHD_H

#include <cstdint>
#include <string>

#include <libusb-1.0/libusb.h>

#include <gchd/settings.hpp>

// endpoints
#define EP_OUT		0x02
#define DATA_BUF	0x4000
#define TIMEOUT		5000

// system commands
#define SCMD_IDLE		1
#define SCMD_INIT		4
#define SCMD_STATE_CHANGE	5

enum class DeviceType {
	Unknown,
	GameCaptureHD,
	GameCaptureHDNew,
	GameCaptureHD60,
	GameCaptureHD60S
};

class GCHD {
	public:
		int init();
		void stream(unsigned char *data, int length);
		GCHD(Settings *settings);
		~GCHD();

	private:
		int libusb_;
		bool isInitialized_;
		std::string firmwareIdle_;
		std::string firmwareEnc_;
		struct libusb_device_handle *devh_;
		int checkFirmware();
		int openDevice();
		int getInterface();
		void initializeDevice();
		void closeDevice();
		void configure_dev_mode();
		void configure_dev_composite_480i();
		void configure_dev_composite_576i();
		void configure_dev_hdmi_720p();
		void configure_dev_hdmi_720p_rgb();
		void configure_dev_hdmi_1080p();
		void configure_dev_hdmi_1080p_rgb();
		void configure_dev_component_480p();
		void configure_dev_component_576p();
		void configure_dev_component_720p();
		void configure_dev_component_1080i();
		void configure_dev_component_1080p();
		void read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);
		int read_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3);
		void write_config2(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1);
		void write_config3(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2);
		void write_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3);
		void write_config5(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4);
		void write_config6(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5);
		void write_config8(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7);
		void scmd(uint8_t command, uint8_t mode, uint16_t data);
		void sparam(uint16_t wIndex, uint8_t shift, uint8_t range, uint16_t data);
		void slsi(uint16_t wIndex, uint16_t data);
		void dlfirm(const char *file);
		void receiveData();
		void uninitDevice();
		DeviceType deviceType_;
		Settings *settings_;
};

#endif
