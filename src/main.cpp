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
#include <cmath>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE //Add support for getopt_long_only.
#endif

#include <getopt.h>

#include "gchd.hpp"
#include "process.hpp"
#include "streamer.hpp"
#include "utility.hpp"

#define PORT_NUM    "57384"

void help(std::string name, bool full) {
	std::cerr << "Usage:" << std::endl
		  << "   " << name << " [options] [<destination>]" << std::endl
		  << "      For `disk` and `fifo` output formats <destination> is a filename." << std::endl
		  << "      For the `socket` output format, <destination> is [<ip address>][:<port>]." << std::endl
		  << std::endl
		  << "      The default for `fifo` is `/tmp/gchd.ts`." << std::endl
		  << "      The default for `socket` is `0.0.0.0:" << PORT_NUM << "`" << std::endl
		  << "      There is no default for `disk`, <destination> is required." << std::endl
		  << std::endl
		  << "Input Options:" << std::endl
		  << "   -i, -input <input-source>"  << std::endl
		  << "      Set input source to `hdmi`, `component`, `composite` or `auto` (default)" << std::endl
		  << "      With no input, or multiple input types `auto` may detect incorrectly." << std::endl
		  << std::endl
		  << "   -c, -color-space <color-space>"  << std::endl
		  << "      Set the input color space to `yuv`, `rgb`, or `auto` (default)." << std::endl
		  << "      Only has meaning for hdmi and component input." << std::endl
		  << std::endl;
	if( full ) {
		std::cerr
				<< "   -ir, -input-resolution <resolution>" << std::endl
				<< "      Input resolution can be `ntsc`, `pal`, `720`, `1080`, or `auto`." << std::endl
				<< "      The default is `auto`. `480` and `576` can be used instead of `ntsc`" << std::endl
				<< "      and  `pal`. You can also use <xres>x<yres>, IE 720x480." << std::endl
				<< std::endl
				<< "   -irr, -refresh, -input-refresh <refresh rate>" <<std::endl
				<< "       0.0 designates autodetect." << std::endl
				<< std::endl
				<< "   -ii, -interlaced, -input-interlaced" << std::endl
				<< "       Use interlaced scan mode." << std::endl
				<< "   -ip, -progressive, -input-progressive" << std::endl
				<< "       Use progressive scan mode." << std::endl
				<< "   -i?, -autoscan, -input-autoscan (default)" <<std::endl
				<< "       Autodetect scan mode." << std::endl
				<< std::endl
				<< "   -is, -stretch, -input-stretch" << std::endl
				<< "      Stretches SD input signals to 16:9 if set." << std::endl
				<< "   -in, -no-stretch, -input-no-stretch (default)" << std::endl
				<< "      Keeps SD signals 4:3." << std::endl
				<< std::endl;
	}

	std::cerr
			<< "Output Options:" << std::endl
			<< "   -of, -output-format <format>" << std::endl
			<< "      Format is `disk`, `socket, or `fifo`. `disk` is default if a " << std::endl
			<< "      <destination> file is specified, otherwise the default is `fifo`" << std::endl
			<< std::endl
			<< "   -or, -output-resolution <resolution>" << std::endl
			<< "      Output resolution can be `ntsc`, `pal`, `720`, `1080`, or `auto`." << std::endl
			<< "      `auto` (default) matches input resolution. `480` and `576` can be used"  << std::endl
			<< "      instead of `ntsc` and `pal`. You can also use <xres>x<yres>, IE 720x480." << std::endl
			<< std::endl
			<< "   -br, -bit-rate <mbit-rate>" << std::endl
			<< "      <mbit-rate> is either `auto` which will do relatively high quality output," << std::endl
			<< "      or a bit rate number in mbps. You can do variable bit rate by specifying" << std::endl
			<< "      the bit rate in a [max]:[average]:[min] format." << std::endl
			<< std::endl;

	if( full ) {
		std::cerr
				<< "   -abr, -audio-bit-rate <kbit-rate>" << std::endl
				<< "      <kbit-rate> is a audio bit rate number in kbps. It can be 64, 96, 112," << std::endl
				<< "      128, 160, 192, 224, 256, 320, or 384.  The default is 320." << std::endl
				<< std::endl
				<< "   -fr, -frame-rate <frame-rate>" << std::endl
				<< "      Specifies output frame rate, 0.0 (default) means match input rate" << std::endl
				<< "      (to the limits of the device, IE 1080p60 input will be 1080p30 output)." << std::endl
				<< std::endl
				<< "   -hp, -h264-profile <profile>" << std::endl
				<< "      h.264 profile. Set to `baseline`, `main`, or `high` (default) " << std::endl
				<< std::endl
				<< "   -hl, -h264-level <level>" << std::endl
				<< "      h.264 level. Level can be `1.0`, `1.1`, `1.2`, `1.3`, `2.0`, `2.1`," << std::endl
				<< "      `2.2`, `3.0`, `3.1`, `3.2`, `4.0`, `4.1` or `auto` (default)." << std::endl
				<< std::endl;
	}
	std::cerr
			<< "General Options:" << std::endl
			<< "   -h, -?, -help" << std::endl
			<< "      Displays commonly used switches" << std::endl
			<< std::endl
			<< "   -hh, -??, -full-help" << std::endl
			<< "      Displays extended help" << std::endl
			<< std::endl;

	if( full ) {
		std::cerr
				<< "   -v, -version" << std::endl
				<< "      Displays program version" << std::endl
				<< std::endl
				<< "   -P, -pid <pid-path>" << std::endl
				<< "      Set path to pid file (default is: /var/run/gchd.pid)" << std::endl
				<< std::endl;
	}
}

