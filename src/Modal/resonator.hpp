// 2nd-order resonant IIR filter

#pragma once
#include <cmath>
#include <algorithm>

class IIRResonator
{
protected:
	float b0 = 0.0;
	float a1 = 0.0, a2 = 0.0;

	float y1 = 0.0, y2 = 0.0;

public:
	// void setSamplerate();
	// Phase offest?
	void set(float fs, float frequency, float T60, float amplitude);
	void reset();
	float process(float x);
};


// in hal chamberlin book
// chamberlin svf
class ChamberlinSVF
{
protected:
	float f1 = 0.f;
	float q1 = 0.f;
	
	float d1 = 0.f;
	float d2 = 0.f;

	float h = 0.f;
	float b = 0.f;
	float n = 0.f;
	float l = 0.f;

public:
	void reset();
	void setCoefficients(float f, float q, int fs);
	void process(float x);
	float highpass();
	float bandpass();
	float notch();
	float lowpass();
};