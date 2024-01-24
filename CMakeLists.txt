cmake_minimum_required(VERSION 3.14)  # CMake version check
project(avxemulator)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_OSX_ARCHITECTURES "x86_64")

include(ExternalProject)

set(ExternalBuildcommandArgs "clang -target x86_64-darwin-macho")
ExternalProject_Add(project_injector
  GIT_REPOSITORY https://github.com/kubo/injector.git
  GIT_TAG        998349585c5ad1c48c919118ca533e9c4370ad1a
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make CC=${ExternalBuildcommandArgs}
  BUILD_IN_SOURCE true
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/project_injector-prefix/src/project_injector/src/macos/libinjector.dylib
  )
ExternalProject_Get_Property(project_injector SOURCE_DIR)
set(injector_BINARY_DIR "${SOURCE_DIR}/src/macos")
set(injector_SOURCE_DIR "${SOURCE_DIR}/include")

add_library(injector STATIC IMPORTED)
add_dependencies(injector project_injector)
set_property(TARGET injector PROPERTY IMPORTED_LOCATION ${injector_BINARY_DIR}/libinjector.dylib)

add_library(avxhandler SHARED handler.c handler.h)

add_executable(inject main.c)
add_dependencies(inject injector)
target_include_directories(inject PUBLIC ${injector_SOURCE_DIR})
target_link_libraries(inject PRIVATE ${injector_BINARY_DIR}/libinjector.dylib)