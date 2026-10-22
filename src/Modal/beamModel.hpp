// Free and Cantilever Beam Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class BeamModel : public BaseModel {
public:
	BeamModel();
	void setSamplerate(int samplerate) override;
	void setParams(const StructureParams& params) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	int samplerate = 48000;

	std::array<float, MAX_MODES> freeBeamRoot = {};
	std::array<float, MAX_MODES> cantBeamRoot = {};

	constexpr static float minFreq = 20.f;
	float maxFreq = samplerate * 0.5f;

	float fundamentalFreq = 220.f;
	//float position = 0.5f;
	float constLoss = 0.f;
	float freqLoss = 0.f;

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildRoots();
};