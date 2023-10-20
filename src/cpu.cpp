#include "cpu.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

namespace ez
{

    Cpu::Cpu(const uint8_t *program, size_t size)
    {

        std::array<uint8_t, 0x80> font = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };

        memcpy(m_memory.data(), font.data(), font.size());

        const auto program_start = 0x200;
        assert(size < MEM_SIZE_BYTES - program_start);
        memcpy(m_memory.data() + program_start, program, size);
        m_pc = program_start;
    }

    Cpu::OpCode Cpu::fetch()
    {
        auto bytes = (m_memory.data() + m_pc);
        uint16_t code = (bytes[0] << 8) | bytes[1];
        m_pc += 2;
        assert(m_pc < MEM_SIZE_BYTES);
        return code;
    }

    void Cpu::tick()
    {

        const auto opCode = fetch();
        const auto largeNib = (0xF000 & opCode);
        const auto regIdx = (0x0F00 & opCode) >> 8;
        log_info("Opcode {:x} LN: {:x} REG: {:x}", opCode, largeNib, regIdx);

        switch (largeNib)
        {
        case 0x0000:
        {
            if (opCode == 0x00E0)
            {
                // clear display
                m_display.clear();
            }
            else if (opCode == 0x00EE)
            {
                // return
                assert(!m_stack.empty());
                m_pc = m_stack.back();
                m_stack.pop_back();
            }
            else
            {
                log_info("Ignoring SYS opcode: {}", opCode);
            }
            break;
        }
        case 0x1000: // jmp
            m_pc = opCode & 0x0FFF;
            break;
        case 0x2000: // call _NNN
            m_stack.push_back(m_pc);
            m_pc = opCode & 0x0FFF;
            break;
        case 0x3000: // skip equal _xkk
            if (m_reg_v[regIdx] == 0x00FF & opCode)
            {
                m_pc += 2;
            }
            break;
        case 0x4000: // skip not equal _xkk
            if (m_reg_v[regIdx] != 0x00FF & opCode)
            {
                m_pc += 2;
            }
            break;
        case 0x5000: // skip equal reg v _xy_
            assert(0x000F & opCode == 0);
            if (m_reg_v[regIdx] == m_reg_v[(0x00F0 & opCode) > 4])
            {
                m_pc += 2;
            }
            break;
        case 0x6000: // set _XNN
            m_reg_v[regIdx] = static_cast<uint8_t>(0x00FF & opCode);
            break;
        case 0x7000: // add _xNN
            m_reg_v[regIdx] += static_cast<uint8_t>(0x00FF & opCode);
            break;
        case 0x8000:
            break;
        case 0x9000:
            break;
        case 0xA000: // set I _NNN
            m_reg_i = 0x0FFF & opCode;
            break;
        case 0xB000: // jmp v0 + _NNN
            m_pc = m_reg_v[0] + (0x0FFF & opCode);
            break;
        case 0xC000:
            break;
        case 0xD000: // draw _xyn
        {
            uint8_t n = 0x000F & opCode;
            uint8_t x_start = m_reg_v[(0x0F00 & opCode) >> 8] % Display::WIDTH_PX;
            uint8_t y_start = m_reg_v[(0x00F0 & opCode) >> 4] % Display::HEIGHT_PX;
            bool erased_pixel = false;
            for (auto y_offset = 0; y_offset < n; ++y_offset)
            {
                auto sprite_byte = m_memory[m_reg_i + y_offset];
                for (auto x_offset = 0; x_offset < 8; ++x_offset)
                {
                    bool bitVal = (0x1 << (7 - x_offset)) & sprite_byte;
                    erased_pixel |= m_display.write(x_start + x_offset, y_start + y_offset, bitVal);
                }
            }
            m_reg_v[0xF] = erased_pixel ? 1 : 0;
            m_display.dump();
            break;
        }
        case 0xE000:
            break;
        case 0xF000:
            break;
        default:
            log_error("Unknown opcode: {:x}", opCode);
            break;
        }
    }

    bool Display::write(uint8_t x, uint8_t y, bool new_val)
    {
        if (!(x < WIDTH_PX && y < HEIGHT_PX))
        {
            log_info("Clipped px {} {}", x, y);
            // clipping is ok
            return false;
        }

        auto &val = m_display[x + y * WIDTH_PX];
        const bool is_set = val;
        val = is_set ^ new_val ? 0xFF : 0x00;
        return is_set;
    }

    void Display::dump()
    {
        static int i = 0;
        if (0 == stbi_write_bmp(std::format("display_{}.bmp", i++).c_str(), WIDTH_PX, HEIGHT_PX, 1, m_display.data()))
        {
            log_error("Failed to write image");
        }
    }
}