#include "resonator.hpp"

void IIRResonator::set(float fs, float frequency, float T60, float amplitude)
{
	float omega = 2.0 * M_PI * frequency / fs;

	float decay = std::exp(-6.91 / (T60 * fs));

	a1 = -2.0 * decay * std::cos(omega);
	a2 = decay * decay;

	b0 = amplitude * (1.0 - decay); 
}

void IIRResonator::reset()
{
	y1 = 0.0;
	y2 = 0.0;
}

float IIRResonator::process(float x)
{
	double y = b0 * x - a1 * y1 - a2 * y2;

	y2 = y1;
	y1 = y;

	return y;
}