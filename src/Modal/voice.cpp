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
}


void Voice::setDynamicParams(Voice::DynamicParams& p) {
	exciterParams.velocity = p.velocity;
	exciterParams.texture = p.exciterTexture;
	exciterParams.density = p.exciterDensity;
	
	modelParams.morph = p.morph;
	modelParams.position = p.position;
	modelParams.decay = p.decay;
	modelParams.timbre = p.timbre;
}


bool Voice::isActive() const {
	return active;
}


float Voice::getEnvelopeLevel() const {
	return envelopeFollower;
}


void Voice::noteOn() {
	active = true;

	envelopeFollower = 0.f;
	quietTime = 0.f;

	// Set excitor params if in the on trigger mode
	//this->exciterParams.velocity = mClamp(exciterParams.velocity, 0.f, 1.f);
	//this->exciterParams.texture = mClamp(exciterParams.texture, 0.f, 1.f);
	//this->exciterParams.density = mClamp(exciterParams.density, 0.f, 1.f);

	// Trigger excitor
	exciter.trigger();


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

	// Start attack env
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

	output *= outputScaler;
	output = std::tanh(output); // easy soft clipping pre voice

	// Compute envelope follower or even:
	// If output has been near zero (below a small threshold) for x amount of samps
	// active = false
	

	// Envelope follower
	float input = std::abs(output);

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
		active = false;
		envelopeFollower = 0.f;
		quietTime = 0.f;
	}
	
	return output;
}