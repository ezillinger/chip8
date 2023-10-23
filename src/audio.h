#pragma once
#include "base.h"

namespace ez {

class Audio {
  public:
    Audio();
    ~Audio();

    Audio(Audio&) = delete;
    Audio(Audio&&) = delete;

    float getNextSample();

    void setPause(bool p) { m_pause = p; }
    bool isPaused() { return m_pause; }

  private:
    bool m_pause = false;

    float m_amplitudeScale = 0.0f;

    size_t samplesSincePause = 0;
    size_t m_sampleIdx = 0;
};
} // namespace ez