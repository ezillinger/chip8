#pragma once
#include "base.h"

namespace ez {

// bit mask, bit 0 - 16 indicate the corresponding key is down
using KeypadInput = uint32_t;

class Display {
  public:
    static constexpr int WIDTH_PX = 64;
    static constexpr int HEIGHT_PX = 32;
    void clear() { memset(m_frameBuffer.data(), 0, m_frameBuffer.size()); }

    // returns true if value is erased, clips if out of bounds
    bool write(uint8_t x, uint8_t y,
               bool val); 

    const uint8_t* data() const { return m_frameBuffer.data(); }

  private:
    std::array<uint8_t, WIDTH_PX * HEIGHT_PX> m_frameBuffer{};
};

class Emu {
  public:
    Emu(const uint8_t* program, size_t size);

    bool shouldExit() const { return false; }

    void tick(KeypadInput keysDown);
    const Display& getDisplay() { return m_display; }

    void setPause(bool p) { m_pause = p; }
    bool isPaused() const { return m_pause; }

    bool shouldPlaySound() { return m_soundTimer > 0; }

  private:

    using OpCode = uint16_t;
    void runOneInstruction(KeypadInput keysDown);
    OpCode fetchInstruction();

    bool m_pause = false;

    struct KeyWaitInfo {
        uint8_t m_regIdx = 0;
        KeypadInput m_keysDownLastTick = 0;
    };
    // if this exists we're waiting for a keypress, to be stored in vx
    std::optional<KeyWaitInfo> m_waitingForKeypressRegIdx;

    std::array<uint8_t, 16> m_regV{};
    uint16_t m_regI = 0;
    uint16_t m_pc = 0;
    uint8_t m_sp = 0;
    uint8_t m_delayTimer = 0;
    uint8_t m_soundTimer = 0;

    static constexpr int MEM_SIZE_BYTES = 4096;
    std::array<uint8_t, MEM_SIZE_BYTES> m_memory{};
    std::vector<uint16_t> m_stack{};

    chrono::steady_clock::time_point m_lastTickTime = chrono::steady_clock::now();
    chrono::nanoseconds m_timeElapsedSinceLastTimerTick = 0ns;
    chrono::nanoseconds m_timeElapsedSinceLastInstruction = 0ns;

    // should saving/loading registers to memory increment regI
    bool m_legacyMemoryIncrement = true;
    // should shift load vy into vx first
    bool m_legacyShift = true;
    // should jump use v0 + _nnn offset (or newer vx + _xnn) if false
    bool m_legacyJump = true;

    Display m_display{};
};
} // namespace ez