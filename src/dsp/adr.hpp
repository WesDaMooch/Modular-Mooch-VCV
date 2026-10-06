// ADR Envelope


#pragma once


class ADR
{
public:
    struct Params {
        float attack = 0.01f;
        float decay = 0.1f;
        float release = 0.2f;
        float decayLevel = 0.7f;

        Params(
            float attack = 0.01f,
            float decay = 0.1f,
            float release = 0.2f,
            float decayLevel = 0.7f)
            : attack(attack)
            , decay(decay)
            , release(release)
            , decayLevel(decayLevel)
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