/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Elgato Game Capture HD Linux driver and is
 * distributed under the MIT License. For more information, see LICENSE file.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <libusb-1.0/libusb.h>

#include "commands.h"
#include "common.h"
#include "init_720p.h"
#include "init_1080p.h"
#include "remove.h"

// constants
#define ELGATO_VENDOR			0x0fd9
#define GAME_CAPTURE_HD_PID_0	0x0044
#define GAME_CAPTURE_HD_PID_1	0x004e
#define GAME_CAPTURE_HD_PID_2	0x0051

#define EP_OUT			0x02
#define INTERFACE_NUM	0x00
#define CONFIGURATION	0x01

// scmd commands
#define IDLE			1
#define INIT			4
#define STATE_CHANGE	5

// globals
static volatile sig_atomic_t is_running = 1;
int fd_fifo;
char *fifo_path = "/tmp/elgato_gchd.ts";

void sig_handler(int sig) {
	switch(sig) {
		case SIGINT:
			is_running = 0;
			break;
		case SIGTERM:
			is_running = 0;
			break;
	}
}

int init_dev_handler() {
	devh = NULL;

	if (libusb_init(NULL)) {
		fprintf(stderr, "Error initializing libusb.");
		return 1;
	}

	//libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);

	devh = libusb_open_device_with_vid_pid(NULL, ELGATO_VENDOR, GAME_CAPTURE_HD_PID_0);
	if (devh) {
		return 0;
	}

	devh = libusb_open_device_with_vid_pid(NULL, ELGATO_VENDOR, GAME_CAPTURE_HD_PID_1);
	if (devh) {
		return 0;
	}

	devh = libusb_open_device_with_vid_pid(NULL, ELGATO_VENDOR, GAME_CAPTURE_HD_PID_2);
	if (devh) {
		return 0;
	}

	fprintf(stderr, "Unable to find device.");
	return 1;
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

void receive_data() {
	int transfer;
	unsigned char data[DATA_BUF] = {0};

	libusb_bulk_transfer(devh, 0x81, data, DATA_BUF, &transfer, 5000);
	write(fd_fifo, (char *)data, DATA_BUF);
}

void clean_up() {
	remove_elgato();

	if (devh) {
		libusb_release_interface(devh, INTERFACE_NUM);
		libusb_close(devh);
	}

	libusb_exit(NULL);
	close(fd_fifo);
	unlink(fifo_path);
}

int main() {
	// signal handling
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	// ignore SIGPIPE: causes the program to terminate on unsuccessful write()
	signal(SIGPIPE, SIG_IGN);

	// initialize device handler
	if (init_dev_handler()) {
		goto end;
	}

	// detach kernel driver and claim interface
	if (get_interface()) {
		goto end;
	}

	// create the FIFO (also known as named pipe)
	mkfifo(fifo_path, 0644);

	// open FIFO
	fd_fifo = open(fifo_path, O_WRONLY);

	// configure device
	configure_dev_1080p();

	while(is_running) {
		receive_data();
	}

end:
	// clean up
	clean_up();

	return 0;
}
