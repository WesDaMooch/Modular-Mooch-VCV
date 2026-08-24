// fxChain.hpp
// Part of the Modular Mooch Wolfram module (VCV Rack)
//
// GitHub: https://github.com/WesDaMooch/Modular-Mooch-VCV
// 
// Copyright (c) 2026 Wesley Lawrence Leggo-Morrell
// License: GPL-3.0-or-later


#pragma once
#include <cmath>
#include <algorithm>


enum class FX
{
	Gain,
	Slew,
	Num_FX
};


class Gain
{
public:
	Gain();
	void set(float v);
	void reset();
	void process(float& x);

protected:
	float gainAmount = 0.5f;
};


class Slew
{
public:
	Slew();
	void set(float v);
	void setSamplerate(int samplerate);
	void setAudioRateMode(bool audioRate);
	void reset();
	void process(float& x);

protected:	
	const float maxCvSlewMs = 5000.0f;
	const float minCvSlewMs = 1e-3f;
	const float maxAudioSlewMs = 5.0f;
	const float minAudioSlewMs = 1e-6f;

	bool audioRateMode = false;
	int sr = 48000;

	float prevValue = 0.0f;
	float slewAmount = 0.0f;
	float y = 0.0f;

	inline float msToSlew(float slewMs) const {
		return (1000.0f / sr) / slewMs;
	}
};


class FxChain
{
public:
	Gain gain;
	Slew slew;

	FxChain();
	void reset();
	void setSamplerate(int samplerate);
	void setAudioRateMode(bool audioRate);
	void process(float& x);
};