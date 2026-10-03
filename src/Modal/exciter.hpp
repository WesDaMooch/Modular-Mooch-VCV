#pragma once
#include <array>
#include <cmath>
#include <random>
#include <rack.hpp>
#include "common.hpp"


struct GranulatorTriggerParams
{
	float shape = 0.f;
};

struct ExciterTriggerParams
{
	GranulatorTriggerParams g;

	float velocity = 0.f;
	float noise = 0.f;

	//Delay
	float delayTime = 0.f;
	float delayType = 0.f;
};


struct Granulator
{
	Granulator();
	void buildTables();
	// void setSamplerate(int newSamplerate);
	void trigger(GranulatorTriggerParams& p);
	void spawnGrain();
	float process();

	int sr = 48000; 

	static constexpr int MAX_CHANNELS = 4;
	static constexpr int NUM_TABLES = 3;
	static constexpr int TABLE_LEN = 1024;

	int channels = 1; // not used... yet?

	float density = 1.f;
	float spawnProbability = 0.f;
	float spawnProbabilityDecay = 0.f;

	float spawnPhase = 0.f;
	float spawnRate = 100.f;

	float shapeParam = 0.f;
	float shape = 0.f;

	float tablePlaySpeed = 8.f; // TODO: each grain could have a different speed

	std::array<bool, MAX_CHANNELS> tableActive = {};
	std::array<size_t, MAX_CHANNELS> currentTableIdx = {};
	std::array<float, MAX_CHANNELS> tablePhase = {};

	std::array<std::array<float, TABLE_LEN>, NUM_TABLES> table = {};

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


class Exciter
{
public:
	Exciter();

	void trigger(ExciterTriggerParams& p);
	void update(float deltaTime);
	void advanceWriteIdx();
	void reset();
	float get(int modeIdx);

protected:
	void buildLuts();

	Granulator granulator;

	// Velocity
	float velocityParam = 0.f;

	// Noise
	float noiseParam = 0.f;

	// Delay
	static constexpr int MAX_DELAY_SAMPLES = 144000; // 3 seconds ish

	std::array<float, MAX_DELAY_SAMPLES> exciterDelayBuffer = {};
	int writeIdx;

	float delayTime = 0.f;
	float delayType = 0.f;

	// Delay types
	static constexpr int NUM_DELAY_TYPES = 5;
	std::array<std::array<float, MAX_MODES>, NUM_DELAY_TYPES> delayTypeLuts = {};

	// TODO: replace with rack random or just remove all together
	std::mt19937 rng{ std::random_device{}() };

	inline float randomValue() {
		std::uniform_real_distribution<float> dist(0.f, 1.f);
		return dist(rng);
	}

	// Delay
	void buildRandomDelay();
	float getDelay(int modeIdx);
};



