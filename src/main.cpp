#include "base.h"

#include "cpu.h"
#include <iostream>
#include <fstream>

#include <SDL.h>

void sdl_assert(int rc) {
    if(rc){
        ez::log_error("SDL Error: {}", SDL_GetError());
        assert(0);
    }
}

int main(int, char **)
{

    using namespace ez;

    const auto romPath = "./roms/1-chip8-logo.ch8";
    //const auto romPath = "./roms/2-ibm-logo.ch8";

    log_info("CWD {}", std::filesystem::current_path().c_str());
    assert(std::filesystem::exists(romPath));
    const auto romSize = std::filesystem::file_size(romPath);
    auto is = std::ifstream(romPath, std::ios::binary);
    auto rom = std::vector<uint8_t>(romSize);
    is.read(reinterpret_cast<char *>(rom.data()), romSize);

    auto cpu = Cpu(rom.data(), rom.size());

    assert(0 == SDL_Init(SDL_INIT_EVERYTHING));
    // ain't nobody got time to destroy these, that's what my OS is for
    auto window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 500, SDL_WINDOW_RESIZABLE);
    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, Display::WIDTH_PX, Display::HEIGHT_PX);
    if(!texture){
        log_error("{}", SDL_GetError());
    }

    auto event = SDL_Event{};
    bool shouldExit = false;
    while (!shouldExit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                shouldExit = true;
                break;

            default:
                break;
            }
        }
        cpu.tick();

        sdl_assert(SDL_RenderClear(renderer));
        uint8_t *pixels = nullptr;
        int pitch = 0;
        sdl_assert(SDL_LockTexture(texture, nullptr, reinterpret_cast<void **>(&pixels), &pitch));
        memset(pixels, 0, pitch * Display::HEIGHT_PX);
        const auto bytesPerPx = 4;
        const auto srcPixels = cpu.getDisplay().data();
        for (int y = 0; y < Display::HEIGHT_PX; ++y)
        {
            const auto dstRowPtr = pixels + (y * pitch);
            const auto srcRowPtr =  srcPixels + (y * Display::WIDTH_PX);
            for (int x = 0; x < Display::WIDTH_PX; ++x)
            {
                const auto dstPtr = dstRowPtr + (bytesPerPx * x);
                for (int px = 0; px < bytesPerPx; ++px)
                {
                    dstPtr[px] = srcRowPtr[x];
                }
            }
        }

        SDL_UnlockTexture(texture);
        sdl_assert(SDL_RenderCopy(renderer, texture, nullptr, nullptr));
        SDL_RenderPresent(renderer);
    }
    log_info("Goodbye");
}
