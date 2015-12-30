/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Elgato Game Capture HD Linux driver and is
 * distributed under the MIT License. For more information, see LICENSE file.
 */

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define ELGATO_VENDOR		0x0fd9
#define GAME_CAPTURE_HD_PID_0	0x0044
#define GAME_CAPTURE_HD_PID_1	0x004e
#define GAME_CAPTURE_HD_PID_2	0x0051

#define EP_OUT		0x02
#define INTERFACE_NUM	0x00
#define CONFIGURATION	0x01

// globals
static volatile sig_atomic_t is_running = 1;
int libusb_ret = 1;
int fd_fifo = 0;
char *fifo_path = "/tmp/elgato_gchd.ts";

enum video_resoltion {
	v720p,
	v1080p
};

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
	libusb_ret = libusb_init(NULL);

	if (libusb_ret) {
		fprintf(stderr, "Error initializing libusb.\n");
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

	return 1;
}

int get_interface() {
	if (libusb_kernel_driver_active(devh, INTERFACE_NUM)) {
		libusb_detach_kernel_driver(devh, INTERFACE_NUM);
	}

	if (libusb_set_configuration(devh, CONFIGURATION)) {
		fprintf(stderr, "Could not activate configuration.\n");
		return 1;
	}

	if (libusb_claim_interface(devh, INTERFACE_NUM)) {
		fprintf(stderr, "Failed to claim interface.\n");
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
	if (devh) {
		remove_elgato();
		libusb_release_interface(devh, INTERFACE_NUM);
		libusb_close(devh);
	}

	if (libusb_ret == 0) {
		libusb_exit(NULL);
	}

	close(fd_fifo);
	unlink(fifo_path);
}

int main(int argc, char *argv[]) {
	// signal handling
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	signal(SIGPIPE, SIG_IGN);

	// handling command-line options
	static struct option longOptions[] = {
		{"resolution", required_argument, 0, 'r'},
	};

	int opt, index, resolution = 0;

	while ((opt = getopt_long(argc, argv, "r:", longOptions, &index)) != -1) {
		switch (opt) {
			case 'r':
				if (strcmp(optarg, "720p") == 0) {
					resolution = v720p;
					break;
				} else if (strcmp(optarg, "1080p") == 0) {
					resolution = v1080p;
					break;
				}

				fprintf(stderr, "Unsupported resolution.\n");
				return EXIT_FAILURE;
			case ':':
				fprintf(stderr, "Missing argument.\n");
				return EXIT_FAILURE;
			case '?':
				fprintf(stderr, "Unrecognized option.\n");
				return EXIT_FAILURE;
			default:
				fprintf(stderr, "Unexpected error.\n");
				return EXIT_FAILURE;
		}
	}

	if (access(FW_MB86H57_H58_IDLE, F_OK) != 0
	    || access(FW_MB86H57_H58_ENC, F_OK) != 0) {
		    fprintf(stderr, "Firmware files missing.\n");
		    goto end;
	    }

	// initialize device handler
	if (init_dev_handler()) {
		fprintf(stderr, "Unable to find device.\n");
		goto end;
	}

	// detach kernel driver and claim interface
	if (get_interface()) {
		fprintf(stderr, "Could not claim interface.\n");
		goto end;
	}

	// create the FIFO (also known as named pipe)
	mkfifo(fifo_path, 0644);

	// open FIFO
	fd_fifo = open(fifo_path, O_WRONLY);

	// set device configuration
	switch (resolution) {
		case v720p: configure_dev_720p(); break;
		case v1080p: configure_dev_1080p(); break;
		default: clean_up();
	}

	// receive audio and video from device
	while(is_running) {
		receive_data();
	}

end:
	// clean up
	clean_up();

	return 0;
}
