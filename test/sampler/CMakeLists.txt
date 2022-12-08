cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler_test VERSION 0.9.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

include_directories("../../app/inc")

file(GLOB SOURCES "*.cpp")
file(GLOB UI "*.ui")

add_executable(eegneo_sampler_test ${SOURCES} "../../app/src/acquisition/wave_plotter.cpp")
target_link_libraries(eegneo_sampler_test PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts)
