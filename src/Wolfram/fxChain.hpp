#pragma once
#include <cmath>
#include <algorithm>

enum class FX
{
	Gain,
	Slew,
	Fold,
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


// TODO: look at max msp book 
class Fold
{
public:
	void set(float v);
	void reset();
	void process(float& x);

protected:
	const float maxFold = 4.0f;
	float foldAmount = 1.0f;
};


class FxChain
{
public:

	Fold fold;
	Slew slew;
	Gain gain;

	FxChain();
	void reset();
	void setSamplerate(int samplerate);
	void setAudioRateMode(bool audioRate);
	void setFxValue(float v, FX fx);
	void process(float& x);

protected:
	const float valueThreshold = 1e-5f;
	float prevValue = 0.0f;
};