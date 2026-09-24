#include "structures.hpp"


String::String() {
	buildLuts();
	update();
}

void String::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
	maxFreq = sr * 0.5f;
}

void String::setParams(StructureParams& newParams) {
	if (newParams == prevParams)
		return;
	
	fundamentalFreq = mClamp(newParams.fundamentalFreq, minFreq, maxFreq);
	inharmonicity = mMap(newParams.morph, -0.999f, 0.999f);
	position = mMap(newParams.position, 0.0001f, 0.9999f);
	baseDecay = mMap(newParams.decay, 0.5f, 250.0f);
	timbre = mMap(newParams.timbre, -3.0f, 3.0f);
	amplitudeDialIdx = static_cast<int>(mMap(newParams.timbre, 0.f, (float)(DIAL_RESOLUTION - 1)));
	decaySlopeIdx = static_cast<int>(mMap(newParams.timbre, 0.f, (float)(DIAL_RESOLUTION - 1)));

	prevParams = newParams;
	update();
}

void String::update() {
	//float qMin = 1.0f;
	activeModes = 0;

	for (size_t idx = 0; idx < MAX_MODES; idx++) {
		int n = idx + 1;

		// Pitch
		float f = fundamentalFreq * std::pow(static_cast<float>(n), 1.f + inharmonicity);
		coefs[idx].freq = f;

		if (f < minFreq || f > maxFreq) {
			coefs[idx].amplitude = 0.0f;
			coefs[idx].q = 1.f;
			continue;
		}

		// Amplitude
		/*
		float modePosition = float(n - 1) / float(MAX_MODES - 1);

		float modeAmp = 1.0f;

		if (timbre < 0.25f) {
			float fade = mRemap(timbre, 0.0f, 0.25f);

			// fade: 0 = strong low emphasis, 1 = flat
			float tilt = mInterp(fade, 1.0f, 0.0f);

			modeAmp = 1.0f - (modePosition * tilt);
		}
		else if (timbre > 0.75f) {
			float fade = mRemap(timbre, 0.75f, 1.0f);

			// fade: 0 = flat, 1 = strong high emphasis
			float tilt = fade;

			modeAmp = 1.0f - ((1.0f - modePosition) * (1.0f - tilt));
		}
		*/

		float positionAmp = std::sin(M_PI * n * position);
		coefs[idx].amplitude = positionAmp * amplitudeDialLut[idx][amplitudeDialIdx];

		// Amplitude
		//float positionAmplitude = std::sin(M_PI * n * position);
		//coefs[idx].amplitude = positionAmplitude * std::pow(float(n), -amplitudeSlopeLut[amplitudeSlopeIdx]);
			
		// Decay (Q)
		coefs[idx].q = baseDecay * std::pow(float(n), -decaySlopeLut[decaySlopeIdx]);

		activeModes++;		
	}
}

float String::getActiveModesScaler() const {
	return activeModes > 0 ? 1.0f / activeModes : 0.0f;
}

SvfCoefficients String::getCoefficients(int idx) {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}

void String::buildLuts() {
	static constexpr float delaySlopeMax = 2.f;	// Damped or muted

	for (int dialIdx = 0; dialIdx < DIAL_RESOLUTION; dialIdx++) {
		float value = static_cast<float>(dialIdx) / (DIAL_RESOLUTION - 1);

		float delaySlope = mRemapInv(value, 0.20f, 0.66f);
		decaySlopeLut[dialIdx] = delaySlope * delaySlopeMax;

		for (int modeIdx = 0; modeIdx < MAX_MODES; modeIdx++) {

			float modePosition = float(modeIdx) / float(MAX_MODES - 1);

			float amp = 1.f;

			if (value < 0.20f) {
				// High modes fade out
				float modeCurve = std::pow(modePosition, 0.01f);
				float fade = mRemap(value, 0.f, 0.20f);
				amp = 1.0f - modeCurve * (1.f - fade);
			}
			else if (value > 0.66f) {
				// Low modes fade out
				float fade = mRemap(value, 0.66f, 1.f);
				amp = modePosition + (1.f - modePosition) * (1.f - fade);
			}

			amplitudeDialLut[modeIdx][dialIdx] = amp;
		}
	}
}







Drum2::Drum2() {
	update();
}

void Drum2::setSamplerate(int newSamplerate) {
	sr = std::max(newSamplerate, 1);
	maxFreq = sr * 0.5f; //0.25f?
}

void Drum2::setParams(StructureParams& newParams) {
	if (newParams == prevParams)
		return;

	tuning = mClamp(newParams.fundamentalFreq, minFreq, maxFreq);
	size = mMap(newParams.morph, 0.0001f, 6.f);
	position = newParams.position;
	damping = newParams.decay;
	overtones = newParams.timbre;

	prevParams = newParams;
	update();
}

