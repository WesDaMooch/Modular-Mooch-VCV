#include "adr.hpp"


ADR::ADR() {
    reset();
}


void ADR::setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate > 0 ? sampleRate : 1;
}


void ADR::setParams(float attack, float decay, float decayLevel, float release) {
    const float EPSILON = 1e-6f;
    this->attack      = std::max(attack, EPSILON);
    this->decay       = std::max(decay, EPSILON);
    this->decayLevel  = std::max(0.f, std::min(decayLevel, 1.f));
    this->release     = std::max(release, EPSILON);
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
        value += 1.f / (attack * sampleRate);

        if (value >= 1.f) {
            value = 1.f;
            stage = DECAY;
        }
        break;

    case DECAY:
        value -= (1.f - decayLevel) / (decay * sampleRate);

        if (value <= decayLevel) {
            value = decayLevel;
            stage = RELEASE;
        }
        break;

    case RELEASE:
        value -= decayLevel / (release * sampleRate);

        if (value <= 0.f) {
            value = 0.f;
            stage = IDLE;
        }
        break;
    }

    return value;
}


void ADR::reset() {
    stage = IDLE;
    value = 0.f;
}