void option_error(std::string name, std::string option, std::string error) {
	std::cerr << "Invalid option `" << option << "':" << std::endl;
	std::cerr << error << std::endl;
}

void parameter_unknown(std::string name, std::string option, const std::vector<std::string> arguments) {
	std::cerr << "Invalid argument '" << optarg << "' for '" << option << "'" << std::endl
		  << "Valid arguments are:" << std::endl;

	// print list of valid arguments
	for (auto it : arguments) {
		std::cerr << "  - '" << it << "'" << std::endl;
	}
	std::cerr << std::endl;
	std::cerr << "Try '" << name << " -h' for more information." << std::endl;
}

void parameter_error(std::string name, std::string option, std::string error) {
	std::cerr << "Invalid argument '" << optarg << "' for '" << option << "':" << std::endl;
	std::cerr << error << std::endl;
}

void version(std::string name, std::string version) {
	std::cerr << name << " " << version << std::endl;
}

bool parseNumericResolution( unsigned long &x, unsigned long &y, const std::string input ) {
	std::vector<std::string> splitValues=Utility::split(input,'x');
	std::vector<unsigned long> numbers=std::vector<unsigned long>();

	for( auto it = splitValues.begin(); it != splitValues.end(); ++it) {
		char *endPtr;
		std::string copy=*it;
		copy=Utility::trim(copy); //Modifies copy

		long value=strtoul( copy.c_str(), &endPtr, 10);

		if( *endPtr != 0 ) {
			return false;
		}
		numbers.push_back(value);
	}
	if( numbers.size() != 2 ) {
		return false;
	}
	x=numbers[0];
	y=numbers[1];
	return true;
}

enum class Args:int {
	INPUT_SOURCE=1,
	COLOR_SPACE,
	INPUT_RESOLUTION,
	INPUT_SCAN_INTERLACED,
	INPUT_SCAN_PROGRESSIVE,
	INPUT_SCAN_AUTO,
	INPUT_REFRESH_RATE,
	INPUT_STRETCH,
	INPUT_NO_STRETCH,
	OUTPUT_FORMAT,
	OUTPUT_DESTINATION,
	IP_ADDRESS,
	PORT,
	OUTPUT_RESOLUTION,
	DEFUNCT_RESOLUTION,
	FRAME_RATE,
	BIT_RATE,
	AUDIO_BIT_RATE,
	H264_PROFILE,
	H264_LEVEL,
	HELP,
	FULL_HELP,
	VERSION,
	PID_PATH,
};

