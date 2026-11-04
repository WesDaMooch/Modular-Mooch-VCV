// Voice.cpp


#include "voice.hpp"

Voice::Voice() {
	
}


void Voice::setSampleRate(int sampleRate) {
	sampleRate = std::max(sampleRate, 1);
	
	exciter.setSampleRate(sampleRate);

	for (auto* structure : structures) 
		structure->setSamplerate(sampleRate);
	
	for (auto& resonator : resonators)
		resonator.setSamplerate(sampleRate);
}


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



int Voice::getOrder() {
	return voiceOrder;
}


void Voice::setOrder(int order) {
	order = mClamp(order, 0, NUM_VOICES - 1);

	if (order == voiceOrder)
		return;

	voiceOrder = order;

	switch (voiceOrder) {
	case 0:
		// Modes decay naturally for the first order voice
		// Run attack mode and sustain forever (until note becomes inactive)
		break;
	case 1:
		// 
		ampEnvelope.setParams(attackTime, 2.f);
		ampEnvelope.release();
		break;
	case 2:
		// Start to decay to 0 in 1 secs
		ampEnvelope.setParams(attackTime, 0.5f);
		ampEnvelope.release();
		break;
	case 3:
		// Start to decay to 0 in 0.05 secs
		ampEnvelope.setParams(attackTime, 0.05f);
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
	//return releaseLevel;
	return ampEnvelope.getValue();
}


void Voice::noteOn() {
	
	// If voice is already active don't retrigger
	// the attack envelope
	//if (!active) {
		// attackLevel = releaseLevel;
		//attackLevel = 0.f;
	//}

	active = true;
	ampEnvelope.setParams(attackTime, 2.f);
	ampEnvelope.trigger();

	//releaseLevel = 1.f;
	//stealVoice = false;
	//stealGain = 1.f;

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

	envelopeFollower = 0.f;
	quietTime = 0.f;

	//releasing = false;
	//releaseLevel = 1.f;

	// Reset envelope follower
	//envelopeFollower = 0.f;
	//quietTime = 0.f;

	// Reset filters?
	//for (auto& resonator : resonators)
	//	resonator.reset();
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
		resonators[i].setCoefficients(coefs);
		resonators[i].process(excitation);

		float modeOut = resonators[i].bandpass();
		// TODO: per mode soft clipping? tanh?

		output += modeOut;
	}
	
	float envelope = ampEnvelope.process();

	if (ampEnvelope.getValue() <= 0.f) {
		deactivate();
	}

	output *= outputScaler * envelope;
	float envelopeFollowerInput = output;

	// Attack envelope
	/*
	if (attackTime <= 0.f) {
		attackLevel = 1.f;
	}
	else {
		attackLevel += sampleTime / attackTime;
		attackLevel = std::min(attackLevel, 1.f);
	}
	output *= attackLevel;
	*/

	//output = std::tanh(output); // easy soft clipping pre voice

	// Compute envelope follower or even:
	// If output has been near zero (below a small threshold) for x amount of samps
	// active = false
	

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

	if (quietTime >= sleepTime) {
		deactivate();
		//active = false;
	}
	
	return output;
}