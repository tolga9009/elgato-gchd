/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include "settings.hpp"

ColorSpace Settings::getColorSpace() {
	return colorSpace_;
}

void Settings::setColorSpace(ColorSpace colorSpace) {
	colorSpace_ = colorSpace;
}

HDMIColorSpace Settings::getHDMIColorSpace() {
	return hdmiColorSpace_;
}

void Settings::setHDMIColorSpace(HDMIColorSpace hdmiColorSpace) {
	hdmiColorSpace_ = hdmiColorSpace;
}

InputSource Settings::getInputSource() {
	return inputSource_;
}

void Settings::setInputSource(InputSource inputSource) {
	inputSource_ = inputSource;
}

Resolution Settings::getOutputResolution() {
	return outputResolution_;
}

void Settings::setOutputResolution(Resolution resolution) {
	outputResolution_ = resolution;
}

Resolution Settings::getInputResolution() {
	return inputResolution_;
}

void Settings::setInputResolution(Resolution resolution) {
	inputResolution_ = resolution;
}

bool Settings::getAnalogAudio() {
	return useAnalogAudio_;
}

void Settings::setAnalogAudio(bool useAnalogAudio) {
	useAnalogAudio_ = useAnalogAudio;
}

bool Settings::getHighFPS() {
	return allowHighFPS_;
}

void Settings::setHighFPS(bool allowHighFPS) {
	allowHighFPS_ = allowHighFPS;
}

//0.0 means variable frame rate.
void Settings::setFixedFrameRate(double frameRate) {
	frameRate_ = frameRate;
}

double Settings::getFixedFrameRate() {
	return frameRate_;
}

bool Settings::getOverscan() {
	return useOverscan_;
}

void Settings::setOverscan(bool useOverscan) {
	useOverscan_ = useOverscan;
}

bool Settings::getSDConvert() {
	return shouldConvertSD_;
}

void Settings::setSDConvert(bool shouldConvertSD) {
	shouldConvertSD_ = shouldConvertSD;
}

bool Settings::getSDStretch() {
	return shouldStretchSD_;
}

void Settings::setSDStretch(bool shouldStretchSD) {
	shouldStretchSD_ = shouldStretchSD;
}

int Settings::getBrightness() {
	return brightness_;
}

void Settings::setBrightness(int brightness) {
	brightness_ = brightness;
}

int Settings::getContrast() {
	return contrast_;
}

void Settings::setContrast(int contrast) {
	contrast_ = contrast;
}

int Settings::getSaturation() {
	return saturation_;
}

void Settings::setSaturation(int saturation) {
	saturation_ = saturation;
}

int Settings::getHue() {
	return hue_;
}

void Settings::setHue(int hue) {
	hue_ = hue;
}

int Settings::getAnalogAudioGain() {
	return analogAudioGain_;
}

void Settings::setAnalogAudioGain(int analogAudioGain) {
	analogAudioGain_ = analogAudioGain;
}

int Settings::getDigitalAudioGain() {
	return digitalAudioGain_;
}

void Settings::setDigitalAudioGain(int digitalAudioGain) {
	digitalAudioGain_ = digitalAudioGain;
}

Settings::Settings() {
	// default values
	useAnalogAudio_ = false;
	allowHighFPS_ = true;
	useOverscan_ = false;
	shouldConvertSD_ = false;
	shouldStretchSD_ = false;
	brightness_ = 0;
	contrast_ = 0;
	saturation_ = 0;
	hue_ = 0;
	analogAudioGain_ = 0;
	digitalAudioGain_ = 0;
	colorSpace_ = ColorSpace::YUV;
	hdmiColorSpace_ = HDMIColorSpace::Limited;
	inputSource_ = InputSource::Unknown;
	inputResolution_ = Resolution::Unknown;
	outputResolution_ = Resolution::HD720; //Will change this to Unknown in future, and
	//(maybe) add code to match inputResolution.
	frameRate_ = 0.0;
}
