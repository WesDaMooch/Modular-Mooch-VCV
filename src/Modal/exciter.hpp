#pragma once
#include <array>
#include <cmath>
#include <random>
#include <rack.hpp>
#include "common.hpp"


struct ExciterParams
{
	float velocity = 0.f;
	float shape = 0.f;	
	float noise = 0.f;

	//Delay
	float delayTime = 0.f;
	float delayType = 0.f;
};


class Exciter
{
public:
	Exciter();

	void trigger(ExciterParams params);
	void update(float deltaTime);
	void advanceWriteIdx();
	void reset();
	float get(int modeIdx);

protected:
	void buildLuts();

	// Exciter shape table
	static constexpr int NUM_TABLES = 3;
	static constexpr int TABLE_LEN = 1024; //1024; // TODO: Find length in ms at 48kHz sr

	std::array<std::array<float, TABLE_LEN>, NUM_TABLES> shapeTable = {};

	int ampTableIndex = 0;
	float ampTablePhase = 0.f;
	float ampTableIncrement = 8.f;
	
	float shapeParam = 0.f;
	int currentTable = 0;
	bool tableActive = false;

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


	inline void normaliseAmplitudeEnvelope(int tableIdx, float peak) {
		tableIdx = mClamp(tableIdx, 0, (NUM_TABLES - 1));
		if (peak > 0.f)
		{
			for (int i = 0; i < TABLE_LEN; i++)
				shapeTable[tableIdx][i] /= peak;
		}
	}
	

	// Raised cosine helpers
	inline float sinc(float x)
	{
		if (std::fabsf(x) < 1e-6f)
			return 1.0f;

		return sinf(M_PI * x) / (M_PI * x);
	}

	inline float raisedCosine(float t, float T, float beta)
	{
		float x = t / T;

		// Special case: t = ±T/(2*beta)
		float denom = 1.0f - std::powf(2.0f * beta * x, 2.0f);

		if (std::fabsf(denom) < 1e-6f)
		{
			return (M_PI / 4.0f) * sinc(1.0f / (2.0f * beta));
		}

		return sinc(x) * std::cosf(M_PI * beta * x) / denom;
	}

	// Delay
	void buildRandomDelay();
	float getDelay(int modeIdx);
};