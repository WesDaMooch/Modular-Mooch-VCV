#include "chamberlinSVF.hpp"

void ChamberlinSVF::reset()
{
	s1 = 0.f;
	s2 = 0.f;
}

void ChamberlinSVF::setCoefficients(float f, float q, int fs)
{
	f = std::max(20.f, std::min(f, fs / 2.f));
	K = std::tan(M_PI * f / fs);

	Q = std::max(q, 1e-6f);
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

	//n = hp + lp;
	//p = hp + lp + (1.f / Q) * bp;

}

float ChamberlinSVF::highpass()
{
	return hp;
}

float ChamberlinSVF::lowpass()
{
	return lp;
}

float ChamberlinSVF::bandpass()
{
	return bp;
}
