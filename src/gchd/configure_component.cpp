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

//make sure on bank 0x4e, (0x00, 0xcc) before using.
void GCHD::readComponentSignalInformation(unsigned &sum6867, unsigned &countSum6867,
                                          unsigned &sum6665, unsigned &countSum6665)
{
    unsigned value6665  = readDevice0x9DCD(0x66) <<8; 
    value6665 |= readDevice0x9DCD(0x65); 
    sum6665 += value6665;
    countSum6665 += 1;
 
    unsigned value6867  = readDevice0x9DCD(0x68) <<8;
    value6867 |= readDevice0x9DCD(0x67);
    sum6867 += value6867;
    countSum6867 += 1;

    //register 0x60 might contain interlacing information.
    // 0xbd for HD1080i60/0xb8 for 576i60/0xb2 for 1080p30, 576p60
}

void GCHD::configureComponent()
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
	mailWrite( 0x4e, VC{0x51, 0xed} ); //Changed from HDMI.
	readDevice0x9DCD(0x88); //EXPECTED 0xb0
	mailWrite( 0x4e, VC{0xb7, 0xce} ); //Changed from HDMI

    //Do nothing enable. No idea what it really checks
	doEnable( 0x0000, 0x0000); //state 031e->031e, enable 000a->000a

    mailWrite( 0x4e, VC{0x24, 0x0c} );
    mailWrite( 0x4e, VC{0x21, 0xcd} );
    mailWrite( 0x4e, VC{0x91, 0xc8} );
    mailWrite( 0x4e, VC{0x0e, 0x8c} );
    mailWrite( 0x4e, VC{0x11, 0xec} );
	mailWrite( 0x4e, VC{0x71, 0x4c} );
	mailWrite( 0x4e, VC{0x04, 0xcc} ); //THIS HAS DIFFERENT VALUE
                                       //For non-1080 captures, but I have no idea where and how they
                                       //detect it, or what setting it is based on.
                                       //Leaving it 0xcc right now for all and hoping it still works

    //MODE AUTODETECT IS HERE
    unsigned sum6867=0;
    unsigned sum6665=0;
    unsigned countSum6867=0;
    unsigned countSum6665=0;
    double value6665;

    unsigned iterationCount=0;
    do {
        if(iterationCount >= 10) { //Roughly 2 seconds given usleep below, after firmware load.
            throw runtime_error( "No HDMI signal found.");
        }
        sum6665=0;
        sum6867=0;
        countSum6665=0;
        countSum6867=0;

        for( unsigned j = 0; j<10; ++j ) { //Read 10 times for average. Probably not at all necessary.
             readComponentSignalInformation(sum6867, countSum6867, sum6665, countSum6665);
        }
        value6665=((double)sum6665) / countSum6665;

        usleep( 1000000 / 5 ); //1/5th of a second, long enough time for more than four frames at low frame rate.
                               //Enough time to lock

        iterationCount+=1;
    } while( (iterationCount < 3) || (std::abs( value6665 - 0xad4d )<10.0) );
    //^^^^^ Need to go through loop at least twice, to stabilize read, and value read must not be value
    //gotten when no signal

    Resolution autodetectResolution=Resolution::Unknown;
    ScanMode autodetectScanMode=ScanMode::Progressive; //Just defaults to progressive in absence of other
                                                       //information for Component.
    double autodetectRefreshRate=0.0;

    double value6867=((double)sum6867) / countSum6867;
    if(fabs( value6867 - 0xbbf4 )<10.0) {
        //HD1080p30
        autodetectResolution=Resolution::HD1080;
        autodetectRefreshRate=30;
    } else if (fabs( value6867 - 0xa03d)<10.0) { //Allow for error.
        //HD1080i60
        autodetectResolution=Resolution::HD1080;
        autodetectScanMode=ScanMode::Interlaced;
        autodetectRefreshRate=60;
    } else if (fabs( value6867 - 0xbf59)<10.0) {
        //HD720p60
        autodetectResolution=Resolution::HD720;
        autodetectRefreshRate=60;
    }  else if(fabs( value6867 - 0xa6b7 )<10.0) {
        //576p50
        autodetectResolution=Resolution::PAL;
        autodetectRefreshRate=50;
    }  else if(fabs( value6867 - 0x9ab9 )<10.0) {
        //576i50
        autodetectResolution=Resolution::PAL;
        autodetectScanMode=ScanMode::Interlaced;
        autodetectRefreshRate=50;
    }  else if(fabs( value6867 - 0xa150 )<10.0) {
        //480p60
        autodetectResolution = Resolution::NTSC;
        autodetectRefreshRate=60;
    }  else if(fabs( value6867 - 0x9576 )<10.0) {
        //480i60
        autodetectResolution=Resolution::NTSC;
        autodetectScanMode=ScanMode::Interlaced;
        autodetectRefreshRate=60;
    } else {
        if( passedInputSettings_.getResolution() == Resolution::Unknown ) {
            throw runtime_error( "Mode detection failed, does not appear to be a supported mode for Component.");
        }
    }
    ScanMode passedMode = passedInputSettings_.getScanMode();

    //Use autodetect or passed mode?
    ScanMode compareMode = (passedMode==ScanMode::Unknown) ? autodetectScanMode : passedMode;

    if( autodetectRefreshRate == 0.0 ) {
        switch( passedInputSettings_.getResolution() ) {
           case Resolution::HD1080:
                if( compareMode == ScanMode::Interlaced ) {
                    autodetectRefreshRate=60.0;
                } else {
                    autodetectRefreshRate=30.0;
                }
                break;
           case Resolution::PAL:
                autodetectRefreshRate=50.0;
                break;
           default:
                autodetectRefreshRate=60.0;
                break;
        }
    }

    //Merge passed arguments and autodetect information.
    currentInputSettings_.mergeAutodetect( passedInputSettings_, autodetectResolution, autodetectScanMode, autodetectRefreshRate );
    if( passedInputSettings_.getColorSpace()==ColorSpace::Unknown ) {
        currentInputSettings_.setColorSpace( ColorSpace::YUV ); //Component=YUV unless overridden.
    } else {
        currentInputSettings_.setColorSpace( passedInputSettings_.getColorSpace() );
    }
    currentTranscoderSettings_.mergeAutodetect( passedTranscoderSettings_, currentInputSettings_ );

    switch(currentInputSettings_.getResolution()) {
        case Resolution::NTSC:
        case Resolution::PAL:
            mailWrite( 0x4e, VC{0xb2, 0xcf} );
            break;
        default: //All others.
            mailWrite( 0x4e, VC{0xb2, 0xcc} );
            break;
    }
    mailWrite( 0x4e, VC{0xb5, 0xcc} );

    bool interlaced = currentInputSettings_.getScanMode()==ScanMode::Interlaced;

    switch(currentInputSettings_.getResolution()) {
        case Resolution::HD1080:
            if( interlaced ) { // Assuming HD1080i60
                mailWrite( 0x4e, VC{0x03, 0x94} );
                mailWrite( 0x4e, VC{0x05, 0xf4} );
                mailWrite( 0x4e, VC{0x06, 0xec} );
                mailWrite( 0x4e, VC{0x07, 0xfc} );
                mailWrite( 0x4e, VC{0x1e, 0xdd} );
                mailWrite( 0x4e, VC{0x1f, 0xcd} );
                mailWrite( 0x4e, VC{0x01, 0x45} );
                mailWrite( 0x4e, VC{0x02, 0xbc} );
            } else { //Assuming HD1080p30
                mailWrite( 0x4e, VC{0x03, 0x24} );
                mailWrite( 0x4e, VC{0x05, 0xf4} );
                mailWrite( 0x4e, VC{0x06, 0xfc} );
                mailWrite( 0x4e, VC{0x07, 0xac} );
                mailWrite( 0x4e, VC{0x1e, 0xcc} );
                mailWrite( 0x4e, VC{0x1f, 0xcc} );
                mailWrite( 0x4e, VC{0x01, 0x45} );
                mailWrite( 0x4e, VC{0x02, 0xbc} );
            }
            break;

        case Resolution::HD720:
            mailWrite( 0x4e, VC{0x03, 0x94} );
            mailWrite( 0x4e, VC{0x05, 0xf4} );
            mailWrite( 0x4e, VC{0x06, 0xec} );
            mailWrite( 0x4e, VC{0x07, 0xfc} );
            mailWrite( 0x4e, VC{0x1e, 0xdd} );
            mailWrite( 0x4e, VC{0x1f, 0xcd} );
            mailWrite( 0x4e, VC{0x01, 0xab} );
            mailWrite( 0x4e, VC{0x02, 0xdc} );
            break;

        case Resolution::PAL:
            if( interlaced ) {
                mailWrite( 0x4e, VC{0x03, 0xc4} );
                mailWrite( 0x4e, VC{0x05, 0xc4} );
                mailWrite( 0x4e, VC{0x06, 0xc4} );
                mailWrite( 0x4e, VC{0x07, 0xd4} );
                mailWrite( 0x4e, VC{0x1e, 0x88} );
                mailWrite( 0x4e, VC{0x1f, 0xc8} );
                mailWrite( 0x4e, VC{0x01, 0xf9} );
                mailWrite( 0x4e, VC{0x02, 0x3c} );
            } else {
                mailWrite( 0x4e, VC{0x03, 0xd4} );
                mailWrite( 0x4e, VC{0x05, 0xc4} );
                mailWrite( 0x4e, VC{0x06, 0xc4} );
                mailWrite( 0x4e, VC{0x07, 0xd4} );
                mailWrite( 0x4e, VC{0x1e, 0xff} );
                mailWrite( 0x4e, VC{0x1f, 0xcf} );
                mailWrite( 0x4e, VC{0x01, 0xf9} );
                mailWrite( 0x4e, VC{0x02, 0x3c} );
            }
            break;
        case Resolution::NTSC:
            if( interlaced ) {
                mailWrite( 0x4e, VC{0x03, 0xc4} );
                mailWrite( 0x4e, VC{0x05, 0xc4} );
                mailWrite( 0x4e, VC{0x06, 0xc4} );
                mailWrite( 0x4e, VC{0x07, 0xd4} );
                mailWrite( 0x4e, VC{0x1e, 0x88} );
                mailWrite( 0x4e, VC{0x1f, 0xc8} );
                mailWrite( 0x4e, VC{0x01, 0xf9} );
                mailWrite( 0x4e, VC{0x02, 0x5c} );
            } else {
                mailWrite( 0x4e, VC{0x03, 0xec} );
                mailWrite( 0x4e, VC{0x05, 0xc4} );
                mailWrite( 0x4e, VC{0x06, 0xc4} );
                mailWrite( 0x4e, VC{0x07, 0xd4} );
                mailWrite( 0x4e, VC{0x1e, 0xff} );
                mailWrite( 0x4e, VC{0x1f, 0xcf} );
                mailWrite( 0x4e, VC{0x01, 0xf9} );
                mailWrite( 0x4e, VC{0x02, 0x5c} );
            }
            break;
        default:
            throw runtime_error( "Current selected video mode is not a supported mode for Component.");
            break;
    }
    readDevice0x9DCD(0x2f); //EXPECTED 0x0a
    mailWrite( 0x4e, VC{0x10, 0x71} );
    readDevice0x9DCD(0x30); //EXPECTED 0xba
    mailWrite( 0x4e, VC{0x0f, 0xe4} );
    mailWrite( 0x4e, VC{0x12, 0xc8} );
    readDevice0x9DCD(0x28); //EXPECTED 0xb2
    mailWrite( 0x4e, VC{0x17, 0xce} );
    switch(currentInputSettings_.getResolution()) {
        case Resolution::NTSC:
        case Resolution::PAL:
            mailWrite( 0x4e, VC{0x12, 0xc8} );
            break;
        default: //All others.
            mailWrite( 0x4e, VC{0x12, 0xcc} );
            break;
    }
    mailWrite( 0x4e, VC{0x13, 0xd8} );
    mailWrite( 0x4e, VC{0x0b, 0x4c} );
    mailWrite( 0x4e, VC{0x0c, 0xac} );
    mailWrite( 0x4e, VC{0x0d, 0x4c} );
    mailWrite( 0x4e, VC{0x1b, 0xcc} );
    mailWrite( 0x4e, VC{0x1c, 0xcc} );
    mailWrite( 0x4e, VC{0x1d, 0xcc} );
    mailWrite( 0x4e, VC{0x18, 0xdc} );
    mailWrite( 0x4e, VC{0x19, 0xdc} );
    mailWrite( 0x4e, VC{0x1a, 0xdc} );
    mailWrite( 0x4e, VC{0x2d, 0xdd} );
    mailWrite( 0x4e, VC{0x2e, 0xdd} );
    mailWrite( 0x4e, VC{0x2f, 0xce} );
    mailWrite( 0x4e, VC{0x3a, 0xc0} );
    mailWrite( 0x4e, VC{0x3b, 0xc4} );
    readDevice0x9DCD(0x39); //EXPECTED 0x92/0x82 first is HD1080i60, second 108p30  might be interlace bit
    switch(currentInputSettings_.getResolution()) {
        case Resolution::HD1080:
            if( interlaced ) { // Assuming HD1080i60
                mailWrite( 0x4e, VC{0x39, 0x2c} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0x8c} );
                mailWrite( 0x4e, VC{0x12, 0xc8} );
                mailWrite( 0x4e, VC{0x13, 0xd8} );
                mailWrite( 0x4e, VC{0x17, 0xc7} );
                mailWrite( 0x4e, VC{0x21, 0xc6} );
                mailWrite( 0x4e, VC{0x20, 0xcc} );
                mailWrite( 0x4e, VC{0x40, 0xcc} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0x73} );
                mailWrite( 0x4e, VC{0x82, 0xcb} );
                mailWrite( 0x4e, VC{0x83, 0x4c} );
                mailWrite( 0x4e, VC{0x84, 0xdf} );
                mailWrite( 0x4e, VC{0x85, 0xee} );
                mailWrite( 0x4e, VC{0x86, 0xd0} );
            } else { //Assuming HD1080p30
                mailWrite( 0x4e, VC{0x39, 0x3c} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0x24} );
                mailWrite( 0x4e, VC{0x12, 0xcc} );
                mailWrite( 0x4e, VC{0x13, 0xc6} );
                mailWrite( 0x4e, VC{0x17, 0xc7} );
                mailWrite( 0x4e, VC{0x21, 0xcb} );
                mailWrite( 0x4e, VC{0x20, 0xcc} );
                mailWrite( 0x4e, VC{0x40, 0xcc} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0x73} );
                mailWrite( 0x4e, VC{0x82, 0xcb} );
                mailWrite( 0x4e, VC{0x83, 0x4c} );
                mailWrite( 0x4e, VC{0x84, 0xe4} );
                mailWrite( 0x4e, VC{0x85, 0xc8} );
                mailWrite( 0x4e, VC{0x86, 0xf4} );
            }
            break;
        case Resolution::HD720:
            mailWrite( 0x4e, VC{0x39, 0x2c} );
            mailWrite( 0x4e, VC{0x2c, 0x51} );
            mailWrite( 0x4e, VC{0x03, 0x8c} );
            mailWrite( 0x4e, VC{0x12, 0xc8} );
            mailWrite( 0x4e, VC{0x13, 0xd8} );
            mailWrite( 0x4e, VC{0x17, 0xc7} );
            mailWrite( 0x4e, VC{0x21, 0xc6} );
            mailWrite( 0x4e, VC{0x20, 0xcc} );
            mailWrite( 0x4e, VC{0x40, 0xcc} );
            mailWrite( 0x4e, VC{0x80, 0xcd} );
            mailWrite( 0x4e, VC{0x81, 0xcf} );
            mailWrite( 0x4e, VC{0x82, 0xc9} );
            mailWrite( 0x4e, VC{0x83, 0xcc} );
            mailWrite( 0x4e, VC{0x84, 0xd4} );
            mailWrite( 0x4e, VC{0x85, 0xce} );
            mailWrite( 0x4e, VC{0x86, 0x1c} );
            break;
        case Resolution::PAL:
            if( interlaced ) {
                mailWrite( 0x4e, VC{0x39, 0x54} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0xcc} );
                mailWrite( 0x4e, VC{0x12, 0xc8} );
                mailWrite( 0x4e, VC{0x13, 0xc2} );
                mailWrite( 0x4e, VC{0x17, 0xc6} );
                mailWrite( 0x4e, VC{0x21, 0xc6} );
                mailWrite( 0x4e, VC{0x20, 0xec} );
                mailWrite( 0x4e, VC{0x40, 0xc4} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0x4f} );
                mailWrite( 0x4e, VC{0x82, 0xce} );
                mailWrite( 0x4e, VC{0x83, 0x1c} );
                mailWrite( 0x4e, VC{0x84, 0xd9} );
                mailWrite( 0x4e, VC{0x85, 0xed} );
                mailWrite( 0x4e, VC{0x86, 0xec} );
            } else {
                mailWrite( 0x4e, VC{0x39, 0x54} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0x8c} );
                mailWrite( 0x4e, VC{0x12, 0xc8} );
                mailWrite( 0x4e, VC{0x13, 0xe4} );
                mailWrite( 0x4e, VC{0x17, 0xc7} );
                mailWrite( 0x4e, VC{0x21, 0xc6} );
                mailWrite( 0x4e, VC{0x20, 0xcc} );
                mailWrite( 0x4e, VC{0x40, 0xcc} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0x8f} );
                mailWrite( 0x4e, VC{0x82, 0xce} );
                mailWrite( 0x4e, VC{0x83, 0x1c} );
                mailWrite( 0x4e, VC{0x84, 0xe7} );
                mailWrite( 0x4e, VC{0x85, 0xce} );
                mailWrite( 0x4e, VC{0x86, 0x8c} );
            }
            break;
        case Resolution::NTSC:
            if( interlaced ) {
                mailWrite( 0x4e, VC{0x39, 0x54} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0xcc} );
                mailWrite( 0x4e, VC{0x12, 0xc8} );
                mailWrite( 0x4e, VC{0x13, 0xd8} );
                mailWrite( 0x4e, VC{0x17, 0xc7} );
                mailWrite( 0x4e, VC{0x21, 0xc6} );
                mailWrite( 0x4e, VC{0x20, 0xec} );
                mailWrite( 0x4e, VC{0x40, 0xc4} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0xf4} );
                mailWrite( 0x4e, VC{0x82, 0xce} );
                mailWrite( 0x4e, VC{0x83, 0x1c} );
                mailWrite( 0x4e, VC{0x84, 0xdd} );
                mailWrite( 0x4e, VC{0x85, 0xec} );
                mailWrite( 0x4e, VC{0x86, 0x3c} );
            } else {
                mailWrite( 0x4e, VC{0x39, 0x54} );
                mailWrite( 0x4e, VC{0x2c, 0x51} );
                mailWrite( 0x4e, VC{0x03, 0x8c} );
                mailWrite( 0x4e, VC{0x12, 0xc8} );
                mailWrite( 0x4e, VC{0x13, 0xec} );
                mailWrite( 0x4e, VC{0x17, 0xc7} );
                mailWrite( 0x4e, VC{0x21, 0xc6} );
                mailWrite( 0x4e, VC{0x20, 0xcc} );
                mailWrite( 0x4e, VC{0x40, 0xcc} );
                mailWrite( 0x4e, VC{0x80, 0xcc} );
                mailWrite( 0x4e, VC{0x81, 0xf7} );
                mailWrite( 0x4e, VC{0x82, 0xce} );
                mailWrite( 0x4e, VC{0x83, 0x1c} );
                mailWrite( 0x4e, VC{0x84, 0xef} );
                mailWrite( 0x4e, VC{0x85, 0xcd} );
                mailWrite( 0x4e, VC{0x86, 0x2c} );
            }
            break;
         default:
            throw runtime_error( "Current selected video mode is not a supported mode for Component.");
            break;
    }

    mailWrite( 0x4e, VC{0xb0, 0xe8} );
    mailWrite( 0x4e, VC{0xb1, 0x0c} );

    if(( currentInputSettings_.getResolution()==Resolution::PAL ) && interlaced ) {
        mailWrite( 0x4e, VC{0xad, 0xcc} );
        mailWrite( 0x4e, VC{0xb0, 0xf9} );
        mailWrite( 0x4e, VC{0xb1, 0xcc} );
    } else if(( currentInputSettings_.getResolution()==Resolution::NTSC ) && interlaced ) {
        mailWrite( 0x4e, VC{0xad, 0xcc} );
        mailWrite( 0x4e, VC{0xb0, 0xf9} );
        mailWrite( 0x4e, VC{0xb1, 0x4c} );
    } else { //Everything else.
        mailWrite( 0x4e, VC{0xad, 0xc9} );
        mailWrite( 0x4e, VC{0xb0, 0xe9} );
    }

    //Color space configuration is in separate thread and tends to happen at odd times.
    if( currentInputSettings_.getColorSpace()==ColorSpace::Unknown ) {
        currentInputSettings_.setColorSpace( ColorSpace::YUV ); //No autodetect currently. component signals usually YUV.
    }
    configureColorSpace();

    if(( currentInputSettings_.getResolution()==Resolution::NTSC ) && interlaced ) {
        mailWrite( 0x4e, VC{0xb0, 0xf9} );
        mailWrite( 0x4e, VC{0xb1, 0xcc} );
    }

    mailWrite( 0x4e, VC{0xab, 0xcc} );
    mailWrite( 0x4c, VC{0x04, 0x95} );

    configureSetupSubblock();
    configureCommonBlockB1( false );

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

    configureCommonBlockA();
    configureSetupSubblock();

    bool mysteryParameter=false;
    if (currentInputSettings_.getResolution() == Resolution::HD1080) {
        if( !interlaced ) { //1080p30 --ONLY TRUE FOR
            mysteryParameter=true;
        }
    }
    configureCommonBlockB1(mysteryParameter);
    configureCommonBlockB2();
    configureCommonBlockB3();

    mailWrite( 0x4e, VC{0x0f, 0xe4} );
    //There is an annoying thread that starts about here/changes behaviour
    //in the original driver..
    //
    //It can be identified by the instructions being one of the following:
    //   * readDevice0x9DCD(??)
    //   * mailWrite( 0x4e, VC{ 0x04, ??} )
    //   * mailWrite( 0x4e, VC{ 0xe0, 6c} )
    //   * mailWrite( 0x4e, VC{ 0xe0, 8c} )
    //   * mailWrite( 0x4e, VC{ 0xe0, cc} )
    //   * mailWrite( 0x4e, VC{ 0xe0, ec} )
    //   * mailWrite( 0x33, VC{0x99, 0x89, 0xf5} ); followed by a
    //   * mailRead( 0x33, 1 );
    //It is safe to remove these and still get a capture.
    configureCommonBlockC();

    transcoderFinalConfigure( currentInputSettings_ , currentTranscoderSettings_ );
    transcoderSetup( currentInputSettings_, currentTranscoderSettings_ );

    scmd(SCMD_INIT, 0xa0, 0x0000);
    uint16_t state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); //EXPECTED=0x0001
    scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_START);
    completeStateChange(state, SCMD_STATE_START); //EXPECTED 0x0001
}

