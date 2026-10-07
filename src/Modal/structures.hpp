#pragma once
#include "common.hpp"
#include <array>
#include <cmath>


static constexpr int NUM_STRUCTURES = 2;

// TODO: clamp freq range

struct StructureParams {
	float fundamentalFreq = 220.0f;
	float morph = 0.f;
	float position = 0.f;
	float decay = 0.f;
	float timbre = 0.f;

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


class StructureBase {
public:
	virtual ~StructureBase() = default;  

	virtual void setSamplerate(int newSamplerate) = 0;
	virtual void setParams(const StructureParams& newParams) = 0;
	virtual void update() = 0;
	const virtual SvfCoefficients& getCoefficients(int idx) const = 0;

	// TODO: Implement this cloning thing?
	//virtual std::unique_ptr<StructureBase> clone() const = 0;
};


// String 
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







class Drum2 : public StructureBase {
public:
	Drum2();
	void setSamplerate(int newSamplerate) override;
	void setParams(const StructureParams& newParams) override;
	void update() override;
	const SvfCoefficients& getCoefficients(int idx) const override;
	
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

	constexpr static float minFreq = 20.f;
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