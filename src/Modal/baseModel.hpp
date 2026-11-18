// baseModel.hpp

#pragma once
#include <array>
#include <cmath>
#include "common.hpp"

static constexpr int NUM_STRUCTURES = 3;

class DecayResponse {
public:
	enum class Type {
		LOWPASS_SECOND_ORDER,
		BANDPASS_SECOND_ORDER,
		HIGHPASS_SECOND_ORDER,
		RAMP_DESCENDING,
		NUM_TYPES
	};


	DecayResponse() {
		set(0.f, 0.f, Type::LOWPASS_SECOND_ORDER, latestFrequenies);
	}

	inline void set(float cutoff, float resonance, Type type,
		std::array<float, MAX_MODES>& frequencies) {
		bool update = false;

		cutoff = mClamp(cutoff, 0.f, 1.f);
		resonance = mClamp(resonance, 0.f, 1.f);
		
		if (cutoff != this->cutoffParam || resonance != this->resonanceParam || type != this->type) {
			this->cutoffParam = cutoff;
			this->resonanceParam = resonance;
			this->type = type;

			update = true;	
		}

		if (!std::equal(frequencies.begin(), frequencies.end(), latestFrequenies.begin())) {
			latestFrequenies = frequencies;
			update = true;
		}

		if (update)
			compute(latestFrequenies);
	}

	inline float get(int modeIdx) const{
		modeIdx = mClamp(modeIdx, 0, MAX_MODES - 1);
		return qArray[modeIdx];
	}

protected:

	inline void compute(std::array<float, MAX_MODES>& frequency) {

		float cutoff = 0.f;
		float resonance = 0.f;

		if (type == Type::RAMP_DESCENDING) {
			cutoff = mMap(cutoffParam, 0.0001f, static_cast<float>(MAX_MODES));
			resonance = 0.f;
		}
		else {
			const float maxCutoffFreq = 6000.f;	// Hz
			cutoff = 20.f * std::pow((maxCutoffFreq) / 20.f, mClamp(cutoffParam, 0.f, 1.f));
			resonance = std::pow(resonanceParam, 4.f);
		}

		float q = 1.f;

		switch (type) {
		case Type::LOWPASS_SECOND_ORDER:
			q = mMap(resonance, 0.7071f, 50.f);
			break;

		case Type::BANDPASS_SECOND_ORDER:
			q = mMap(resonance, 0.5f, 30.f);
			break;

		case Type::HIGHPASS_SECOND_ORDER:
			q = mMap(resonance, 0.7071f, 50.f);
			break;

		case Type::RAMP_DESCENDING:
		case Type::NUM_TYPES:
			break;
		}

		for (int i = 0; i < MAX_MODES; i++) {

			if (type == Type::RAMP_DESCENDING) {
				qArray[i] = 0.f;
				continue;
			}

			float ratio = frequencies[i] / cutoff;

			float a = 1.f - ratio * ratio;
			float b = ratio / q;

			float denominator = std::sqrt(a * a + b * b);

			switch (type) {
			case Type::LOWPASS_SECOND_ORDER:
				qArray[i] = 1.f / denominator;
				break;

			case Type::BANDPASS_SECOND_ORDER:
				qArray[i] = b / denominator;
				break;

			case Type::HIGHPASS_SECOND_ORDER:
				qArray[i] = ratio * ratio / denominator;
				break;

			default:
				qArray[i] = 0.f;
				break;
			}
		}

		/*
		for (int i = 0; i < MAX_MODES; i++) {
			if (type == Type::LOWPASS_SECOND_ORDER) {
				// Butterworth (q = 0.707) second order lowpass response
				const float maxQ = 50.f;
				float ratio = frequency[i] / cutoff;
				float q = mMap(resonance, 0.7071f, maxQ);
				float denominator = std::sqrt(std::pow(1.f - ratio * ratio, 2.f) + std::pow(ratio / q, 2.f));
				qArray[i] = 1.f / denominator;
			}

			else if (type == Type::BANDPASS_SECOND_ORDER) {
				// Second order bandpass response
				const float maxQ = 30.f;
				float ratio = frequency[i] / cutoff;
				float q = mMap(resonance, 0.5f, maxQ);
				float denominator = std::sqrt(std::pow(1.f - ratio * ratio, 2.f) + std::pow(ratio / q, 2.f));
				qArray[i] = (ratio / q) / denominator;
			}

			else if (type == Type::HIGHPASS_SECOND_ORDER) {
				// Second order highpass response
				const float maxQ = 50.f;
				float ratio = frequency[i] / cutoff;
				float q = mMap(resonance, 0.7071f, maxQ);
				float denominator = std::sqrt(std::pow(1.f - ratio * ratio, 2.f) + std::pow(ratio / q, 2.f));
				qArray[i] = (ratio * ratio) / denominator;
			}

			else if (type == Type::RAMP_DESCENDING) {
				qArray[i] = 0.f;
			}
		}
		*/
	}

	Type type = Type::LOWPASS_SECOND_ORDER;
	float cutoffParam = 0.f;
	float resonanceParam = 0.f;

	std::array<float, MAX_MODES> qArray{};
	std::array<float, MAX_MODES> latestFrequenies{};
};



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

// TODO: rename BaseModel
class BaseModel {
public:
	virtual ~BaseModel() = default;  

	virtual void setSamplerate(int samplerate) = 0;
	virtual void setParams(const StructureParams& newParams) = 0;
	virtual void update() = 0;
	const virtual SvfCoefficients& getCoefficients(int idx) const = 0;
	const virtual std::array<SvfCoefficients, MAX_MODES>& getAllCoefs() const = 0;
	
	// const virtual std::array<float, MAX_MODES>& getFrequenies() const = 0
	// const virtual std::array<float, MAX_MODES>& getWeights() const = 0

	// TODO: Implement this cloning thing?
	//virtual std::unique_ptr<StructureBase> clone() const = 0;

protected:
	std::array<float, MAX_MODES> frequencies{};
	std::array<float, MAX_MODES> weights{};

	// TODO: tune max and min q
	static constexpr float MIN_Q = 0.00001f;
	static constexpr float MAX_Q = 2000.f;

	// TODO: move to decayResponse
	constexpr static float MAX_BASE_Q = 1000.f;
	float baseQ = MIN_Q;

	inline void normaliseAmplitude(std::array<SvfCoefficients, MAX_MODES>& c, float energy) {
		// Normalise energy
		float gain = 1.f / std::sqrt(std::max(energy, 1e-12f));

		for (int i = 0; i < MAX_MODES; i++)
			c[i].amplitude *= gain;
	}
};
