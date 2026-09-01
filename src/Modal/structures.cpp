#include "structures.hpp"


String::String() {
	update();
}

void String::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
	maxFreq = sr * 0.5f;
}

void String::setParams(StructureParams& newParams) {
	params.pitch = clamp11(newParams.pitch, minFreq, maxFreq);
	params.decay = clamp11(newParams.decay, 0.0f, 1000.0f);

	if (params != prevParams)
		update();
}

void String::update() {
	activeModes = 0;
	float f = 0.0f;

	for (size_t idx = 0; idx < MAX_MODES; idx++) {
		int n = idx + 1;
		f = params.pitch * n;

		if (f < minFreq || f > maxFreq) {
			coefs[idx].amplitude = 0.0f;
			continue;
		}
		
		coefs[idx].freq = f;
		coefs[idx].q = 10.0f;

		activeModes++;
	}
}

SvfCoefficients String::getCoefficients(size_t idx) {
	idx = clamp11(idx, 0, MAX_MODES);
	return coefs[idx];
}

int String::getActiveModesScaler() {
	return 1.0f / activeModes;
}




// Drum
Drum::Drum()
{
	update();
}

float Drum::bessel(int order, float x) 
{
	const float EPS = 1e-8;
	const int MAX_TERMS = 20;

	// J0
	if (order == 0)
	{
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do
		{
			term = std::pow(-1.0, k) *
				std::pow(x / 2.0, 2 * k) /
				(factorial(k) * factorial(k));

			sum += term;
			k++;
		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// J1
	if (order == 1)
	{
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) *
				std::pow(x / 2.0, 2 * k + 1) /
				(factorial(k) * factorial(k + 1));

			sum += term;
			k++;

		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// higher orders (recurrence)
	if (x == 0.0)
		return 0.0;

	return (2.0 * (order - 1) / x) * bessel(order - 1, x)
		- bessel(order - 2, x);
}

void Drum::calculateWeights()
{
	for (int i = 0; i < MAX_MODES; i++)
	{
		const Root& r = roots[i];
		float weight = bessel(r.order, r.value * position);
		weights[i] = scale(weight, 0.f, 1.f, overtones, 1, damping);
	}
}

void Drum::calculateFreqs()
{
	for (int i = 0; i < MAX_MODES; i++)
	{
		const Root& r = roots[i];
		freqs[i] = std::max(20.f, std::min((r.value * tuning) / size, samplerate * 0.25f));
	}

	/*
	const float root0 = roots[0].value;
	for (int i = 0; i < MAX_MODES; i++)
	{
		freqs[i] = fundamentalPitch * (roots[i].value / root0);
		freqs[i] = std::max(20.f, std::min(freqs[i], samplerate * 0.25f));
	}
	*/
}

void Drum::update()
{
	calculateWeights();
	calculateFreqs();
}

void Drum::setSamplerate(int newSamplerate)
{
	if (samplerate != newSamplerate)
	{
		samplerate = newSamplerate;
		update();
	}
}

void Drum::setPitch(float newFreq)
{
	newFreq = std::max(20.f, std::min(newFreq, 10000.f));

	if (tuning != newFreq)
	{
		tuning = newFreq;
		update();
	}
}

void Drum::setPosition(float newPosition)
{
	newPosition = std::max(0.f, std::min(newPosition, 1.f));

	if (position != newPosition)
	{
		position = newPosition;
		update();
	}
}

void Drum::setSize(float newSize)
{
	newSize = std::max(0.0001f, std::min(newSize, 6.f));

	if (size != newSize)
	{
		size = newSize;
		update();
	}
	
}

void Drum::setDamping(float newDamping)
{
	newDamping = std::max(0.f, std::min(newDamping, 1.f));

	if (damping != newDamping)
	{
		damping = newDamping;
		update();
	}
}

void Drum::setOvertones(float newOvertones)
{
	newOvertones = std::max(0.f, std::min(newOvertones, 1.f));

	if (overtones != newOvertones)
	{
		overtones = newOvertones;
		update();
	}
}

float Drum::getWeight(int index)
{
	int i = std::max(0, std::min(index, MAX_MODES - 1));
	return weights[i];
}

float Drum::getFreq(int index)
{
	int i = std::max(0, std::min(index, MAX_MODES - 1));
	return freqs[i];
}