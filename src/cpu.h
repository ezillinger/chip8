#pragma once
#include "base.h"

namespace ez
{

    class Display
    {
    public:
        static constexpr int WIDTH_PX = 64;
        static constexpr int HEIGHT_PX = 32;
        void clear() { memset(m_display.data(), 0, m_display.size()); }

        bool write(uint8_t x, uint8_t y, bool val); // returns true if value is erased, clips if out of bounds

        void dump();

        const uint8_t *data() const
        {
            return m_display.data();
        }

    private:
        std::array<uint8_t, WIDTH_PX * HEIGHT_PX> m_display{};
    };

    class Cpu
    {
    public:
        Cpu(const uint8_t *program, size_t size);

        bool should_exit() const
        {
            return false;
        }

        void tick();
        const Display &getDisplay()
        {
            return m_display;
        }

    private:
        using OpCode = uint16_t;
        OpCode fetch();

        std::array<uint8_t, 16> m_reg_v{};
        uint16_t m_reg_i = 0;
        uint16_t m_pc = 0;
        uint8_t m_sp = 0;
        uint8_t m_timer = 0;
        uint8_t m_sound_timer = 0;

        static constexpr int MEM_SIZE_BYTES = 4096;
        std::array<uint8_t, MEM_SIZE_BYTES> m_memory{};
        std::vector<uint16_t> m_stack{};

        Display m_display{};
    };
}