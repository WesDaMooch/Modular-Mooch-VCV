#include "asr.hpp"


void ASR::setSampleRate(float sampleRate) {
    this->sampleRate = std::max(sampleRate, 1.f);
}


void ASR::setParams(float attackTime, float releaseTime) {
    const float EPSILON = 1e-6f;
    this->attackTime = std::max(attackTime, EPSILON);
    this->releaseTime = std::max(releaseTime, EPSILON);
}

void ASR::trigger() {
    stage = ATTACK;
}

void ASR::release() {
    if (stage == IDLE)
        return;

    // Calculate release slope from the current level.
    releaseStep = value / (releaseTime * sampleRate);
    stage = RELEASE;
}

float ASR::process() {
    switch (stage) {
    case IDLE:
        value = 0.f;
        break;

    case ATTACK:
        value += 1.f / (attackTime * sampleRate);

        if (value >= 1.f) {
            value = 1.f;
            stage = SUSTAIN;
        }
        break;

    case SUSTAIN:
        value = 1.f;
        break;

    case RELEASE:
        value -= releaseStep;

        if (value <= 0.f) {
            value = 0.f;
            stage = IDLE;
        }
        break;
    }

    return value;
}

void ASR::reset() {
    stage = IDLE;
    value = 0.f;
    releaseStep = 0.f;
}