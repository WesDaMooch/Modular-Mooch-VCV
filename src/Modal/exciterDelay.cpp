#include "exciterDelay.hpp"


ExciterDelay::ExciterDelay() {
	buildLuts();
}

void ExciterDelay::buildLuts() {
	static constexpr float stateTwoCurve = 200.0f;

	for (int i = 0; i < MAX_MODES; i++) {
		float x = static_cast<float>(i) / (MAX_MODES - 1);

		// State 0 - Decreasing linear
		modeDelayDecreasingLinearLut[i] = x;

		// State 1 - Decreasing exponential
		modeDelayDecreasingExpLut[i] = (std::pow(stateTwoCurve, x) - 1.0f) / (stateTwoCurve - 1.0f);

		// TODO: State 2 - Random grain
		modeDelayRandomGrainLut[i] = 0.0f;

		float reverseX = (x - 1.0f) * -1.0f;
		// State 3 - Increasing exponential
		modeDelayIncreasingExpLut[i] = (std::pow(stateTwoCurve, reverseX) - 1.0f) / (stateTwoCurve - 1.0f);

		// State 4 - Increasing linear
		modeDelayIncreasingLinearLut[i] = reverseX;
	}
}

float ExciterDelay::getNorm(int modeIdx, float typeParam) {

	modeIdx = mClamp(modeIdx, 0, MAX_MODES);
	typeParam = mClamp(typeParam, 0.0f, 1.0f);

	float state = typeParam * (DIAL_STATES - 1);

	if (state <= 1.0f) {
		// Index by Mode (Linear)
		return mInterp(state, modeDelayDecreasingLinearLut[modeIdx], modeDelayDecreasingExpLut[modeIdx]);
	}
	else if (state > 1.0f && state <= 2.0f) {
		// State 1 - Index by Mode (Exponential)
		float xFade = state - 1.0f;
		return mInterp(xFade, modeDelayDecreasingExpLut[modeIdx], modeDelayRandomGrainLut[modeIdx]);
	}
	else if (state > 2.0f && state <= 3.0f) {
		float xFade = state - 2.0f;
		return mInterp(xFade, modeDelayRandomGrainLut[modeIdx], modeDelayIncreasingExpLut[modeIdx]);
	}
	else if (state > 3.0f && state <= 4.0f) {
		float xFade = state - 3.0f;
		return mInterp(xFade, modeDelayIncreasingExpLut[modeIdx], modeDelayIncreasingLinearLut[modeIdx]);
	}

	return 0.0f;
}
