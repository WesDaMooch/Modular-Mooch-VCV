#pragma once
#include <algorithm>

static constexpr int MAX_MODES = 6;

struct SvfCoefficients {
	float freq = 220.0f;
	float q = 1.0f;
	float amplitude = 1.0f;
};

template <typename T>
inline T mClamp(T value, T low, T high) {
	return std::max(low, std::min(value, high));
}

template <typename T>
inline T mMap(T value, T low, T high) {
	return low + value * (high - low);
}
