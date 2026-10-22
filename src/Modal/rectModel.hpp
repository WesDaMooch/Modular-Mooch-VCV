// Rect Model

// TODO: this whole model

#pragma once
#include <cmath>
#include "common.hpp"
#include "baseModel.hpp"

class RectModel : public BaseModel {
public:
	RectModel();
	void setSamplerate(int newSamplerate) override;
	void setParams(const StructureParams& newParams) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	int samplerate = 48000;

	constexpr static float minFreq = 20.f;
	float maxFreq = samplerate * 0.5f;

	float fundamentalFreq = 220.f;
	float aspectRatio = 1.f;
	float baseDecay = 0.5f;

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildTables();
};