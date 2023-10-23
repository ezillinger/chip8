#include "audio.h"
#include <SDL.h>

namespace ez {

static constexpr int SAMPLE_RATE = 44'100;
static constexpr int FORMAT = AUDIO_F32;
static constexpr int BUFFER_SIZE = 2048;

static void audioCallback(void* userdata, uint8_t* stream, int len) {
    auto& synth = *reinterpret_cast<Audio*>(userdata);
    for (auto i = 0; i < len / int(sizeof(float)); ++i) {
        reinterpret_cast<float*>(stream)[i] = synth.getNextSample();
    }
}

Audio::Audio() {
    SDL_AudioSpec spec{};
    spec.freq = SAMPLE_RATE;
    spec.format = FORMAT;
    spec.samples = BUFFER_SIZE;
    spec.callback = audioCallback;
    spec.userdata = this;
    assert(0 == SDL_OpenAudio(&spec, nullptr));
    SDL_PauseAudio(0);
}

Audio::~Audio() { SDL_CloseAudio(); }

float Audio::getNextSample() { 

    // reduce/increase amplitude when pausing/unpausing instead of cutting off abruptly
    static constexpr float amplitudeRamp = 10.0f;
    static constexpr float maxAmplitude = 0.4f;
    const auto dt = 1.0f / SAMPLE_RATE;
    if(m_pause){
        m_amplitudeScale -= amplitudeRamp * dt;
    }
    else{
        m_amplitudeScale += amplitudeRamp * dt;
    }
    m_amplitudeScale = clamp(m_amplitudeScale, 0.0f, 1.0f);

    const auto time = m_sampleIdx++ * dt;
    static constexpr auto frequencies = std::array<float, 3>{110.0f, 130.81, 164.81}; // Am
    float sample = 0.0f;
    for (auto f : frequencies) {
        sample += sinf(float(2.0 * float(M_PI) * f * time));
    }
    return sample * maxAmplitude * m_amplitudeScale / float(frequencies.size());
}

} // namespace ez