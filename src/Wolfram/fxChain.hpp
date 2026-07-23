#pragma once
#include <algorithm>

class Gain
{
public:
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
	inline float msToSlew(float slewMs) const {
		return ((audioRateMode ? 50000.0f : 1000.0f) / sr) / slewMs;
	}

	const float maxSlewMs = 1000.0f;
	const float minSlewMs = 1e-4f; //1e-3f

	int sr = 48000;
	bool audioRateMode = false;

	float prevValue = 0.0f;

	float slewAmount = 0.0f;
	float y = 0.0f;
};


class FxChain
{
public:
	enum class FX
	{
		Gain,
		Slew,
		Num_FX
	};

	void reset();
	void setSamplerate(int samplerate);
	void setAudioRateMode(bool audioRate);
	void setFxValue(float v, FX fx);
	void process(float& x);

protected:
	Slew slew;
	Gain gain;

	bool audioRateMode = false;

	const float valueThreshold = 1e-4f;
	float prevValue = 0.0f;
};