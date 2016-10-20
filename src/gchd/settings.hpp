/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdexcept>

#define MAXIMUM_AUDIO_BIT_RATE 576 //Maximum AAC audio bit rate for two channels
#define MAXIMUM_BIT_RATE 40.0
#define MINIMUM_BIT_RATE .032 //Woah that is slow.

enum class ColorSpace {
	Unknown,
	RGB,
	YUV
};

enum class HDMIColorSpace {
	Unknown,
	Limited,
	Full
};

enum class InputSource {
	Unknown,
	Composite,
	Component,
	HDMI
};

enum class Resolution {
	Unknown,
	NTSC,
	PAL,
	HD720,
	HD1080
};

enum class ScanMode {
	Unknown,
	Progressive,
	Interlaced
};

enum class BitRateMode {
	Constant,
	Variable,
};

enum class H264Profile {
	Baseline,
	Main,
	High
};

void convertResolution( unsigned &horizontal, unsigned &vertical, const Resolution resolution );

//Unsigned longs so I can use strtoul, just convenience
Resolution convertResolution( unsigned long horizontal, unsigned long vertical );

using std::runtime_error;
class setting_error: public std::runtime_error
{
		using runtime_error::runtime_error; //This inherits all constructors.
};

//This is used both to hold forced modes from the command line
//AND to carry the currently being used mode.
class InputSettings {
	public:
		InputSettings();

		InputSource getSource();
		void setSource(InputSource inputSource);

		Resolution getResolution();
		void getResolution(unsigned &horizontal, unsigned &vertical);
		void setResolution(Resolution resolution);

		ScanMode getScanMode();
		void setScanMode(ScanMode mode);

		double getRefreshRate();  //0.0=autodetect
		void setRefreshRate(double);    //0.0=autodetect

		ColorSpace getColorSpace();
		void setColorSpace(ColorSpace colorSpace);

		//Does not do anything currently
		HDMIColorSpace getHDMIColorSpace();
		void setHDMIColorSpace(HDMIColorSpace hdmiColorSpace);

		bool getSDStretch();
		void setSDStretch(bool shouldStretchSD);

		//Sets current inputsettings based on prototype
		//and autodetected settings.
		void mergeAutodetect( InputSettings &prototype,
				      Resolution resolution,
				      ScanMode scanMode,
				      double refreshRate);

		void checkInputSettingsValidity(bool configured);

	private:
		void checkRefresh( double value, const char *string );

		InputSource source_;
		Resolution resolution_;
		ScanMode scanMode_;
		double refreshRate_;
		ColorSpace colorSpace_;
		HDMIColorSpace hdmiColorSpace_;
		bool stretchedSD_;
};

class TranscoderSettings {
	public:
		TranscoderSettings();

		void getResolution( unsigned &x, unsigned &y);
		void setResolution( unsigned x, unsigned y);
		void setResolution( const Resolution &resolution );

		void setBitRateMode(BitRateMode bitRateMode);
		BitRateMode getBitRateMode();

		//Sets values use by variable bit rate
		//All 0s means default high quality settings will be used.
		//values are in mbps
		void setVariableBitRateMbps( float max, float average, float minimum );
		void getVariableBitRateMbps( float &max, float &average, float &minimum );
		void getVariableBitRateKbps( unsigned &max, unsigned &average, unsigned &minimum );

		//Sets value used by constant bit rate.
		//0 means default high quality settings will be used.
		//Values are in mbps.

		void setConstantBitRateMbps( float bitRate );
		float getConstantBitRateMbps();
		unsigned getConstantBitRateKbps();

		//Maximum bitrate based on settings, variable or constant.
		unsigned getRealMaxBitRateKbps();

		void setFrameRate(double frameRate); //0.0 means auto.
		double getFrameRate();               //0.0 means auto.
		double getEffectiveFrameRate(); //Refresh rate if frame rate is 0.0

		//Value is in kbps
		void setAudioBitRate(unsigned bitRate);
		unsigned getAudioBitRate();

		void setH264Profile( H264Profile value );
		H264Profile getH264Profile();

		//Setting it to 0.0 will make it auto-set
		//based on bit rates and whatnot.
		void setH264Level( float value );
		float getH264Level();

		static unsigned unsignedH264Level(float value);

		void mergeAutodetect( TranscoderSettings &prototype, InputSettings &currentInput );
	private:
		unsigned resolution_[2];
		BitRateMode bitRateMode_;

		unsigned constantBitRate_;
		unsigned maxVariableBitRate_;
		unsigned averageVariableBitRate_;
		unsigned minVariableBitRate_;

		unsigned audioBitRate_;
		double frameRate_;
		double effectiveFrameRate_; //If frameRate_ ends up 0, this will be refresh rate.
		H264Profile h264Profile_;
		float h264Level_;

};


//Unsupported options: TODO
//  HDMIColorSpace
//  useAnalogAudio
//  allowHighFPS,
//	analogAudioGain
//	digitalAudioGain
//  brightness, contrast, saturation, hue
//  overscan, convertSD


#endif
