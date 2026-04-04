// 2nd-order resonant IIR filter

#pragma once
#include <cmath>

class IIRResonator
{
protected:
	float b0 = 0.0;
	float a1 = 0.0, a2 = 0.0;

	float y1 = 0.0, y2 = 0.0;

public:
	// void setSamplerate();
	void set(float fs, float frequency, float T60, float amplitude);
	void reset();
	float proccess(float x);
};