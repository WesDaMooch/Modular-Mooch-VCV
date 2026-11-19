// Free and Cantilever Beam Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class BeamModel : public BaseModel {
public:
	BeamModel();
	void setSamplerate(int samplerate) override;
	void setParams(float pitch, float position) override;
	//void update() override;

protected:
	void computeFrequencies() override;
	void computeWeights() override;

	int samplerate = 48000;

	std::array<float, MAX_MODES> freeBeamRoot = {};
	std::array<float, MAX_MODES> cantBeamRoot = {};

	float maxFrequency = samplerate * 0.45f;

	float pitch = 220.f;
	float position = 0.5f;

	void buildRoots();
};