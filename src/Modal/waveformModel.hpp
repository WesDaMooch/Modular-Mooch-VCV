// Simple waveform model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class WaveformModel : public BaseModel {
public:
	WaveformModel();
	void setSamplerate(int samplerate) override;
	void setParams(const StructureParams& params) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;

protected:
	constexpr static int NUM_WAVEFORMS = 2;

	std::array<std::array<float, MAX_MODES>, NUM_WAVEFORMS> waveformAmplitude = {};

	int samplerate = 48000;

	constexpr static float minFreq = 20.f;
	float maxFreq = samplerate * 0.5f;

	float fundamentalFreq = 220.f;
	int waveformIdxA = 0;
	int waveformIdxB = 0;
	float waveformXFade = 0.f;

	float constLoss = 0.f;
	float freqLoss = 0.f;

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildRoots();
};