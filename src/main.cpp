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

void help(std::string name) {
	std::cerr << "Usage: " << name << " [options]" << std::endl
		  << std::endl
		  << "Options:" << std::endl
		  << "  -c <color-space>   Color Space settings [default: yuv]" << std::endl
		  << "  -d                 Capture to <output> instead of using FIFO" << std::endl
		  << "  -i <input-source>  Input Source [default: hdmi]" << std::endl
		  << "  -o <output>        Output Path [default: /tmp/gchd.ts]" << std::endl
		  << "  -p <pid-path>      PID path [default: /var/run/gchd.pid]" << std::endl
		  << "  -r <resolution>    Resolution of Input Source [default: 1080]" << std::endl;
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
	std::string pidPath = "/var/run/gchd.pid";
	std::string outputPath = "/tmp/gchd.ts";
	bool useFifo = true;

	// handling command-line options
	int opt;

	while ((opt = getopt(argc, argv, ":c:dhi:o:p:r:")) != -1) {
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
			case 'd':
				useFifo = false;
				break;
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
			case 'o':
				outputPath = std::string(optarg);
				break;
			case 'p':
				pidPath = std::string(optarg);
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

	// device initialization
	if(gchd.init()) {
		return EXIT_FAILURE;
	}

	// helper class for streaming audio and video from device
	Streamer streamer(&process);

	// TODO move to Streamer class. Let it decide, what to do.
	if (useFifo) {
		if (streamer.createFifo(outputPath)) {
			return EXIT_FAILURE;
		}

		// when FIFO file has been opened
		streamer.streamToFifo(&gchd);
	} else {
		streamer.streamToDisk(&gchd, outputPath);
	}

	std::cerr << "Terminating." << std::endl;

	return EXIT_SUCCESS;
}
