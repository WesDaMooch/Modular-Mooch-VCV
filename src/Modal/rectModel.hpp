// Rect Model

// TODO: this whole model

#pragma once
#include <cmath>
#include "common.hpp"
#include "baseModel.hpp"

class RectModel : public BaseModel {
public:
	RectModel();
	void setSamplerate(int samplerate) override;
	void setParams(float pitch, float position) override;

protected:
	void computeFrequencies() override;
	void computeWeights() override;

	int samplerate = 48000;

	float maxFrequency = samplerate * 0.45f;

	float pitch = 220.f;
	float position = 1.f; // Aspect ratio
};