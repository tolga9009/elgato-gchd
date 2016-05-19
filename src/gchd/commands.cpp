/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <vector>

#include <gchd.hpp>

void GCHD::read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	std::vector<unsigned char> recv(wLength);

	libusb_control_transfer(devh_, 0xc0, bRequest, wValue, wIndex, recv.data(), recv.size(), 0);
}

int GCHD::read_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3) {
	std::vector<unsigned char> recv(4);

	libusb_control_transfer(devh_, 0xc0, bRequest, wValue, wIndex, recv.data(), recv.size(), 0);

	if (recv[0] == data0 && recv[1] == data1 && recv[2] == data2 && recv[3] == data3) {
		return 1;
	}

	return 0;
}

void GCHD::write_config2(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1) {
	unsigned char send[2] = {data0, data1};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 2, 0);
}

void GCHD::write_config3(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2) {
	unsigned char send[3] = {data0, data1, data2};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 3, 0);
}

void GCHD::write_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3) {
	unsigned char send[4] = {data0, data1, data2, data3};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 4, 0);
}

void GCHD::write_config5(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4) {
	unsigned char send[5] = {data0, data1, data2, data3, data4};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 5, 0);
}

void GCHD::write_config6(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5) {
	unsigned char send[6] = {data0, data1, data2, data3, data4, data5};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 6, 0);
}

void GCHD::write_config8(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
	unsigned char send[8] = {data0, data1, data2, data3, data4, data5, data6, data7};
	libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send, 8, 0);
}

uint8_t count_bits(uint16_t data) {
	uint8_t i = 0;

	for (i = 0; data; i++) {
		data &= data - 1;
	}

	return i;
}

/**
 * Reverse engineered function from official drivers. Used to set device states
 * and modes. scmd() is the short form for system command.
 *
 * @param command there are currently three commands, we have identified. A
 *  value of 1 stands for IDLE command. A value of 4 means INIT and is used for
 *  loading firmwares onto the device. A value of 5 means STATE_CHANGE and is
 *  used to control the device's encoding procedure.
 * @param mode there is only one use case known so far. It's for setting
 *  encoding mode.
 * @param data applies to send[4] and send[5]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds data to further specify
 *  the command parameter. In conjunction with STATE_CHANGE, it is used to set
 *  specific encoding states: 1 means STOP, 2 means START and 4 means NULL.
 *  Setting STATE_CHANGE to NULL will make the device output an empty data
 *  stream during the encoding process.
 */
void GCHD::scmd(uint8_t command, uint8_t mode, uint16_t data) {
	uint8_t send[6] = {0};
	send[2] = command;
	send[3] = mode;

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[4] = data >> 8;
	send[5] = data & 0xff;

	libusb_control_transfer(devh_, 0x40, 0xb8, 0x0000, 0x0000, send, 6, 0);
}

/**
 * Reverse engineered function from official drivers. Used to set specific
 * device parameters, like H.264 profile, video size and audio bitrate.
 *
 * @param wIndex offset, where requests are passed to.
 * @param shift bit shift left operations to the whole send[8] array. If any
 *  bit gets cut off, substracting wIndex by 1 will be equal to a bit shift
 *  right operation of 8. It's used for readibility. Minimum value is 0 and
 *  maximum is 16.
 * @param range applies to send[4] and send[5]. Sets n bits to 1, starting from
 *  the first bit in send[5]. If it's greater than 8, bits are set in send[4].
 *  Minimum value is 0 and maximum is 16.
 * @param data applies to send[0] and send[1]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds the actual data, like video
 *  vertical size and bitrate.
 */
