#include "emu.h"

namespace ez {

    static const std::array<uint8_t, 0x80> font = {
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
    static const int BYTES_PER_FONT_GLYPH = 5;

    Emu::Emu(const uint8_t* program, size_t size) {

        memcpy(m_memory.data(), font.data(), font.size());

        const auto programStart = 0x200;
        assert(size < MEM_SIZE_BYTES - programStart);
        memcpy(m_memory.data() + programStart, program, size);
        m_pc = programStart;
}

Emu::OpCode Emu::fetch() {
    auto bytes = (m_memory.data() + m_pc);
    uint16_t code = (bytes[0] << 8) | bytes[1];
    m_pc += 2;
    assert(m_pc < MEM_SIZE_BYTES);
    return code;
}

void Emu::tick(KeypadInput keysDown) {

    static constexpr chrono::nanoseconds timerDuration = chrono::duration_cast<chrono::nanoseconds>(16.66666ms);
    const auto now = chrono::steady_clock::now();
    while ((now - m_timerStart) > timerDuration) {
        m_timerStart += timerDuration;
        if(m_delayTimer > 0){
            --m_delayTimer;
        }
        if(m_soundTimer > 0){
            --m_soundTimer;
        }
    }

    if(m_waitingForKeypressRegIdx){
        for(int i = 0; i < 16; ++i){
            if(0 != (keysDown & (0b1 << i))){
                m_regV[*m_waitingForKeypressRegIdx] = i;
                m_waitingForKeypressRegIdx = {};
                return;
            }
        }
        return;
    }

    const auto opCode = fetch();
    const auto lowByte = 0xFF & opCode;
    const auto nib3 = (0xF000 & opCode) >> 12;
    const auto nib2 = (0x0F00 & opCode) >> 8;
    const auto nib1 = (0x00F0 & opCode) >> 4;
    const auto nib0 = (0x000F & opCode);

    auto& vx = m_regV[nib2];
    const auto vy = m_regV[nib1];
    auto& flags = m_regV[0xF];

    log_info("Opcode {:x}", opCode);

    switch (nib3) {
    case 0x0: {
        if (opCode == 0x00E0) {
            // clear display
            m_display.clear();
        } else if (opCode == 0x00EE) {
            // return
            assert(!m_stack.empty());
            m_pc = m_stack.back();
            m_stack.pop_back();
        } else {
            log_info("Ignoring SYS opcode: {}", opCode);
        }
        break;
    }
    case 0x1: // jmp
        m_pc = opCode & 0x0FFF;
        break;
    case 0x2: // call _NNN
        m_stack.push_back(m_pc);
        m_pc = opCode & 0x0FFF;
        break;
    case 0x3: // skip equal _xkk
        if (vx == (0x00FF & opCode)) {
            m_pc += 2;
        }
        break;
    case 0x4: // skip not equal _xkk
        if (vx != (0x00FF & opCode)) {
            m_pc += 2;
        }
        break;
    case 0x5: // skip equal reg v _xy_
        assert(nib0 == 0);
        if (vx == vy) {
            m_pc += 2;
        }
        break;
    case 0x6: // set _XNN
        vx = static_cast<uint8_t>(0x00FF & opCode);
        break;
    case 0x7: // add _xNN
        vx += static_cast<uint8_t>(0x00FF & opCode);
        break;
    case 0x8: // math - all opcodes are _xy_
    {
        switch (nib0) {
        case 0x0: // ld vx->vy
            vx = vy;
            break;
        case 0x1: // or vx vy
            vx = vy | vx;
            break;
        case 0x2: // and vx vy
            vx = vy & vx;
            break;
        case 0x3: // xor vx vy
            vx = vy ^ vx;
            break;
        case 0x4: // add vx vy
        {
            const bool carry = (int(vx) + vy) > 0xFF;
            vx = vy + vx;
            flags = carry ? 1 : 0;
            break;
        }
        case 0x5: // sub vx vy
        {
            bool borrow = vy > vx;
            vx = vx - vy;
            flags = borrow ? 0 : 1;
            break;
        }
        case 0x6: // shr vx vy
        {
            bool vxOdd = 0b1 & vx;
            vx = vx >> 1;
            flags = vxOdd ? 1 : 0;
            break;
        }
        case 0x7: // subn vx vy
        {
            bool borrow = vx > vy;
            vx = vy - vx;
            flags = borrow ? 0 : 1;
            break;
        }
        case 0xE: {
            bool vxHighBitSet = 0b1000'0000 & vx;
            vx = vx << 1;
            flags = vxHighBitSet ? 1 : 0;
            break;
        }
        default:
            fail("Invalid opcode");
            break;
        }
    } break;
    case 0x9: // SNE vx vy _xy_
        assert(nib0 == 0);
        if (vx != vy) {
            m_pc += 2;
        }
        break;
    case 0xA: // set I _NNN
        m_regI = 0x0FFF & opCode;
        break;
    case 0xB: // jmp v0 + _NNN
        m_pc = m_regV[0] + (0x0FFF & opCode);
        break;
    case 0xC: // rand vx AND kk _xkk
        vx = rand() & lowByte;
        break;
    case 0xD: // draw _xyn
    {
        uint8_t x_start = vx % Display::WIDTH_PX;
        uint8_t y_start = vy % Display::HEIGHT_PX;
        bool erasedPixel = false;
        for (auto y_offset = 0; y_offset < nib0; ++y_offset) {
            auto sprite_byte = m_memory[m_regI + y_offset];
            for (auto x_offset = 0; x_offset < 8; ++x_offset) {
                bool bitVal = (0x1 << (7 - x_offset)) & sprite_byte;
                erasedPixel |= m_display.write(x_start + x_offset, y_start + y_offset, bitVal);
            }
        }
        flags = erasedPixel ? 1 : 0;
        // m_display.dump();
        break;
    }
    case 0xE: // keyboard input
        if ((lowByte) == 0x9E) {
            // skip vx _x__
            if (((keysDown && 0b1) << vx) != 0) {
                m_pc += 2;
            }
        } else {
            // skipn vx _x__
            assert((lowByte) == 0xA1);
            if (((keysDown && 0b1) << vx) == 0) {
                m_pc += 2;
            }
        }
        break;
    case 0xF: // timers
        switch (lowByte) {
        case 0x07: // ld vx dt
            vx = m_delayTimer;
            break;
        case 0x0A: // wait for any keypress, store in vx
            m_waitingForKeypressRegIdx = nib2;
            break;
        case 0x15: // ld dt vx
            m_delayTimer = vx;
            break;
        case 0x18: // ld st vx
            m_soundTimer = vx;
            break;
        case 0x1E: // add I vx
            m_regI += vx;
            break;
        case 0x29: // ld F vx - set I to address of font glyph stored in vx
            m_regI = vx * BYTES_PER_FONT_GLYPH;
            break;
        case 0x33: // ld B vx - set I - I + 2 to decimal representation of vx
            m_memory[m_regI] = vx / 100;
            m_memory[m_regI + 1] = vx / 10;
            m_memory[m_regI + 2] = vx % 10;
            break;
        case 0x55: // ld [I] vx
            for(auto i = 0; i <= vx; ++i){
                m_memory[m_regI + i] = m_regV[i];
            }
            break;
        case 0x65: // ld vx [I]
            for(auto i = 0; i <= vx; ++i){
                m_regV[i] = m_memory[m_regI + i];
            }
            break;
        default:
            fail("Invalid opcode");
        }
        break;
    default:
        fail("Invalid opcode: {:x}", opCode);
    }
}

bool Display::write(uint8_t x, uint8_t y, bool newVal) {
    if (!(x < WIDTH_PX && y < HEIGHT_PX)) {
        // clipping is ok
        log_info("Clipped px {} {}", x, y);
        return false;
    }

    auto& val = m_frameBuffer[x + y * WIDTH_PX];
    const bool isSet = val;
    val = isSet ^ newVal ? 0xFF : 0x00;
    return isSet;
}

} // namespace ez