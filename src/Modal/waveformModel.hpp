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


	constexpr static int NUM_WAVEFORMS = 4;
	std::array<std::array<float, MAX_MODES>, NUM_WAVEFORMS> waveformAmplitude = {};

	float duty = 0.5f;

	// TODO: rename sampleRate
	int samplerate = 48000;

	constexpr static float minFreq = 20.f;
	float maxFreq = samplerate * 0.5f;

	float fundamentalFreq = 220.f;
	int waveformIdxA = 0;
	int waveformIdxB = 0;
	float waveformXFade = 0.f;

	// Old 
	float constLoss = 0.f;
	float freqLoss = 0.f;

	// new decay
	float baseQ = MIN_Q;
	float cutoff = 20.f;

	std::array<SvfCoefficients, MAX_MODES> coefs = {};
	StructureParams prevParams;

	void buildRoots();
	void updateSquarePWM(float duty);
	void updateSawPWM(float duty);


	enum class FilterSlope {
		SLOPE_12DB,
		SLOPE_24DB
	};

	// Buttworth 
	inline float lowpassResponse(float frequency, float cutoff, FilterSlope slope) {
		float order = (slope == FilterSlope::SLOPE_12DB) ? 1.f : 2.f;

		float ratio = frequency / cutoff;

		float response = 1.f / std::sqrt(
			1.f + std::pow(ratio, 2.f * order)
		);

		return response;
	}
};