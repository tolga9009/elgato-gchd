/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <unistd.h>
#include <exception>
#include <cmath>
#include "../gchd.hpp"

void GCHD::readHdmiSignalInformation( unsigned &sum6463, unsigned &countSum6463,
				      unsigned &sum6665, unsigned &countSum6665, bool &rgbBit)
{
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	unsigned value6665  = readDevice0x9DCD(0x66) <<8;
	value6665 |= readDevice0x9DCD(0x65);
	sum6665 += value6665;
	countSum6665 += 1;

	unsigned value6463=readDevice0x9DCD(0x64) <<8;
	value6463 |= readDevice0x9DCD(0x63);
	sum6463 += value6463;
	countSum6463 += 1;

	mailWrite( 0x4e, VC{0x00, 0xce} ); //bank switch.
	uint8_t value=readDevice0x9DCD(0x34);
	rgbBit=(value >>2) & 1;
}

void GCHD::configureHDMI()
{
	mailWrite( 0x33, VC{0x94, 0x41, 0x37} );
	mailWrite( 0x33, VC{0x94, 0x4a, 0xaf} );
	mailWrite( 0x33, VC{0x94, 0x4b, 0xaf} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	readDevice0x9DCD(0x94); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0xab, 0x4c} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0x00, 0xce} );
	mailWrite( 0x4e, VC{0x1b, 0x33} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	readDevice0x9DCD(0x88); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0xb7, 0xce} );
	mailWrite( 0x4e, VC{0xb8, 0xdc} );
	mailWrite( 0x4e, VC{0xb8, 0xcc} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0x00, 0xce} );
	mailWrite( 0x4e, VC{0x07, 0x38} );
	mailWrite( 0x4e, VC{0x07, 0xc8} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	mailWrite( 0x4e, VC{0x51, 0x45} );
	readDevice0x9DCD(0x88); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0xb7, 0xcc} );

	//Do nothing enable. No idea what it really checks
	doEnable( 0x0000, 0x0000); //state 031e->031e, enable 000a->000a
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0x00, 0xce} );
	readDevice0x9DCD(0x3e); //EXPECTED 0xd3
	mailWrite( 0x4e, VC{0x01, 0xad} );
	readDevice0x9DCD(0x3b); //EXPECTED 0xb3
	mailWrite( 0x4e, VC{0x04, 0xcd} );
	mailWrite( 0x4e, VC{0x06, 0xc4} );
	readDevice0x9DCD(0x36); //EXPECTED 0xba
	mailWrite( 0x4e, VC{0x09, 0xe4} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	readDevice0x9DCD(0x6b); //EXPECTED 0x82
	mailWrite( 0x4e, VC{0x54, 0xec} );
	readDevice0x9DCD(0x93); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0xac, 0x4c} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0x00, 0x4c} );
	readDevice0x9DCD(0x3f); //EXPECTED 0x32
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	readDevice0x9DCD(0xf1); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0xce, 0x4c} );
	readDevice0x9DCD(0xf0); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0xcf, 0xce} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2

	unsigned sum6665=0;
	unsigned sum6463=0;
	unsigned countSum6665=0;
	unsigned countSum6463=0;
	double value6665;
	bool rgbBit;

	unsigned iterationCount=0;
	do {
		if(iterationCount >= 10) { //Roughly 2 seconds given usleep below, after firmware load.
			throw runtime_error( "No HDMI signal found.");
		}
		sum6665=0;
		sum6463=0;
		countSum6665=0;
		countSum6463=0;
		//When first read information is bad, and it seems to kick off lock.
		for( unsigned j = 0; j<10; ++j ) { //Read 10 times for average. Probably not at all necessary.
			readHdmiSignalInformation(sum6463, countSum6463, sum6665, countSum6665, rgbBit);
		}
		value6665=((double)sum6665) / countSum6665;

		usleep( 1000000 / 5 ); //1/5th of a second, long enough time for more than 4 frames at low frame rate.
		//Should be enough to lock on.

		iterationCount+=1;
	} while( (iterationCount < 2) || (std::abs( value6665 - 0xad4d )<10.0) );
	//^^^^^ Need to go through loop at least twice, to stabilize read, and value read must not be value
	//gotten when no signal

	interlaced_=false;
	if (settings_->getInputResolution() == Resolution::Unknown) {
		double value6463=((double)sum6463) / countSum6463;
		//0xb690
		if(fabs( value6463 - 0xb6d7 )<10.0) { //Allow for error.
			//1080p
			settings_->setInputResolution( Resolution::HD1080 );
		} else if(fabs( value6463 - 0xb081 )<10.0) { //0xb123
			//1080i
			settings_->setInputResolution( Resolution::HD1080 );
			interlaced_=true;
		} else if(std::abs( value6463 - 0xb05c )<10.0) { //Allow for error.
			//720p
			settings_->setInputResolution( Resolution::HD720 );
		} else if(std::abs( value6463 - 0xb0c1 )<12.0) { //480p is 0xb0bf,
			//576p is 0xb0c3
			//midpoint is 0xb0c1
			//0xba95 is NTSC, bb75 is PAL. Code here has a slight NTSC
			//region bias. ;)
			printf("6665: 0x%4.4x\n", (unsigned)value6665);
			settings_->setInputResolution( Resolution::NTSC );
			if( std::abs( value6665 - 0xbb75 ) < 10 ) {
				settings_->setInputResolution( Resolution::PAL );
			}
		} else {
			throw runtime_error( "Mode detection failed, does not appear to be a suported mode for HDMI.");
		}
	}
	refreshRate_=60; //Currently we don't support anything other than 60hz / 59.94hz modes.
	mailWrite( 0x4e, VC{0x00, 0xcc} );

	//Mystery setup difference. May be more appropriate to switch on reading back 0x95 instead of 0x97
	//Based on input resolution.
	if (settings_->getInputResolution() == Resolution::HD720) {
		mailWrite( 0x4e, VC{0xb2, 0xcc} );
		mailWrite( 0x4e, VC{0xb5, 0xcc} );
	} else { //Assumed 1080.
		readDevice0x9DCD(0x8d); //EXPECTED 0xb2
		mailWrite( 0x4e, VC{0xb2, 0xc4} );
		readDevice0x9DCD(0x8a); //EXPECTED 0xb2
		mailWrite( 0x4e, VC{0xb5, 0xd0} );
	}
	mailWrite( 0x4e, VC{0x00, 0xce} );
	mailWrite( 0x4e, VC{0x1b, 0x30} );

	mailWrite( 0x4e, VC{0x1f, 0xdc} );
	readDevice0x9DCD(0x29); //EXPECTED 0xba
	mailWrite( 0x4e, VC{0x1f, 0xcc} );
	readDevice0x9DCD(0x3b); //EXPECTED 0xb3
	mailWrite( 0x4e, VC{0x04, 0xcf} );
	mailWrite( 0x4e, VC{0x04, 0xcd} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0x00, 0xcc} );
	mailWrite( 0x4e, VC{0x40, 0xcc} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2
	mailWrite( 0x4e, VC{0x00, 0xcd} );

	mailWrite( 0x4e, VC{0x00, 0xcc} );
	mailWrite( 0x4e, VC{0xb0, 0xe8} );
	mailWrite( 0x4e, VC{0xb1, 0x0c} );
	mailWrite( 0x4e, VC{0xad, 0xc9} );
	readDevice0x9DCD(0x8f); //EXPECTED 0x96
	mailWrite( 0x4e, VC{0xb0, 0xe9} );
	readDevice0x9DCD(0x3f); //EXPECTED 0xb2

	mailWrite( 0x4e, VC{0x00, 0xcc} );
	mailWrite( 0x4e, VC{0xab, 0xcc} );
	mailWrite( 0x33, VC{0x99, 0x89, 0xf5} );
	mailRead( 0x33, 1 ); //EXPECTED {0x82}
	mailWrite( 0x33, VC{0x99, 0x89, 0xfd} );
	mailRead( 0x33, 1 ); //EXPECTED {0x6f}
	mailWrite( 0x33, VC{0x99, 0x89, 0xf5} );
	mailRead( 0x33, 1 ); //EXPECTED {0x82}
	mailWrite( 0x33, VC{0x99, 0x89, 0xfc} );
	mailRead( 0x33, 1 ); //EXPECTED {0x6e}
	mailWrite( 0x33, VC{0x99, 0x89, 0xf3} );
	mailRead( 0x33, 1 ); //EXPECTED {0x6e}
	mailWrite( 0x4c, VC{0x0c, 0x89} );
	mailWrite( 0x33, VC{0x99, 0x89, 0xf5} );
	mailRead( 0x33, 1 ); //EXPECTED {0x82}
	mailWrite( 0x4c, VC{0x0e, 0x65} );
	mailWrite( 0x4c, VC{0x0e, 0x64} );
	configureCommonBlockA();

	configureSetupSubblock();

	mailWrite( 0x33, VC{0x99, 0x89, 0x6b} );
	mailRead( 0x33, 1 ); //EXPECTED {0x6e}
	mailWrite( 0x4c, VC{0xc0, 0x80} );
	mailWrite( 0x4c, VC{0xa2, 0x77} );
	mailWrite( 0x33, VC{0x99, 0x89, 0x58} );
	mailRead( 0x33, 1 ); //EXPECTED {0x91}
	mailWrite( 0x4c, VC{0xc0, 0x77} );

	bool mysteryParameter=false;
	if (settings_->getInputResolution() != Resolution::HD1080) {
		if( !interlaced_ ) {
			mysteryParameter=true; //Old theory was that high speed
			//images need this and it controls a clock
			//rate. But probably unlikely.
			//
			//Just as likely, user configureable small
			//detail---I've now seen HDMI 1080p captures with this on
			//and off.
		}
	}
	configureCommonBlockB1(mysteryParameter);
	configureCommonBlockB2();
	configureCommonBlockB3();
	configureCommonBlockC();
	readHdmiSignalInformation( sum6463, countSum6463, sum6665, countSum6665, rgbBit);

	if( settings_->getColorSpace()==ColorSpace::Unknown ) {
		if( rgbBit ) {
			settings_->setColorSpace(ColorSpace::RGB);
		} else {
			settings_->setColorSpace(ColorSpace::YUV);
		}
	}

	mailWrite( 0x4e, VC{0x00, 0xcc} ); //Make sure on right bank for color space configure.
	configureColorSpace();
	transcoderFinalConfigure();
	transcoderSetup();

	scmd(SCMD_INIT, 0xa0, 0x0000);
	uint16_t state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); //EXPECTED=0x0001
	scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_START);
	completeStateChange(state, SCMD_STATE_START); //EXPECTED 0x0001
}




