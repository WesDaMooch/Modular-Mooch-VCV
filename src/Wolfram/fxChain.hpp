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
		return (1000.0f / sr) / slewMs;
	}

	const float maxCvSlewMs = 1000.0f;
	const float minCvSlewMs = 1e-3f;
	const float maxAudioSlewMs = 5.0f;
	const float minAudioSlewMs = 1e-6f;

	bool audioRateMode = false;
	int sr = 48000;

	float prevValue = 0.0f;
	float slewAmount = 0.0f;
	float y = 0.0f;
};


class Fold
{
	//https://ccrma.stanford.edu/~jatin/ComplexNonlinearities/Wavefolder.html
public:
	void set(float v);
	void reset();
	void process(float& x);

protected:
	float foldAmount = 0.5f;
};


class FxChain
{
public:
	enum class FX
	{
		Gain,
		Slew,
		Fold,
		Num_FX
	};

	void reset();
	void setSamplerate(int samplerate);
	void setAudioRateMode(bool audioRate);
	void setFxValue(float v, FX fx);
	void process(float& x);

protected:
	Fold fold;
	Slew slew;
	Gain gain;

	const float valueThreshold = 1e-5f;
	float prevValue = 0.0f;
};