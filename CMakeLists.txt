cmake_minimum_required(VERSION 3.14)  # CMake version check
project(avxemulator)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_ARCHITECTURES "x86_64")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -mtls-direct-seg-refs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -mtls-direct-seg-refs")

add_library(avxhandler SHARED
    handler.c
    handler.h
    encoder.cpp
    encoder.h
    decoder.c
    decoder.h
    memmanager.cpp
    memmanager.h
    printinstr.h
    printinstr.c
        Compiler/Compiler.h
    Compiler/Compiler.cpp
    Instructions/Instructions.h
    Instructions/Instruction.h
    Instructions/Instruction.cpp
    Instructions/Operand.h
    Instructions/Operand.cpp
    Instructions/VMOVUPS.h
    Instructions/VMOVSS.h
    Instructions/VXORPS.h
    utils.c
    )
target_include_directories(avxhandler PRIVATE ../xed/kits/xed/include)
target_link_directories(avxhandler PRIVATE ../xed/kits/xed/lib)
target_link_libraries(avxhandler PRIVATE xed)