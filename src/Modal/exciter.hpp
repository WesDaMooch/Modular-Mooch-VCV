#pragma once
#include <array>
#include <cmath>
#include "common.hpp"


struct ExciterParams
{
	// Amp env
	float type = 0.f;
	
	// Amp spectal env
	float spectralEnv = 0.f;

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
	enum Stage {
		IDLE,
		ATTACK,
		DECAY
	};

	// Amplitude envelope
	Stage ampStage = IDLE;
	float ampValue = 0.f;
	float ampAttack = 0.f;
	float ampDecay = 0.f;

	// Spectral envelope
	std::array<float, MAX_MODES> spectralEnvLut = {};

	// Delay
	static constexpr int MAX_DELAY_SAMPLES = 144000; // 3 seconds ish

	std::array<float, MAX_DELAY_SAMPLES> exciterDelayBuffer = {};
	int writeIdx;

	float delayTime = 0.f;
	float delayType = 0.f;

	// Delay types
	static constexpr int NUM_DELAY_TYPES = 5;
	std::array<std::array<float, MAX_MODES>, NUM_DELAY_TYPES> delayTypeLuts = {};

	void buildLuts();
	float getDelay(int modeIdx);
};