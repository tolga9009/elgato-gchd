/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Elgato Game Capture HD Linux driver and is
 * distributed under the MIT License. For more information, see LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "commands.h"
#include "common.h"

void read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	unsigned char *recv;
	recv = calloc(wLength, sizeof(unsigned char));
	libusb_control_transfer(devh, 0xc0, bRequest, wValue, wIndex, recv, wLength, 0);
	free(recv);
}

int read_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3) {
	unsigned char recv[4];
	libusb_control_transfer(devh, 0xc0, bRequest, wValue, wIndex, recv, 4, 0);

	if (recv[0] == data0 && recv[1] == data1 && recv[2] == data2 && recv[3] == data3) {
		return 1;
	}

	return 0;
}

void write_config2(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1) {
	unsigned char send[2] = {data0, data1};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 2, 0);
}

void write_config3(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2) {
	unsigned char send[3] = {data0, data1, data2};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 3, 0);
}

void write_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3) {
	unsigned char send[4] = {data0, data1, data2, data3};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 4, 0);
}

void write_config5(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4) {
	unsigned char send[5] = {data0, data1, data2, data3, data4};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 5, 0);
}

void write_config6(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5) {
	unsigned char send[6] = {data0, data1, data2, data3, data4, data5};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 6, 0);
}

void write_config8(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
	unsigned char send[8] = {data0, data1, data2, data3, data4, data5, data6, data7};
	libusb_control_transfer(devh, 0x40, bRequest, wValue, wIndex, send, 8, 0);
}

uint8_t count_bits(uint16_t data) {
	uint8_t i = 0;

	for (i = 0; data; i++) {
		data &= data - 1;
	}

	return i;
}

void scmd(uint8_t command, uint8_t mode, uint16_t data) {
	uint8_t send[6] = {0};
	send[2] = command;
	send[3] = mode;

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[4] = data >> 8;
	send[5] = data & 0xff;

	libusb_control_transfer(devh, 0x40, 0xb8, 0x0000, 0x0000, send, 6, 0);
}

void sparam(uint16_t wIndex, uint8_t shift, uint8_t range, uint16_t data) {
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

	libusb_control_transfer(devh, 0x40, 0xbc, 0x0001, wIndex, send, 8, 0);
}

void slsi(uint16_t wIndex, uint16_t data) {
	uint8_t send[2] = {0};

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[0] = data >> 8;
	send[1] = data & 0xff;

	libusb_control_transfer(devh, 0x40, 0xbc, 0x0000, wIndex, send, 2, 0);
}

void dlfirm(const char *file) {
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

		libusb_bulk_transfer(devh, 0x02, data, bytes_remain, &transfer, 0);
	}

	fclose(bin);
}

void receive_data() {
	int transfer;
	unsigned char data[DATA_BUF] = {0};

	libusb_bulk_transfer(devh, 0x81, data, DATA_BUF, &transfer, 5000);
	write(fd_fifo, (char *)data, DATA_BUF);
}
