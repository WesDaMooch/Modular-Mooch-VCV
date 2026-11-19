#include "rectModel.hpp"



RectModel::RectModel() {
	//buildTables();
	computeFrequencies();
	computeWeights();
}


void RectModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFrequency = this->samplerate * 0.45f;
}


void RectModel::setParams(float pitch, float position) {

	pitch = mClamp(pitch, 20.f, maxFrequency);
	position = mMap(position, 0.5f, 3.f);

	if (this->pitch != pitch) {
		this->pitch = pitch;
		computeFrequencies();
	}

	if (this->position != position) {
		this->position = position;
		computeWeights();
	}
}




void RectModel::computeFrequencies() {

	constexpr int MAX_INDEX = MAX_MODES / 4;

	int i = 0;

	// TODO: find lowest MAX_MODES...

	for (int m = 1; m <= MAX_INDEX; ++m) {
		for (int n = 1; n <= MAX_INDEX; ++n) {

			float ratio = std::sqrt(
				(m * m + (n * n) / (position * position)) /
				(1.f + 1.f / (position * position))
			);

			frequencies[i] = pitch * ratio;
			i++;
		}
	}
}


void RectModel::computeWeights() {

	//float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		if (frequencies[i] < 20.f || frequencies[i] > maxFrequency) {
			weights[i] = 0.f;
			continue;
		}

		// Little decay, just shit really
		float t = static_cast<float>(i) / (MAX_MODES - 1);
		weights[i] = 1.f - 0.9f * t;

		//energy += weight * weight;
	}
	//normaliseWeights(weights, energy);
}



/*
void RectModel::update() {
	constexpr int MAX_INDEX = MAX_MODES / 4;

	int idx = 0;

	// TODO: find lowest MAX_MODES...

	for (int m = 1; m <= MAX_INDEX; ++m)
	{
		for (int n = 1; n <= MAX_INDEX; ++n)
		{
			float ratio = std::sqrt(
				(m * m + (n * n) / (aspectRatio * aspectRatio)) /
				(1.0f + 1.0f / (aspectRatio * aspectRatio))
			);

			coefs[idx].freq = fundamentalFreq * ratio;
			coefs[idx].q = baseDecay;
			//coefs[idx].amplitude = 1.f;
			//coefs[idx].amplitude = (idx == 0) ? 1.f : 0.f;
			float t = static_cast<float>(idx) / (MAX_MODES - 1);
			coefs[idx].amplitude = 1.f - 0.9f * t;
			idx++;
		}
	}
}
*/