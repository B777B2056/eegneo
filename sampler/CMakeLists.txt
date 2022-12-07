cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler)

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

include_directories("inc")

file(GLOB SOURCES "src/*.cpp")
add_executable(eegneo_sampler ${SOURCES})
target_link_libraries(eegneo_sampler PRIVATE Qt6::Core Qt6::SerialPort)
