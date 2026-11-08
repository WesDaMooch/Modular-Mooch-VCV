// Voice.hpp


// TODO: find when to update filters
// TODO: seperate update amp, q, and freq in the structs 
// or send bools for which ones to compute

#pragma once
#include "common.hpp"
#include "exciter.hpp"
#include "waveformModel.hpp"
#include "stringModel.hpp"
#include "beamModel.hpp"
#include "stmlib/dsp/filter.h"
#include "chamberlinSVF.hpp"
#include "../dsp/asr.hpp"

constexpr static int NUM_VOICES = 1; //4


class Voice {
public:
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

	int getOrder();
	void setOrder(int order);

	bool isActive() const;
	float getEnvelopeLevel() const;
	void noteOn();
	float process(float sampleTime);

protected:
	bool active = false;
	int voiceOrder = -1;

	//
	ASR ampEnvelope;

	// Engine
	int sampleRate = 48000;

	Exciter exciter;
	ExciterParams exciterParams;

	SvfCoefficients coefs;
	//std::array<ChamberlinSVF, MAX_MODES> resonators = {};
	std::array<stmlib::Svf, MAX_MODES> resonators = {};

	//one-pole smoothing for filter frequency 
	constexpr static float freqSlewTime = 0.001f; // 1 ms
	std::array<float, MAX_MODES> freqSlew = {};	// TODO: would linear slew be better?

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

	float outputScaler = 1.f / (MAX_MODES * 0.5f);

	// Attack Envelope
	float attackTime = 0.01f; // 10ms

	// Envelope follower
	constexpr static float envelopeAttack = 0.005f;	// 5ms
	constexpr static float envelopeRelease = 0.03f;	// 50ms

	constexpr static float sleepThreshold = 0.0001f;
	constexpr static float sleepTime = 0.01f;	// 10ms

	float envelopeFollower = 0.f;
	float quietTime = 0.f;

	void deactivate();
};


