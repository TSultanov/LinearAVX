cmake_minimum_required(VERSION 3.14)  # CMake version check
project(avxemulator)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_ARCHITECTURES "x86_64")

add_library(avxhandler SHARED
    handler.c
    handler.h
    encoder.cpp
    encoder.h
    memmanager.cpp
    memmanager.h
    Instructions/Instructions.h
    Instructions/Instruction.h
    Instructions/Instruction.cpp
    Instructions/Operand.h
    Instructions/Operand.cpp
    Instructions/VMOVUPS.h
    Instructions/VMOVSS.h
    Instructions/VXORPS.h
    )
target_include_directories(avxhandler PRIVATE ../xed/kits/xed/include)
target_link_directories(avxhandler PRIVATE ../xed/kits/xed/lib)
target_link_libraries(avxhandler PRIVATE xed)