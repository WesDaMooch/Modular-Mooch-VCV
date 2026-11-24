#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "common.hpp"
#include "models.h"
#include "stmlib/dsp/filter.h"


class ResonatorBank {
public:
	// Pitch can change every sample (punch envelope), so the expensive
	// recompute (virtual calls, sinf, decayResponse) runs at control rate.
	// The per-slot frequency slew smooths between updates.
	static constexpr int CONTROL_INTERVAL = 16;

	void setSampleRate(float sr) {
		sampleRate = std::max(sr, 1.f);
		fadeStep = 1.f / (fadeTime * sampleRate);
		slewCoeff = std::exp(-1.f / (freqSlewTime * sampleRate));
		maxFreq = sampleRate * 0.45f;
	}

	// Idempotent; cheap to call from the dial handler.
	void setModel(int m, BaseModel* model) {
		Mode& md = modes[m];

		if (model == md.slot[md.cur].model) {          // back to current: cancel queued swap
			
			if (md.hasPending) { 
				md.hasPending = false; 
				numPending--; 
			}

			return;
		}

		if (md.hasPending && md.pending == model)
			return;

		if (!md.hasPending) { 
			md.hasPending = true; 
			numPending++; 
		}

		md.pending = model;                            // latest wins
	}

	void setParams(float pitch_, float position_, float baseQ_,
		float timbre_, float timbreMod_, DecayResponse::Type type_) {

		if (pitch_ != pitch || position_ != position || baseQ_ != baseQ ||
			timbre_ != timbre || timbreMod_ != timbreMod || type_ != timbreType)
			dirty = true;

		pitch = pitch_; position = position_; baseQ = baseQ_;
		timbre = timbre_; timbreMod = timbreMod_; timbreType = type_;
	}

	float process(float excitation) {
		if (numPending > 0)
			startSwaps();

		if (dirty && ++counter >= CONTROL_INTERVAL) {
			counter = 0;
			dirty = false;
			update();
		}

		float out = 0.f;

		for (Mode& md : modes) {
			for (Slot& s : md.slot) {
				if (!s.live()) 
					continue;

				// Gain ramp toward target (linear, fixed time)
				const float diff = s.target - s.gain;
				s.gain += mClamp(diff, -fadeStep, fadeStep);

				// Frequency slew (de-clicks fast pitch moves); only touch coefficients when needed
				if (s.freq > 0.f) {
					s.slewed = s.freq + slewCoeff * (s.slewed - s.freq);
					if (std::fabs(s.slewed - s.freq) < 1e-4f * s.freq)
						s.slewed = s.freq;                     // settled
					else
						s.coefDirty = true;
				}
				if (s.coefDirty) {
					s.svf.set_f_q<stmlib::FREQUENCY_FAST>(s.slewed / sampleRate, s.q);
					s.coefDirty = (s.slewed != s.freq);       // keep going until settled
				}

				out += s.gain * s.svf.Process<stmlib::FILTER_MODE_BAND_PASS>(excitation);
			}
		}
		return out;
	}

private:
	struct Slot {
		stmlib::Svf svf;
		BaseModel* model = nullptr;
		float freq = 0.f;       // target frequency, 0 = unused
		float slewed = 0.f;     // frequency the filter is actually at
		float q = 1.f;
		float target = 0.f;     // weight we're heading to
		float gain = 0.f;       // weight currently applied
		bool coefDirty = false;

		bool live() const { 
			return gain != 0.f || target != 0.f; 
		}
	};

	struct Mode {
		std::array<Slot, 2> slot;
		int cur = 0;
		BaseModel* pending = nullptr;
		bool hasPending = false;
	};

	void startSwaps() {
		for (Mode& md : modes) {
			if (!md.hasPending || md.slot[1 - md.cur].live())
				continue;

			Slot& n = md.slot[1 - md.cur];
			n.model = md.pending;
			n.freq = n.slewed = 0.f;
			n.gain = n.target = 0.f;
			n.svf.Init();
			md.cur = 1 - md.cur;       // old current becomes the outgoing slot
			md.hasPending = false;
			numPending--;
			dirty = true;
			counter = CONTROL_INTERVAL; // update on this sample
		}
	}

	void update() {
		// 1. Frequencies (both slots keep tracking pitch while fading)
		std::array<float, MAX_MODES> curFreqs = {};

		for (int m = 0; m < MAX_MODES; m++) {
			Mode& md = modes[m];

			for (int s = 0; s < 2; s++) {

				Slot& sl = md.slot[s];

				if (!sl.model || (s != md.cur && !sl.live())) {
					sl.freq = 0.f; 
					continue;
				}
				

				float f = sl.model->frequency(m, pitch);
				f = (f > 0.f) ? mClamp(f, 20.f, maxFreq) : 0.f;

				if (f != sl.freq) {
					if (sl.freq <= 0.f) sl.slewed = f;      // starting up: don't sweep from 0
					sl.freq = f;
					sl.coefDirty = true;
				}
			}
			curFreqs[m] = md.slot[md.cur].freq;
		}

		// 2. Decay response needs the current frequencies of all modes
		decayResponse.set(timbre, timbreMod, timbreType, curFreqs);

		// 3. Q and target weight
		for (int m = 0; m < MAX_MODES; m++) {
			Mode& md = modes[m];

			for (int s = 0; s < 2; s++) {
				Slot& sl = md.slot[s];

				const float q = mClamp(decayResponse.get(m) * baseQ, 1e-4f, 2000.f);

				if (q != sl.q) {
					sl.q = q;
					sl.coefDirty = true;
				}

				if (s != md.cur || sl.freq <= 0.f) { 
					sl.target = 0.f; 
					continue; 
				}

				const float w = sl.model->weight(m, sl.freq, position);
				sl.target = (std::fabs(w) >= 1e-4f) ? w : 0.f;
			}
		}
	}

	std::array<Mode, MAX_MODES> modes;
	DecayResponse decayResponse;

	float sampleRate = 48000.f, maxFreq = 21600.f;
	float fadeTime = 0.02f;
	float fadeStep = 1.f / (0.02f * 48000.f);
	float freqSlewTime = 0.001f;
	float slewCoeff = 0.f;

	float pitch = 220.f, position = 0.5f, baseQ = 1.f;
	float timbre = 0.f, timbreMod = 0.f;
	DecayResponse::Type timbreType = DecayResponse::Type::LOWPASS_SECOND_ORDER;

	bool dirty = true;
	int counter = CONTROL_INTERVAL;
	int numPending = 0;
};