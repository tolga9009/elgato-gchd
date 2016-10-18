/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include "settings.hpp"
#include <cmath>
#include <iostream>

InputSettings::InputSettings() {
    source_ = InputSource::Unknown;
    resolution_ = Resolution::Unknown;
    scanMode_ = ScanMode::Unknown;
    refreshRate_ = 0.0; //refresh rate, 0 = unknown
    colorSpace_ = ColorSpace::Unknown;
    //currently this option does nothing right now.
    hdmiColorSpace_ = HDMIColorSpace::Limited;
    stretchedSD_ = false; //if set, 4:3 SD video input is
                          //stretched to 16:9
}

InputSource InputSettings::getSource() {
	return source_;
}

void InputSettings::setSource(InputSource inputSource) {
	source_ = inputSource;
}

Resolution InputSettings::getResolution() {
	return resolution_;
}

void InputSettings::getResolution(unsigned &horizontal, unsigned &vertical) {
    Resolution resolution = getResolution();

    switch (resolution) {
        case Resolution::Unknown:
            horizontal=0;
            vertical=0;
            break;

		case Resolution::NTSC:
            horizontal=720;
            vertical=480;
            break;

	    case Resolution::PAL:
            horizontal=720;
            vertical=480;
            break;

        case Resolution::HD720:
            horizontal=1280;
            vertical=720;
            break;

        case Resolution::HD1080:
            horizontal=1920;
            vertical=1080;
            break;

		default:
            throw std::runtime_error("Unsupported configuration.");
            break;
	}
}

void InputSettings::setResolution(Resolution resolution) {
	resolution_ = resolution;
}

ScanMode InputSettings::getScanMode() {
    return scanMode_;
}

void InputSettings::setScanMode(ScanMode scanMode) {
    scanMode_ = scanMode;
}

double InputSettings::getRefreshRate() { //0.0 autodetect
    return refreshRate_;
}

void InputSettings::setRefreshRate(double rate) {
    refreshRate_=rate;
}

ColorSpace InputSettings::getColorSpace() {
	return colorSpace_;
}

void InputSettings::setColorSpace(ColorSpace colorSpace) {
	colorSpace_ = colorSpace;
}

HDMIColorSpace InputSettings::getHDMIColorSpace() {
	return hdmiColorSpace_;
}

void InputSettings::setHDMIColorSpace(HDMIColorSpace hdmiColorSpace) {
	hdmiColorSpace_ = hdmiColorSpace;
}

bool InputSettings::getSDStretch() {
	return stretchedSD_;
}

void InputSettings::setSDStretch(bool stretchedSD) {
	stretchedSD_ = stretchedSD;
}

void InputSettings::checkRefresh( double value, const char *string ) {
    if ((refreshRate_ != 0.0) && (refreshRate_ != value )) {
        throw std::logic_error( string );
    }
}

void InputSettings::mergeAutodetect( InputSettings &prototype,
                                     Resolution resolution,
                                     ScanMode scanMode,
                                     double refreshRate)
{
    //Merge passed arguments and autodetect information.
    if( prototype.getResolution()==Resolution::Unknown) {
        setResolution( resolution );
    } else {
        setResolution( prototype.getResolution() );
    }
    if( prototype.getScanMode()==ScanMode::Unknown) {
        setScanMode( scanMode );
    } else {
        setScanMode( prototype.getScanMode() );
    }
    if( prototype.getRefreshRate() == 0.0) {
        setRefreshRate( refreshRate );
    } else {
        setRefreshRate( prototype.getRefreshRate() );
    }
    checkInputSettingsValidity(true);
}

