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

void GCHD::configureComposite()
{
	mailWrite( 0x33, VC{0x94, 0x41, 0x37} );
	mailWrite( 0x33, VC{0x94, 0x4a, 0xaf} );
	mailWrite( 0x33, VC{0x94, 0x4b, 0xaf} );

	//MODE AUTODETECT IS HERE
	mailWrite( 0x33, VC{0x89, 0x89, 0xfa} );
	uint8_t value=mailRead( 0x33, 1 )[0];
	value = value & 0xf;

	ScanMode autodetectScanMode=ScanMode::Interlaced;
	double autodetectRefreshRate=0.0;
	Resolution autodetectResolution;
	try {
		if( value == 6 ) {
			autodetectResolution = Resolution::NTSC;
			autodetectRefreshRate = 60.0;
		} else if ( value == 7 ) {
			autodetectResolution = Resolution::PAL;
			autodetectRefreshRate = 50.0;
		} else {
			if( passedInputSettings_.getResolution() == Resolution::Unknown ) {
				throw std::runtime_error( "Mode detection failed, does not appear to be a supported mode for Composite.");
			}
		}
		if (autodetectRefreshRate == 0.0) {
			switch(currentInputSettings_.getResolution()) {
				case Resolution::NTSC:
					autodetectRefreshRate=60.0;
					break;
				case Resolution::PAL:
					autodetectRefreshRate=50.0;
					break;
				default:
					throw setting_error( "Mode is not a supported mode for Composite.");
					break;
			}
		}
		//Merge passed arguments and autodetect information.
		currentInputSettings_.mergeAutodetect( passedInputSettings_,
						       autodetectResolution,
						       autodetectScanMode,
						       autodetectRefreshRate );
		if( passedInputSettings_.getColorSpace()==ColorSpace::Unknown ) {
			currentInputSettings_.setColorSpace( ColorSpace::YUV ); //Composite=YUV unless overridden.
		} else {
			currentInputSettings_.setColorSpace( passedInputSettings_.getColorSpace() );
		}
		currentTranscoderSettings_.mergeAutodetect( passedTranscoderSettings_, currentInputSettings_ );
	} catch( usb_error &error ) {
		throw;
	} catch( std::runtime_error &error ) {
		//This snippet prevents lockup.
		//No idea how, seems all of it needs to be there.
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
		throw; //Do not "throw error", that causes exception slicing.
	}

	if (currentInputSettings_.getResolution()==Resolution::NTSC) {
		mailWrite( 0x44, VC{0x07, 0x8a} );
		mailWrite( 0x44, VC{0x08, 0x9b} );
		mailWrite( 0x44, VC{0x09, 0x7a} );
		mailWrite( 0x44, VC{0x28, 0x88} );
		mailWrite( 0x33, VC{0x89, 0x89, 0xfd} );
		mailRead( 0x33, 1 ); //EXPECTED {0x6e}
		mailWrite( 0x44, VC{0x06, 0x08} );
	}
	mailWrite( 0x33, VC{0x89, 0x89, 0xe7} );
	mailRead( 0x33, 1 ); //EXPECTED {0xe9}

	if (currentInputSettings_.getResolution()==Resolution::PAL) {
		mailWrite( 0x33, VC{0x89, 0x89, 0xf1} );
		mailRead( 0x33, 1 ); //EXPECTED {0x63}
		mailWrite( 0x4c, VC{0x04, 0x95} );
	}
	//Really probably are fine just being repeated
	//below, some captures have this here,
	//some skip it, probably based on thread readiness.
	configureSetupSubblock();
	configureCommonBlockB1( false );

	//Should subroutineize this!
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
	mailWrite( 0x4c, VC{0x0d, 0xc8} ); //This isn't in all captures at this point, but usually appears soon later
	//Putting it here probably won't hurt, and will help it from being omitted.
	//It may also be a bank switch that is checked before being written.
	mailWrite( 0x33, VC{0x99, 0x89, 0xf5} );
	uint8_t status=mailRead( 0x33, 1 )[0];
	uint8_t mask  = status & 0x10;

	mailWrite( 0x4c, VC{0x0e, (uint8_t) (mask | 0x65)} );
	mailWrite( 0x4c, VC{0x0e, (uint8_t) (mask | 0x64)} );

	//VERY LITTLE BEFORE THIS
	configureCommonBlockA();
	configureSetupSubblock();
	configureCommonBlockB1(false);
	configureCommonBlockB2();
	configureCommonBlockB3();

	configureCommonBlockC();
	configureCommonBlockB3(); //Pretty sure not necessary to repeat.

	transcoderFinalConfigure( currentInputSettings_ , currentTranscoderSettings_ );
	transcoderSetup( currentInputSettings_, currentTranscoderSettings_ );

	scmd(SCMD_INIT, 0xa0, 0x0000);
	uint16_t state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); //EXPECTED=0x0001
	scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_START);
	completeStateChange(state, SCMD_STATE_START); //EXPECTED 0x0001
}
