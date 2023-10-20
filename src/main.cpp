#include "base.h"

#include "cpu.h"
#include <iostream>
#include <fstream>

int main(int, char**){
    using namespace ez;
    const auto romPath = "./roms/1-chip8-logo.ch8";
    //const auto romPath = "./roms/2-ibm-logo.ch8";

    log_info("CWD {}", std::filesystem::current_path().c_str());
    assert(std::filesystem::exists(romPath));
    const auto romSize = std::filesystem::file_size(romPath);
    auto is = std::ifstream(romPath, std::ios::binary);
    auto rom = std::vector<uint8_t>(romSize);
    is.read(reinterpret_cast<char*>(rom.data()), romSize);

    auto cpu = Cpu(rom.data(), rom.size());
    while(!cpu.should_exit()){
        cpu.tick();
    }
    log_info("Goodbye");
}
