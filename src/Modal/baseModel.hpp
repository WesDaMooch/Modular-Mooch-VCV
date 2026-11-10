// baseModel.hpp

#pragma once
#include <array>
#include <cmath>
#include "common.hpp"

static constexpr int NUM_STRUCTURES = 3;

// TODO: this is compute for every model so pass a ref
// or pointer?
class DecayResponse {
public:
	enum class Mode {
		LOWPASS_12DB,
		LOWPASS_24DB
	};


	DecayResponse() {
		set(0.f, 0.f, Mode::LOWPASS_24DB);
	}

	inline void set(float cutoffNormalised, float resonanceNormalised, Mode mode) {
		const float maxCutoffFreq = 6000.f;	// Hz

		cutoffNormalised = mClamp(cutoffNormalised, 0.f, 1.f);
		resonanceNormalised = mClamp(resonanceNormalised, 0.f, 1.f);

		this->cutoff = 20.f * std::pow((maxCutoffFreq) / 20.f, mClamp(cutoffNormalised, 0.f, 1.f));
		this->resonance = std::pow(resonanceNormalised, 4.f) * 5.f;

		this->mode = mode;
	}


	inline float get(float frequency) {

		if (mode == Mode::LOWPASS_24DB) {
			// Butterworth (q = 0.707) second order lowpass response (24dB)
			float ratio = frequency / cutoff;
			float q = 0.7071f + resonance * 9.2929f;
			float denominator = std::sqrt(std::pow(1.f - ratio * ratio, 2.f) + std::pow(ratio / q, 2.f));
			return 1.f / denominator;
		}

		/*
		float order = (slope == FilterMode::LOWPASS_12DB) ? 1.f : 2.f;

		float ratio = frequency / cutoff;

		float response = 1.f / std::sqrt(1.f + std::pow(ratio, 2.f * order));

		return response;
		*/
		return 0.f;
	}

protected:
	Mode mode = Mode::LOWPASS_24DB;
	float cutoff = 0.f;
	float resonance = 0.f;
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

	// TODO: Implement this cloning thing?
	//virtual std::unique_ptr<StructureBase> clone() const = 0;

protected:
	// TODO: tune max and min q
	static constexpr float MIN_Q = 0.00001f;
	static constexpr float MAX_Q = 2000.f;




	// new decay
	constexpr static float MAX_BASE_Q = 1000.f;
	float baseQ = MIN_Q;
	//float cutoff = 20.f;
	//float resonance = 0.f;

	// TODO: this is compute for every model so pass a ref
	// or pointer?
	DecayResponse decayResponse;

	inline void normaliseAmplitude(std::array<SvfCoefficients, MAX_MODES>& c, float energy) {
		// Normalise energy
		float gain = 1.f / std::sqrt(std::max(energy, 1e-12f));

		for (int i = 0; i < MAX_MODES; i++)
			c[i].amplitude *= gain;
	}
};
