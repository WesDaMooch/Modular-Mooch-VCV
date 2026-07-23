#include "fxChain.hpp"


// Gain
void Gain::set(float v) {
	gainAmount = v;
}

void Gain::reset() {
	gainAmount = 0.5f;
}

void Gain::process(float& x) {
	x *= gainAmount;
}

// Slew
Slew::Slew() {
	slewAmount = msToSlew(minSlewMs);
}

void Slew::set(float v) {
	float slewSkew = v * v;
	float slewMs = std::max(slewSkew * maxSlewMs, minSlewMs);
	slewAmount = msToSlew(slewMs);

	prevValue = v;
}

void Slew::setSamplerate(int samplerate) {
	sr = samplerate;
	set(prevValue);
}

void Slew::setAudioRateMode(bool audioRate) {
	audioRateMode = audioRate;
	set(prevValue);
}

void Slew::reset() {
	slewAmount = msToSlew(minSlewMs);
	prevValue = 0.0f;
	y = 0.0f;
}

void Slew::process(float& x) {
	y += std::min(std::max(x - y, -slewAmount), slewAmount);
	x = y;
}


// FX chain
void FxChain::reset() {
	slew.reset();
	gain.reset();
}

void FxChain::setSamplerate(int samplerate) {
	int sr = std::max(samplerate, 1);
	slew.setSamplerate(sr);
}

void FxChain::setAudioRateMode(bool audioRate) {
	audioRateMode = audioRate;
	slew.setAudioRateMode(audioRate);
}

void FxChain::setFxValue(float v, FX fx) {
	v = std::min(std::max(v, 0.0f), 1.0f);

	if (std::abs(v - prevValue) > valueThreshold) {
		switch (fx) {
		case FX::Gain:
			gain.set(v);
			break;

		case FX::Slew:
			slew.set(v);
			break;

		default:
			break;
		}

		prevValue = v;
	}
}

void FxChain::process(float& x) {
	// Clamp input
	x = std::min(std::max(x, -1.0f), 1.0f);

	float out = audioRateMode ? (x - 0.5f) : x;

	slew.process(out);
	gain.process(out);
	// DC Filter audio
	x = out;
}