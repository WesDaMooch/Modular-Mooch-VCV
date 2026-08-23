// fxChain.cpp
// Part of the Modular Mooch Wolfram module (VCV Rack)
//
// GitHub: https://github.com/WesDaMooch/Modular-Mooch-VCV
// 
// Copyright (c) 2026 Wesley Lawrence Leggo-Morrell
// License: GPL-3.0-or-later


#include "fxChain.hpp"


// Gain
Gain::Gain() {
	reset();
}

void Gain::set(float v) {
	gainAmount = v;
}

void Gain::reset() {
	gainAmount = 0.5f;
}

void Gain::process(float& x) {
	x *= gainAmount;
}


// Slew
Slew::Slew() {
	reset();
}

void Slew::set(float v) {
	float maxSlewMs = audioRateMode ? maxAudioSlewMs : maxCvSlewMs;
	float minSlewMs = audioRateMode ? minAudioSlewMs : minCvSlewMs;
	float slewSkew = v * v;
	float slewMs = std::max(slewSkew * maxSlewMs, minSlewMs);
	slewAmount = msToSlew(slewMs);
	prevValue = v;
}

void Slew::setSamplerate(int samplerate) {
	sr = samplerate;
	set(prevValue);
}

void Slew::setAudioRateMode(bool audioRate) {
	audioRateMode = audioRate;
	set(prevValue);
}

void Slew::reset() {
	slewAmount = msToSlew(audioRateMode ? minAudioSlewMs : minCvSlewMs);
	prevValue = 0.0f;
	y = 0.0f;
}

void Slew::process(float& x) {
	y += std::min(std::max(x - y, -slewAmount), slewAmount);
	x = y;
}


// FX chain
FxChain::FxChain() {
	reset();
}

void FxChain::reset() {
	gain.reset();
	slew.reset();
}

void FxChain::setSamplerate(int samplerate) {
	int sr = std::max(samplerate, 1);
	slew.setSamplerate(sr);
}

void FxChain::setAudioRateMode(bool audioRate) {
	slew.setAudioRateMode(audioRate);
}

void FxChain::process(float& x) {
	// Clamp input
	x = std::min(std::max(x, 0.0f), 1.0f);
	
	slew.process(x);
	gain.process(x);
}