void Drum2::update() {
	for (int i = 0; i < MAX_MODES; i++)
	{
		const Root& r = roots[i];
		float weight = bessel(r.order, r.value * position);
		coefs[i].amplitude = scale(weight, 0.f, 1.f, overtones, 1, damping);
		coefs[i].freq = std::max(minFreq, std::min((r.value * tuning) / size, maxFreq));
		coefs[i].q = 1.f;

		// TODO: get activemodes
	}
}

SvfCoefficients Drum2::getCoefficients(int idx) {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}

float Drum2::getActiveModesScaler() const {
	//return activeModes > 0 ? 1.0f / activeModes : 0.0f;
	return 1.f;
}


float Drum2::bessel(int order, float x)
{
	const float EPS = 1e-8;
	const int MAX_TERMS = 20;

	// J0
	if (order == 0) {
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) * std::pow(x / 2.0, 2 * k) /(factorial(k) * factorial(k));

			sum += term;
			k++;
		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// J1
	if (order == 1) {
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) * std::pow(x / 2.0, 2 * k + 1) / (factorial(k) * factorial(k + 1));

			sum += term;
			k++;

		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// higher orders (recurrence)
	if (x == 0.0)
		return 0.0;

	return (2.0 * (order - 1) / x) * bessel(order - 1, x) - bessel(order - 2, x);
}
















// Drum
Drum::Drum()
{
	update();
}

float Drum::bessel(int order, float x) 
{
	const float EPS = 1e-8;
	const int MAX_TERMS = 20;

	// J0
	if (order == 0)
	{
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do
		{
			term = std::pow(-1.0, k) *
				std::pow(x / 2.0, 2 * k) /
				(factorial(k) * factorial(k));

			sum += term;
			k++;
		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// J1
	if (order == 1)
	{
		float sum = 0.0;
		float term = 1.0;
		int k = 0;

		do {
			term = std::pow(-1.0, k) *
				std::pow(x / 2.0, 2 * k + 1) /
				(factorial(k) * factorial(k + 1));

			sum += term;
			k++;

		} while (std::fabs(term) > EPS && k < MAX_TERMS);

		return sum;
	}

	// higher orders (recurrence)
	if (x == 0.0)
		return 0.0;

	return (2.0 * (order - 1) / x) * bessel(order - 1, x)
		- bessel(order - 2, x);
}

void Drum::calculateWeights()
{
	for (int i = 0; i < MAX_MODES; i++)
	{
		const Root& r = roots[i];
		float weight = bessel(r.order, r.value * position);
		weights[i] = scale(weight, 0.f, 1.f, overtones, 1, damping);
	}
}

void Drum::calculateFreqs()
{
	for (int i = 0; i < MAX_MODES; i++)
	{
		const Root& r = roots[i];
		freqs[i] = std::max(20.f, std::min((r.value * tuning) / size, samplerate * 0.25f));
	}

	/*
	const float root0 = roots[0].value;
	for (int i = 0; i < MAX_MODES; i++)
	{
		freqs[i] = fundamentalPitch * (roots[i].value / root0);
		freqs[i] = std::max(20.f, std::min(freqs[i], samplerate * 0.25f));
	}
	*/
}

void Drum::update()
{
	calculateWeights();
	calculateFreqs();
}

void Drum::setSamplerate(int newSamplerate)
{
	if (samplerate != newSamplerate)
	{
		samplerate = newSamplerate;
		update();
	}
}

void Drum::setPitch(float newFreq)
{
	newFreq = std::max(20.f, std::min(newFreq, 10000.f));

	if (tuning != newFreq)
	{
		tuning = newFreq;
		update();
	}
}

void Drum::setPosition(float newPosition)
{
	newPosition = std::max(0.f, std::min(newPosition, 1.f));

	if (position != newPosition)
	{
		position = newPosition;
		update();
	}
}

void Drum::setSize(float newSize)
{
	newSize = std::max(0.0001f, std::min(newSize, 6.f));

	if (size != newSize)
	{
		size = newSize;
		update();
	}
	
}

void Drum::setDamping(float newDamping)
{
	newDamping = std::max(0.f, std::min(newDamping, 1.f));

	if (damping != newDamping)
	{
		damping = newDamping;
		update();
	}
}

void Drum::setOvertones(float newOvertones)
{
	newOvertones = std::max(0.f, std::min(newOvertones, 1.f));

	if (overtones != newOvertones)
	{
		overtones = newOvertones;
		update();
	}
}

float Drum::getWeight(int index)
{
	int i = std::max(0, std::min(index, MAX_MODES - 1));
	return weights[i];
}

float Drum::getFreq(int index)
{
	int i = std::max(0, std::min(index, MAX_MODES - 1));
	return freqs[i];
}