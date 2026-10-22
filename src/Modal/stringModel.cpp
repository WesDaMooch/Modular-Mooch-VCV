#include "stringModel.hpp"


StringModel::StringModel() {
	buildTables();
	update();
}


void StringModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFreq = this->samplerate * 0.5f;
}


void StringModel::setParams(const StructureParams& params) {
	if (params == prevParams)
		return;

	fundamentalFreq = mClamp(params.fundamentalFreq, minFreq, maxFreq);

	// stiffness 0 = ideal string
	float morph = 0.f;
	if (params.morph < 0.5f) {
		morph = mRemap(params.morph, 0.f, 0.5f);
		stiffness = mMap(morph, -0.06f, 0.f);
	}
	else {
		morph = mRemap(params.morph, 0.5f, 1.f);
		//morph = std::pow(morph, 3.f);	// dial feel shaping
		stiffness = mMap(morph, 0.f, 2.f);
	}

	position = mMap(params.position, 0.001f, 0.999f);

	// TODO: needs a bit of tuning but its nice
	float decay = 1.f - params.decay;
	decay = std::pow(decay, 2.f);
	constLoss = mMap(decay, 0.f, 150.f); 

	// TODO: low end of timbre remove high overtons?
	float timbre = 1.f - params.timbre;
	timbre = std::pow(timbre, 2.f);
	freqLoss = mMap(timbre, 0.f, 1000.f);

	// Old
	float timbreDialPos = mMap(params.timbre, 0.f, float(DIAL_RESOLUTION - 1));
	amplitudeDialIdxA = static_cast<int>(std::floor(timbreDialPos));
	amplitudeDialIdxB = std::min(amplitudeDialIdxA + 1, DIAL_RESOLUTION - 1);
	amplitudeXFade = timbreDialPos - amplitudeDialIdxA;

	decayDialIdxA = static_cast<int>(std::floor(timbreDialPos));
	decayDialIdxB = std::min(decayDialIdxA + 1, DIAL_RESOLUTION - 1);
	decayXFade = timbreDialPos - decayDialIdxA;

	prevParams = params;
	update();
}


void StringModel::update() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		// TODO: morph 0 = ideal string, morph 1 = Euler-Bernoulli beam
		// Maybe Euler-Bernoulli beam its own model

		// Stiff sting
		// From Gillet, Émilie. 2015. Mutable Instruments Elements
		float B = stiffness;
		float f = fundamentalFreq * n * (1.f + (n - 1) * B);

		if (f < minFreq || f > maxFreq) {
			coefs[i].amplitude = 0.f;
			continue;
		}

		coefs[i].freq = f;

		// Amplitude // 
		// Position can change after excitation, not realistic but sounds cool
		// From Banks, Balázs et al. 2010. 'A Modal-Based Real-Time Piano Synthesizer'
		float positionAmplitude = std::sin(M_PI * n * position);
		coefs[i].amplitude = positionAmplitude;


		//float amplitude = mInterp(amplitudeXFade, amplitudeDialTable[i][amplitudeDialIdxA], amplitudeDialTable[i][amplitudeDialIdxB]);
		//coefs[i].amplitude = amplitude * positionAmplitude;

		// Decay (Q) //
		// From Modalys
		float freqRatio = f / 1000.f;
		float loss = constLoss + freqLoss * freqRatio * freqRatio;
		float q = float(M_PI) * f / loss;
		coefs[i].q = mClamp(q, MIN_Q, MAX_Q);

		//float decay = mInterp(decayXFade, decayDialTable[decayDialIdxA], decayDialTable[decayDialIdxB]);
		//coefs[i].q = baseDecay * std::pow(float(n), -decay);
	}
}


const SvfCoefficients& StringModel::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


void StringModel::buildTables() {
	static constexpr float decaySlopeMax = 2.f;	// Damped / muted
	static constexpr float decayDialStart = 0.2f;
	static constexpr float decayDialEnd = 0.66f;

	static constexpr float highModeAmpDialExp = 0.01f;
	static constexpr float highModeAmpDialEnd = 0.2f;
	static constexpr float lowModeAmpDialStart = 0.66f;

	for (int dialIdx = 0; dialIdx < DIAL_RESOLUTION; dialIdx++) {
		float value = static_cast<float>(dialIdx) / (DIAL_RESOLUTION - 1);

		float delaySlope = mRemapInv(value, decayDialStart, decayDialEnd);
		decayDialTable[dialIdx] = delaySlope * decaySlopeMax;

		for (int modeIdx = 0; modeIdx < MAX_MODES; modeIdx++) {

			float modePosition = float(modeIdx) / float(MAX_MODES - 1);

			float amp = 1.f;

			if (value < highModeAmpDialEnd) {
				// High modes fade out
				float modeCurve = std::pow(modePosition, highModeAmpDialExp);
				float fade = mRemap(value, 0.f, highModeAmpDialEnd);
				amp = 1.0f - modeCurve * (1.f - fade);
			}
			else if (value > lowModeAmpDialStart) {
				// Low modes fade out
				float fade = mRemap(value, lowModeAmpDialStart, 1.f);
				fade = 1.f - std::pow(1.f - fade, 2.f);	// Exponent - how quicky lows die out
				amp = modePosition + (1.f - modePosition) * (1.f - fade);
			}

			amplitudeDialTable[modeIdx][dialIdx] = amp;
		}
	}
}