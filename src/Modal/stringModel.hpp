// String Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class StringModel : public BaseModel {
public:
	StringModel();
	void setSamplerate(int samplerate) override;
	void setParams(float pitch, float position) override;

protected:
	void computeFrequencies() override;
	void computeWeights() override;

	int samplerate = 48000;

	float maxFrequency = samplerate * 0.45f;

	float pitch = 220.f;
	float stiffness = 0.f;
	float position = 0.5f;
};