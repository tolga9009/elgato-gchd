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

#define CHUNK_SIZE		0x4000

/* global structs */
static struct libusb_device_handle *devh = NULL;

int init_dev_handler() {
	if (libusb_init(NULL)) {
		fprintf(stderr, "Error initializing libusb.");
		return 1;
	}

	//libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);

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

	/* this is an important step for sending the firmware. TODO: more elegant */
	unsigned char send[2];

	send[0] = 0x00;
	send[1] = 0x04;

	libusb_control_transfer(devh, 0x40, 0xbc, 0x0900, 0x0074, send, 2, 0);

	write_config(0x0900, 0x01b0, 2);
}

void load_firmware(const char *file) {
	int transfer;
	unsigned char data[CHUNK_SIZE];

	for (int i = 0; i < CHUNK_SIZE; i++) {
		data[i] = 0;
	}

	FILE *bin;
	bin = fopen(file, "rb");

	while(1) {
		for (int i = 0; i < CHUNK_SIZE; i++) {
			data[i] = 0;
		}

		int ret = fread(data, sizeof(data), 1, bin);

		/* TODO: only transfer amount of read Bytes */
		libusb_bulk_transfer(devh, 0x02, data, sizeof(data), &transfer, 1000);

		if (!ret) {
			break;
		}

		printf("Bulk transfer: %d Bytes\n", transfer);
	}

	fclose(bin);
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

	/* load "idle" firmware */
	load_firmware("firmware/mb86h57_h58_idle.bin");

end:
	/* clean up */
	clean_up();

	return 0;
}
