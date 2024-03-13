cmake_minimum_required(VERSION 3.26)
project(6502_Emulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(6502_Emulator main_6502.h
        main_6502.cpp)
