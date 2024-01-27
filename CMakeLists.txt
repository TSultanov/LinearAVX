cmake_minimum_required(VERSION 3.14)  # CMake version check
project(avxemulator)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_OSX_ARCHITECTURES "x86_64")

add_library(avxhandler SHARED
    handler.c 
    handler.h
    encoder.c
    encoder.h)
target_include_directories(avxhandler PRIVATE ../xed/kits/xed/include)
target_link_directories(avxhandler PRIVATE ../xed/kits/xed/lib)
target_link_libraries(avxhandler PRIVATE xed)