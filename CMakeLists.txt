cmake_minimum_required(VERSION 3.24)
project(eegneo VERSION 0.9.0 LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.4.1/msvc2019_64")

set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
endif()

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(Python3_ROOT_DIR "${CMAKE_SOURCE_DIR}/app/pyenv")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")

add_definitions(-D_TOPO_PIC_PATH=\"${CMAKE_SOURCE_DIR}/app/resource/Images/eegtopo.png\")
add_definitions(-D_BACKEND_EXE_PATH=\"${Python3_ROOT_DIR}/eegneo_sampler.exe\")

add_subdirectory("app")
# add_subdirectory("test")
