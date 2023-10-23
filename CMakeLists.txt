cmake_minimum_required(VERSION 3.18.0)
project(chip8 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(chip8 
               src/main.cpp
               src/base.cpp
               src/emu.cpp
              )

target_compile_options(chip8 PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

find_package(SDL2 REQUIRED)
target_link_libraries(chip8 SDL2::SDL2)

file(COPY roms DESTINATION ${CMAKE_CURRENT_BINARY_DIR})