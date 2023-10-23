#include "base.h"
#include "emu.h"
#include <SDL.h>
#include <fstream>
#include <iostream>

namespace ez {

static void sdl_assert(int rc) {
    if (rc) {
        fail("SDL Error: {}", SDL_GetError());
    }
}

static KeypadInput get_keys() {
    KeypadInput keysDown = 0;
    auto keystateSize = 0;
    const auto keystate = SDL_GetKeyboardState(&keystateSize);
    const auto isKeyDown = [&keystate](SDL_KeyCode keycode) { return static_cast<bool>(keystate[SDL_GetScancodeFromKey(keycode)]); };
    keysDown |= isKeyDown(SDLK_x) ? 0b1 << 0x0 : 0;
    keysDown |= isKeyDown(SDLK_1) ? 0b1 << 0x1 : 0;
    keysDown |= isKeyDown(SDLK_2) ? 0b1 << 0x2 : 0;
    keysDown |= isKeyDown(SDLK_3) ? 0b1 << 0x3 : 0;
    keysDown |= isKeyDown(SDLK_q) ? 0b1 << 0x4 : 0;
    keysDown |= isKeyDown(SDLK_w) ? 0b1 << 0x5 : 0;
    keysDown |= isKeyDown(SDLK_e) ? 0b1 << 0x6 : 0;
    keysDown |= isKeyDown(SDLK_a) ? 0b1 << 0x7 : 0;
    keysDown |= isKeyDown(SDLK_s) ? 0b1 << 0x8 : 0;
    keysDown |= isKeyDown(SDLK_d) ? 0b1 << 0x9 : 0;
    keysDown |= isKeyDown(SDLK_z) ? 0b1 << 0xA : 0;
    keysDown |= isKeyDown(SDLK_c) ? 0b1 << 0xB : 0;
    keysDown |= isKeyDown(SDLK_4) ? 0b1 << 0xC : 0;
    keysDown |= isKeyDown(SDLK_r) ? 0b1 << 0xD : 0;
    keysDown |= isKeyDown(SDLK_f) ? 0b1 << 0xE : 0;
    keysDown |= isKeyDown(SDLK_v) ? 0b1 << 0xF : 0;
    if (keysDown) {
        log_info("Keypad {}", keysDown);
    }
    return keysDown;
}

static void runApplication() {
    size_t romIdx = 0;
    std::vector<std::filesystem::path> roms;
    for (const auto& file : std::filesystem::directory_iterator("./roms")) {
        const auto& path = file.path();
        if (path.extension() == ".ch8") {
            log_info("Found rom: {}", path.c_str());
            roms.push_back(path);
        }
    }
    std::sort(roms.begin(), roms.end());
    assert(roms.size() > 0);

    bool paused = false;
    const auto loadRom = [&](const std::filesystem::path& romPath) {
        log_info("Loading rom {}", romPath.c_str());
        const auto romSize = std::filesystem::file_size(romPath);
        auto is = std::ifstream(romPath, std::ios::binary);
        auto rom = std::vector<uint8_t>(romSize);
        is.read(reinterpret_cast<char*>(rom.data()), romSize);

        auto emu = Emu(rom.data(), rom.size());
        emu.setPause(paused);
        return emu;
    };

    auto emu = loadRom(roms.front());

    sdl_assert(SDL_Init(SDL_INIT_EVERYTHING));
    // todo, raii
    auto window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 500, SDL_WINDOW_RESIZABLE );
    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // SDL_PIXELFORMAT_RGB888 is 4 bytes per pixel - alpha is always 255
    const auto bytesPerPx = 4;
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, Display::WIDTH_PX, Display::HEIGHT_PX);
    if (!texture) {
        log_error("{}", SDL_GetError());
    }

    auto event = SDL_Event{};
    bool shouldExit = false;
    while (!shouldExit) {

        bool singleStep = false;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                shouldExit = true;
                break;
            case SDL_KEYDOWN:
                // keypad for emulator is polled lower
                switch (event.key.keysym.sym) {
                case SDLK_TAB:
                    romIdx = (romIdx + 1) % roms.size();
                    emu = loadRom(roms[romIdx]);
                    break;
                case SDLK_SPACE:
                    paused = !paused;
                    emu.setPause(paused);
                    break;
                case SDLK_RIGHT:
                    emu.setPause(false);
                    singleStep = true;
                    break;
                }
                break;
            default:
                break;
            }
        }

        const auto keysDown = get_keys();
        emu.tick(keysDown);
        if (singleStep) {
            emu.setPause(true);
            paused = true;
            singleStep = false;
        }

        sdl_assert(SDL_RenderClear(renderer));
        uint8_t* pixels = nullptr;
        int pitch = 0;
        sdl_assert(SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch));
        memset(pixels, 0, pitch * Display::HEIGHT_PX);
        const auto srcPixels = emu.getDisplay().data();
        for (auto y = 0; y < Display::HEIGHT_PX; ++y) {
            const auto dstRowPtr = pixels + (y * pitch);
            const auto srcRowPtr = srcPixels + (y * Display::WIDTH_PX);
            for (auto x = 0; x < Display::WIDTH_PX; ++x) {
                const auto dstPtr = dstRowPtr + (bytesPerPx * x);
                for (auto px = 0; px < bytesPerPx; ++px) {
                    dstPtr[px] = srcRowPtr[x];
                }
            }
        }

        SDL_UnlockTexture(texture);
        sdl_assert(SDL_RenderCopy(renderer, texture, nullptr, nullptr));
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    log_info("Goodbye");
}

} // namespace ez

int main(int, char**) {

    ez::runApplication();
    return 0;
}
