#include "chamberlinSVF.hpp"


void ChamberlinSVF::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
}


void ChamberlinSVF::setCoefficients(SvfCoefficients& c)
{
	float freq = std::max(20.f, std::min(c.freq, sr * 0.45f));
	K = std::tan(M_PI * freq / sr);

	Q = std::max(c.q, 1e-6f);

	amplitude = clamp11(c.amplitude, 0.0f, 1.0f);
}


void ChamberlinSVF::process(float x)
{
	float kdiv = 1.f + (K / Q) + (K * K);

	hp = (x - ((1.f / Q + K) * s1) - s2) / kdiv;

	float u = hp * K;
	bp = u + s1;
	s1 = u + bp;

	u = bp * K;
	lp = u + s2;
	s2 = u + lp;
}


void ChamberlinSVF::reset()
{
	s1 = 0.f;
	s2 = 0.f;
}


float ChamberlinSVF::highpass()
{
	return hp * amplitude;
}


float ChamberlinSVF::lowpass()
{
	return hp * amplitude;
}


float ChamberlinSVF::bandpass()
{
	return hp * amplitude;
}
