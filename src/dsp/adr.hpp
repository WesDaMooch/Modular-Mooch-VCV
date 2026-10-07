// ADR Envelope


#pragma once
#include <algorithm>

// TODO: no need for params 

class ADR
{
public:
    struct Params {
        float attack = 0.01f;
        float decay = 0.1f;
        float decayLevel = 0.7f;
        float release = 0.2f;

        Params(
            float attack = 0.01f,
            float decay = 0.1f,
            float decayLevel = 0.7f,
            float release = 0.2f)
            : attack(attack)
            , decay(decay)
            , decayLevel(decayLevel)
            , release(release)
        {
        }
    };

    ADR();

    void setSampleRate(int sampleRate);
    void setParams(const Params& params);

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
    Params p;

    int sr = 48000;
    float value = 0.f;
};