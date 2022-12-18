cmake_minimum_required(VERSION 3.24)
project(eegneo_erp VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

include_directories("inc")

file(GLOB HEADERS "inc/*.h")
file(GLOB SOURCES "src/*.cpp" "../../utils/ipc.cpp" "../../utils/config.cpp")
file(GLOB UI "ui/*.ui")

qt_add_executable(eegneo_erp "../../resource.qrc" ${HEADERS} ${UI} ${SOURCES})
target_link_libraries(eegneo_erp PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)