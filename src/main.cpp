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

#define PORT_NUM    "57384"

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
          << "  -v                 Print program version" << std::endl
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

void version(std::string name, std::string version) {
    std::cerr << name << " " << version << std::endl;
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

    while ((opt = getopt(argc, argv, ":c:f:hi:n:o:p:r:vP:")) != -1) {
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
                    settings.setOutputResolution(Resolution::NTSC);
                    break;
                } else if (std::string(optarg) == "pal") {
                    settings.setOutputResolution(Resolution::PAL);
                    break;
                } else if (std::string(optarg) == "720") {
                    settings.setOutputResolution(Resolution::HD720);
                    break;
                } else if (std::string(optarg) == "1080") {
                    settings.setOutputResolution(Resolution::HD1080);
                    break;
                }

                const std::vector<std::string> arguments = {"ntsc", "pal", "720", "1080"};
                usage(process.getName(), optarg, "-r", arguments);
                return EXIT_FAILURE;
            }
            case 'v':
                version(process.getName(), process.getVersion());
                return EXIT_SUCCESS;
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
//  if (process.createPid(pidPath)) {
//      return EXIT_FAILURE;
//  }

    //Try block wraps GCHD creation so if exception gets thrown,
    //stack unwinding will destruct object, calling uninit.
    try {
        GCHD gchd(&process, &settings);

        if(gchd.checkDevice()) {
            return EXIT_FAILURE;
        }

        // helper class for streaming audio and video from device
        Streamer streamer(&gchd, &process);

        // enable output
        int ret;

        switch (format) {
            case Format::Disk: ret = streamer.disk.enable(output); break;
            case Format::FIFO: ret = streamer.fifo.enable(output); break;
            case Format::Socket: ret = streamer.socket.enable(ip, port); break;
        }
        if (ret) {
            return EXIT_FAILURE;
        }

        //Likely aborted during waiting for user to open fifo.
        if( process.isActive() ) {
            if(gchd.init()) {
                return EXIT_FAILURE;
            }
        }

        // immediately start receive loop after device init
        streamer.loop();
    } catch ( std::runtime_error &error ) {
        //All my exceptions inherit from runtime_error or logic_error.
        std::cerr << std::endl << "FATAL ERROR: " << error.what() << std::endl << std::endl;
        return EXIT_FAILURE;
    } catch ( std::logic_error &error ) {
        throw; //Rethrow exception.
    }

    return EXIT_SUCCESS;
}