void GCHD::sparam(uint16_t wIndex, uint8_t shift, uint8_t range, uint16_t data) {
	uint8_t send[8] = {0};

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[0] = data >> 8;
	send[1] = data & 0xff;

	/*
	 * to make the for-loop simpler, we're using a temporary 16-bit integer
	 * instead of implementing a for-loop, which takes care of two 8-bit
	 * integers. If you have found a way, how to do it more efficiently or
	 * better, please share with us.
	 */
	uint16_t range_bits = 0;

	for (uint8_t i = 0; i <= range; i++) {
		range_bits |= 1 << i;
	}

	send[4] = range_bits >> 8;
	send[5] = range_bits & 0xff;

	/*
	 * we need to bit shift left the whole array by the value of shift. First of
	 * all, we need to check, if any overflow would happen in send[0]. If yes,
	 * we need to subtract wIndex by 1, which equals to a bit shift right of 8.
	 * Next, we need to check, if an overflow would happen in send[1]. If yes,
	 * we need to subtract wIndex by another 1. We now have a total right shift
	 * of 16. This is the maximum value per definition, we don't need to check
	 * for more overflows.
	 */
	while (count_bits(data) != count_bits(data << shift) && send[7] == 0) {
		// right cycling send[8] array
		for (uint8_t i = 7; i > 0; i--) {
			send[i] = send[i - 1];
		}

		send[0] = 0;

		// to compensate this, we decrease wIndex by 1 and right shift data by 8
		wIndex--;
		data >>= 8;
	}

	uint64_t data_shifted = 0;

	for (uint8_t i = 0; i < 8; i++) {
		data_shifted |= send[i] << (8 * (7 - i));
	}

	data_shifted <<= shift;

	for (uint8_t i = 0; i < 8; i++) {
		send[i] = (data_shifted >> (8 * (7 - i))) & 0xff;
	}

	libusb_control_transfer(devh_, 0x40, 0xbc, 0x0001, wIndex, send, 8, 0);
}

/**
 * Reverse engineered function from official drivers. Used to configure the
 * device.
 *
 * @param wIndex offset, where requests are passed to.
 * @param data applies to send[0] and send[1]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds the actual data, like video
 *  vertical size and bitrate.
 */
void GCHD::slsi(uint16_t wIndex, uint16_t data) {
	uint8_t send[2] = {0};

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[0] = data >> 8;
	send[1] = data & 0xff;

	libusb_control_transfer(devh_, 0x40, 0xbc, 0x0000, wIndex, send, 2, 0);
}

/**
 * Loads firmware to the device
 *
 * @param file relative path to binary firmware file
 */
void GCHD::dlfirm(const char *file) {
	int transfer;

	FILE *bin;
	bin = fopen(file, "rb");

	// get filesize
	fseek(bin, 0L, SEEK_END);
	long filesize = ftell(bin);
	rewind(bin);

	// read firmware from file to buffer and bulk transfer to device
	for (int i = 0; i <= filesize; i += DATA_BUF) {
		unsigned char data[DATA_BUF] = {0};
		int bytes_remain = filesize - i;

		if ((bytes_remain) > DATA_BUF) {
			bytes_remain = DATA_BUF;
		}

		fread(data, bytes_remain, 1, bin);

		libusb_bulk_transfer(devh_, EP_OUT, data, bytes_remain, &transfer, 0);
	}

	fclose(bin);
}

