cmake_minimum_required(VERSION 3.14)
project(testapp)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -march=haswell")

add_executable(testapp main.c)