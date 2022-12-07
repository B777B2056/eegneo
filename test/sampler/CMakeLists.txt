cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler VERSION 0.9.0 LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.4.1/msvc2019_64")

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")
add_compile_definitions("/O2")

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts SerialPort)

file(GLOB SOURCES "*.cpp" "../../app/src/acquisition/sampler.cpp")
file(GLOB UI "*.ui")

add_executable(eegneo_sampler ${SOURCES})
target_link_libraries(eegneo_sampler PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts Qt6::SerialPort)
