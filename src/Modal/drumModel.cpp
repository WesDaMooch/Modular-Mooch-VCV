#include "drumModel.hpp"


Drum2::Drum2() {
	update();
}


void Drum2::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
	maxFreq = sr * 0.25f;
}


void Drum2::setParams(const StructureParams& newParams) {
	if (newParams == prevParams)
		return;

	tuning = mClamp(newParams.fundamentalFreq, minFreq, maxFreq);
	size = mMap(newParams.morph, 0.0001f, 6.f);
	position = newParams.position;
	damping = newParams.decay;
	overtones = newParams.timbre;

	prevParams = newParams;
	update();
}


void Drum2::update() {
	for (size_t idx = 0; idx < MAX_MODES; idx++)
	{
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


const SvfCoefficients& Drum2::getCoefficients(int idx)  const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


float Drum2::bessel(int order, float x)
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