void InputSettings::checkInputSettingsValidity(bool configured)
{

    if( configured ) {
        if( source_ == InputSource::Unknown ) {
            throw std::logic_error( "Input source mode not getting configured." );
        }
        if( resolution_ == Resolution::Unknown ) {
            throw std::logic_error( "Input resolution not getting configured." );
        }
        if( scanMode_ == ScanMode::Unknown ) {
            throw std::logic_error( "Input scan mode not getting configured." );
        }
        if( refreshRate_ == 0.0 ) {
            throw std::logic_error( "Refresh rate not getting configured." );
        }
    }

    //Use setting_error for errors that can be caused by command line
    //parameters, logic_error for states that should be impossible.

    switch( source_ ) {
        case InputSource::Composite:
            switch( scanMode_ ) {
                case ScanMode::Progressive:
                    throw setting_error( "Composite input does not support non-interlaced." );
                    break;
                case ScanMode::Interlaced:
                case ScanMode::Unknown:
                    break;
                default:
                    throw std::logic_error( "Composite input scan mode is unknown value." );
                    break;
            }
            switch( resolution_ ) {
                case Resolution::NTSC:
                    checkRefresh( 60.0, "NTSC signal only supports 60hz refresh rate." );
                    break;
                case Resolution::PAL:
                    checkRefresh( 50.0, "NTSC signal only supports 60hz refresh rate." );
                    break;
                case Resolution::Unknown:
                    break;
                default:
                    throw setting_error( "Composite signal only support NTSC or PAL resolution." );
                    break;
            }
            if ( (refreshRate_ != 50.0) && (refreshRate_ != 60.0) ) {
                throw std::logic_error( "Composite signal only support 50 or 60hz refresh rate." );
            }
            break;
        case InputSource::Component:
            switch( resolution_ ) {
                case Resolution::HD1080:
                    switch( scanMode_ ) {
                        case ScanMode::Progressive:
                            checkRefresh( 30.0, "Component 1080p only supports 30hz refresh rate." );
                            break;
                        case ScanMode::Interlaced:
                            checkRefresh( 60.0, "Component 1080i only supports 60hz refresh rate." );
                            break;
                        case ScanMode::Unknown:
                            break;
                    }
                    break;
                case Resolution::HD720:
                    switch( scanMode_ ) {
                        case ScanMode::Progressive:
                            checkRefresh( 60.0, "Component 720p only supports 60hz refresh rate." );
                            break;
                        case ScanMode::Interlaced:
                            throw setting_error( "1280x720 does not support interlaced mode." );
                            break;
                        case ScanMode::Unknown:
                            break;
                    }
                    break;
                case Resolution::NTSC:
                    checkRefresh( 60.0, "NTSC signal only supports 60hz refresh rate." );
                    break;
                case Resolution::PAL:
                    checkRefresh( 50.0, "PAL signal only supports 50hz refresh rate." );
                    break;
                case Resolution::Unknown:
                    break;
            }
            break;
        case InputSource::HDMI:
            switch( resolution_ ) {
                case Resolution::HD1080:
                    switch( scanMode_ ) {
                        case ScanMode::Progressive:
                            checkRefresh( 60.0, "HDMI 1080p only supports 60hz refresh rate." );
                            break;
                        case ScanMode::Interlaced:
                            checkRefresh( 60.0, "HDMI 1080i only supports 60hz refresh rate." );
                            break;
                        case ScanMode::Unknown:
                            break;
                    }
                    break;
                case Resolution::HD720:
                    switch( scanMode_ ) {
                        case ScanMode::Progressive:
                            checkRefresh( 60.0, "HDMI 720p only supports 60hz refresh rate." );
                            break;
                        case ScanMode::Interlaced:
                            throw setting_error( "1280x720 does not support interlaced mode." );
                            break;
                        case ScanMode::Unknown:
                            break;
                    }
                    break;
                //deliberately allowing interlaced to be set, because I've seen
                //480i and 576i over HDMI, and it would probably work if I had mode
                //numbers.
                case Resolution::NTSC:
                    checkRefresh( 60.0, "NTSC signal only supports 60hz refresh rate." );
                    break;
                case Resolution::PAL:
                    checkRefresh( 50.0, "PAL signal only supports 50hz refresh rate." );
                    break;
                case Resolution::Unknown:
                    break;
            }
            break;

        case InputSource::Unknown:
            break;
    }
}


TranscoderSettings::TranscoderSettings() {
    //These are the options that meet the following criteria:
    //  1. We understand how they are set and configured
    //  2. We can come up with a simple reason for an end user
    //     to want to change them.
    //
    // There are many more options that we could export
    // but they don't meet criteria 1 or 2 currently.

    //These are also roughly in the order
    //of how important it might be for a user to tweak these.

    resolution_[0] =  0;  //setting both to 0 matches source.
    resolution_[1] =  0;

    bitRateMode_= BitRateMode::Constant;

    //0 means these will be set based on
    //default high quality settings for video mode.
    constantBitRate_=0;
    maxVariableBitRate_=0;
    averageVariableBitRate_=0;
    minVariableBitRate_=0;

    audioBitRate_=320;
    frameRate_=0.0; //Auto
    effectiveFrameRate_=0.0;
    h264Level_=0.0; //Auto
}

