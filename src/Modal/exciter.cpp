#include "exciter.hpp"


Exciter::Exciter() {
    buildLuts();
}


void Exciter::buildLuts() {
    static constexpr float delayExpCurve = 200.0f;

    for (int i = 0; i < MAX_MODES; i++) {
        float x = static_cast<float>(i) / (MAX_MODES - 1);
        float reverseX = (x - 1.0f) * -1.0f;

        // Specral Env
        spectralEnvLut[i] = reverseX;

        // Delay //
        // State 0 - Decreasing linear
        delayTypeLuts[0][i] = x;

        // State 1 - Decreasing exponential
        delayTypeLuts[1][i] = (std::pow(delayExpCurve, x) - 1.f) / (delayExpCurve - 1.f);

        // TODO: State 2 - Random grain
        delayTypeLuts[2][i] = 0.f;

        // State 3 - Increasing exponential
        delayTypeLuts[3][i] = (std::pow(delayExpCurve, reverseX) - 1.f) / (delayExpCurve - 1.f);

        // State 4 - Increasing linear
        delayTypeLuts[3][i] = reverseX;
    }
}

void Exciter::trigger(ExciterParams params) {
    // Amplitude envelope
    ampAttack = 0.0002f; //0.005f;
    ampDecay = 0.002f;//0.005f;
	ampValue = 0.f;    
	ampStage = ATTACK;

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

    // Write into delay buffers
    exciterDelayBuffer[writeIdx] = ampValue;
}


void Exciter::reset() {
	ampValue = 0.f;
}

float Exciter::get(int modeIdx) {
    // Delay
    float delayNorm = getDelay(modeIdx);
    int delaySamples = static_cast<int>(delayNorm * MAX_DELAY_SAMPLES * delayTime);

    int readIdx = writeIdx - delaySamples;

    while (readIdx < 0)
        readIdx += MAX_DELAY_SAMPLES;
    
    return exciterDelayBuffer[readIdx] * spectralEnvLut[modeIdx];
}

void Exciter::advanceWriteIdx() {
    writeIdx++;
    if (writeIdx >= MAX_DELAY_SAMPLES)
        writeIdx = 0;
}


float Exciter::getDelay(int modeIdx) {
    modeIdx = mClamp(modeIdx, 0, MAX_MODES);

    float state = delayType * (NUM_DELAY_TYPES - 1);

    if (state <= 1.0f) {
        // Index by Mode (Linear)
        return mInterp(state, delayTypeLuts[0][modeIdx], delayTypeLuts[1][modeIdx]);
    }
    else if (state > 1.0f && state <= 2.0f) {
        // State 1 - Index by Mode (Exponential)
        float xFade = state - 1.0f;
        return mInterp(xFade, delayTypeLuts[1][modeIdx], delayTypeLuts[2][modeIdx]);
    }
    else if (state > 2.0f && state <= 3.0f) {
        float xFade = state - 2.0f;
        return mInterp(xFade, delayTypeLuts[2][modeIdx], delayTypeLuts[3][modeIdx]);
    }
    else if (state > 3.0f && state <= 4.0f) {
        float xFade = state - 3.0f;
        return mInterp(xFade, delayTypeLuts[3][modeIdx], delayTypeLuts[4][modeIdx]);
    }

    return 0.0f;
}