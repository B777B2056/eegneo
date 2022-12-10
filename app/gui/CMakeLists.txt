cmake_minimum_required(VERSION 3.24)
project(eegneo VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

include_directories("inc")

file(GLOB HEADERS "inc/*.h" "inc/*/*.h")
file(GLOB SOURCES "src/mainwindow.cpp" "src/*/*.cpp" "../utils/filter.cpp")
file(GLOB EDFLIB_SOURCES "../third/edflib/*.c")
file(GLOB UI "ui/*.ui")

add_executable(eegneo ${HEADERS} ${UI} "src/main.cpp" ${SOURCES} ${EDFLIB_SOURCES})
target_link_libraries(eegneo PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts Qt6::Network)