void TranscoderSettings::getResolution( unsigned &x, unsigned &y) {
    x=resolution_[0];
    y=resolution_[1];
}

void TranscoderSettings::setResolution( unsigned x, unsigned y) {
    if((x>1920) || (y>1080)) {
       throw setting_error( "Output resolution cannot be made greater than 1920x1080" );
    }
    resolution_[0]=x;
    resolution_[1]=y;
}

void TranscoderSettings::setBitRateMode( BitRateMode mode ) {
    bitRateMode_=mode;
}

BitRateMode TranscoderSettings::getBitRateMode() {
    return bitRateMode_;
}

void TranscoderSettings::setVariableBitRate( unsigned maximum, unsigned average, unsigned minimum ) {
    if (( maximum < average ) || (average < minimum)) {
       throw setting_error( "Bit rate setting max:average:min must satisfy max>=average>=min." );
    }
    if ( maximum > MAXIMUM_BIT_RATE ) {
       throw setting_error( "Maximum bit rate supported is " + std::to_string(MAXIMUM_BIT_RATE) + " kbps." );
    }
    maxVariableBitRate_=maximum;
    averageVariableBitRate_=average;
    minVariableBitRate_=minimum;
}

void TranscoderSettings::getVariableBitRate( unsigned &maximum, unsigned &average, unsigned &minimum ) {
    maximum=maxVariableBitRate_;
    average=averageVariableBitRate_;
    minimum=minVariableBitRate_;
}

void TranscoderSettings::setConstantBitRate( unsigned bitRate ) {
    if (bitRate > MAXIMUM_BIT_RATE ) {
       throw setting_error( "Maximum bit rate supported is " + std::to_string(MAXIMUM_BIT_RATE) + " kbps." );
    }
    constantBitRate_=bitRate;
}

unsigned TranscoderSettings::getConstantBitRate() {
    return constantBitRate_;
}

unsigned TranscoderSettings::getRealMaxBitRate() {
    switch( bitRateMode_ ) {
        case BitRateMode::Constant:
            return constantBitRate_;
            break;
        case BitRateMode::Variable:
            return maxVariableBitRate_;
            break;
        default:
            throw std::logic_error( "Bit rate mode corrupt." );
            break;
    }
}


void TranscoderSettings::setFrameRate(double frameRate) {
    if (frameRate > 60.0) {
       throw( setting_error( "Maximum frame rate supported is 60.0 frames per second." ) );
    }
    frameRate_=frameRate;
}

double TranscoderSettings::getFrameRate() {
    return frameRate_;
}

double TranscoderSettings::getEffectiveFrameRate() {
    return effectiveFrameRate_;
}

unsigned TranscoderSettings::getAudioBitRate() {
    return audioBitRate_;
}

void TranscoderSettings::setAudioBitRate( unsigned bitRate ) {
    if (bitRate > MAXIMUM_AUDIO_BIT_RATE ) {
       throw( setting_error( "Maximum constant bit rate supported is " + std::to_string(MAXIMUM_AUDIO_BIT_RATE) + " kbps." ) );
    }
    audioBitRate_=bitRate;
}

unsigned TranscoderSettings::unsignedH264Level(float value) {
    float integerF;
    float fractionF;
    integerF=std::modf(value, &fractionF);
    unsigned integer=(unsigned)integerF;

    unsigned fraction=(int)round( fractionF / .1 );
    return integer*10+fraction;
}

void TranscoderSettings::setH264Level(float level){
    unsigned value=unsignedH264Level( level );

    bool valid;
    switch( value ) {
        case 0: //default case
        case 10:
        case 11:
        case 12:
        case 13:
        case 20:
        case 21:
        case 22:
        case 30:
        case 31:
        case 32:
        case 40:
        case 41:
            valid=true;
            break;
        default:
            valid=false;
            break;
    }
    if( !valid ) {
        if ( value > 41 ) {
            throw( setting_error( "h.264 levels greater than 4.1 are not supported." ) );
        } else {
            throw( setting_error( "Illegal h.264 level selected." ) );
        }
    }
    h264Level_=level;
}

float TranscoderSettings::getH264Level(){
    return h264Level_;
}

