#pragma once
#include <cmath>
#include <algorithm>

// Chamberlin's State Variable Digital Filter
// from Musical Applications of Microprocessors

// Updated version from
// Improving the Chamberlin Digital State Variable Filter 
// Victor Lazzarini and Joseph Timoney
class ChamberlinSVF
{
protected:
	float K = 1.f;
	float Q = 1.f;

	float s1 = 0.f;
	float s2 = 0.f;

	float hp = 0.f;
	float bp = 0.f;
	float lp = 0.f;
	//float n = 0.f;
	//float p = 0.f;

public:
	void reset();
	void setCoefficients(float f, float q, int fs);
	void process(float x);
	float highpass();
	float lowpass();
	float bandpass();
};