#include "stringModel.hpp"


StringModel::StringModel() {
	computeFrequencies();
	computeWeights();
}


void StringModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFrequency = this->samplerate * 0.45f;
}

void StringModel::setParams(float pitch, float position) {

	pitch = mClamp(pitch, 20.f, maxFrequency);
	position = mMap(position, 0.02f, 0.98f);

	if (this->pitch != pitch) {
		this->pitch = pitch;
		computeFrequencies();
	}

	if (this->position != position) {
		this->position = position;
		computeWeights();
	}
}

void StringModel::computeFrequencies() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		// Stiff sting
		// From Gillet, Émilie. 2015. Mutable Instruments Elements
		//float B = stiffness;
		//float f = fundamentalFreq * n * (1.f + (n - 1) * B);
		frequencies[i] = pitch * n;
	}
}


void StringModel::computeWeights() {

	// Position can change after excitation, not realistic but sounds cool
	// From Banks, Balázs et al. 2010. 'A Modal-Based Real-Time Piano Synthesizer'

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		if (frequencies[i] < 20.f || frequencies[i] > maxFrequency) {
			weights[i] = 0.f;
			continue;
		}
			
		const int n = i + 1;

		float weight = std::sin(M_PI * n * position);
		weights[i] = weight;

		energy += weight * weight;
	}
	normaliseWeights(weights, energy);
}

/*
void StringModel::update() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;
		
		// Stiff sting
		// From Gillet, Émilie. 2015. Mutable Instruments Elements
		//float B = stiffness;
		//float f = fundamentalFreq * n * (1.f + (n - 1) * B);
		frequencies[i] = pitch * n;

		// Amplitude // 
		// Position can change after excitation, not realistic but sounds cool
		// From Banks, Balázs et al. 2010. 'A Modal-Based Real-Time Piano Synthesizer'
		float positionAmplitude = std::sin(M_PI * n * position);
		weights[i] = positionAmplitude;
		energy += positionAmplitude * positionAmplitude;
	}
	normaliseWeights(weights, energy);
}
*/