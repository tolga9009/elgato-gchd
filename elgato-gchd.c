/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
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
#include "init.h"
#include "uninit.h"

// USB VID & PIDs
#define VENDOR_ELGATO		0x0fd9
#define GAME_CAPTURE_HD_0	0x0044
#define GAME_CAPTURE_HD_1	0x004e
#define GAME_CAPTURE_HD_2	0x0051
#define GAME_CAPTURE_HD_3	0x005d // new revision GCHD - unsupported
#define GAME_CAPTURE_HD60	0x005c // Game Capture HD60 - unsupported
#define GAME_CAPTURE_HD60_S	0x004f // Game Capture HD60 S - unsupported

// constants
#define INTERFACE_NUM		0x00
#define CONFIGURATION_VALUE	0x01

enum video_resolution {
	v720p,
	v1080p,
	v576i,
	vc576p,
	vc720p,
	vc1080i,
	vc1080p
};

// globals
static volatile sig_atomic_t is_running = 1;
int libusb_ret = 1;
int fd_fifo = 0;
int is_initialized = 0;
char *fifo_path = "/tmp/elgato_gchd.ts";

void sig_handler(int sig) {
	fprintf(stderr, "\nStop signal received.\n");

	switch(sig) {
		case SIGINT:
			is_running = 0;
			break;
		case SIGTERM:
			is_running = 0;
			break;
	}
}

int open_device() {
	devh = NULL;
	libusb_ret = libusb_init(NULL);

	if (libusb_ret) {
		fprintf(stderr, "Error initializing libusb.\n");
		return 1;
	}

	// uncomment for verbose debugging
	//libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD_0);
	if (devh) {
		return 0;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD_1);
	if (devh) {
		return 0;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD_2);
	if (devh) {
		return 0;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD_3);
	if (devh) {
		fprintf(stderr, "This revision of the Elgato Game Capture HD is currently not supported.\n");
		return 1;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD60);
	if (devh) {
		fprintf(stderr, "The Elgato Game Capture HD60 is currently not supported.\n");
		return 1;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ELGATO, GAME_CAPTURE_HD60_S);
	if (devh) {
		fprintf(stderr, "The Elgato Game Capture HD60 S is currently not supported.\n");
		return 1;
	}

	return 1;
}

int get_interface() {
	if (libusb_kernel_driver_active(devh, INTERFACE_NUM)) {
		libusb_detach_kernel_driver(devh, INTERFACE_NUM);
	}

	if (libusb_set_configuration(devh, CONFIGURATION_VALUE)) {
		fprintf(stderr, "Could not activate configuration.\n");
		return 1;
	}

	if (libusb_claim_interface(devh, INTERFACE_NUM)) {
		fprintf(stderr, "Failed to claim interface.\n");
		return 1;
	}

	return 0;
}

void close_device() {
	libusb_release_interface(devh, INTERFACE_NUM);
	libusb_close(devh);
}

void clean_up() {
	if (devh) {
		if (is_initialized) {
			fprintf(stderr, "Your device is going to be reset. Please wait and do not interrupt or unplug your device.\n");
			uninit_device();
			fprintf(stderr, "Device has been reset.\n");
		}

		close_device();
	}

	// deinitialize libusb
	if (!libusb_ret) {
		libusb_exit(NULL);
	}

	close(fd_fifo);
	unlink(fifo_path);

	fprintf(stderr, "Terminating.\n");
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

	int opt, index;
	enum video_resolution resolution;

	while ((opt = getopt_long(argc, argv, "r:", longOptions, &index)) != -1) {
		switch (opt) {
			case 'r':
				if (strcmp(optarg, "720p") == 0) {
					resolution = v720p;
					break;
				} else if (strcmp(optarg, "1080p") == 0) {
					resolution = v1080p;
					break;
				} else if (strcmp(optarg, "576i") == 0) {
					resolution = v576i;
					break;
				} else if (strcmp(optarg, "c576p") == 0) {
					resolution = vc576p;
					break;
				} else if (strcmp(optarg, "c720p") == 0) {
					resolution = vc720p;
					break;
				} else if (strcmp(optarg, "c1080i") == 0) {
					resolution = vc1080i;
					break;
				} else if (strcmp(optarg, "c1080p") == 0) {
					resolution = vc1080p;
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

	// initialize device handler
	if (open_device()) {
		fprintf(stderr, "Unable to find device.\n");
		goto end;
	}

	// check for firmware files
	if (access(FW_MB86H57_H58_IDLE, F_OK) != 0
	    || access(FW_MB86H57_H58_ENC, F_OK) != 0) {
		fprintf(stderr, "Firmware files missing.\n");
		goto end;
	}

	// detach kernel driver and claim interface
	if (get_interface()) {
		fprintf(stderr, "Could not claim interface.\n");
		goto end;
	}

	// create and open FIFO
	if (mkfifo(fifo_path, 0644)) {
		fprintf(stderr, "Error creating FIFO.\n");
		goto end;
	}

	fprintf(stderr, "%s has been created. Waiting for user to open it.\n", fifo_path);
	fd_fifo = open(fifo_path, O_WRONLY);

	// set device configuration
	if (is_running) {
		fprintf(stderr, "Initializing device.\n");
		is_initialized = 1;

		switch (resolution) {
			case v720p: configure_dev_720p(); break;
			case v1080p: configure_dev_1080p(); break;
			case v576i: configure_dev_576i(); break;
			case vc576p: configure_dev_component_576p(); break;
			case vc720p: configure_dev_component_720p(); break;
			case vc1080i: configure_dev_component_1080i(); break;
			case vc1080p: configure_dev_component_1080p(); break;
			default: clean_up();
		}

		fprintf(stderr, "Streaming data from device now.\n");
	}

	// receive audio and video from device
	while (is_running) {
		receive_data();
	}

end:
	clean_up();

	return 0;
}
