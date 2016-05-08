/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <atomic>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <libusb-1.0/libusb.h>

#include <core/gchd.hpp>
#include <process.hpp>

// globals
std::atomic<bool> isRunning;
int fd = 0;
std::string pidPath = "/var/run/gchd.pid";
std::string fifoPath = "/tmp/gchd.ts";

// TODO stop GCHD aswell, when signal is received
void sigHandler(int sig) {
        fprintf(stderr, "\nStop signal received.\n");

        switch(sig) {
                case SIGINT:
			isRunning = false;
                        break;
                case SIGTERM:
			isRunning = false;
                        break;
        }
}

int main(int argc, char *argv[]) {
	// signal handling
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	signal(SIGPIPE, SIG_IGN);

	// object for storing device settings, needed for GCHD constructor
	Settings settings;

	// handling command-line options
	static struct option longOptions[] = {
		{"resolution", required_argument, 0, 'r'},
	};

	int opt, index;

	while ((opt = getopt_long(argc, argv, "r:", longOptions, &index)) != -1) {
		switch (opt) {
			case 'r':
				if (std::string(optarg) == "SD") {
					settings.setResolution(Resolution::Standard);
					break;
				} else if (std::string(optarg) == "720") {
					settings.setResolution(Resolution::HD720);
					break;
				} else if (std::string(optarg) == "1080") {
					settings.setResolution(Resolution::HD1080);
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

	auto process = Process();

	// create PID file for single instance mechanism
	if (process.createPid(pidPath)) {
		std::cerr << "Error creating PID file." << std::endl;
		return EXIT_FAILURE;
	}

	auto gchd = GCHD(&settings);

	// device initialization
	if(gchd.init()) {
		return EXIT_FAILURE;
	}

	// TODO check flag, if FIFO or record to HDD is preferred
	if (process.createFifo(fifoPath)) {
		return EXIT_FAILURE;
	}

	// when FIFO file has been opened
	isRunning = true;
	fprintf(stderr, "Streaming data from device now.\n");

	// receive audio and video from device
	while (isRunning) {
		unsigned char data[DATA_BUF] = {0};

		gchd.stream(data, DATA_BUF);
		write(fd, (char *)data, DATA_BUF);
	}

	fprintf(stderr, "Terminating.\n");

	return EXIT_SUCCESS;
}
