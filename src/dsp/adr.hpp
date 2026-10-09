// ADR Envelope


#pragma once
#include <algorithm>


class ADR
{
public:
    ADR();
    void setSampleRate(int sampleRate);
    void setParams(float attack = 0.01f, float decay = 0.1f, float decayLevel = 0.7f, float release = 0.2f);
    void trigger();
    float process();
    void reset();

private:
    enum Stage {
        IDLE,
        ATTACK,
        DECAY,
        RELEASE
    };

    Stage stage = IDLE;

    int sampleRate = 48000;

    float attack = 0.01f;
    float decay = 0.1f;
    float decayLevel = 0.7f;
    float release = 0.2f;
    float value = 0.f;
};