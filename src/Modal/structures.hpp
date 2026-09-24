#pragma once
#include "common.hpp"
#include <array>
#include <cmath>

// TODO: clamp freq range

struct StructureParams {
	float fundamentalFreq = 220.0f;
	float morph = 0.5f;
	float position = 0.5f;
	float decay = 0.5f;
	float timbre = 0.5f;

	bool operator==(const StructureParams& other) const {
		return	fundamentalFreq	== other.fundamentalFreq && 
			morph				== other.morph &&
			position			== other.position &&
			decay				== other.decay &&
			timbre				== other.timbre;
	}

	bool operator!=(const StructureParams& other) const {
		return !(*this == other);
	}
};


// String 
class String
{
public:
	String();
	void setSamplerate(int newSamplerate);
	void setParams(StructureParams& newParams);
	void update();
	float getActiveModesScaler() const; 
	SvfCoefficients getCoefficients(int idx);

protected:
	int sr = 48000;
	int activeModes = 0;

	constexpr static float minFreq = 20.0f;
	float maxFreq = sr * 0.5f;

	float fundamentalFreq = 220.0f;
	float inharmonicity = 0.f;
	float position = 0.0001f;
	float baseDecay = 0.5f;
	float timbre = 0.f;

	int decaySlopeIdx = 0;
	std::array<float, DIAL_RESOLUTION> decaySlopeLut = {};

	int amplitudeDialIdx = 0;
	//std::array<float, DIAL_RESOLUTION> amplitudeSlopeLut = {};
	//std::array<float, DIAL_RESOLUTION> amplitudeDialLut = {};
	std::array<std::array<float, DIAL_RESOLUTION>, MAX_MODES> amplitudeDialLut = {};

	std::array<SvfCoefficients, MAX_MODES> coefs;
	StructureParams prevParams;

	void buildLuts();
};







class Drum2
{
public:
	Drum2();
	void setSamplerate(int newSamplerate);
	void setParams(StructureParams& newParams);
	void update();
	float getActiveModesScaler() const;
	SvfCoefficients getCoefficients(int idx);
	
protected:
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


	int sr = 48000;
	int activeModes = 0;

	constexpr static float minFreq = 20.0f;
	float maxFreq = sr * 0.25f;

	std::array<SvfCoefficients, MAX_MODES> coefs;
	StructureParams prevParams;

	float tuning = 220.f;  //pitch
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

	int samplerate = 48000;
	float tuning = 220.f;  //pitch
 	float size = 1.f;   // pitch + morph
	float position = 0.3f; // position
	float damping = 0.f; // decay
	float overtones = 0.f; // timbre / brightness

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