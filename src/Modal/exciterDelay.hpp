#include <array>
#include <cmath>
#include "common.hpp"

// TODO: random / granular delays

class ExciterDelay
{
public:
	ExciterDelay();
	void buildLuts();
	float getNorm(int modeIdx, float typeParam);

protected:
	// Delay states
	static constexpr int DIAL_STATES = 5;

	std::array<float, MAX_MODES> modeDelayDecreasingLinearLut = {};
	std::array<float, MAX_MODES> modeDelayDecreasingExpLut = {};
	std::array<float, MAX_MODES> modeDelayRandomGrainLut = {};
	std::array<float, MAX_MODES> modeDelayIncreasingExpLut = {};
	std::array<float, MAX_MODES> modeDelayIncreasingLinearLut = {};
};