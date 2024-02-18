cmake_minimum_required(VERSION 3.14)  # CMake version check
project(tests)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_ARCHITECTURES "x86_64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -mtls-direct-seg-refs -march=core-avx2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -mtls-direct-seg-refs -march=core-avx2")

add_executable(tests
    Tests.cpp
    TestCompiler.cpp
    Harness.cpp
    ../memmanager.cpp
    ../Compiler/Compiler.cpp
    ../Instructions/Instruction.cpp
    ../Instructions/Operand.cpp
    ../utils.c
    ../printinstr.c
    )
target_include_directories(tests PRIVATE ../../xed/kits/xed/include)
target_link_directories(tests PRIVATE ../../xed/kits/xed/lib)
target_link_libraries(tests PRIVATE xed)