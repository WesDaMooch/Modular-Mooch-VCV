#pragma once
#include <algorithm>


static constexpr int MAX_MODES = 16;
static constexpr int DIAL_RESOLUTION = 512;


struct SvfCoefficients {
	float freq = 220.0f;
	float q = 1.0f;
	float amplitude = 1.0f;
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

