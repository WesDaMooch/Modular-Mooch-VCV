#include "rectModel.hpp"



RectModel::RectModel() {
	//buildTables();
	update();
}


void RectModel::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
	maxFreq = this->samplerate * 0.5f;
}


void RectModel::setParams(const StructureParams& newParams) {
	if (newParams == prevParams)
		return;

	fundamentalFreq = mClamp(newParams.fundamentalFreq, minFreq, maxFreq);
	aspectRatio = mMap(newParams.morph, 0.5f, 3.f);
	baseDecay = mMap(newParams.decay, 0.5f, 400.0f);

	prevParams = newParams;
	update();
}


void RectModel::update() {
	constexpr int MAX_INDEX = MAX_MODES / 4;

	int idx = 0;

	// TODO: find lowest MAX_MODES...

	for (int m = 1; m <= MAX_INDEX; ++m)
	{
		for (int n = 1; n <= MAX_INDEX; ++n)
		{
			float ratio = std::sqrt(
				(m * m + (n * n) / (aspectRatio * aspectRatio)) /
				(1.0f + 1.0f / (aspectRatio * aspectRatio))
			);

			coefs[idx].freq = fundamentalFreq * ratio;
			coefs[idx].q = baseDecay;
			//coefs[idx].amplitude = 1.f;
			//coefs[idx].amplitude = (idx == 0) ? 1.f : 0.f;
			float t = static_cast<float>(idx) / (MAX_MODES - 1);
			coefs[idx].amplitude = 1.f - 0.9f * t;
			idx++;
		}
	}
}


const SvfCoefficients& RectModel::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


void RectModel::buildTables() {
	
}