
#pragma once
#include <array>
#include <cmath>
#include <random>
//#include "..\dsp\biquad.hpp" //TODO: use elements filter
#include "stmlib/dsp/filter.h"
#include "..\dsp\adr.hpp"
#include "common.hpp"
#include <rack.hpp> // TODO: dont want to use rack api here?

struct Grain
{
	bool active = false;
	float phase = 0.f;

	float speed = 2.f;
	float texture = 0.f;
	float gain = 0.f;
};
// TODO: remove exciter params
struct ExciterParams
{
	float texture = 0.f;
	float density = 0.f;
	float velocity = 0.f;
};

class Exciter
{
public:
	Exciter() {
		//velocityFilter.setLowpass(1000.f);
		velocityFilter2.Init();

		// Exciter tables //

	   // Impulse table 
	   // TODO: Dont like the impulse, maybe replace with decaying impulse train?
	   //for (int i = 0; i < TABLE_LEN; i++)
	   //    ampTable[0][i] = (i == 0) ? 1.f : 0.f;

	   // TODO: sine wave & triangle wave

		size_t tableIdx = 0;
		float peak = 0.f;


		// Turkey window
		/*
		const float alpha = 0.1f; // 0 = rectangular, 1 = Hann

		for (int i = 0; i < TABLE_LEN; i++) {
			float value = 0.f;
			float x = static_cast<float>(i) / (TABLE_LEN - 1);

			if (x < alpha * 0.5f) {
				// Rising cosine taper
				value = 0.5f * (1.f + std::cosf(M_PI * (2.f * x / alpha - 1.f)));
			}
			else if (x <= 1.f - alpha * 0.5f) {
				// Flat section
				value = 1.0f;
			}
			else {
				// Falling cosine taper
				value = 0.5f * (1.f + std::cosf(M_PI * (2.f * x / alpha - 2.f / alpha + 1.f)));
			}

			table[tableIdx][i] = value;
			peak = std::max(peak, std::fabsf(value));
		}
		normaliseAmplitudeEnvelope(tableIdx, peak);
		*/

		// Raise cosine pulse
		tableIdx = 0;
		peak = 0.f;

		const float T = TABLE_LEN / 8.f;
		const float beta = 0.5f;
		const float leftZeroCrossing = 2.f;    // Start point
		const float rightZeroCrossing = 5.f;   // End point

		for (int i = 0; i < TABLE_LEN; i++) {
			float value = 0.f;

			// Asymmetric time range
			float t = -leftZeroCrossing * T + (leftZeroCrossing + rightZeroCrossing) * T * static_cast<float>(i) / (TABLE_LEN - 1);
			float x = t / T;

			float denominator = 1.f - std::powf(2.f * beta * x, 2.f);

			if (std::fabsf(denominator) > 1e-12f) {
				float sinc = 1.f;

				if (std::fabsf(x) > 1e-12f)
					sinc = std::sinf(M_PI * x) / (M_PI * x);

				value = sinc * std::cosf(M_PI * beta * x) / denominator;
			}
			else {
				value = (M_PI * 0.25f) * (std::sinf(M_PI / (2.f * beta)) / (M_PI / (2.f * beta)));
			}

			table[tableIdx][i] = value;
			peak = std::max(peak, std::fabsf(value));
		}
		normaliseAmplitudeEnvelope(tableIdx, peak);


		// Triangle window
		/*
		tableIdx = 1;
		peak = 0.f;

		for (int i = 0; i < TABLE_LEN; i++) {
			float phase = static_cast<float>(i) / (TABLE_LEN - 1);
			float value = 1.f - std::fabsf(2.f * phase - 1.f);

			table[tableIdx][i] = value;
			peak = std::max(peak, std::fabsf(value));
		}
		normaliseAmplitudeEnvelope(tableIdx, peak);
		*/


		// Hann window
		tableIdx = 1;
		peak = 0.f;

		for (int i = 0; i < TABLE_LEN; i++)
		{
			float value = 0.5f * (1.f - std::cosf(2.f * M_PI * i / (TABLE_LEN - 1)));
			table[tableIdx][i] = value;
			peak = std::max(peak, std::fabsf(value));
		}
		normaliseAmplitudeEnvelope(tableIdx, peak);
	}

	void setSampleRate(int sampleRate) {
		this->sampleRate = std::max(sampleRate, 1);

		//velocityFilter.setSampleRate(this->sampleRate);
		amplitudeADR.setSampleRate(this->sampleRate);
	}

	void trigger() {
		// Density Dial states
	    // Full pluck (0 - 0.33)
	    // Damped pluck (0.33 - 0.5);

	    // Pluck
		float pluckGain = 2.f;
		if (density_ > 0.33f)
			pluckGain = 1.f - mRemap(density_, 0.33f, 0.5f);

		// Pluck first grain
		grain[0].active = true;
		grain[0].phase = 0.f;
		grain[0].speed = 1.f;
		grain[0].texture = texture_;
		grain[0].gain = pluckGain;


		// Granular bow / blow
		grainGain = mRemap(density_, 0.f, 0.33f);   // Grains fade in

		// Short attack, short decay 0 - 33
		// short attack, long medium decay 33 - 50
		// medium attack, short decay, low level, long release, 50 - 66
		// long attack, long decay, high level, short release? 66 - 75
		// 75 - 100 ? 

		float attack = 0.1f;
		float decay = 0.7f;
		float decayLevel = 0.5f;
		float release = 0.5f;

		if (density_ <= 0.33f) {
			//float param = mRemap(densityParam, 0.f, 0.33f);

			decay = 0.5f;
		}
		else if (density_ > 0.33f && density_ <= 0.5f) {
			decay = 2.75f;
		}

		amplitudeADR.setParams(attack, decay, decayLevel, release);
		amplitudeADR.trigger();
	}

