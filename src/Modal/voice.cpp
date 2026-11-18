// Voice.cpp


#include "voice.hpp"

Voice::Voice() {
	for (auto& resonator : resonators)
		resonator.Init();
}


void Voice::setSampleRate(int sampleRate) {
	this->sampleRate = std::max(sampleRate, 1);
	
	exciter.setSampleRate(this->sampleRate);

	for (auto* structure : structures) 
		structure->setSamplerate(this->sampleRate);
}

void Voice::setParams(
	float model,
	float freq,
	float timbre,
	float timbreMod,
	float timbreSelect,
	float position,
	float decay,
	float velocity,
	float exciterTexture,
	float exciterDensity
) {
	// Model param
	modelParam = mClamp(model, 0.f, 1.f);

	// Model params
	bool updateModelParams = false;
	if (freq != modelParams.fundamentalFreq) {
		modelParams.fundamentalFreq = freq;
		updateModelParams = true;
	}
	if (position != modelParams.position) {
		modelParams.position = position;
		updateModelParams = true;
	}
	if (decay != modelParams.decay) {
		modelParams.decay = decay;
		updateModelParams = true;
	}

	if (updateModelParams) {
		// Update model params
		if (structA != nullptr) {
			structA->setParams(modelParams);

			if (structB != nullptr)
				structB->setParams(modelParams);
		}
	}

	// Timbre params
	//DecayResponse::Type type = static_cast<DecayResponse::Type>(index);
	//decayResponse.set(timbre, timbreMod, type);

	// Timbre //
	float timbreSelectDifference = timbreSelect - prevTimbreSelectValue;
	int timbreSelectDelta = std::round(timbreSelectDifference / TIMBRE_SELECT_INDENT);

	if (timbreSelectDelta != 0) {
		prevTimbreSelectValue += timbreSelectDelta * TIMBRE_SELECT_INDENT;

		int typeIndex = static_cast<int>(timbreType) + timbreSelectDelta;

		while (typeIndex < 0)
			typeIndex += DecayResponse::Type::NUM_TYPES;

		while (typeIndex >= DecayResponse::Type::NUM_TYPES)
			typeIndex -= DecayResponse::Type::NUM_TYPES;

		timbreType = static_cast<DecayResponse::Type>(typeIndex);
	}

	this->timbre = timbre;
	this->timbreMod = timbreMod;


	// Exciter params
	exciterParams.velocity = velocity;
	exciterParams.texture = exciterTexture;
	exciterParams.density = exciterDensity;
}


int Voice::getOrder() {
	return voiceOrder;
}


void Voice::setOrder(int order) {
	order = mClamp(order, 0, NUM_VOICES - 1);

	if (order == voiceOrder)
		return;

	voiceOrder = order;

	// Modes decay naturally for the first & second order voices
	// Run attack mode and sustain forever (until note becomes inactive)
	if (voiceOrder <= 0)
		return;
	
	switch (voiceOrder) {
	case 1:
		ampEnvelope.setParams(attackTime, 2.f);	// 2s
		ampEnvelope.release();
		break;
	case 2:
		ampEnvelope.setParams(attackTime, 1.f);	// 1s
		ampEnvelope.release();
		break;
	case 3:
		ampEnvelope.setParams(attackTime, 0.05f); // 50ms
		ampEnvelope.release();
		break;
	default:
		ampEnvelope.setParams(attackTime, 0.05f);	// 50ms
		ampEnvelope.release();
		break;
	}
}


bool Voice::isActive() const {
	return active;
}

// For debug
float Voice::getEnvelopeLevel() const {
	//return envelopeFollower;
	return ampEnvelope.getValue();
}


void Voice::noteOn() {
	
	active = true;

	ampEnvelope.setParams(attackTime, 2.f);
	ampEnvelope.trigger();

	// Reset envelope follower
	envelopeFollower = 0.f;
	quietTime = 0.f;


	// Trigger excitor
	exciter.trigger();
}


