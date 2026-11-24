
#pragma once
#include <array>
#include <cmath>
#include "common.hpp"


static constexpr int NUM_STRUCTURES = 2;

// TODO: move to seperate file
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

	inline float get(int modeIdx) const {
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




class BaseModel {
public:
    virtual ~BaseModel() = default;

    virtual void setSamplerate(int samplerate) = 0;
    virtual float frequency(int mode, float pitch) const = 0;
    virtual float weight(int mode, float frequency, float position) const = 0;
};


class StringModel : public BaseModel {
public:
    void setSamplerate(int samplerate) override {
        maxFrequency = std::max(samplerate, 1) * 0.45f;
    }

    float frequency(int mode, float pitch) const override {
        const float f = mClamp(pitch, 20.f, maxFrequency) * float(mode + 1);
        return f > maxFrequency ? 0.f : f;
    }

    float weight(int mode, float frequency,float position) const override {
        const float p = mMap(position, 0.02f, 0.98f);
        return sinf(float(M_PI) * float(mode + 1) * p);
    }

private:
    float maxFrequency = 20000.f;
};


class BeamModel : public BaseModel {
public:
    BeamModel() {
        // From Natan Ho - Exploring Modal Synthesis
        // Normalised frequency ratio (fk) proportional to f0

        //const float freeFundamentalRoot = 1.50561873f;
        const float cantFundamentalRoot = 0.59686416f;

        for (int i = 0; i < MAX_MODES; i++) {
            //float freeRoot = float(i) + 1.5f;
            float cantRoot = float(i) + 0.5f;

            //freeBeamRoot[i] = std::pow(freeRoot / freeFundamentalRoot, 2.f);
            cantBeamRoot[i] = std::pow(cantRoot / cantFundamentalRoot, 2.f);
        }

        // Override the first few modes with Natan Ho roots
        //freeBeamRoot[0] = 1.f;
        //freeBeamRoot[1] = std::pow(2.49975267f / freeFundamentalRoot, 2.f);

        cantBeamRoot[0] = 1.f;
        cantBeamRoot[1] = std::pow(1.49417561f / cantFundamentalRoot, 2.f);
        cantBeamRoot[2] = std::pow(2.50024695f / cantFundamentalRoot, 2.f);
    }

    void setSamplerate(int samplerate) override {
        maxFrequency = std::max(samplerate, 1) * 0.45f;
    }

    float frequency(int mode, float pitch) const override {
        const float f = mClamp(pitch, 20.f, maxFrequency) * cantBeamRoot[mode];
        return f > maxFrequency ? 0.f : f;
    }

    float weight(int mode, float frequency, float position) const override {
        // TODO: Add beam position
        return 1.f;
    }

private:
    float maxFrequency = 20000.f;
    //std::array<float, MAX_MODES> freeBeamRoot = {};
    std::array<float, MAX_MODES> cantBeamRoot = {};
};