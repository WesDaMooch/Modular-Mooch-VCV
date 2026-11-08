#include "waveformModel.hpp"


WaveformModel::WaveformModel() {
	buildRoots();
	update();
}

void WaveformModel::buildRoots() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		// Saw Wave
		updateSawPWM(1.f);

		waveformAmplitude[1][i] = 1.f / float(n); //(n > 1) ? -sawToothPatial : sawToothPatial; //(n % 2 == 1 ? 1.f : -1.f) / float(n); // 1.f / float(n)

		// Square Wave
		waveformAmplitude[2][i] = (n % 2 == 1) ? 1.f / float(n) : 0.f;
		
		// PWM squarewave
		updateSquarePWM(0.5f);
	}
}

// TODO: this could be remove and just filtered in Voice()
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

	// TODO: not using the waveform stuff now
	// Can remove it...
	// Position //
	// Morph between different waveform partials
	float waveformSelect = mMap(params.position, 0.f, float(NUM_WAVEFORMS - 1));
	waveformIdxA = static_cast<int>(std::floor(waveformSelect));
	waveformIdxB = std::min(waveformIdxA + 1, NUM_WAVEFORMS - 1);
	waveformXFade = waveformSelect - waveformIdxA;


	if (waveformSelect < 1.f) {
		// Saw PWM
		float pwmPosition = mClamp(waveformSelect, 0.f, 1.f);
		float duty = mMap(pwmPosition, 1.f, 0.1f);
		updateSawPWM(duty);
	}
	else if (waveformSelect > 2.f) {
		// Square PWM
		float pwmPosition = mClamp(waveformSelect - 2.f, 0.f, 1.f);
		float duty = mMap(pwmPosition, 0.5f, 0.999f);
		updateSquarePWM(duty);
	}

	duty = mMap(params.position, 0.5f, 0.9999f);

	prevParams = params;
	update();
}


void WaveformModel::update() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		float f = fundamentalFreq * n;

		if (f < minFreq || f > maxFreq) {
			coefs[i].amplitude = 0.f;
			continue;
		}

		coefs[i].freq = f;

		// Amplitude // 
		/*
		float waveformA = waveformAmplitude[waveformIdxA][i];
		float wavefromB = waveformAmplitude[waveformIdxB][i];

		float amplitude = mInterp(waveformXFade, waveformA, wavefromB);

		coefs[i].amplitude = amplitude;
		energy += amplitude * amplitude;
		*/

		// Square wave PMW
		float amplitude = std::fabsf(std::sinf(float(n) * M_PI * duty)) / (float(n) * std::sinf(M_PI * duty));

		coefs[i].amplitude = amplitude;
		energy += amplitude * amplitude;


		// Decay (Q) //
		// From Modalys
		float freqRatio = f / 1000.f;
		float loss = constLoss + freqLoss * freqRatio * freqRatio;
		float q = float(M_PI) * f / loss;
		coefs[i].q = mClamp(q, MIN_Q, MAX_Q);
	}

	// Normalise energy
	float gain = 1.f / std::sqrt(std::max(energy, 1e-12f));

	for (int i = 0; i < MAX_MODES; i++)
		coefs[i].amplitude *= gain;
}


const SvfCoefficients& WaveformModel::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


// Dont like this
void WaveformModel::updateSawPWM(float duty) {
	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		float w = 2.f * M_PI * float(n);
		float wd = w * duty;

		float a = 2.f * std::sin(wd) / (w * w * duty) - std::sin(wd) / w;
		float b = 2.f * (1.f - std::cos(wd)) / (w * w * duty) + (1.f - std::cos(wd)) / w;

		waveformAmplitude[0][i] = std::sqrt(a * a + b * b);
	}
}


void WaveformModel::updateSquarePWM(float duty) {
	// TODO: this is expensive, optimise
	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		float amplitude = std::fabsf(std::sinf(float(n) * M_PI * duty)) / (float(n) * std::sinf(M_PI * duty));

		waveformAmplitude[3][i] = amplitude;
	}
}
