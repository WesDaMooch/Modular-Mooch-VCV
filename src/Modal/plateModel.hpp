// Plate Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"

class Plate : public baseModel {
public:
	Plate();
	void setSamplerate(int newSamplerate) override;
	void setParams(const StructureParams& newParams) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	int samplerate = 48000;

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildTables();
};