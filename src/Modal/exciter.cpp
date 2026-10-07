#include "exciter.hpp"


// Granular exciter
Exciter::Exciter() {
    setSampleRate(sr);
    buildTables();

    velocityFilter.setLowpass(sr * 0.49f); // TODO: set a good default value
}

void Exciter::setSampleRate(int newSampleRate) {
    sr = mClamp(newSampleRate, 20, static_cast<int>(sr * 0.5f));
    velocityFilter.setSampleRate(sr);
    amplitudeADR.setSampleRate(sr);
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
    // Density Dial states
    // Full pluck (0 - 0.33)
    // Damped pluck (0.33 - 0.5);

    // Pluck
    float pluckGain = 2.f;
    if (densityParam > 0.33f)
        pluckGain = 1.f - mRemap(densityParam, 0.33f, 0.5f);
    
    // Pluck first grain
    grain[0].active = true;
    grain[0].phase = 0.f;
    grain[0].speed = 8.f;
    grain[0].texture = textureParam;
    grain[0].gain = pluckGain;


    // Granular bow / blow
    grainGain = mRemap(densityParam, 0.f, 0.33f);   // Grains fade in

    // Short attack, short decay 0 - 33
    // short attack, long medium decay 33 - 50
    // medium attack, short decay, low level, long release, 50 - 66
    // long attack, long decay, high level, short release? 66 - 75
    // 75 - 100 ? 
    
    float attack = 0.1f;
    float decay = 0.7f;
    float decayLevel = 0.5f;
    float release = 0.5f;

    if (densityParam <= 0.33f) {
        //float param = mRemap(densityParam, 0.f, 0.33f);

        decay = 0.5f;
    }
    else if (densityParam > 0.33f && densityParam <= 0.5f) {
        decay = 2.75f;
    }

    amplitudeADR.setParams(ADR::Params(attack, decay, decayLevel, release));
    amplitudeADR.trigger();
}

void Exciter::spawnGrain()
{
    // Modulatables:
    // Shape,
    // PlaybackSpeed,
    // Gain,

    const float shapeVarianceAmount = 0.05f; // 5%, 10%, 15%?
    const float playbackSpeedVarianceAmount = 0.3f;
    const float grainSpeedTemp = 3.5f; //TODO: think I want to control from somewhere else...
    // Make grain speed dependant on module pitch input?
        
    // Granular bow / blow
    for (int channel = 1; channel < MAX_CHANNELS; channel++)
    {
        // Find inactive channel
        if (!grain[channel].active)
        {
            // Set grain parameters
            grain[channel].gain = grainGain * 0.25f; // 1 / (MAX_CHANNEL * 0.5)

            float shapeVariance = (rack::random::uniform() * 2.f - 1.f) * shapeVarianceAmount;
            grain[channel].texture = mClamp(textureParam + shapeVariance, 0.f, 1.f);
            
            float playbackSpeedVariance = (rack::random::uniform() * 2.f - 1.f) * playbackSpeedVarianceAmount;
            grain[channel].speed = mClamp(grainSpeedTemp + playbackSpeedVariance, 1.f, 8.f);

            grain[channel].phase = 0.f;
            grain[channel].active = true;
            return;
        }
    }
}

void Exciter::process(float deltaTime, ExciterParams& p) {
    densityParam = p.density;
    textureParam = p.texture;

    // Spawn grains
    //spawnPhase += deltaTime; 
    spawnPhase += spawnRate / sr; // todo use delta time

    if (spawnPhase >= 1.f)
    {
        spawnPhase -= 1.f;

        float probability = 0.25f;

        if (rack::random::uniform() < probability)
            spawnGrain();
    }

    // Process active grains
    output = 0.f;
    float amplitude = amplitudeADR.process();

    for (int channel = 0; channel < MAX_CHANNELS; channel++)
    {
        if (!grain[channel].active)
            continue;

        float tablePosition = grain[channel].texture * (NUM_TABLES - 1);

        int typeA = static_cast<int>(std::floorf(tablePosition));
        int typeB = static_cast<int>(std::ceilf(tablePosition));

        float typeFrac = tablePosition - typeA;

        int index0 = static_cast<int>(grain[channel].phase);
        int index1 = index0 + 1;

        float frac = grain[channel].phase - index0;

        if (index1 < TABLE_LEN)
        {
            float sampleA = mInterp(frac, table[typeA][index0], table[typeA][index1]);
            float sampleB = mInterp(frac, table[typeB][index0], table[typeB][index1]);
            float wave = mInterp(typeFrac, sampleA, sampleB);
            
            // Bypass amplitude envelope for pluck
            float amplitudeEnvelope = (channel == 0) ? 1.f : amplitude;

            output += wave * grain[channel].gain * amplitudeEnvelope;

            grain[channel].phase += grain[channel].speed;
        }
        else
        {
            grain[channel].active = false;
        }
    }

    // TODO: Mix in highpassed 'Air' noise
    // TODO: have an air param?

    // Velocity lowpass filter
    if (velocityParam != p.velocity) {
        velocityParam = mClamp(p.velocity, 0.f, 1.f);
        // TODO: is this the best cutoff mapping...
        float cutoffHz = 20.f * std::pow(20000.f / 20.f, velocityParam);
        velocityFilter.setLowpass(cutoffHz);
        velocityParam = p.velocity;
    }
      
    output = velocityFilter.process(output);
}

float Exciter::get() {
    return output;
}