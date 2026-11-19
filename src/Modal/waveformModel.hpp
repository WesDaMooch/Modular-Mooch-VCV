// Simple waveform model


#pragma once
#include "common.hpp"
#include "baseModel.hpp"


class WaveformModel : public BaseModel {
public:
	WaveformModel();
	void setSamplerate(int samplerate) override;
	void setParams(float pitch, float position) override;
	//void update() override;

protected:
	void computeFrequencies() override;
	void computeWeights() override;

	constexpr static int NUM_WAVEFORMS = 4; // Not used
	std::array<std::array<float, MAX_MODES>, NUM_WAVEFORMS> waveformAmplitude = {}; // Not used

	float position = 0.5f;

	int samplerate = 48000;

	float maxFrequency = samplerate * 0.45f;

	float pitch = 220.f;
	int waveformIdxA = 0; // Not used
	int waveformIdxB = 0; // Not used
	float waveformXFade = 0.f; // Not used


	//void buildRoots();
	//void updateSquarePWM(float duty);
	//void updateSawPWM(float duty);
};