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


//////////////

void ChamberlinSVF::reset()
{
	l = 0.f;
	h = 0.f;
	b = 0.f;
	n = 0.f;

	d1 = 0.f;
	d2 = 0.f;
}

void ChamberlinSVF::setCoefficients(float f, float q, int fs)
{
	f = std::max(20.f, std::min(f, fs * 0.45f));
	f1 = 2.f * std::sin(M_PI * f / fs);
	f1 = std::min(f1, 1.f);

	// q range 0 to 1
	//1e-6f
	q *= 250;
	q = std::max(1e-6f, q);
	q = 1 / q;
	q1 = std::max(0.f, std::min(q, 2.f));

	//float Q = 0.5f + q * 200.0f;
	//q1 = 1.0f / Q;
}

void ChamberlinSVF::process(float x)
{ 
	l = d2 + f1 * d1;
	h = x - l - q1 * d1;
	b = f1 * h + d1;
	n = h + l;

	d1 = b;
	d2 = l;
}

float ChamberlinSVF::highpass()
{
	return h;
}

float ChamberlinSVF::bandpass()
{
	return b;
}

float ChamberlinSVF::notch()
{
	return n;
}

float ChamberlinSVF::lowpass()
{
	return l;
}