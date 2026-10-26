// String Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class StringModel : public BaseModel {
public:
	StringModel();
	void setSamplerate(int samplerate) override;
	void setParams(const StructureParams& params) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	int samplerate = 48000;

	constexpr static float minFreq = 20.f;
	float maxFreq = samplerate * 0.5f;

	float fundamentalFreq = 220.f;
	float stiffness = 0.f;
	float position = 0.5f;
	float constLoss = 0.f;
	float freqLoss = 0.f;

	// TODO: Not used //
	int decayDialIdxA = 0;
	int decayDialIdxB = 0;
	float decayXFade = 0.f;
	std::array<float, DIAL_RESOLUTION> decayDialTable = {};

	int amplitudeDialIdxA = 0;
	int amplitudeDialIdxB = 0;
	float amplitudeXFade = 0.f;
	std::array<std::array<float, DIAL_RESOLUTION>, MAX_MODES> amplitudeDialTable = {};

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildTables();
};