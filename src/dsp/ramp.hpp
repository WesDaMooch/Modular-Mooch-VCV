// Ramp
// Header-only

#pragma once
#include <algorithm>


class Ramp
{
public:
	void setTarget(float startValue, float endValue, int rampTimeSamps) {
		this->startValue = startValue;
		this->endValue = endValue;
		this->rampTimeSamps = std::max(rampTimeSamps, 0);

		currentSample = 0;

		if (this->rampTimeSamps == 0)
			currentValue = endValue;
		else
			currentValue = startValue;
	}


	void setCurrentValue(float currentValue) {
		this->currentValue = currentValue;
	}


	float process() {
		if (currentSample >= rampTimeSamps)
			return endValue;

		currentSample++;

		const float t = static_cast<float>(currentSample) / static_cast<float>(rampTimeSamps);

		currentValue = startValue + (endValue - startValue) * t;

		return currentValue;
	}


	bool isFinished() const {
		return currentSample >= rampTimeSamps;
	}


	void reset() {
		currentSample = 0;
		currentValue = startValue;
	}


	float getValue() const {
		return currentValue;
	}


protected:
	float startValue = 0.f;
	float endValue = 0.f;
	float currentValue = 0.f;

	int rampTimeSamps = 0;
	int currentSample = 0;
};