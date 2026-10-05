#include "biquad.hpp"


Biquad::Biquad() {}


void Biquad::setSampleRate(int sampleRate) {
	sr = sampleRate > 0 ? sampleRate : 1;
}


void Biquad::setLowpass(float cutoffHz, float q) {
    cutoffHz = std::max(20.f, std::min(cutoffHz, sr * 0.49f));

    float w0 = 2.f * M_PI * cutoffHz / sr;
    float cosW = std::cos(w0);
    float sinW = std::sin(w0);

    float alpha = sinW / (2.f * q); 

    float a0 = 1.f + alpha;

    b0 = ((1.f - cosW) * 0.5f) / a0;
    b1 = (1.f - cosW) / a0;
    b2 = ((1.f - cosW) * 0.5f) / a0;

    a1 = (-2.f * cosW) / a0;
    a2 = (1.f - alpha) / a0;
}


float Biquad::process(float input)
{
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 -  a2 * y2;

    x2 = x1;
    x1 = input;

    y2 = y1;
    y1 = output;

    return output;
}