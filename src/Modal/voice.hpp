// Voice.hpp


#pragma once
#include "common.hpp"
#include "exciter.hpp"
#include "waveformModel.hpp"
#include "stringModel.hpp"
#include "chamberlinSVF.hpp"


constexpr static int NUM_VOICES = 4;




class Voice {
public:
	struct StaticParams {
		float model = 0.f;
		float freq = 220.f;
	};

	struct DynamicParams {
		// Exciter
		float velocity = 0.f;
		float exciterTexture = 0.f;
		float exciterDensity = 0.f;

		// Model
		float morph = 0.f;
		float position = 0.f;
		float decay = 0.f;
		float timbre = 0.f;
	};

	Voice();
	void setSampleRate(int sampleRate);

	void setStaticParams(StaticParams& p);
	void setDynamicParams(DynamicParams& p);

	bool isActive() const;
	float getEnvelopeLevel() const;
	void noteOn();
	float process(float sampleTime);

protected:
	bool active = false;

	// Engine


	Exciter exciter;
	ExciterParams exciterParams;

	SvfCoefficients coefs;
	std::array<ChamberlinSVF, MAX_MODES> resonators = {};

	// Could the models not be in every voice?
	StructureParams modelParams;

	WaveformModel waveformModel;
	StringModel stringModel;

	std::array<BaseModel*, NUM_STRUCTURES> structures = {
		&waveformModel,
		&stringModel
	};

	BaseModel* structA = nullptr;
	BaseModel* structB = nullptr;
	float structCrossfade = 0.f;
	float modelParam = 0.f;

	float outputScaler = 1.f / MAX_MODES;


	// env follower
	float envelopeFollower = 0.0f;

	float envelopeAttack = 0.005f;  // 5 ms
	float envelopeRelease = 0.05f;  // 50 ms

	float sleepThreshold = 0.0001f;
	float sleepTime = 0.5f;

	float quietTime = 0.0f;
};


