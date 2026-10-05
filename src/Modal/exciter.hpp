#pragma once
#include <array>
#include <cmath>
#include <random>
#include <rack.hpp>
#include "..\dsp\biquad.hpp"
#include "..\dsp\adsr.hpp"
#include "common.hpp"

struct ExciterParams
{
	float shape = 0.f;
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
	void process(ExciterParams& p);
	float get();

protected:
	void spawnGrain();

	static constexpr int MAX_CHANNELS = 4;
	static constexpr int NUM_TABLES = 3;
	static constexpr int TABLE_LEN = 1024;

	std::array<bool, MAX_CHANNELS> tableActive = {};
	std::array<size_t, MAX_CHANNELS> currentTableIdx = {};
	std::array<float, MAX_CHANNELS> tablePhase = {};
	std::array<std::array<float, TABLE_LEN>, NUM_TABLES> table = {};

	int sr = 48000;
	float output = 0.f;

	ADSR adsr;

	float density = 1.f; // TODO: not used?
	float spawnProbability = 0.f;
	float spawnProbabilityDecay = 0.f;

	float spawnPhase = 0.f;
	float spawnRate = 400.f;

	std::array<float, MAX_CHANNELS> grainPlaybackSpeed = {};

	float shapeParam = 0.f;
	std::array<float, MAX_CHANNELS> shape = {};

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