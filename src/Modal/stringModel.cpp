#include "stringModel.hpp"


String::String() {
	buildTables();
	update();
}


void String::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
	maxFreq = sr * 0.5f;
}


void String::setParams(const StructureParams& newParams) {
	if (newParams == prevParams)
		return;

	fundamentalFreq = mClamp(newParams.fundamentalFreq, minFreq, maxFreq);
	inharmonicity = mMap(newParams.morph, -0.999f, 0.999f); // not used
	detune = mMap(newParams.morph, -1.f, 1.f); // not used
	position = mMap(newParams.position, 0.001f, 0.999f);
	baseDecay = mMap(newParams.decay, 0.5f, 400.0f);

	float timbreDialPos = mMap(newParams.timbre, 0.f, float(DIAL_RESOLUTION - 1));
	amplitudeDialIdxA = static_cast<int>(std::floor(timbreDialPos));
	amplitudeDialIdxB = std::min(amplitudeDialIdxA + 1, DIAL_RESOLUTION - 1);
	amplitudeXFade = timbreDialPos - amplitudeDialIdxA;

	decayDialIdxA = static_cast<int>(std::floor(timbreDialPos));
	decayDialIdxB = std::min(decayDialIdxA + 1, DIAL_RESOLUTION - 1);
	decayXFade = timbreDialPos - decayDialIdxA;

	prevParams = newParams;
	update();
}


void String::update() {

	const float maxDetune = 5.f; // Hz, not used
	for (int idx = 0; idx < MAX_MODES; idx++) {
		int n = idx + 1;

		// Pitch
		//float f = fundamentalFreq * std::pow(static_cast<float>(n), 1.f + inharmonicity);

		float activeDetune = detune * maxDetune * idx;

		// TODO: this detune isnt very interesting
		if (n % 2 != 0) {
			// Odd
			activeDetune *= -1;
		}

		float f = fundamentalFreq * n + activeDetune;

		coefs[idx].freq = f;

		if (f < minFreq || f > maxFreq) {
			coefs[idx].amplitude = 0.f;
			continue;
		}

		// Amplitude
		float positionAmplitude = std::sin(M_PI * n * position);
		float amplitude = mInterp(amplitudeXFade, amplitudeDialTable[idx][amplitudeDialIdxA], amplitudeDialTable[idx][amplitudeDialIdxB]);
		coefs[idx].amplitude = amplitude * positionAmplitude;

		// Decay (Q)
		float decay = mInterp(decayXFade, decayDialTable[decayDialIdxA], decayDialTable[decayDialIdxB]);
		coefs[idx].q = baseDecay * std::pow(float(n), -decay);
	}
}


const SvfCoefficients& String::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


void String::buildTables() {
	// TODO: high timbre quick damps only the fundimental mode,
	// and as it increases damps the next 6 - 10?
	// Use and exponetial curve
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