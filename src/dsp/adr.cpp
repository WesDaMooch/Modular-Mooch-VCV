#include "adr.hpp"


ADR::ADR() {}


void ADR::setSampleRate(int sampleRate) {
    sr = sampleRate > 0 ? sampleRate : 1;
}


void ADR::setParams(const Params& params) {
    this->p = p;
}


void ADR::trigger() {
    stage = ATTACK;
}


float ADR::process() {
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
        value -= (1.f - p.decayLevel) / (p.decay * sr);

        if (value <= p.decayLevel) {
            value = p.decayLevel;
            stage = RELEASE;
        }
        break;

    case RELEASE:
        value -= p.decayLevel / (p.release * sr);

        if (value <= 0.f) {
            value = 0.f;
            stage = IDLE;
        }
        break;
    }

    return value;
}
