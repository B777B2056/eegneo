cmake_minimum_required(VERSION 3.24)
project(eegneo VERSION 0.9.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts SerialPort Network)

file(GLOB SOURCES "src/*/*.cpp" "src/*.cpp")
file(GLOB UI "ui/*.ui")

add_executable(eegneo ${UI} ${SOURCES})
target_link_libraries(eegneo PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts Qt6::SerialPort Qt6::Network libfftw3.a)
