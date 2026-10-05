// ADSR Envelope


#pragma once


class ADSR
{
public:
    struct Params {
        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.2f;
    };

	ADSR();
	void setSampleRate(int sampleRate);
    void setParams(const Params& p);
    void gate(bool gate);
    float process();
    void reset();

protected:
    enum Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    Stage stage = IDLE;
    Params p;

	int sr = 48000;

    float value = 0.0f;
};