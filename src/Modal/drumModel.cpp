#include "drumModel.hpp"


DrumModel::DrumModel() {
	computeFrequencies();
	computeWeights();
}


void DrumModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFrequency = this->samplerate * 0.25f;
}


void DrumModel::setParams(float pitch, float position) {
	pitch = mClamp(pitch, 20.f, maxFrequency);
	size = 3.f; //mMap(newParams.morph, 0.0001f, 6.f);
	//position = position; // Map??
	damping = 0.5f; //newParams.decay;
	overtones = 0.5f; //newParams.timbre;

	if (this->pitch != pitch) {
		this->pitch = pitch;
		computeFrequencies();
	}

	if (this->position != position) {
		this->position = position;
		computeWeights();
	}
}


void DrumModel::computeFrequencies() {

	for (int i = 0; i < MAX_MODES; i++) {
		const Root& r = roots[i];
		frequencies[i] = (r.value * pitch) / size;
	}
}


void DrumModel::computeWeights() {

	//float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		if (frequencies[i] < 20.f || frequencies[i] > maxFrequency) {
			weights[i] = 0.f;
			continue;
		}

		// TODO: Old, this needs a work to sort out
		// TODO: doesnt work
		/*
		float weight = bessel(roots.order, roots.value * position);
		weights[i] = scale(weight, 0.f, 1.f, overtones, 1, damping) * 100.f;
		*/

		//energy += weight * weight;
	}
	//normaliseWeights(weights, energy);
}



/*
void DrumModel::update() {
	for (size_t idx = 0; idx < MAX_MODES; idx++) {
		const Root& r = roots[idx];
		float f = (r.value * tuning) / size;

		if (f < minFreq || f > maxFreq) {
			coefs[idx].freq = 220.f;
			coefs[idx].amplitude = 0.f;
			coefs[idx].q = 1.f;
			continue;
		}

		coefs[idx].freq = f;

		float weight = bessel(r.order, r.value * position);
		coefs[idx].q = scale(weight, 0.f, 1.f, overtones, 1, damping) * 100.f;
		coefs[idx].amplitude = 1.f;
	}
}
*/


float DrumModel::bessel(int order, float x)
{
	const float EPS = 1e-8;
	const int MAX_TERMS = 20;

	// J0
	if (order == 0) {
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) * std::pow(x / 2.0, 2 * k) / (factorial(k) * factorial(k));

			sum += term;
			k++;
		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// J1
	if (order == 1) {
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) * std::pow(x / 2.0, 2 * k + 1) / (factorial(k) * factorial(k + 1));

			sum += term;
			k++;

		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// higher orders (recurrence)
	if (x == 0.0)
		return 0.0;

	return (2.0 * (order - 1) / x) * bessel(order - 1, x) - bessel(order - 2, x);
}