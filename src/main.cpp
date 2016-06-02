/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include <gchd.hpp>
#include <process.hpp>
#include <streamer.hpp>

#define PORT_NUM	"57384"

void help(std::string name) {
	std::cerr << "Usage: " << name << " [options]" << std::endl
		  << std::endl
		  << "Options:" << std::endl
		  << "  -c <color-space>   Color Space settings [default: yuv]" << std::endl
		  << "  -f <format>        Format for <output> [default: fifo]" << std::endl
		  << "  -h                 Print this screen" << std::endl
		  << "  -i <input-source>  Input Source [default: hdmi]" << std::endl
		  << "  -n <ip-address>    IP address for UDP streaming [default: 0.0.0.0]" << std::endl
		  << "  -o <output>        Output Path [default: /tmp/gchd.ts]" << std::endl
		  << "  -p <port>          Port for UDP streaming [default: " << PORT_NUM << "]" << std::endl
		  << "  -r <resolution>    Resolution of Input Source [default: 1080]" << std::endl
		  << "  -P <pid-path>      PID path [default: /var/run/gchd.pid]" << std::endl;
}

void usage(std::string name, std::string optarg, std::string option, const std::vector<std::string> arguments) {
	std::cerr << "Invalid argument '" << optarg << "' for '" << option << "'" << std::endl
		  << "Valid arguments are:" << std::endl;

	// print list of valid arguments
	for (auto it : arguments) {
		std::cerr << "  - '" << it << "'" << std::endl;
	}

	std::cerr << "Try '" << name << " -h' for more information." << std::endl;
}

int main(int argc, char *argv[]) {
	// object for managing runtime information
	Process process;

	// set program name
	process.setName(argv[0]);

	// object for storing device settings
	Settings settings;

	// commandline-specific settings
	std::string ip;
	std::string port = PORT_NUM;
	std::string output = "/tmp/gchd.ts";
	std::string pid = "/var/run/gchd.pid";

	// output format, default is set to FIFO
	enum class Format {
		Disk,
		FIFO,
		Socket
	} format = Format::FIFO;

	// handling command-line options
	int opt;

	while ((opt = getopt(argc, argv, ":c:f:hi:n:o:p:r:P:")) != -1) {
		switch (opt) {
			case 'c': {
				if (std::string(optarg) == "yuv") {
					settings.setColorSpace(ColorSpace::YUV);
					break;
				} else if (std::string(optarg) == "rgb") {
					settings.setColorSpace(ColorSpace::RGB);
					break;
				}

				const std::vector<std::string> arguments = {"yuv", "rgb"};
				usage(process.getName(), optarg, "-c", arguments);
				return EXIT_FAILURE;
			}
			case 'f': {
				if (std::string(optarg) == "disk") {
					format = Format::Disk;
					break;
				} else if (std::string(optarg) == "fifo") {
					format = Format::FIFO;
					break;
				} else if (std::string(optarg) == "socket") {
					format = Format::Socket;
					break;
				}

				const std::vector<std::string> arguments = {"disk", "fifo", "socket"};
				usage(process.getName(), optarg, "-f", arguments);
				return EXIT_FAILURE;
			}
			case 'h':
				help(process.getName());
				return EXIT_SUCCESS;
			case 'i': {
				if (std::string(optarg) == "composite") {
					settings.setInputSource(InputSource::Composite);
					break;
				} else if (std::string(optarg) == "component") {
					settings.setInputSource(InputSource::Component);
					break;
				} else if (std::string(optarg) == "hdmi") {
					settings.setInputSource(InputSource::HDMI);
					break;
				}

				const std::vector<std::string> arguments = {"composite", "component", "hdmi"};
				usage(process.getName(), optarg, "-i", arguments);
				return EXIT_FAILURE;
			}
			case 'n':
				ip = std::string(optarg);
				break;
			case 'o':
				output = std::string(optarg);
				break;
			case 'p':
				port = std::string(optarg);
				break;
			case 'r': {
				if (std::string(optarg) == "ntsc") {
					settings.setResolution(Resolution::NTSC);
					break;
				} else if (std::string(optarg) == "pal") {
					settings.setResolution(Resolution::PAL);
					break;
				} else if (std::string(optarg) == "720") {
					settings.setResolution(Resolution::HD720);
					break;
				} else if (std::string(optarg) == "1080") {
					settings.setResolution(Resolution::HD1080);
					break;
				}

				const std::vector<std::string> arguments = {"ntsc", "pal", "720", "1080"};
				usage(process.getName(), optarg, "-r", arguments);
				return EXIT_FAILURE;
			}
			case 'P':
				pid = std::string(optarg);
				break;
			case ':':
				std::cerr << "Missing argument." << std::endl;
				return EXIT_FAILURE;
			case '?':
				std::cerr << "Unknown option." << std::endl;
				return EXIT_FAILURE;
			default:
				std::cerr << "Unexpected error." << std::endl;
				return EXIT_FAILURE;
		}
	}

	// TODO not ready for primetime yet, program needs to be restarted too
	// often at the moment
	// create PID file for single instance mechanism
//	if (process.createPid(pidPath)) {
//		return EXIT_FAILURE;
//	}

	GCHD gchd(&process, &settings);

	// helper class for streaming audio and video from device
	Streamer streamer(&gchd, &process);

	switch (format) {
		case Format::Disk:
			if (streamer.enableDisk(output)) {
				return EXIT_FAILURE;
			}

			break;
		case Format::FIFO:
			if (streamer.fifo.enable(output)) {
				return EXIT_FAILURE;
			}

			break;
		case Format::Socket:
			if (streamer.socket.enable(ip, port)) {
				return EXIT_FAILURE;
			}

			break;
	}

	// immediately start receive loop after device init
	if(gchd.init()) {
		return EXIT_FAILURE;
	}

	streamer.loop();

	return EXIT_SUCCESS;
}
