#include "beamModel.hpp"


BeamModel::BeamModel() {
	buildRoots();

	computeFrequencies();
	computeWeights();
}

void BeamModel::buildRoots() {
	// From Natan Ho - Exploring Modal Synthesis
	// Normalised frequency ratio (fk) proportional to f0

	const float freeFundamentalRoot = 1.50561873f;
	const float cantFundamentalRoot = 0.59686416f;

	for (int i = 0; i < MAX_MODES; i++) {
		float freeRoot = float(i) + 1.5f;
		float cantRoot = float(i) + 0.5f;

		freeBeamRoot[i] = std::pow(freeRoot / freeFundamentalRoot, 2.f);
		cantBeamRoot[i] = std::pow(cantRoot / cantFundamentalRoot, 2.f);
	}

	// Override the first few modes with Natan Ho roots
	freeBeamRoot[0] = 1.f;
	freeBeamRoot[1] = std::pow(2.49975267f / freeFundamentalRoot, 2.f);

	cantBeamRoot[0] = 1.f;
	cantBeamRoot[1] = std::pow(1.49417561f / cantFundamentalRoot, 2.f);
	cantBeamRoot[2] = std::pow(2.50024695f / cantFundamentalRoot, 2.f);
}

void BeamModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFrequency = this->samplerate * 0.45f;
}


void BeamModel::setParams(float pitch, float position) {

	pitch = mClamp(pitch, 20.f, maxFrequency);
	position = mMap(position, 0.001f, 0.999f);

	if (this->pitch != pitch) {
		this->pitch = pitch;
		computeFrequencies();
	}

	if (this->position != position) {
		this->position = position;
		computeWeights();
	}
}


void BeamModel::computeFrequencies() {

	for (int i = 0; i < MAX_MODES; i++) {
		frequencies[i] = pitch * cantBeamRoot[i];
	}
}


void BeamModel::computeWeights() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		if (frequencies[i] < 20.f || frequencies[i] > maxFrequency) {
			weights[i] = 0.f;
			continue;
		}

		float weight = 1.f;
		weights[i] = weight;

		energy = weight * weight;

	}
	normaliseWeights(weights, energy);
}


/*
void BeamModel::update() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		float f = fundamentalFreq * cantBeamRoot[i];
		frequencies[i] = f;

		// Amplitude // 
		// Position can change after excitation, not realistic but sounds cool
		// From Banks, Balázs et al. 2010. 'A Modal-Based Real-Time Piano Synthesizer'
		//float positionAmplitude = std::sin(M_PI * n * position);
		float a = 1.f;
		weights[i] = a;
		energy = a * a;

	}
	normaliseWeights(weights, energy);
}
*/