	float process(float deltaTime, float velocity, float texture, float density) {
		
		texture_ = mClamp(texture, 0.f, 1.f);
		density_ = mClamp(density, 0.f, 1.f);

		// Spawn grains
		//spawnPhase += deltaTime; 
		spawnPhase += spawnRate / sampleRate; // todo use delta time

		if (spawnPhase >= 1.f) {
			spawnPhase -= 1.f;

			float probability = 0.25f;

			if (rack::random::uniform() < probability)
				spawnGrain();
		}

		// Process active grains
		float output = 0.f;
		float amplitude = amplitudeADR.process();

		for (int channel = 0; channel < MAX_CHANNELS; channel++) {
			if (!grain[channel].active)
				continue;

			float tablePosition = grain[channel].texture * (NUM_TABLES - 1);

			int typeA = static_cast<int>(std::floorf(tablePosition));
			int typeB = static_cast<int>(std::ceilf(tablePosition));

			float typeFrac = tablePosition - typeA;

			int index0 = static_cast<int>(grain[channel].phase);
			int index1 = index0 + 1;

			float frac = grain[channel].phase - index0;

			if (index1 < TABLE_LEN) {
				float sampleA = mInterp(frac, table[typeA][index0], table[typeA][index1]);
				float sampleB = mInterp(frac, table[typeB][index0], table[typeB][index1]);
				float wave = mInterp(typeFrac, sampleA, sampleB);

				// Bypass amplitude envelope for pluck
				float amplitudeEnvelope = (channel == 0) ? 1.f : amplitude;

				output += wave * grain[channel].gain * amplitudeEnvelope;

				grain[channel].phase += grain[channel].speed;
			}
			else {
				grain[channel].active = false;
			}
		}

		// TODO: Mix in highpassed 'Air' noise
		// TODO: have an air param?

		// Velocity lowpass filter
		if (velocity_ != velocity) {
			velocity_ = mClamp(velocity, 0.f, 1.f);
			velocity_ = velocity;

			// TODO: is this the best cutoff mapping...
			float cutoffHz = 20.f * std::pow(20000.f / 20.f, velocity_);
			//velocityFilter.setLowpass(cutoffHz);
			velocityFilter2.set_f_q<stmlib::FREQUENCY_FAST>(cutoffHz / sampleRate, 0.7071f); // correct q?
		}

		//return velocityFilter.process(output);
		return velocityFilter2.Process<stmlib::FILTER_MODE_LOW_PASS>(output);
	}

protected:

	void spawnGrain() {
		// Modulatables:
		// Shape,
		// PlaybackSpeed,
		 // Gain

		const float shapeVarianceAmount = 0.05f; // 5%, 10%, 15%?
		const float playbackSpeedVarianceAmount = 0.3f;
		const float grainSpeedTemp = 1.f; // 0.5f is a bit less clicky TODO: think I want to control from somewhere else...
		// Make grain speed dependant on module pitch input?

		// Granular bow / blow
		for (int channel = 1; channel < MAX_CHANNELS; channel++)
		{
			// Find inactive channel
			if (!grain[channel].active)
			{
				// Set grain parameters
				grain[channel].gain = grainGain * 0.25f; // 1 / (MAX_CHANNEL * 0.5)

				float shapeVariance = (rack::random::uniform() * 2.f - 1.f) * shapeVarianceAmount;
				grain[channel].texture = mClamp(texture_ + shapeVariance, 0.f, 1.f);

				float playbackSpeedVariance = (rack::random::uniform() * 2.f - 1.f) * playbackSpeedVarianceAmount;
				grain[channel].speed = mClamp(grainSpeedTemp + playbackSpeedVariance, 1.f, 8.f);

				grain[channel].phase = 0.f;
				grain[channel].active = true;
				return;
			}
		}
	}


	inline void normaliseAmplitudeEnvelope(int tableIdx, float peak) {
		tableIdx = mClamp(tableIdx, 0, (NUM_TABLES - 1));
		if (peak > 0.f)
		{
			for (int i = 0; i < TABLE_LEN; i++)
				table[tableIdx][i] /= peak;
		}
	}
	// Raised cosine helpers
	inline float sinc(float x)
	{
		if (std::fabsf(x) < 1e-6f)
			return 1.f;

		return std::sinf(M_PI * x) / (M_PI * x);
	}

	inline float raisedCosine(float t, float T, float beta)
	{
		float x = t / T;
		float denom = 1.0f - std::powf(2.f * beta * x, 2.f);

		if (std::fabsf(denom) < 1e-6f)
			return (M_PI / 4.f) * sinc(1.f / (2.f * beta));

		return sinc(x) * std::cosf(M_PI * beta * x) / denom;
	}

	int sampleRate = 48000;

	static constexpr int MAX_CHANNELS = 8; // are all of these used
	static constexpr int NUM_TABLES = 2;
	static constexpr int TABLE_LEN = 256;

	std::array<Grain, MAX_CHANNELS> grain = {};
	std::array<std::array<float, TABLE_LEN>, NUM_TABLES> table = {};

	float velocity_ = 0.f;
	float density_ = 0.f;
	float texture_ = 0.f;

	//float output = 0.f;

	ADR amplitudeADR;

	float spawnPhase = 0.f;
	float spawnRate = 600.f;

	//float densityParam = 0.f;
	//float textureParam = 0.f;
	float grainGain = 0.f;

	//float velocityParam = 0.f;
	//Biquad velocityFilter;
	stmlib::Svf velocityFilter2;
};