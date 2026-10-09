#pragma once
#include <algorithm>

static constexpr int MAX_MODES = 16;
static constexpr int DIAL_RESOLUTION = 512;

struct SvfCoefficients {
	float freq = 220.f;
	float q = 1.f;
	float amplitude = 0.f;
};


template <typename T>
inline T mClamp(T value, T low, T high) {
	return std::max(low, std::min(value, high));
}

// Expand a normalized value into a range
template <typename T>
inline T mMap(T value, T low, T high) {
	return low + value * (high - low);
}

// Compress a range into normalized 0 to 1
template <typename T>
T mRemap(T value, T inMin, T inMax) {
	if (inMin == inMax)
		return value >= inMax ? T(1) : T(0);

	return mClamp((value - inMin) / (inMax - inMin), T(0), T(1));
}

// Compress a range into normalized 1 to 0
template <typename T>
T mRemapInv(T value, T inMin, T inMax) {
	return T(1) - mRemap(value, inMin, inMax);
}

template <typename T>
inline T mInterp(T xFade, T value1, T value2) {
	return (T(1) - xFade) * value1 + xFade * value2;
}

inline void mInterpSvfCoefs(float xfade, SvfCoefficients& output, const SvfCoefficients& coef1, const SvfCoefficients& coef2) {
	output.freq			= mInterp(xfade, coef1.freq, coef2.freq);
	output.q			= mInterp(xfade, coef1.q, coef2.q);
	output.amplitude	= mInterp(xfade, coef1.amplitude, coef2.amplitude);
}