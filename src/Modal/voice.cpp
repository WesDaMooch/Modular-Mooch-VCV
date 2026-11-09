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
	
	//for (auto& resonator : resonators)
	//	resonator.setSamplerate(sampleRate);
}

/*
void Voice::setStaticParams(Voice::StaticParams& p) {
	modelParams.fundamentalFreq = p.freq;
	modelParam = mClamp(p.model, 0.f, 1.f);
	attackTime = p.attack;
}


void Voice::setDynamicParams(Voice::DynamicParams& p) {
	modelParams.morph = p.morph;
	modelParams.position = p.position;
	modelParams.decay = p.decay;
	modelParams.timbre = p.timbre;
}

void Voice::setGlobalParams(Voice::GlobalParams& p) {
	exciterParams.velocity = p.velocity;
	exciterParams.texture = p.exciterTexture;
	exciterParams.density = p.exciterDensity;
}
*/

void Voice::setParams(
	float model,
	float freq,
	float morph,
	float position,
	float decay,
	float timbre,
	float velocity,
	float exciterTexture,
	float exciterDensity
) {

	modelParam = mClamp(model, 0.f, 1.f);
	modelParams.fundamentalFreq = freq;
	modelParams.morph = morph;
	modelParams.position = position;
	modelParams.decay = decay;
	modelParams.timbre = timbre;
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

	// Set model params
	if (structA != nullptr) {
		structA->setParams(modelParams);

		if (structB != nullptr)
			structB->setParams(modelParams);
	}

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

		// Filter bank
		coefs.freq = mClamp(coefs.freq, 1.f, sampleRate * 0.5f);
		coefs.q = std::max(coefs.q, 0.00001f); //TODO: make MIN_Q in baseModel.h common

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
	
	return output * outputScaler;
}