// Biquad filter
// 2nd order with Butterworth lowpass


#pragma once
#include <algorithm> 
#include <cmath>     


class Biquad
{
public:
	Biquad();
	void setSampleRate(int sampleRate);
	void setLowpass(float cutoffHz, float q = BUTTERWORTH_LOWPASS_Q);
	float process(float input);

    static constexpr float BUTTERWORTH_LOWPASS_Q = 0.70710678f;

protected:
	int sr = 48000;

    float b0 = 1.f;
    float b1 = 0.f;
    float b2 = 0.f;
    float a1 = 0.f;
    float a2 = 0.f;

    float x1 = 0.f;
    float x2 = 0.f;
    float y1 = 0.f;
    float y2 = 0.f;
};