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

	Voice();
	void setSampleRate(int sampleRate);

	void setParams(
		float model,
		float pitch,
		float timbre,
		float timbreMod,
		float timbreSelect,
		float position,
		float decay,
		float velocity,
		float exciterTexture,
		float exciterDensity
	);

	int getOrder();
	void setOrder(int order);

	bool isActive() const;
	float getDebugOutput() const;
	void noteOn(float punchAmount, float punchDecay);
	float process(float sampleTime);

protected:
	void deactivate();

	// Voice //
	bool active = false;
	int voiceOrder = -1;
	int sampleRate = 48000;

	ASR ampEnvelope;
	float attackTime = 0.01f; // 10ms

	// Envelope follower
	constexpr static float envelopeAttack = 0.005f;	// 5ms
	constexpr static float envelopeRelease = 0.03f;	// 50ms

	constexpr static float sleepThreshold = 0.0001f;
	constexpr static float sleepTime = 0.01f;	// 10ms

	float envelopeFollower = 0.f;
	float quietTime = 0.f;


	// Exciter //
	Exciter exciter;
	ExciterParams exciterParams;


	// Model //
	//WaveformModel waveformModel;
	StringModel stringModel;
	BeamModel beamModel;

	std::array<BaseModel*, NUM_STRUCTURES> models = {
		//&waveformModel,
		&stringModel,
		&beamModel
	};

	BaseModel* modelA = models[0];
	BaseModel* modelB = nullptr;
	float modelCrossfade = 0.f;
	float modelParam = 0.f;
	float pitch = 220.f;
	float position = 0.5f;

	//one-pole smoothing for filter frequency 
	constexpr static float freqSlewTime = 0.001f; // 1 ms
	std::array<float, MAX_MODES> freqSlew = {};	// TODO: would linear slew be better?


	// Timbre //
	DecayResponse decayResponse;
	DecayResponse::Type timbreType = DecayResponse::Type::LOWPASS_SECOND_ORDER;
	static constexpr float TIMBRE_SELECT_INDENT = 1.f / 30.f;
	float prevTimbreSelectValue = 0.f;
	float timbre = 0.f;
	float timbreMod = 0.f;
	float baseQ = 1.f;


	// Punch //
	// Max 24 semitones range - 'midi' 
	static constexpr float SEMITONE_TO_OCTAVE = 1.f / 12.f;
	float punchAmount = 0.f;
	float punchEnvelope = 0.f;
	float punchDecayCoefficient = 0.f;

	// Space //


	// Resonator //
	SvfCoefficients coefs;
	std::array<stmlib::Svf, MAX_MODES> resonators = {};


	// Output //
	float outputScaler = 1.f / (MAX_MODES * 0.5f);
};


