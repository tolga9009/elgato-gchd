/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

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

class Settings {
	public:
		ColorSpace getColorSpace();
		void setColorSpace(ColorSpace colorSpace);
		HDMIColorSpace getHDMIColorSpace();
		void setHDMIColorSpace(HDMIColorSpace hdmiColorSpace);
		InputSource getInputSource();
		void setInputSource(InputSource inputSource);
		Resolution getResolution();
		void setResolution(Resolution resolution);
		bool getAnalogAudio();
		void setAnalogAudio(bool useAnalogAudio);
		bool getHighFPS();
		void setHighFPS(bool allowHighFPS);
		bool getOverscan();
		void setOverscan(bool useOverscan);
		bool getSDConvert();
		void setSDConvert(bool shouldConvertSD);
		bool getSDStretch();
		void setSDStretch(bool shouldStretchSD);
		int getBrightness();
		void setBrightness(int brightness);
		int getContrast();
		void setContrast(int contrast);
		int getSaturation();
		void setSaturation(int saturation);
		int getHue();
		void setHue(int hue);
		int getAnalogAudioGain();
		void setAnalogAudioGain(int analogAudioGain);
		int getDigitalAudioGain();
		void setDigitalAudioGain(int digitalAudioGain);
		Settings();

	private:
		bool useAnalogAudio_;
		bool allowHighFPS_;
		bool useOverscan_;
		bool shouldConvertSD_;
		bool shouldStretchSD_;
		int brightness_;
		int contrast_;
		int saturation_;
		int hue_;
		int analogAudioGain_;
		int digitalAudioGain_;
		ColorSpace colorSpace_;
		HDMIColorSpace hdmiColorSpace_;
		InputSource inputSource_;
		Resolution resolution_;
};

#endif