typedef struct {
    float level;
    uint32_t maxMacroBlocksPerSecond;
    uint32_t maxFrameMacroBlocks;
    uint32_t maxKBitrateProfile; //ordered based on what the profiles mean v_h264_profile (theoretically)
} h264_level_description_t;

// See: ITU-T Rec H.264 Appendix A.3 (Levels)
// Hardware is specified as supporting up to 4.0 (0x40), but may
// go a little higer.
static const h264_level_description_t h264_levels[]= {
    {1.0,    1485,    99,     64},
    {1.1,    3000,   396,    192},
    {1.2,    6000,   396,    384},
    {1.3,   11880,   396,    768},
    {2.0,   11880,   396,   2000},
    {2.1,   19800,   792,   4000},
    {2.2,   20250,  1620,   4000},
    {3.0,   40500,  1620,  10000},
    {3.1,  108000,  3600,  14000},
    {3.2,  216000,  5120,  20000},
    {4.0,  245760,  8192,  20000},
    {4.1,  245760,  8192,  50000},
    {4.2,  522240,  8704,  50000},
    {5.0,  589824, 22080, 135000},
    {5.1,  983040, 36864, 240000},
    {5.2, 2073600, 36864, 240000},
};

static float determineH264Level( uint32_t xRes, uint32_t yRes, float frameRate, uint32_t kbitRate )
{
    unsigned frameMacroBlocks=(xRes * yRes ) / (16*16);
    unsigned macroBlocksPerSecond=frameMacroBlocks * frameRate;

    unsigned tableLength= sizeof( h264_levels ) / sizeof(h264_level_description_t);
    for( unsigned i=0; i<tableLength ;++i)  {
        if ((macroBlocksPerSecond <= h264_levels[i].maxMacroBlocksPerSecond) &&
            (frameMacroBlocks <= h264_levels[i].maxFrameMacroBlocks ) &&
            (kbitRate <= h264_levels[i].maxKBitrateProfile)) //TODO...evaluate to see if using more than 8 bit color channels
                                                             //is done and how it affects this.
        {
            float level = h264_levels[i].level;
            return level;
        }
    }
    throw std::logic_error( "Illegal settings, no valid h.264 level would work.");
}

void TranscoderSettings::mergeAutodetect( TranscoderSettings &prototype, InputSettings &currentInput ) {
    *this=prototype;

    unsigned horizontal, vertical;
    prototype.getResolution(horizontal, vertical);
    if((horizontal==0) && (vertical==0 )) { //setting for set to input
        currentInput.getResolution(horizontal, vertical);
    }
    this->setResolution(horizontal, vertical);

    double frameRate=prototype.getFrameRate();
    double refreshRate = currentInput.getRefreshRate();
    if( frameRate > refreshRate ) {
        std::cerr << "Frame rate too fast, forced to refresh rate of " << refreshRate << std::endl;
        frameRate=refreshRate;
    }
    setFrameRate( frameRate );  //0.0 is acceptable value, transcoder doesn't
                                //need fixed frame rate specified.
    effectiveFrameRate_=frameRate;
    if( effectiveFrameRate_==0.0) {
        effectiveFrameRate_=currentInput.getRefreshRate();
    }

    //Figure out bit rates. This sets reasonable defaults.
    uint32_t reasonableBitrate=int(31.25 * horizontal); //Seems to be what a lot of settings use.
    double highestBitrate=MAXIMUM_BIT_RATE;

    unsigned autoBitrate        = std::min(reasonableBitrate*1.0, highestBitrate );
	unsigned autoMaxBitrate     = std::min(reasonableBitrate*.9, highestBitrate);
	unsigned autoAverageBitrate = std::min(reasonableBitrate*.5, highestBitrate);
	unsigned autoMinBitrate     = std::min(reasonableBitrate*.35, highestBitrate);

    if( prototype.constantBitRate_ == 0 ) {
        this->constantBitRate_ = autoBitrate;
    }
    if( prototype.maxVariableBitRate_ == 0 ) {
        this->maxVariableBitRate_ = autoMaxBitrate;
        this->averageVariableBitRate_ = autoAverageBitrate;
        this->minVariableBitRate_ = autoMinBitrate;
    }
    if( prototype.h264Level_ == 0.0 ) {
        h264Level_=determineH264Level( horizontal, vertical, frameRate, getRealMaxBitRate() );
    }
}