void GCHD::uninitDevice() {
	// state change - output null
	scmd(SCMD_STATE_CHANGE, 0x00, 0x0004);

	// usually, this is done by a seperate thread, which runs in parallel.
	// I'm just quickly hacking a way around it, to test some stuff. Taking
	// a big, random number, which is slightly based on the USB traffic logs
	// I'm working with. Receive empty data, after setting state change to
	// null transfer.
	for (int i = 0; i < 5000; i++) {
		receiveData();
	}

	// we probably need some sleeps here and receive null output, to give
	// the device enough time to gracefully reset. Else, the next scmd()
	// command seems to be ignored and we're leaving the device in an
	// undefined state.
	read_config(0xbc, 0x0800, 0x2008, 2);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	read_config(0xbc, 0x0900, 0x0074, 2);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	read_config(0xbc, 0x0900, 0x01b0, 2);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	read_config(0xbc, 0x0800, 0x2008, 2);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	read_config(0xbc, 0x0800, 0x2008, 2);


	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	write_config2(0xbc, 0x0900, 0x0074, 0x00, 0x04);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	write_config2(0xbc, 0x0900, 0x01b0, 0x00, 0x00);

	for (int i = 0; i < 50; i++) {
		receiveData();
	}

	// state change - stop encoding
	scmd(SCMD_STATE_CHANGE, 0x00, 0x0001);

	for (int i = 0; i < 5; i++) {
		receiveData();
	}

	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0900, 0x0074, 2);
	read_config(0xbc, 0x0900, 0x01b0, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config2(0xbc, 0x0900, 0x0074, 0x00, 0x04);
	write_config2(0xbc, 0x0900, 0x01b0, 0x00, 0x00);
	write_config2(0xbd, 0x0000, 0x4400, 0x06, 0x86);
	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x1e);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config3(0xbd, 0x0000, 0x3300, 0x89, 0x89, 0xf8);
	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x1e);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x1e);

	read_config(0xbc, 0x0900, 0x001c, 2);
	read_config(0xbd, 0x0000, 0x3300, 1);

	write_config2(0xbd, 0x0000, 0x4400, 0x03, 0x2f);
	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x1e);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config2(0xbc, 0x0900, 0x0000, 0x00, 0x00);

	read_config(0xbc, 0x0900, 0x0014, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config8(0xbc, 0x0001, 0x1a08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00);

	write_config6(0xb8, 0x0000, 0x0000, 0x00, 0x00, 0x04, 0xa0, 0x00, 0x00);

	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x16);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config2(0xbc, 0x0900, 0x0014, 0x03, 0x06);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config2(0xbc, 0x0900, 0x0014, 0x02, 0x06);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config2(0xbc, 0x0900, 0x0014, 0x00, 0x06);

	read_config(0xbc, 0x0900, 0x001c, 2);

	write_config5(0xbd, 0x0000, 0x3300, 0xab, 0xa9, 0x0f, 0xa4, 0x55);
	write_config2(0xbc, 0x0900, 0x0014, 0x00, 0x06);

	read_config(0xbc, 0x0900, 0x001c, 2);
	read_config(0xbd, 0x0000, 0x3300, 3);

	write_config2(0xbc, 0x0900, 0x0014, 0x00, 0x06);
	write_config2(0xbc, 0x0900, 0x0018, 0x00, 0x08);

	read_config(0xbc, 0x0900, 0x0014, 2);
	read_config(0xbc, 0x0900, 0x0018, 2);

	write_config5(0xbd, 0x0000, 0x3300, 0xab, 0xa9, 0x0f, 0xa4, 0x55);
	write_config2(0xbc, 0x0900, 0x0014, 0x00, 0x06);

	read_config(0xbc, 0x0900, 0x001c, 2);
	read_config(0xbd, 0x0000, 0x3300, 3);

	write_config2(0xbc, 0x0900, 0x0014, 0x00, 0x06);
	write_config2(0xbc, 0x0900, 0x0018, 0x00, 0x08);

	read_config(0xbc, 0x0900, 0x0014, 2);
	read_config(0xbc, 0x0900, 0x0018, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config6(0xb8, 0x0000, 0x0000, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00);

	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0900, 0x0074, 2);
	read_config(0xbc, 0x0900, 0x01b0, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config2(0xbc, 0x0900, 0x0074, 0x00, 0x04);
	write_config2(0xbc, 0x0900, 0x01b0, 0x00, 0x00);

	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config6(0xb8, 0x0000, 0x0000, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00);

	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0900, 0x0074, 2);
	read_config(0xbc, 0x0900, 0x01b0, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);
	read_config(0xbc, 0x0800, 0x2008, 2);

	write_config2(0xbc, 0x0900, 0x0074, 0x00, 0x04);
	write_config2(0xbc, 0x0900, 0x01b0, 0x00, 0x00);
}

void GCHD::receiveData() {
	int transfer;
	unsigned char data[DATA_BUF] = {0};

	libusb_bulk_transfer(devh_, 0x81, data, DATA_BUF, &transfer, TIMEOUT);
}
