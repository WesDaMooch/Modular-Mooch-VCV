// String Model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class String : public StructureBase {
public:
	String();
	void setSamplerate(int newSamplerate) override;
	void setParams(const StructureParams& newParams) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	int sr = 48000;

	constexpr static float minFreq = 20.f;
	float maxFreq = sr * 0.5f;

	float fundamentalFreq = 220.f;
	float inharmonicity = 0.f;
	float detune = 0.f;
	float position = 0.0001f;
	float baseDecay = 0.5f;

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