void Voice::deactivate() {
	active = false;
	voiceOrder = -1;

	ampEnvelope.reset();

	// Reset envelope follower
	envelopeFollower = 0.f;
	quietTime = 0.f;
}



float Voice::process(float sampleTime) {
	if (!active)
		return 0.f;

	exciter.process(sampleTime, exciterParams);
	float excitation = exciter.get(); //+ feedback; // Add feedback into exciter

	// Set models
	const float MAX_MODEL = static_cast<float>(NUM_STRUCTURES - 1);
	float modelValue = mClamp(modelParam * MAX_MODEL, 0.f, MAX_MODEL);

	size_t modelAIdx = static_cast<size_t>(std::floor(modelValue));
	size_t modelBIdx = static_cast<size_t>(std::ceil(modelValue));

	structA = structures[modelAIdx];
	structB = nullptr;

	if (modelAIdx != modelBIdx)
		structB = structures[modelBIdx];

	structCrossfade = modelValue - static_cast<float>(modelAIdx);
	
	if (structA == nullptr)
		return 0.f;

	const auto& coefsA = structA->getAllCoefs();
	const auto& coefsB = structB->getAllCoefs();

	// Set decay
	decayResponse.set(timbre, timbreMod, timbreType, );

	// Compute resonators
	float output = 0.f;
	for (int i = 0; i < MAX_MODES; i++) {
		// Structure
		coefs = {};

		if (structA == nullptr)
			continue;

		const SvfCoefficients& structCoefsA = structA->getCoefficients(i);

		if (structB != nullptr) {
			const SvfCoefficients& structCoefsB = structB->getCoefficients(i);
			mInterpSvfCoefs(structCrossfade, coefs, structCoefsA, structCoefsB);
		}
		else {
			coefs = structCoefsA;
		}

		// Dont process resonator if its silent
		if (coefs.amplitude == 0.f)
			continue;

		// Filter bank
		coefs.freq = mClamp(coefs.freq, 20.f, sampleRate * 0.45f);

		//TODO: make MIN and MAX_Q in baseModel.h common
		float maxQ = 5000.f;
		float minQ = 0.0001f; //0.00001
		coefs.q = mClamp(coefs.q, minQ, maxQ); 

		decayResponse.get(i);

		// TODO: not sure if  slew is the best way to stop filter from clicking when changing quickly
		float slewCoeff = std::exp(-sampleTime / freqSlewTime);
		freqSlew[i] = coefs.freq + slewCoeff * (freqSlew[i] - coefs.freq);

		resonators[i].set_f_q<stmlib::FREQUENCY_FAST>(freqSlew[i] / sampleRate, coefs.q);  // TODO: only update when freq and q change?

		float modeOut = resonators[i].Process<stmlib::FILTER_MODE_BAND_PASS>(excitation);
		modeOut *= coefs.amplitude;

		// TODO: per mode soft clipping? tanh?

		output += modeOut;
	}
	
	float envelope = ampEnvelope.process();

	if (ampEnvelope.getValue() <= 0.f)
		deactivate();

	output *= envelope;
	float envelopeFollowerInput = output;

	// TODO: I think this is a bit overkill, 
	// could just count the num samples under a threshold
	// 
	// Envelope follower, pre attack envelope
	// One-pole smoothing
	float input = std::abs(envelopeFollowerInput);
	float time = (input > envelopeFollower) ? envelopeAttack : envelopeRelease;

	time = std::max(time, 0.000001f);
	float coeff = std::exp(-sampleTime / time);
	envelopeFollower = coeff * envelopeFollower + (1.f - coeff) * input;

	// Voice sleep detection
	if (envelopeFollower < sleepThreshold) {
		quietTime += sampleTime;
	}
	else {
		quietTime = 0.0f;
	}

	if (quietTime >= sleepTime)
		deactivate();
	
	return output * 0.5f; //* outputScaler;
}