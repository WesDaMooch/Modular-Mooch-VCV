#pragma once
#include <array>
#include <cmath>
#include "common.hpp"

struct ExciterParams
{
	// Amp env
	float type = 0.f;
	
	// Atk env
	float attack = 0.f;

	//Delay
	float delayTime = 0.f;
	float delayType = 0.f;
};


class Exciter
{
public:

	struct Out
	{
		float value = 0.f;
		float env = 0.f;

		Out(float value = 0.f, float env = 0.f) : value(value), env(env) {}
	};

	Exciter();

	void trigger(ExciterParams params);
	void update(float deltaTime);
	void advanceWriteIdx();
	void reset();
	Out get(int modeIdx);
	//float getEnv(int modeIdx);

protected:
	enum Stage {
		IDLE,
		ATTACK,
		DECAY
	};

	// Amplitude envelope
	Stage ampStage; // = IDLE
	float ampValue = 0.f;
	float ampAttack = 0.f;
	float ampDecay = 0.f;

	// Attack envelope
	Stage atkStage = IDLE;
	float atkValue = 1.f;
	float atkAttack = 0.f;
	static constexpr float atkDecay = 0.05f;

	// Delay
	static constexpr int MAX_DELAY_SAMPLES = 144000; // 3 seconds ish

	std::array<float, MAX_DELAY_SAMPLES> exciterDelayBuffer = {};
	std::array<float, MAX_DELAY_SAMPLES> atkEnvDelayBuffer = {};
	int writeIdx;

	float delayTime = 0.f;
	float delayType = 0.f;

	// Delay types
	static constexpr int NUM_DELAY_STATES = 5; //TODO: rename NUM_DELAY_TYPES
	std::array<float, MAX_MODES> modeDelayDecreasingLinearLut = {};
	std::array<float, MAX_MODES> modeDelayDecreasingExpLut = {};
	std::array<float, MAX_MODES> modeDelayRandomGrainLut = {};
	std::array<float, MAX_MODES> modeDelayIncreasingExpLut = {};
	std::array<float, MAX_MODES> modeDelayIncreasingLinearLut = {};

	void buildLuts();
	float getNorm(int modeIdx);
};