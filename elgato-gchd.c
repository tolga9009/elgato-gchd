#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>

#include <libusb-1.0/libusb.h>

/* constants */
#define ELGATO_VENDOR		0x0fd9
#define GAME_CAPTURE_HD_PID	0x004e

#define EP_OUT			0x02
#define INTERFACE_NUM	0x00
#define CONFIGURATION	0x01

#define DATA_BUF		0x4000

/* global structs */
static struct libusb_device_handle *devh = NULL;

int init_dev_handler() {
	if (libusb_init(NULL)) {
		fprintf(stderr, "Error initializing libusb.");
		return 1;
	}

	libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);

	devh = libusb_open_device_with_vid_pid(NULL, ELGATO_VENDOR, GAME_CAPTURE_HD_PID);
	if (!devh) {
		fprintf(stderr, "Unable to find device.");
		return 1;
	}

	return 0;
}

int get_interface() {
	if (libusb_kernel_driver_active(devh, INTERFACE_NUM)) {
		libusb_detach_kernel_driver(devh, INTERFACE_NUM);
	}

	if (libusb_set_configuration(devh, CONFIGURATION)) {
		fprintf(stderr, "Could not activate configuration.");
		return 1;
	}

	if (libusb_claim_interface(devh, INTERFACE_NUM)) {
		fprintf(stderr, "Failed to claim interface.");
		return 1;
	}

	return 0;
}

void clean_up() {
	if (devh) {
		libusb_release_interface(devh, INTERFACE_NUM);
		libusb_close(devh);
	}

	libusb_exit(NULL);
}

void read_config(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	unsigned char *recv;

	recv = calloc(wLength, sizeof(unsigned char));
	libusb_control_transfer(devh, 0xc0, 0xbc, wValue, wIndex, recv, wLength, 0);
	free(recv);
}

void write_config(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	unsigned char *send;

	send = calloc(wLength, sizeof(unsigned char));
	libusb_control_transfer(devh, 0x40, 0xbc, wValue, wIndex, send, wLength, 0);
	free(send);
}

void load_firmware(const char *file) {
	int transfer;

	FILE *bin;
	bin = fopen(file, "rb");

	/* get filesize */
	fseek(bin, 0L, SEEK_END);
	long filesize = ftell(bin);
	rewind(bin);

	/* read firmware from file to buffer and bulk transfer to device */
	for (int i = 0; i <= filesize; i += DATA_BUF) {
		unsigned char data[DATA_BUF] = {0};
		int bytes_remain = filesize - i;

		if ((bytes_remain) > DATA_BUF) {
			bytes_remain = DATA_BUF;
		}

		int ret = fread(data, bytes_remain, 1, bin);

		libusb_bulk_transfer(devh, 0x02, data, bytes_remain, &transfer, 1000);
	}

	fclose(bin);
}

void configure_dev() {
	read_config(0x0800, 0x0094, 4);
	read_config(0x0800, 0x0098, 4);
	read_config(0x0800, 0x0010, 4);
	read_config(0x0800, 0x0014, 4);
	read_config(0x0800, 0x0018, 4);

	write_config(0x0900, 0x0000, 2);

	read_config(0x0900, 0x0014, 2);
	read_config(0x0800, 0x2008, 2);
	read_config(0x0900, 0x0074, 2);
	read_config(0x0900, 0x01b0, 2);
	read_config(0x0800, 0x2008, 2);
	read_config(0x0800, 0x2008, 2);

	/* this is an important step for sending the firmware */
	unsigned char send[2] = {0x00, 0x04};
	libusb_control_transfer(devh, 0x40, 0xbc, 0x0900, 0x0074, send, 2, 0);

	write_config(0x0900, 0x01b0, 2);

	/* load "idle" firmware */
	load_firmware("firmware/mb86h57_h58_idle.bin");

	libusb_control_transfer(devh, 0x40, 0xbc, 0x0900, 0x0070, send, 2, 0);

	read_config(0x0900, 0x0014, 2);
	read_config(0x0900, 0x0018, 2);
	read_config(0x0000, 0x0010, 2);
	read_config(0x0000, 0x0012, 2);
	read_config(0x0000, 0x0014, 2);
	read_config(0x0000, 0x0016, 2);
	read_config(0x0000, 0x0018, 2);
	read_config(0x0000, 0x001a, 2);
	read_config(0x0000, 0x001c, 2);
	read_config(0x0000, 0x001e, 2);
	read_config(0x0800, 0x2008, 2);

	unsigned char send2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
	libusb_control_transfer(devh, 0x40, 0xbc, 0x0010, 0x1a04, send2, 8, 0);

	unsigned char send3[6] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
	libusb_control_transfer(devh, 0x40, 0xbc, 0x0010, 0x1a04, send3, 6, 0);

	/* load "idle" firmware */
	load_firmware("firmware/mb86h57_h58_enc_h.bin");
}

int main() {
	/* initialize device handler */
	if (init_dev_handler()) {
		goto end;
	}

	/* detach kernel driver and claim interface */
	if (get_interface()) {
		goto end;
	}

	/* configure device */
	configure_dev();

end:
	/* clean up */
	clean_up();

	return 0;
}
