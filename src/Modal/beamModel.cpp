#include "beamModel.hpp"


BeamModel::BeamModel() {
	buildRoots();
	update();
}

void BeamModel::buildRoots() {
	// From Natan Ho - Exploring Modal Synthesis
	// Normalised frequency ratio (fk) proportional to f0

	const float freeFundamentalRoot = 1.50561873f;
	const float cantFundamentalRoot = 0.59686416f;

	for (int i = 0; i < MAX_MODES; i++) {
		float freeRoot = float(i) + 1.5f;
		float cantRoot = float(i) + 0.5f;

		freeBeamRoot[i] = std::pow(freeRoot / freeFundamentalRoot, 2.f);
		cantBeamRoot[i] = std::pow(cantRoot / cantFundamentalRoot, 2.f);
	}

	// Override the first few modes with Natan Ho roots
	freeBeamRoot[0] = 1.f;
	freeBeamRoot[1] = std::pow(2.49975267f / freeFundamentalRoot, 2.f);

	cantBeamRoot[0] = 1.f;
	cantBeamRoot[1] = std::pow(1.49417561f / cantFundamentalRoot, 2.f);
	cantBeamRoot[2] = std::pow(2.50024695f / cantFundamentalRoot, 2.f);
}

void BeamModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFreq = this->samplerate * 0.5f;
}


void BeamModel::setParams(const StructureParams& params) {
	if (params == prevParams)
		return;

	fundamentalFreq = mClamp(params.fundamentalFreq, minFreq, maxFreq);

	//position = mMap(params.position, 0.001f, 0.999f);

	// TODO: needs a bit of tuning but its nice
	float decay = 1.f - params.decay;
	decay = std::pow(decay, 2.f);
	constLoss = mMap(decay, 0.f, 150.f);

	// TODO: low end of timbre remove high overtons?
	float timbre = 1.f - params.timbre;
	timbre = std::pow(timbre, 2.f);
	freqLoss = mMap(timbre, 0.f, 1000.f);

	prevParams = params;
	update();
}


void BeamModel::update() {

	for (int i = 0; i < MAX_MODES; i++) {

		float f = fundamentalFreq * cantBeamRoot[i];

		if (f < minFreq || f > maxFreq) {
			coefs[i].amplitude = 0.f;
			continue;
		}

		coefs[i].freq = f;

		// Amplitude // 
		// Position can change after excitation, not realistic but sounds cool
		// From Banks, Balázs et al. 2010. 'A Modal-Based Real-Time Piano Synthesizer'
		//float positionAmplitude = std::sin(M_PI * n * position);
		coefs[i].amplitude = 1.f; // positionAmplitude;


		//float amplitude = mInterp(amplitudeXFade, amplitudeDialTable[i][amplitudeDialIdxA], amplitudeDialTable[i][amplitudeDialIdxB]);
		//coefs[i].amplitude = amplitude * positionAmplitude;

		// Decay (Q) //
		// From Modalys
		float freqRatio = f / 1000.f;
		float loss = constLoss + freqLoss * freqRatio * freqRatio;
		float q = float(M_PI) * f / loss;
		coefs[i].q = mClamp(q, MIN_Q, MAX_Q);
	}
}


const SvfCoefficients& BeamModel::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}
