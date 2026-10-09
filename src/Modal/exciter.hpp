#pragma once
#include <array>
#include <cmath>
#include <random>
#include <rack.hpp>
#include "..\dsp\biquad.hpp"
#include "..\dsp\adr.hpp"
#include "common.hpp"

// TODO: fine tune grain env

struct Grain
{
	bool active = false;
	float phase = 0.f;

	float speed = 2.f;
	float texture = 0.f;
	float gain = 0.f;
};

struct ExciterParams
{
	float texture = 0.f;
	float density = 0.f;
	float velocity = 0.f;
};

class Exciter
{
public:
	Exciter();
	void buildTables();
	void setSampleRate(int newSampleRate);
	void trigger();
	void process(float deltaTime, ExciterParams& p);
	float get();

protected:
	void spawnGrain();

	static constexpr int MAX_CHANNELS = 8;
	static constexpr int NUM_TABLES = 3;
	static constexpr int TABLE_LEN = 1024;

	std::array<Grain, MAX_CHANNELS> grain = {};
	std::array<std::array<float, TABLE_LEN>, NUM_TABLES> table = {};

	int sr = 48000;
	float output = 0.f;

	ADR amplitudeADR;

	float spawnPhase = 0.f;
	float spawnRate = 600.f;

	float densityParam = 0.f;
	float textureParam = 0.f;
	float grainGain = 0.f;

	float velocityParam = 0.f;
	Biquad velocityFilter;

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
};