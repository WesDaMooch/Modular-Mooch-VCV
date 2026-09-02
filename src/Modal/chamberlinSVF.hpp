#pragma once
#include "common.hpp"
#include <cmath>


// Chamberlin's State Variable Digital Filter
// from Musical Applications of Microprocessors

// Updated version
// Improving the Chamberlin Digital State Variable Filter 
// Victor Lazzarini and Joseph Timoney

// TODO: try out this biquad: https://ccrma.stanford.edu/~jos/pasp/Modal_Expansion.html

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

	float amplitude = 1.0f;

	int sr = 48000;

public:
	void setSamplerate(int newSamplerate);
	void setCoefficients(SvfCoefficients& c);
	void process(float x);
	void reset();
	float highpass();
	float lowpass();
	float bandpass();
};