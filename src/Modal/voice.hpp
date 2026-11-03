// Voice.hpp

// TODO:
// Voices knows its poly index
// After index 1 a decay env is applied, 
// subtle at first but inreasing in speed, 
// untill the last voice fades out very fast

#pragma once
#include "common.hpp"
#include "exciter.hpp"
#include "waveformModel.hpp"
#include "stringModel.hpp"
#include "beamModel.hpp"
#include "chamberlinSVF.hpp"


constexpr static int NUM_VOICES = 4;


class Voice {
public:
	int voiceOrder = -1;

	struct StaticParams {
		// Set on a new note
		float model = 0.f;
		float freq = 220.f;
		float attack = 0.01f;
	};

	struct DynamicParams {
		// Used by the latest voice
		float morph = 0.f;
		float position = 0.f;
		float decay = 0.f;
		float timbre = 0.f;
	};

	struct GlobalParams {
		// Used by all voices
		float velocity = 0.f;
		float exciterTexture = 0.f;
		float exciterDensity = 0.f;
	};

	Voice();
	void setSampleRate(int sampleRate);
	void setStaticParams(StaticParams& p);
	void setDynamicParams(DynamicParams& p);
	void setGlobalParams(GlobalParams& p);

	bool isActive() const;
	bool isStealing() const;
	float getEnvelopeLevel() const;
	void noteOn();
	void steal(int priority);
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
	BeamModel beamModel;

	std::array<BaseModel*, NUM_STRUCTURES> structures = {
		&waveformModel,
		&stringModel,
		&beamModel
	};

	BaseModel* structA = nullptr;
	BaseModel* structB = nullptr;
	float structCrossfade = 0.f;
	float modelParam = 0.f;

	float outputScaler = 1.f / MAX_MODES;

	// TODO: remove
	bool stealVoice = false;
	float stealGain = 1.f;
	float stealTime = 0.02f;

	// Attack Envelope
	float attackTime = 0.01f; // 10ms
	float attackLevel = 0.f;

	// Envelope follower
	constexpr static float envelopeAttack = 0.005f;	// 5ms
	constexpr static float envelopeRelease = 0.03f;	// 50ms

	constexpr static float sleepThreshold = 0.0001f;
	constexpr static float sleepTime = 0.01f;	// 10ms

	float envelopeFollower = 0.f;
	float quietTime = 0.f;

	void deactivate();
};