int main(int argc, char *argv[]) {
	// object for managing runtime information
	Process process;

	// set program name
	process.setName(argv[0]);

	// objects for storing device settings
	InputSettings inputSettings=InputSettings();
	TranscoderSettings transcoderSettings=TranscoderSettings();

	// commandline-specific settings
	std::string ip;
	int ipSetIndex=0;

	std::string port = PORT_NUM;
	int portSetIndex=0;

	std::string output = "/tmp/gchd.ts";
	int outputSetIndex=0;

	bool destinationSet=false;
	bool outputFormatSet=false;


	std::string pid = "/var/run/gchd.pid";

	// output format, default is set to FIFO
	enum class Format {
		Disk,
		FIFO,
		Socket
	} format = Format::FIFO;

	// handling command-line options
	int opt;

	struct option longOpts[]={
	{"i", required_argument, NULL, (int)Args::INPUT_SOURCE},
	{"input", required_argument, NULL, (int)Args::INPUT_SOURCE},
	{"c", required_argument, NULL, (int)Args::COLOR_SPACE},
	{"color-space", required_argument, NULL, (int)Args::COLOR_SPACE},
	{"ir", required_argument, NULL, (int)Args::INPUT_RESOLUTION},
	{"input-resolution", required_argument, NULL, (int)Args::INPUT_RESOLUTION},
	{"ii", no_argument, NULL, (int)Args::INPUT_SCAN_INTERLACED},
	{"interlaced", no_argument, NULL, (int)Args::INPUT_SCAN_INTERLACED},
	{"input-interlaced", no_argument, NULL, (int)Args::INPUT_SCAN_INTERLACED},
	{"ip", no_argument, NULL, (int)Args::INPUT_SCAN_PROGRESSIVE},
	{"progressive", no_argument, NULL, (int)Args::INPUT_SCAN_PROGRESSIVE},
	{"input-progressive", no_argument, NULL, (int)Args::INPUT_SCAN_PROGRESSIVE},
	{"i?", no_argument, NULL, (int)Args::INPUT_SCAN_AUTO},
	{"autoscan", no_argument, NULL, (int)Args::INPUT_SCAN_AUTO},
	{"input-autoscan", no_argument, NULL, (int)Args::INPUT_SCAN_AUTO},
	{"irr", required_argument, NULL, (int)Args::INPUT_REFRESH_RATE},
	{"refresh", required_argument, NULL, (int)Args::INPUT_REFRESH_RATE},
	{"input-refresh", required_argument, NULL, (int)Args::INPUT_REFRESH_RATE},
	{"is", no_argument, NULL, (int)Args::INPUT_STRETCH},
	{"stretch", no_argument, NULL, (int)Args::INPUT_STRETCH},
	{"input-stretch", no_argument, NULL, (int)Args::INPUT_STRETCH},
	{"in", no_argument, NULL, (int)Args::INPUT_NO_STRETCH},
	{"no-stretch", no_argument, NULL, (int)Args::INPUT_NO_STRETCH},
	{"no-input-stretch", no_argument, NULL, (int)Args::INPUT_NO_STRETCH},
	{"f", required_argument, NULL, (int)Args::OUTPUT_FORMAT}, //use of 'f' is deprecated.
	{"of", required_argument, NULL, (int)Args::OUTPUT_FORMAT},
	{"output-format", required_argument, NULL, (int)Args::OUTPUT_FORMAT},
	{"o", required_argument, NULL, (int)Args::OUTPUT_DESTINATION},
	{"output", required_argument, NULL, (int)Args::OUTPUT_DESTINATION},
	{"n", required_argument, NULL, (int)Args::IP_ADDRESS}, //use of 'n' is deprecated, use 'o'
	//instead.
	{"p", required_argument, NULL, (int)Args::PORT}, //use of 'p' is deprecated, use 'o'
	//instead.
	{"or", required_argument, NULL, (int)Args::OUTPUT_RESOLUTION},
	{"output-resolution", required_argument, NULL, (int)Args::OUTPUT_RESOLUTION},
	{"r", required_argument, NULL, (int)Args::DEFUNCT_RESOLUTION}, //outputs error message
	{"fr", required_argument, NULL, (int)Args::FRAME_RATE},
	{"frame-rate", required_argument, NULL, (int)Args::FRAME_RATE},
	{"br", required_argument, NULL, (int)Args::BIT_RATE},
	{"bit-rate", required_argument, NULL, (int)Args::BIT_RATE},
	{"abr", required_argument, NULL, (int)Args::AUDIO_BIT_RATE},
	{"audio-bit-rate", required_argument, NULL, (int)Args::AUDIO_BIT_RATE},
	{"hp", required_argument, NULL, (int)Args::H264_PROFILE},
	{"h264-profile", required_argument, NULL, (int)Args::H264_PROFILE},
	{"hl", required_argument, NULL, (int)Args::H264_LEVEL},
	{"h264-level", required_argument, NULL, (int)Args::H264_LEVEL},
	{"h", no_argument, NULL, (int)Args::HELP},
	{"?", no_argument, NULL, (int)Args::HELP},
	{"help", no_argument, NULL, (int)Args::HELP},
	{"hh", no_argument, NULL, (int)Args::FULL_HELP},
	{"??", no_argument, NULL, (int)Args::FULL_HELP},
	{"full-help", no_argument, NULL, (int)Args::FULL_HELP},
	{"v", no_argument, NULL, (int)Args::VERSION},
	{"version", no_argument, NULL, (int)Args::VERSION},
	{"P", required_argument, NULL, (int)Args::PID_PATH},
	{"pid", required_argument, NULL, (int)Args::PID_PATH},
	{NULL, 0, NULL, 0}
};

	int longIndex;
	int optionIndex=0;

	const char emptyString[]="";
	unsigned currentOptionIndex;
	try {
		opterr=0; //Silence getopt error messages.
		while (true) {
			currentOptionIndex=optind;
			opt = getopt_long_only(argc, argv, ":", longOpts, &longIndex);
			if( opt == -1 ) {
				break; //We are done with loop/
			}
			optionIndex+=1;

			if( opt == '?' ) {
				option_error(process.getName(), argv[currentOptionIndex], "Unknown option.");
				std::cerr << std::endl;
				std::cerr << "Try '" << process.getName() << " -h' for more information." << std::endl;
				return EXIT_FAILURE;
			}
			if( opt == ':' ) {
				optarg=(char *)emptyString;
				parameter_error(process.getName(), argv[currentOptionIndex], "Parameter must have non-empty argument.");
				std::cerr << std::endl;
				std::cerr << "Try '" << process.getName() << " -h' for more information." << std::endl;
				return EXIT_FAILURE;
			}

			Args arg=(Args)opt;
			switch (arg) {
				case Args::INPUT_SOURCE: {
					if (std::string(optarg) == "composite") {
						inputSettings.setSource(InputSource::Composite);
					} else if (std::string(optarg) == "component") {
						inputSettings.setSource(InputSource::Component);
					} else if (std::string(optarg) == "hdmi") {
						inputSettings.setSource(InputSource::HDMI);
					} else if (std::string(optarg) == "auto") {
						inputSettings.setSource(InputSource::Unknown);
					} else {
						const std::vector<std::string> arguments = {"composite", "component", "hdmi", "auto"};
						parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
						return EXIT_FAILURE;
					}
					break;
				}
				case Args::COLOR_SPACE: {
					if (std::string(optarg) == "yuv") {
						inputSettings.setColorSpace(ColorSpace::YUV);
					} else if (std::string(optarg) == "rgb") {
						inputSettings.setColorSpace(ColorSpace::RGB);
					} else if (std::string(optarg) == "auto") {
						inputSettings.setColorSpace(ColorSpace::Unknown);
					} else {
						const std::vector<std::string> arguments = {"yuv", "rgb", "auto"};
						parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
						return EXIT_FAILURE;
					}
					break;
				}
				case Args::INPUT_RESOLUTION: {
					if (std::string(optarg) == "ntsc") {
						inputSettings.setResolution(Resolution::NTSC);
					} else if (std::string(optarg) == "480") {
						inputSettings.setResolution(Resolution::NTSC);
					} else if (std::string(optarg) == "pal") {
						inputSettings.setResolution(Resolution::PAL);
					} else if (std::string(optarg) == "576") {
						inputSettings.setResolution(Resolution::PAL);
					} else if (std::string(optarg) == "720") {
						inputSettings.setResolution(Resolution::HD720);
					} else if (std::string(optarg) == "1080") {
						inputSettings.setResolution(Resolution::HD1080);
					} else if (std::string(optarg) == "auto") {
						inputSettings.setResolution(Resolution::Unknown);
					} else {
						unsigned long x,y;
						bool valid=parseNumericResolution( x, y, std::string(optarg) );
						if( valid ) {
							Resolution grabResolution;
							try {
								grabResolution=convertResolution( x, y );
							} catch (setting_error &e) {
								throw setting_error( "Input resolution specified is not supported." );
							}
							inputSettings.setResolution(grabResolution);
						} else {
							const std::vector<std::string> arguments = {"ntsc", "pal", "720", "1080", "auto"};
							parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
							return EXIT_FAILURE;
						}
					}
					break;
				}
				case Args::INPUT_SCAN_INTERLACED: {
					inputSettings.setScanMode(ScanMode::Interlaced);
					break;
				}
				case Args::INPUT_SCAN_PROGRESSIVE: {
					inputSettings.setScanMode(ScanMode::Progressive);
					break;
				}
				case Args::INPUT_SCAN_AUTO: {
					inputSettings.setScanMode(ScanMode::Unknown);
					break;
				}
				case Args::INPUT_REFRESH_RATE: {
					char *end;
					double value=strtod(optarg, &end);
					if( *end != 0 ) {
						parameter_error(process.getName(), argv[currentOptionIndex], "Must be a number.");
						return EXIT_FAILURE;
					}
					if(value < 0.0) { //Check for negatives, the
						parameter_error(process.getName(), argv[currentOptionIndex], "Must not be negative.");
						return EXIT_FAILURE;
					}
					//Let the options sanity checker handle subsequent restrictions.
					//Oh, let's round this, so we handle inputs like 29.97 instead of 30
					value=std::round(value);
					inputSettings.setRefreshRate(value);
					break;
				}
				case Args::INPUT_STRETCH: {
					inputSettings.setSDStretch(true);
					break;
				}
				case Args::INPUT_NO_STRETCH: {
					inputSettings.setSDStretch(false);
					break;
				}
				case Args::OUTPUT_FORMAT: {
					outputFormatSet=true;
					if (std::string(optarg) == "disk") {
						format = Format::Disk;
					} else if (std::string(optarg) == "fifo") {
						format = Format::FIFO;
					} else if (std::string(optarg) == "socket") {
						format = Format::Socket;
					} else {
						const std::vector<std::string> arguments = {"disk", "fifo", "socket"};
						parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
						return EXIT_FAILURE;
					}
					break;
				}
				case Args::OUTPUT_DESTINATION: {
					destinationSet=true;
					output = std::string(optarg);
					outputSetIndex=optionIndex; //Deal with merging with -n -p options.
					break;
				}
				case Args::IP_ADDRESS: {
					ip = std::string(optarg);
					ipSetIndex=optionIndex;
					break;
				}
				case Args::PORT: {
					port = std::string(optarg);
					portSetIndex=optionIndex;
					break;
				}
				case Args::OUTPUT_RESOLUTION: {
					if (std::string(optarg) == "ntsc") {
						transcoderSettings.setResolution(Resolution::NTSC);
					} else if (std::string(optarg) == "480") {
						transcoderSettings.setResolution(Resolution::NTSC);
					} else if (std::string(optarg) == "pal") {
						transcoderSettings.setResolution(Resolution::PAL);
					} else if (std::string(optarg) == "576") {
						transcoderSettings.setResolution(Resolution::PAL);
					} else if (std::string(optarg) == "720") {
						transcoderSettings.setResolution(Resolution::HD720);
					} else if (std::string(optarg) == "1080") {
						transcoderSettings.setResolution(Resolution::HD1080);
					} else if (std::string(optarg) == "auto") {
						transcoderSettings.setResolution(Resolution::Unknown);
					} else {
						unsigned long x,y;
						bool valid=parseNumericResolution( x, y, std::string(optarg) );
						if( valid ) {
							transcoderSettings.setResolution(x, y);
						} else {
							const std::vector<std::string> arguments = {"ntsc", "pal", "720", "1080", "auto"};
							parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
							return EXIT_FAILURE;
						}
					}
					break;
				}
				case Args::DEFUNCT_RESOLUTION: {
					option_error(process.getName(), argv[currentOptionIndex], "This is no longer an accepted argument. Use '-ir' or '-or' instead.");
					return EXIT_FAILURE;
					break;
				}
				case Args::FRAME_RATE: {
					option_error(process.getName(), argv[currentOptionIndex], "Frame rate settings do not currently work.");
					return EXIT_FAILURE;
#if 0
					char *end;
					double value=strtod(optarg, &end);
					if( *end != 0 ) {
						parameter_error(process.getName(), argv[currentOptionIndex], "Must be a number.");
						return EXIT_FAILURE;
					}
					if(value < 0.0) { //Check for negatives, the
						parameter_error(process.getName(), argv[currentOptionIndex], "Must not be negative.");
						return EXIT_FAILURE;
					}
					//Let the options sanity checker handle subsequent restrictions.
					transcoderSettings.setFrameRate( value );
#endif
					break;
				}
				case Args::BIT_RATE: {
					if (std::string(optarg) == "auto") {
						transcoderSettings.setBitRateMode( BitRateMode::Constant );
						transcoderSettings.setConstantBitRateMbps( 0.0 );
						transcoderSettings.setVariableBitRateMbps( 0.0, 0.0, 0.0 );
					} else {
						std::vector<std::string> bitRateParts=Utility::split(optarg, ':');
						std::vector<float> bitRateFloatParts;

						for( auto it = bitRateParts.begin(); it != bitRateParts.end(); ++it) {
							char *endPtr;
							std::string copy=*it;
							copy=Utility::trim(copy); //Modifies copy

							float value=strtof( copy.c_str(), &endPtr );

							if( *endPtr != 0 ) {
								//Wasn't a valid number.
								std::cerr << "Could not understand bit rate string: '" << optarg << "'" << std::endl;
								return EXIT_FAILURE;
							}
							if( value < 0.0 ) {
								std::cerr << "Cannot specify negative number for bit rate: '" << optarg << "'" << std::endl;
								return EXIT_FAILURE;
							}
							bitRateFloatParts.push_back( value );
						}
						if( bitRateFloatParts.size() == 1 ) {
							transcoderSettings.setConstantBitRateMbps( bitRateFloatParts[0] );
							transcoderSettings.setBitRateMode( BitRateMode::Constant );
							//Let the validity checker find any more errors.
						} else if( bitRateFloatParts.size() == 3 ) {
							transcoderSettings.setVariableBitRateMbps( bitRateFloatParts[0],
									bitRateFloatParts[1],
									bitRateFloatParts[2] );
							transcoderSettings.setBitRateMode( BitRateMode::Variable );
							//Let the validity checker find any more errors.
						} else {
							std::cerr << "Bit rate must be specified as single number, or in form max:average:min" << std::endl;
							return EXIT_FAILURE;
						}
					}
					break;
				}
				case Args::AUDIO_BIT_RATE: {
					char *end;
					float value=strtof(optarg, &end);
					if( *end != 0 ) {
						parameter_error(process.getName(), argv[currentOptionIndex], "Must be a number.");
						return EXIT_FAILURE;
					}
					if(value < 0.0) { //Check for negatives, the
						parameter_error(process.getName(), argv[currentOptionIndex], "Must not be negative.");
						return EXIT_FAILURE;
					}
					try {
						//Let the options sanity checker handle other restrictions.
						transcoderSettings.setAudioBitRate( (unsigned)std::round(value) );
					} catch ( setting_error & e) {
						const std::vector<std::string> arguments =
						{"64", "96", "112", "128", "160", "192", "224", "256", "320", "384"};
						parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
						return EXIT_FAILURE;
					}

					break;
				}
				case Args::H264_PROFILE: {
					if (std::string(optarg) == "baseline") {
						transcoderSettings.setH264Profile( H264Profile::Baseline );
					} else if (std::string(optarg) == "main") {
						transcoderSettings.setH264Profile( H264Profile::Main );
					} else if (std::string(optarg) == "high") {
						transcoderSettings.setH264Profile( H264Profile::High );
					} else {
						const std::vector<std::string> arguments = {"baseline", "main", "high"};
						parameter_unknown(process.getName(), argv[currentOptionIndex], arguments);
						return EXIT_FAILURE;
					}
					break;
				}
				case Args::H264_LEVEL: {
					if (std::string(optarg) == "auto") {
						transcoderSettings.setH264Level( 0.0 );
					} else {
						char *end;
						float value=strtof(optarg, &end);
						if( *end != 0 ) {
							parameter_error(process.getName(), argv[currentOptionIndex], "Must be a number or `auto`.");
							return EXIT_FAILURE;
						}
						//Let the options sanity checker handle restrictions.
						transcoderSettings.setH264Level( value );
					}
					break;
				}
				case Args::HELP: {
					help(process.getName(), false);
					return EXIT_SUCCESS;
					break;
				}
				case Args::FULL_HELP: {
					help(process.getName(), true);
					return EXIT_SUCCESS;
					break;
				}
				case Args::VERSION: {
					version(process.getName(), process.getVersion());
					return EXIT_SUCCESS;
					break;
				}
				case Args::PID_PATH: {
					option_error(process.getName(), argv[currentOptionIndex], "PID file support not currently working.");
					return EXIT_FAILURE;
#if 0
					pid = std::string(optarg);
#endif
					break;
				}
				default: {
					std::cerr << "Unexpected error." << std::endl;
					std::cerr << std::endl;
					std::cerr << "Try '" << process.getName() << " -h' for more information." << std::endl;
					return EXIT_FAILURE;
					break;
				}
			}
		}
		if(( argc - optind ) > 1 ) {
			std::cerr << "Do not understand '" << argv[optind+1] << "'." << std::endl;
			std::cerr << "There can only be one non-option argument." << std::endl;
			std::cerr << std::endl;
			std::cerr << "Try '" << process.getName() << " -h' for more information." << std::endl;
			return EXIT_FAILURE;
		}
		if(( argc - optind ) == 1 ) {
			if( outputSetIndex == 0 ) {
				destinationSet = true;
				output = std::string(argv[optind]);
				outputSetIndex=INT_MAX; //Set at end effectively.
			} else {
				std::cerr << "Non-option parameter `" << argv[optind] << "' cannot specified in addition to -o or -output option." << std::endl;
				return EXIT_FAILURE;
			}
		}
	} catch ( setting_error &error ) {
		std::cerr << std::endl << error.what() << std::endl;
		return EXIT_FAILURE;
	}

	// TODO not ready for primetime yet, program needs to be restarted too
	// often at the moment
	// create PID file for single instance mechanism
	//  if (process.createPid(pidPath)) {
	//      return EXIT_FAILURE;
	//  }

	if( !outputFormatSet ) {
		if( destinationSet ) {
			format = Format::Disk;
		} else {
			format = Format::FIFO;
		}
	}
	if (format == Format::Disk) {
		if( !destinationSet ) {
			std::cerr << "Must specify <destination> file when using `disk` output format." << std::endl;
			return EXIT_FAILURE;
		}
	}

	//Deal with merging output, ip, and port
	if (format == Format::Socket) {
		std::string address;
		std::string ipPort;

		if( !Utility::splitIPAddressAndPort(address, ipPort, output) ) {
			std::cerr << "The address: '" << output << "' is invalid." << std::endl;
			return EXIT_FAILURE;
		}
		if( address.size() > 0 ) {
			if( outputSetIndex > ipSetIndex ) {
				ip=address;
			}
		}
		if( ipPort.size() > 0 ) {
			if( outputSetIndex > portSetIndex ) {
				port=ipPort;
			}
		}
	}

	//Try block wraps GCHD creation so if exception gets thrown,
	//stack unwinding will destruct object, calling uninit.
	try {
		GCHD gchd(&process, inputSettings, transcoderSettings);

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
	} catch ( setting_error &error ) {
		std::cerr << std::endl << error.what() << std::endl;
		return EXIT_FAILURE;
	} catch ( std::logic_error &error ) {
		std::cerr << std::endl << "LOGIC ERROR: " << error.what() << std::endl << std::endl;
		return EXIT_FAILURE;
	} catch ( std::runtime_error &error ) {
		//All my exceptions inherit from runtime_error or logic_error.
		std::cerr << std::endl << "FATAL ERROR: " << error.what() << std::endl << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
