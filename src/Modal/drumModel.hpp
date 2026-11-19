// Drum Model - Circular Membrane 

#pragma once
#include "baseModel.hpp"

class DrumModel : public BaseModel {
public:
	DrumModel();
	void setSamplerate(int samplerate) override;
	void setParams(float pitch, float position) override;

protected:
	void computeFrequencies() override;
	void computeWeights() override;

	struct Root
	{
		int order;
		float value;
	};

	std::array<Root, 16> roots = { {
		{0, 2.4048f}, {0, 5.5201f}, {0, 8.6537f},  {0, 11.7915f},
		{1, 3.8317f}, {1, 7.0156f}, {1, 10.1735f}, {1, 13.3237f},
		{2, 5.1356f}, {2, 8.4172f}, {2, 11.6198f}, {2, 14.7959f},
		{3, 6.3802f}, {3, 9.761f},  {3, 13.0152f}, {3, 16.2235f}
	} };


	int samplerate = 48000;

	float maxFrequency = samplerate * 0.25f;

	float pitch = 220.f;  //pitch
	float size = 1.f;   // pitch + morph
	float position = 0.3f; // position
	float damping = 0.f; // decay
	float overtones = 0.f; // timbre / brightness

	float bessel(int order, float x);

	inline float factorial(int order) {
		if (order <= 1)
			return 1.0;
		return order * factorial(order - 1);
	}

	inline float scale(float x, float inLow, float inHigh, float outLow, float outHigh, float expr) {
		float normalized = (x - inLow) / (inHigh - inLow);

		if (normalized == 0)
		{
			return outLow;
		}
		else if (normalized > 0)
		{
			return outLow + (outHigh - outLow) * std::pow(normalized, expr);
		}
		else
		{
			return outLow + (outHigh - outLow) * -std::pow(-normalized, expr);
		}
	}
};