#include "exciter.hpp"


Exciter::Exciter() {
    buildLuts();
}


void Exciter::buildLuts() {
    // Delay Dial
    static constexpr float stateTwoCurve = 200.0f;

    for (int i = 0; i < MAX_MODES; i++) {
        float x = static_cast<float>(i) / (MAX_MODES - 1);

        // State 0 - Decreasing linear
        modeDelayDecreasingLinearLut[i] = x;

        // State 1 - Decreasing exponential
        modeDelayDecreasingExpLut[i] = (std::pow(stateTwoCurve, x) - 1.0f) / (stateTwoCurve - 1.0f);

        // TODO: State 2 - Random grain
        modeDelayRandomGrainLut[i] = 0.0f;

        float reverseX = (x - 1.0f) * -1.0f;
        // State 3 - Increasing exponential
        modeDelayIncreasingExpLut[i] = (std::pow(stateTwoCurve, reverseX) - 1.0f) / (stateTwoCurve - 1.0f);

        // State 4 - Increasing linear
        modeDelayIncreasingLinearLut[i] = reverseX;
    }
}

void Exciter::trigger(ExciterParams params) {
    // Amplitude envelope
	ampAttack = 0.005f;
	ampDecay = 0.005f;
	ampValue = 0.f;    
	ampStage = ATTACK;

    // Attack envelope
    atkStage = DECAY;
    atkAttack = mClamp(params.attack, 0.f, 1.f) * 0.5f;

    // Delay
    delayTime = mClamp(params.delayTime, 0.f, 1.f);
    delayType = mClamp(params.delayType, 0.0f, 1.0f);
}


void Exciter::update(float deltaTime) {
    // Exciter amplitude envelope
    switch (ampStage) {

    case IDLE:
        ampValue = 0.0f;
        break;

    case ATTACK:
        if (ampAttack <= 0.0f) {
            ampValue = 1.0f;
            ampStage = DECAY;
        }
        else {
            ampValue += deltaTime / ampAttack;

            if (ampValue >= 1.0f) {
                ampValue = 1.0f;
                ampStage = DECAY;
            }
        }
        break;

    case DECAY:
        if (ampDecay <= 0.0f) {
            ampValue = 0.0f;
            ampStage = IDLE;
        }
        else {
            ampValue -= deltaTime / ampDecay;

            if (ampValue <= 0.0f) {
                ampValue = 0.0f;
                ampStage = IDLE;
            }
        }
        break;
    }

    // Attack envelope
    switch (atkStage) {

    case IDLE:
        atkValue = 1.f;
        break;

    case DECAY:
        if (atkDecay <= 0.f) {
            atkValue = 0.f;
            atkStage = ATTACK;
        }
        else {
            atkValue -= deltaTime / atkDecay;

            if (atkValue <= 0.f) {
                atkValue = 0.f;
                atkStage = ATTACK;
            }
        }
        break;

    case ATTACK:
        if (atkAttack <= 0.f) {
            atkValue = 1.f;
            atkStage = IDLE;
        }
        else {
            atkValue += deltaTime / atkAttack;

            if (atkValue >= 1.f) {
                atkValue = 1.f;
                atkStage = IDLE;
            }
        }
        break;
    }

    // Write into delay buffers
    exciterDelayBuffer[writeIdx] = ampValue;
    atkEnvDelayBuffer[writeIdx] = atkValue;
}


void Exciter::reset() {
	ampValue = 0.f;
    atkValue = 0.f;
}

Exciter::Out Exciter::get(int modeIdx) {
    // Delay
    float delayNorm = getNorm(modeIdx);
    int delaySamples = static_cast<int>(delayNorm * MAX_DELAY_SAMPLES * delayTime);

    int readIdx = writeIdx - delaySamples;

    while (readIdx < 0)
        readIdx += MAX_DELAY_SAMPLES;
    
    return { exciterDelayBuffer[readIdx], atkEnvDelayBuffer[readIdx] };
    //return { exciterDelayBuffer[readIdx], atkValue };
}

void Exciter::advanceWriteIdx() {
    writeIdx++;
    if (writeIdx >= MAX_DELAY_SAMPLES)
        writeIdx = 0;
}


float Exciter::getNorm(int modeIdx) {
    modeIdx = mClamp(modeIdx, 0, MAX_MODES);

    float state = delayType * (NUM_DELAY_STATES - 1);

    if (state <= 1.0f) {
        // Index by Mode (Linear)
        return mInterp(state, modeDelayDecreasingLinearLut[modeIdx], modeDelayDecreasingExpLut[modeIdx]);
    }
    else if (state > 1.0f && state <= 2.0f) {
        // State 1 - Index by Mode (Exponential)
        float xFade = state - 1.0f;
        return mInterp(xFade, modeDelayDecreasingExpLut[modeIdx], modeDelayRandomGrainLut[modeIdx]);
    }
    else if (state > 2.0f && state <= 3.0f) {
        float xFade = state - 2.0f;
        return mInterp(xFade, modeDelayRandomGrainLut[modeIdx], modeDelayIncreasingExpLut[modeIdx]);
    }
    else if (state > 3.0f && state <= 4.0f) {
        float xFade = state - 3.0f;
        return mInterp(xFade, modeDelayIncreasingExpLut[modeIdx], modeDelayIncreasingLinearLut[modeIdx]);
    }

    return 0.0f;
}