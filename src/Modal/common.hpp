#pragma once
#include <algorithm>


static constexpr int MAX_MODES = 16;


struct SvfCoefficients {
	float freq = 220.0f;
	float q = 1.0f;
	float amplitude = 1.0f;
};

template <typename T>
inline T clamp11(T value, T low, T high) {
	return std::max(low, std::min(value, high));
}

/*
inline int clamp11(int value, int low, int high) {
	return std::max(low, std::min(value, high));
}
*/
