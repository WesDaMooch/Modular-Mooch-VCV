#pragma once
#include <array>
#include <cmath>
#include <algorithm>

static constexpr int MAX_MODES = 16;

// Circular Membrane 
struct Drum
{
//protected:
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

	std::array<float, MAX_MODES> weights = {};
	std::array<float, MAX_MODES> freqs = {};

	int samplerate = 44100;
	float tuning = 220.f;
	float size = 1.f;
	float position = 0.3f;
	float damping = 0.f;
	float overtones = 0.f;

	Drum();
	float bessel(int order, float x);
	void calculateWeights();
	void calculateFreqs();
	void update();

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

//public:
	void setSamplerate(int newSamplerate);
	void setPitch(float newFreq);
	void setPosition(float newPosition);
	void setSize(float newSize);
	void setDamping(float newDamping);
	void setOvertones(float newOvertones);

	float getWeight(int index);
	float getFreq(int index);
};