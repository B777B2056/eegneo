cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler_test VERSION 0.9.0 LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.4.1/msvc2019_64")

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

include_directories("../../app/inc")

file(GLOB SOURCES "*.cpp")
file(GLOB UI "*.ui")

add_executable(eegneo_sampler_test ${SOURCES} "../../app/src/acquisition/wave_plotter.cpp")
target_link_libraries(eegneo_sampler_test PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts)
