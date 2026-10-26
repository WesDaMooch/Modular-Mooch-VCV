#include "waveformModel.hpp"


WaveformModel::WaveformModel() {
	buildRoots();
	update();
}

void WaveformModel::buildRoots() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		// Sawtooth
		waveformAmplitude[0][i] = 1 / float(n); //(n % 2 == 1 ? 1.f : -1.f) / float(n);

		// Squarewave
		waveformAmplitude[1][i] = (n % 2 == 1) ? 1.f / float(n) : 0.f;

	}
}

void WaveformModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFreq = this->samplerate * 0.5f;
}


void WaveformModel::setParams(const StructureParams& params) {
	if (params == prevParams)
		return;

	fundamentalFreq = mClamp(params.fundamentalFreq, minFreq, maxFreq);

	// TODO: needs a bit of tuning but its nice
	float decay = 1.f - params.decay;
	decay = std::pow(decay, 2.f);
	constLoss = mMap(decay, 0.f, 150.f);

	// TODO: low end of timbre remove high overtons?
	float timbre = 1.f - params.timbre;
	timbre = std::pow(timbre, 2.f);
	freqLoss = mMap(timbre, 0.f, 1000.f);

	// Position //
	float waveformSelect = mMap(params.position, 0.f, float(NUM_WAVEFORMS - 1));
	waveformIdxA = static_cast<int>(std::floor(waveformSelect));
	waveformIdxB = std::min(waveformIdxA + 1, NUM_WAVEFORMS - 1);
	waveformXFade = waveformSelect - waveformIdxA;

	prevParams = params;
	update();
}


void WaveformModel::update() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		float f = fundamentalFreq * n;

		if (f < minFreq || f > maxFreq) {
			coefs[i].amplitude = 0.f;
			continue;
		}

		coefs[i].freq = f;

		// Amplitude // 
		float a = waveformAmplitude[waveformIdxA][i];
		float b = waveformAmplitude[waveformIdxB][i];

		coefs[i].amplitude = mInterp(waveformXFade, a, b); //*pwmAmplitude;

		// Position can change after excitation, not realistic but sounds cool
		// PWM?

		// Decay (Q) //
		// From Modalys
		float freqRatio = f / 1000.f;
		float loss = constLoss + freqLoss * freqRatio * freqRatio;
		float q = float(M_PI) * f / loss;
		coefs[i].q = mClamp(q, MIN_Q, MAX_Q);
	}
}


const SvfCoefficients& WaveformModel::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}
