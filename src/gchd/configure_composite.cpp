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

	interlaced_=true;
	if (settings_->getInputResolution() == Resolution::Unknown) {
		if( value == 6 ) {
			settings_->setInputResolution( Resolution::NTSC );
		} else if ( value == 7 ) {
			settings_->setInputResolution( Resolution::PAL );
		} else {
			throw runtime_error( "Mode detection failed, does not appear to be a supported mode for Composite.");
		}
	}
	switch(settings_->getInputResolution()) {
		case Resolution::NTSC:
			refreshRate_=60;
			break;
		case Resolution::PAL:
			refreshRate_=50;
			break;
		default:
			throw runtime_error( "Mode is not a supported mode for Composite.");
			break;
	}

	if (settings_->getInputResolution()==Resolution::NTSC) {
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


	if (settings_->getInputResolution()==Resolution::PAL) {
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

	transcoderFinalConfigure();
	transcoderSetup();

	scmd(SCMD_INIT, 0xa0, 0x0000);
	uint16_t state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); //EXPECTED=0x0001
	scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_START);
	completeStateChange(state, SCMD_STATE_START); //EXPECTED 0x0001
}
