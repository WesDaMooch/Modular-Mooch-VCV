// asr.hpp
// Attack Sustain Release Envelope


#pragma once
#include <algorithm>


class ASR {
public:
    enum Stage {
        IDLE,
        ATTACK,
        SUSTAIN,
        RELEASE
    };

    void setSampleRate(float sampleRate);
    void setParams(float attackTime, float releaseTime);

    void trigger();
    void release();
    float process();
    void reset();

    float getValue() const { return value; }

private:
    Stage stage = IDLE;

    float sampleRate = 48000.f;
    float attackTime = 0.01f;
    float releaseTime = 0.5f;

    float value = 0.f;
    float releaseStep = 0.f;
};