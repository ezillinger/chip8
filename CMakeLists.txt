cmake_minimum_required(VERSION 3.18.0)
project(chip8 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()

add_executable(chip8 
               src/main.cpp
               src/base.cpp
               src/cpu.cpp
              )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

file(COPY roms DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


find_package(SDL2 REQUIRED)
find_package(SDL2_IMAGE REQUIRED)

#target_include_directories(chip8 PRIVATE ${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR})
target_link_libraries(chip8 SDL2::SDL2 SDL2_image::SDL2_image)