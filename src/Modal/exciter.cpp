#include "exciter.hpp"


Exciter::Exciter() {
    buildLuts();
}


void Exciter::buildLuts() {
    // Exciter tables
    float peak = 0.f;
    
    // Impulse table 
    // TODO: Dont like the impulse, maybe replace with decaying impulse train?
    //for (int i = 0; i < TABLE_LEN; i++)
    //    ampTable[0][i] = (i == 0) ? 1.f : 0.f;


    // Raised cosine pulse
    const float T = TABLE_LEN / 8.f;
    const float beta = 1.f;
    const float zeroCrossingPoint = 3.f;   // Pick third zero-crossing from center

    // Raised cosine pulse
    for (int i = 0; i < TABLE_LEN; i++)
    {
        float t = -zeroCrossingPoint * T + ((zeroCrossingPoint * 2.f) * T * i) / (TABLE_LEN - 1);
        ampTable[1][i] = raisedCosine(t, T, beta);
        peak = std::max(peak, fabsf(ampTable[1][i]));
    }

    normaliseAmplitudeEnvelope(1, peak);


    // Gaussian pulse
    const float sigma = 33.3f;
    const float centre = TABLE_LEN * 0.5f;

    peak = 0.f;

    for (int i = 0; i < TABLE_LEN; i++)
    {
        float x = (i - centre) / sigma;
        ampTable[2][i] = std::expf(-0.5f * x * x);
        peak = std::max(peak, std::fabsf(ampTable[1][i]));
    }

    normaliseAmplitudeEnvelope(2, peak);


    // Delay tables
    static constexpr float delayExpCurve = 200.0f;

    for (int i = 0; i < MAX_MODES; i++) {
        float x = static_cast<float>(i) / (MAX_MODES - 1);
        float reverseX = (x - 1.0f) * -1.0f;

        // Specral Env
        spectralEnvLut[i] = 1.f;  //reverseX;

        // Delay //
        // Type 0 - Decreasing exponential
        delayTypeLuts[0][i] = (std::pow(delayExpCurve, x) - 1.f) / (delayExpCurve - 1.f);
        
        // Type 1 - Decreasing linear
        delayTypeLuts[1][i] = x;

        // Type 3 - Increasing linear
        delayTypeLuts[3][i] = reverseX;

        // Type 4 - Increasing exponential
        delayTypeLuts[4][i] = (std::pow(delayExpCurve, reverseX) - 1.f) / (delayExpCurve - 1.f);
    }

    // Type 2 - Random delay
    buildRandomDelay();
}

void Exciter::trigger(ExciterParams params) {
    // Amplitude envelope
    ampAttack = 0.0002f; //0.005f;
    ampDecay = 0.002f;//0.005f;
	ampValue = 0.f;    
	ampStage = ATTACK;

    type = params.type;

    // Start table playback
    ampTablePhase = 0.f;
    tableActive = true;

    // Delay
    delayTime = mClamp(params.delayTime, 0.f, 1.f);
    delayType = mClamp(params.delayType, 0.0f, 1.0f);
    buildRandomDelay();
}


void Exciter::update(float deltaTime) {
    float excitation = 0.f;

    if (tableActive)
    {
        type = mClamp(type, 0.f, 1.f);

        float tablePosition = type * (NUM_TABLES - 1);

        int typeA = static_cast<int>(std::floorf(tablePosition));
        int typeB = static_cast<int>(std::ceilf(tablePosition));

        float typeFrac = tablePosition - typeA;

        int index0 = static_cast<int>(ampTablePhase);
        int index1 = index0 + 1;

        float frac = ampTablePhase - index0;

        if (index1 < TABLE_LEN)
        {
            float sampleA = mInterp(frac, ampTable[typeA][index0], ampTable[typeA][index1]);
            float sampleB = mInterp(frac, ampTable[typeB][index0], ampTable[typeB][index1]);

            excitation = mInterp(typeFrac, sampleA, sampleB);
            ampTablePhase += ampTableIncrement;
        }
        else
        {
            tableActive = false;
            excitation = 0.f;
        }
    }

    // Write into delay buffers
    exciterDelayBuffer[writeIdx] = excitation;

    // Read excitation wavetable
    /*
    if (tableActive)
    {
        int index = static_cast<int>(ampTablePhase);


        int typeA = std::floor(type);
        int typeB = std::ceil(type);

        float typeCrossfade = type - static_cast<float>(typeA);

        if (index < TABLE_LEN)
        {
            float excitationA = ampTable[typeA][index];
            float excitationB = ampTable[typeB][index];

            excitation = mInterp(typeCrossfade, excitationA, excitationB); //ampTable[currentTable][index];

            ampTablePhase += ampTableIncrement;
        }
        else
        {
            // Table finished
            tableActive = false;
            excitation = 0.f;
        }
    }
    */

    // Exciter amplitude envelope
    /*
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
    */

    // Write into delay buffers
    //exciterDelayBuffer[writeIdx] = ampValue;
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
    
    return exciterDelayBuffer[readIdx];
}

void Exciter::advanceWriteIdx() {
    writeIdx++;
    if (writeIdx >= MAX_DELAY_SAMPLES)
        writeIdx = 0;
}


void Exciter::buildRandomDelay() {
    // Type 2 - Random delay
    for (int i = 0; i < MAX_MODES; i++)
        delayTypeLuts[2][i] = randomValue();
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