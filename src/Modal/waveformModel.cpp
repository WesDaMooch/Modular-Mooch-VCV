#include "waveformModel.hpp"


WaveformModel::WaveformModel() {
	computeFrequencies();
	computeWeights();
}

/*
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
*/


void WaveformModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFrequency = this->samplerate * 0.45f;
}


void WaveformModel::setParams(float pitch, float position) {

	pitch = mClamp(pitch, 20.f, maxFrequency);
	position = mMap(position, 0.5f, 0.9999f);

	if (this->pitch != pitch) {
		this->pitch = pitch;
		computeFrequencies();
	}

	if (this->position != position) {
		this->position = position;
		computeWeights();
	}
	

	// TODO: not using the waveform stuff now
	// Can remove it...
	// Position //
	// Morph between different waveform partials
	/*
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
	*/

}




void WaveformModel::computeFrequencies() {

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		frequencies[i] = pitch * n;
	}
}


void WaveformModel::computeWeights() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {

		if (frequencies[i] < 20.f || frequencies[i] > maxFrequency) {
			weights[i] = 0.f;
			continue;
		}

		const int n = i + 1;

		// Squarewave pulse width modulation
		float weight = std::fabsf(std::sinf(float(n) * M_PI * position)) / (float(n) * std::sinf(M_PI * position));
		weights[i] = weight;

		energy += weight * weight;
	}
	normaliseWeights(weights, energy);
}

/*
void WaveformModel::update() {

	float energy = 0.f;

	for (int i = 0; i < MAX_MODES; i++) {
		const int n = i + 1;

		float f = pitch * n;
		frequencies[i] = f;
			
		// Square wave PMW
		float pwmAmplitude = std::fabsf(std::sinf(float(n) * M_PI * position)) / (float(n) * std::sinf(M_PI * position));
		weights[i] = pwmAmplitude;
		energy += pwmAmplitude * pwmAmplitude;

		// TODO: add a smooth roll off to 0 amp
		// when aproaching the freq limits
	}
	normaliseWeights(weights, energy);
}
*/

// TODO:
// Dont like this, slowww
// ALSO not used
/*
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
*/
