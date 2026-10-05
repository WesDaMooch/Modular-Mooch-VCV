#include "exciter.hpp"


// Granular exciter
Exciter::Exciter() {
    setSampleRate(sr);
    buildTables();

    velocityFilter.setLowpass(sr * 0.49f); // TODO: set a good default value

    // Set default values
    for (size_t channel = 0; channel < MAX_CHANNELS; channel++) {
        shape[channel] = 0.5;
        grainPlaybackSpeed[channel] = 0.01f;
    }
}

void Exciter::setSampleRate(int newSampleRate) {
    sr = mClamp(newSampleRate, 20, static_cast<int>(sr * 0.5f));
    velocityFilter.setSampleRate(sr);
    adsr.setSampleRate(sr);
}

void Exciter::buildTables() {
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

        table[tableIdx][i] = window;

        peak = std::max(peak, fabsf(table[0][i]));
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

        table[tableIdx][i] = value;

        peak = std::max(peak, fabsf(value));
    }

    normaliseAmplitudeEnvelope(tableIdx, peak);


    // Hann pulse
    tableIdx++;
    peak = 0.f;

    for (int i = 0; i < TABLE_LEN; i++)
    {
        float hann = 0.5f * (1.f - std::cosf(2.f * M_PI * i / (TABLE_LEN - 1)));
        table[tableIdx][i] = hann;
        peak = std::max(peak, std::fabsf(table[tableIdx][i]));
    }

    normaliseAmplitudeEnvelope(tableIdx, peak);
}

void Exciter::trigger() {
    // Temp stuff just to get one grain to play
    //tablePhase[0] = 0.f;
    //tableActive[0] = true;

    // Some ramp 1 to 0 (can att attach decay later) that decides whether a grain should be played
    // At 1 - play a grain (but dont play all grains)
    // 0 dont play a grain

    // Retrigger ADSRs
    // TODO: add spawnProbability adsr
    // gain adsr, or just use the spawn one?
    adsr.gate(true);

    spawnProbability = 1.f;
    float spawnProbabiltyTime = 1.5f; // seconds

    spawnProbabilityDecay = 1.f / (spawnProbabiltyTime * sr);
    spawnPhase = 0.f;

    // Immediately spawn grain
    //spawnGrain();
}

void Exciter::spawnGrain()
{
    // Modulatables:
    // Shape,
    // PlaybackSpeed,
    // Density,
    // Gain,
    // SpawnRate?

    const float shapeVarianceAmount = 0.05f; // 5%, 10%, 15%?
    const float playbackSpeedVarianceAmount = 0.2f;

    const float grainSpeedTemp = 4.f; //TODO: think I want to control from somewhere else...

    for (int channel = 0; channel < MAX_CHANNELS; channel++)
    {
        // Find inactive channel
        if (!tableActive[channel])
        {
            // Set grain parameters
            float shapeVariance = (rack::random::uniform() * 2.f - 1.f) * shapeVarianceAmount;
            shape[channel] = mClamp(shapeParam + shapeVariance, 0.f, 1.f);
            
            float playbackSpeedVariance = (rack::random::uniform() * 2.f - 1.f) * playbackSpeedVarianceAmount;
            grainPlaybackSpeed[channel] = mClamp(grainSpeedTemp + playbackSpeedVariance, 1.f, 8.f);

            tablePhase[channel] = 0.f;
            tableActive[channel] = true;
            return;
        }
    }
}

void Exciter::process(ExciterParams& p) {
    shapeParam = p.shape;

    // Update spawn envelope
    spawnProbability -= spawnProbabilityDecay;

    if (spawnProbability < 0.f)
        spawnProbability = 0.f;

    // Spawn grains
    spawnPhase += spawnRate / sr; //TODO: * deltaTime

    if (spawnPhase >= 1.f)
    {
        spawnPhase -= 1.f;

        float probability = spawnProbability * density;

        if (rack::random::uniform() < probability)
            spawnGrain();
    }

    // Process active grains
    output = 0.f;

    for (int channel = 0; channel < MAX_CHANNELS; channel++)
    {
        if (!tableActive[channel])
            continue;

        float tablePosition = shape[channel] * (NUM_TABLES - 1);

        int typeA = static_cast<int>(std::floorf(tablePosition));
        int typeB = static_cast<int>(std::ceilf(tablePosition));

        float typeFrac = tablePosition - typeA;

        int index0 = static_cast<int>(tablePhase[channel]);
        int index1 = index0 + 1;

        float frac = tablePhase[channel] - index0;

        if (index1 < TABLE_LEN)
        {
            float sampleA = mInterp(frac, table[typeA][index0], table[typeA][index1]);
            float sampleB = mInterp(frac, table[typeB][index0], table[typeB][index1]);
            float grain = mInterp(typeFrac, sampleA, sampleB);
            output += grain;

            tablePhase[channel] += grainPlaybackSpeed[channel];
        }
        else
        {
            tableActive[channel] = false;
        }
    }

    // TODO: Mix in highpassed 'Air' noise
    // TODO: have an air param?

    // Velocity lowpass filter
    if (velocityParam != p.velocity) {
        velocityParam = mClamp(p.velocity, 0.f, 1.f);
        float cutoffHz = 20.f * std::pow(20000.f / 20.f, velocityParam);
        velocityFilter.setLowpass(cutoffHz);
        velocityParam = p.velocity;
    }

    //output = velocityFilter.process(output) * 0.25f; // 1/ MAX_CHANNELS
    output = adsr.process(); // 1/ MAX_CHANNELS

}

float Exciter::get() {
    return output;
}