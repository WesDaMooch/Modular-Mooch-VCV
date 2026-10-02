#include "exciter.hpp"


Exciter::Exciter() {
    buildLuts();
}


void Exciter::buildLuts() {
    // Exciter tables
    size_t tableIdx = 0;
    float peak = 0.f;
    
    // Impulse table 
    // TODO: Dont like the impulse, maybe replace with decaying impulse train?
    //for (int i = 0; i < TABLE_LEN; i++)
    //    ampTable[0][i] = (i == 0) ? 1.f : 0.f;

    
    // Turkey, do kinda like could be used...
    tableIdx = 0;
    peak = 0.f;

    const float alpha = 0.1f; // 0 = rectangular, 1 = Hann

    for (int i = 0; i < TABLE_LEN; i++)
    {
        float x = static_cast<float>(i) / (TABLE_LEN - 1);

        float window = 0.0f;

        if (x < alpha * 0.5f)
        {
            // Rising cosine taper
            window = 0.5f * (1.f + cosf(M_PI * (2.f * x / alpha - 1.f)));
        }
        else if (x <= 1.0f - alpha * 0.5f)
        {
            // Flat section
            window = 1.0f;
        }
        else
        {
            // Falling cosine taper
            window = 0.5f * (1.0f + cosf(M_PI * (2.0f * x / alpha - 2.0f / alpha + 1.0f)));
        }

        shapeTable[tableIdx][i] = window;

        peak = std::max(peak, fabsf(shapeTable[0][i]));
    }
    normaliseAmplitudeEnvelope(tableIdx, peak);


    // Raise cosine pulse
    tableIdx++;
    peak = 0.f;

    const float T = TABLE_LEN / 8.f;
    const float beta = 0.5f;
    const float leftZeroCrossing = 2.f;    // Start point
    const float rightZeroCrossing = 5.f;   // End point

    for (int i = 0; i < TABLE_LEN; i++)
    {
        // Asymmetric time range
        float t = -leftZeroCrossing * T + (leftZeroCrossing + rightZeroCrossing) * T * static_cast<float>(i) / (TABLE_LEN - 1);

        float x = t / T;

        float denominator = 1.f - std::powf(2.f * beta * x, 2.f);

        float value = 0.f;

        if (std::fabsf(denominator) > 1e-12f)
        {
            float sinc = 1.f;

            if (std::fabsf(x) > 1e-12f)
                sinc = std::sinf(M_PI * x) / (M_PI * x);

            value = sinc * std::cosf(M_PI * beta * x) / denominator;
        }
        else
        {
            value = (M_PI * 0.25f) * (std::sinf(M_PI / (2.f * beta)) / (M_PI / (2.f * beta)));
        }

        shapeTable[tableIdx][i] = value;

        peak = std::max(peak, fabsf(value));
    }

    normaliseAmplitudeEnvelope(tableIdx, peak);


    // Hann pulse
    tableIdx++;
    peak = 0.f;

    for (int i = 0; i < TABLE_LEN; i++)
    {
        float hann = 0.5f * (1.f - std::cosf(2.f * M_PI * i / (TABLE_LEN - 1)));
        shapeTable[tableIdx][i] = hann;
        peak = std::max(peak, std::fabsf(shapeTable[tableIdx][i]));
    }

    normaliseAmplitudeEnvelope(tableIdx, peak);



    // Delay tables
    static constexpr float delayExpCurve = 200.0f;

    for (int i = 0; i < MAX_MODES; i++) {
        float x = static_cast<float>(i) / (MAX_MODES - 1);
        float reverseX = (x - 1.0f) * -1.0f;

        // Delay 
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
    velocityParam = params.velocity;
    shapeParam =    params.shape;
    noiseParam =    params.noise;

    //ampTableIncrement = mMap(params.noise, 64.f, 0.1f); //64, 4
    ampTableIncrement = 8.f;

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

    // Read shape table
    if (tableActive)
    {
        shapeParam = mClamp(shapeParam, 0.f, 1.f);

        float tablePosition = shapeParam * (NUM_TABLES - 1);

        int typeA = static_cast<int>(std::floorf(tablePosition));
        int typeB = static_cast<int>(std::ceilf(tablePosition));

        float typeFrac = tablePosition - typeA;

        int index0 = static_cast<int>(ampTablePhase);
        int index1 = index0 + 1;

        float frac = ampTablePhase - index0;

        if (index1 < TABLE_LEN)
        {
            float sampleA = mInterp(frac, shapeTable[typeA][index0], shapeTable[typeA][index1]);
            float sampleB = mInterp(frac, shapeTable[typeB][index0], shapeTable[typeB][index1]);

            excitation = mInterp(typeFrac, sampleA, sampleB);
            ampTablePhase += ampTableIncrement;
        }
        else
        {
            tableActive = false;
            excitation = 0.f;
        }
    }

    // Apply noise
    // TODO: Just have a noisy turkey window shape, nah lets go with some granular idea
    //float noise = rack::random::uniform() * 2.0f - 1.0f;
    //excitation += excitation * noise; //* 0.7f; // noiseParam;

    excitation *= velocityParam;

    // Write into delay buffers
    exciterDelayBuffer[writeIdx] = excitation;
}


void Exciter::reset() {

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