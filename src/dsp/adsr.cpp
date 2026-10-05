#include "adsr.hpp"

ADSR::ADSR() {
    reset();
};

void ADSR::setSampleRate(int sampleRate) {
	sr = sampleRate > 0 ? sampleRate : 1;
}

void ADSR::setParams(const Params& p) {
	this->p = p;
}


void ADSR::gate(bool gate)
{
    if (gate) {
        stage = ATTACK;
    }
    else if (stage != IDLE) {
        stage = RELEASE;
    }
}


float ADSR::process() {

    switch (stage)
    {
    case IDLE:
        value = 0.f;
        break;

    case ATTACK:
        value += 1.f / (p.attack * sr);

        if (value >= 1.f) {
            value = 1.f;
            stage = DECAY;
        }
        break;

    case DECAY:
        value -= (1.f - p.sustain) / (p.decay * sr);

        if (value <= p.sustain) {
            value = p.sustain;
            stage = SUSTAIN;
        }
        break;

    case SUSTAIN:
        value = p.sustain;
        break;

    case RELEASE:
        /* OLD
        value -= p.sustain / (p.release * sampleRate);

        if (value <= 0.f) {
            value = 0.f;
            stage = IDLE;
        }
        break;
        */

        value -= 1.f / (p.release * sr);

        if (value <= 0.f) {
            value = 0.f;
            stage = IDLE;
        }
        break;
    }

    return value;
}

void ADSR::reset() {
    stage = IDLE;
    value = 0.f;
}