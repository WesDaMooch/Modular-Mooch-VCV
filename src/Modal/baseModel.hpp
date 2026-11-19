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
		NUM_TYPES
	};


	DecayResponse() {
		// Init 
		const float f = 220.f;
		for (int i = 1; i < MAX_MODES; i++)
			latestFrequenies[i] = f * (i + 1); 

		set(cutoffParam, resonanceParam, Type::LOWPASS_SECOND_ORDER, latestFrequenies);
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

		// TODO: check if type uses freqs
		// only update if it does
		if (!std::equal(frequencies.begin(), frequencies.end(), latestFrequenies.begin())) {
			latestFrequenies = frequencies;
			update = true;
		}

		if (update)
			compute(latestFrequenies);
	}

	inline float get(int modeIdx) const{
		modeIdx = mClamp(modeIdx, 0, MAX_MODES - 1);
		return q[modeIdx];
	}

protected:

	inline void compute(std::array<float, MAX_MODES>& frequencies) {

		float cutoff = 0.f;
		float resonance = 0.f;

		const float maxCutoffFreq = 6000.f;	// Hz
		cutoff = 20.f * std::pow((maxCutoffFreq) / 20.f, mClamp(cutoffParam, 0.f, 1.f));
		resonance = std::pow(resonanceParam, 4.f);
		

		float filterQ = 1.f;

		switch (type) {
		case Type::LOWPASS_SECOND_ORDER:
			filterQ = mMap(resonance, 0.7071f, 50.f);
			break;

		case Type::BANDPASS_SECOND_ORDER:
			filterQ = mMap(resonance, 0.5f, 10.f);
			break;

		case Type::HIGHPASS_SECOND_ORDER:
			filterQ = mMap(resonance, 0.7071f, 50.f);
			break;

		case Type::NUM_TYPES:
			break;
		}

		for (int i = 0; i < MAX_MODES; i++) {

			// TODO: if weights[i] == 0 continue

			float ratio = frequencies[i] / cutoff;

			float a = 1.f - ratio * ratio;
			float b = ratio / filterQ;

			float denominator = std::sqrt(a * a + b * b);

			switch (type) {
			case Type::LOWPASS_SECOND_ORDER:
				q[i] = 1.f / denominator;
				break;

			case Type::BANDPASS_SECOND_ORDER:
				q[i] = b / denominator;
				break;

			case Type::HIGHPASS_SECOND_ORDER:
				q[i] = ratio * ratio / denominator;
				break;

			default:
				q[i] = 0.f;
				break;
			}
		}
	}

	Type type = Type::LOWPASS_SECOND_ORDER;
	float cutoffParam = 0.f;
	float resonanceParam = 0.f;

	std::array<float, MAX_MODES> q{};
	std::array<float, MAX_MODES> latestFrequenies{};
};


/*
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
*/

class BaseModel {
public:
	virtual ~BaseModel() = default;  

	virtual void setSamplerate(int samplerate) = 0;
	//virtual void setParams(const StructureParams& newParams) = 0;
	virtual void setParams(float pitch, float position) = 0;

	//virtual void update() = 0;
	
	inline const std::array<float, MAX_MODES>& getFrequencies() const {
		return frequencies;
	}

	inline const std::array<float, MAX_MODES>& getWeights() const {
		return weights;
	}

protected:
	virtual void computeFrequencies() = 0;
	virtual void computeWeights() = 0;

	std::array<float, MAX_MODES> frequencies{};
	std::array<float, MAX_MODES> weights{};

	inline void normaliseWeights(std::array<float, MAX_MODES>& w, float e) {
		float gain = 1.f / std::sqrt(std::max(e, 1e-12f));

		for (int i = 0; i < MAX_MODES; i++)
			w[i] *= gain;
	}
};
