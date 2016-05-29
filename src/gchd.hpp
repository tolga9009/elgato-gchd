/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef GCHD_H
#define GCHD_H

#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <libusb-1.0/libusb.h>

#include <gchd/settings.hpp>
#include <process.hpp>

// constants
#define EP_OUT		0x02
#define DATA_BUF	0xf000
#define TIMEOUT		5000
#define MAX_QUEUE	3

// system commands
#define SCMD_IDLE		1
#define SCMD_INIT		4
#define SCMD_STATE_CHANGE	5
#define SCMD_STATE_STOP		0x0001
#define SCMD_STATE_START	0x0002
#define SCMD_STATE_NULL		0x0004

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
		std::condition_variable *getCv();
		std::mutex *getMutex();
		std::queue<std::array<unsigned char, DATA_BUF>> *getQueue();
		GCHD(Process *process, Settings *settings);
		~GCHD();

	private:
		int libusb_;
		bool isInitialized_;
		std::condition_variable cv_;
		std::mutex mutex_;
		std::queue<std::array<unsigned char, DATA_BUF>> queue_;
		std::string firmwareIdle_;
		std::string firmwareEnc_;
		std::thread writerThread_;
		struct libusb_device_handle *devh_;
		int checkFirmware();
		int openDevice();
		int getInterface();
		void setupConfiguration();
		void bootDevice();
		void stream(std::array<unsigned char, DATA_BUF> *buffer);
		void writer();
		void closeDevice();
		void configureComposite480i();
		void configureComposite576i();
		void configureHdmi720p();
		void configureHdmi720pRgb();
		void configureHdmi1080p();
		void configureHdmi1080pRgb();
		void configureComponent480p();
		void configureComponent576p();
		void configureComponent720p();
		void configureComponent1080i();
		void configureComponent1080p();
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
		Process *process_;
		Settings *settings_;
};

#endif
