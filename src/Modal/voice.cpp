// Voice.cpp


#include "voice.hpp"

Voice::Voice() {
	for (auto& resonator : resonators)
		resonator.Init();
}


void Voice::setSampleRate(int sampleRate) {
	this->sampleRate = std::max(sampleRate, 1);
	
	exciter.setSampleRate(this->sampleRate);

	for (auto* structure : models) 
		structure->setSamplerate(this->sampleRate);
}

void Voice::setParams(
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
) {
	// Model param
	modelParam = mClamp(model, 0.f, 1.f);

	// Model params
	bool updateModel = false;
	if (this->pitch != pitch) {
		this->pitch = pitch;
		updateModel = true;
	}
	if (this->position != position) {
		this->position = position;
		updateModel = true;
	}

	if (updateModel) {
		// TODO: set model refs here

		// Update model params
		if (modelA != nullptr) {
			modelA->setParams(this->pitch, this->position);

			if (modelB != nullptr)
				modelB->setParams(this->pitch, this->position);
		}
	}

	// Timbre //
	float timbreSelectDifference = timbreSelect - prevTimbreSelectValue;
	int timbreSelectDelta = std::round(timbreSelectDifference / TIMBRE_SELECT_INDENT);

	if (timbreSelectDelta != 0) {
		prevTimbreSelectValue += timbreSelectDelta * TIMBRE_SELECT_INDENT;

		const int numTypes = static_cast<int>(DecayResponse::Type::NUM_TYPES);
		int typeIndex = static_cast<int>(timbreType) + timbreSelectDelta;

		while (typeIndex < 0)
			typeIndex += numTypes;

		while (typeIndex >= numTypes)
			typeIndex -= numTypes;

		timbreType = static_cast<DecayResponse::Type>(typeIndex);
	}

	this->timbre = timbre;
	this->timbreMod = timbreMod;

	baseQ = mMap(std::pow(decay, 3.f), 1e-4f, 1000.f);

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

	modelA = models[modelAIdx];
	modelB = nullptr;

	if (modelAIdx != modelBIdx)
		modelB = models[modelBIdx];

	modelCrossfade = modelValue - static_cast<float>(modelAIdx);
	
	if (modelA == nullptr)
		return 0.f;

	// Local copies
	auto freqs = modelA->getFrequencies();
	auto weights = modelA->getWeights();

	if (modelB != nullptr) {	
		const auto& freqsB = modelB->getFrequencies();
		const auto& weightsB = modelB->getWeights();
		mInterpolateArray(freqs, freqs, freqsB, modelCrossfade);
		mInterpolateArray(weights, weights, weightsB, modelCrossfade);
	}

	// Set decay
	decayResponse.set(timbre, timbreMod, timbreType, freqs);

	// Compute resonators
	float output = 0.f;
	for (int i = 0; i < MAX_MODES; i++) {

		// Filter bank
		float f = mClamp(freqs[i], 20.f, sampleRate * 0.45f);

		// TODO: not sure if  slew is the best way to stop filter from clicking when changing quickly
		float slewCoeff = std::exp(-sampleTime / freqSlewTime);
		freqSlew[i] = f + slewCoeff * (freqSlew[i] - f);

		float q = decayResponse.get(i) * baseQ;
		q = mClamp(q, 1e-4f, 2000.f);
		resonators[i].set_f_q<stmlib::FREQUENCY_FAST>(freqSlew[i] / sampleRate, q);  // TODO: only update when freq and q change?

		float modeOut = resonators[i].Process<stmlib::FILTER_MODE_BAND_PASS>(excitation);
		modeOut *= weights[i];

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