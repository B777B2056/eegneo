cmake_minimum_required(VERSION 3.24)
project(eegneo VERSION 0.9.0 LANGUAGES CXX)

add_definitions(-D_CMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")

set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")

find_package(iir)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

include_directories("inc")

file(GLOB HEADERS "inc/*.h")
file(GLOB SOURCES "src/*.cpp" "${UTILS_PATH}/*.cpp" "${3PARTY_ROOT_PATH}/fft/*.cpp")
file(GLOB EDFLIB_SOURCES "${3PARTY_ROOT_PATH}/third/edf/*.c")
file(GLOB UI "ui/*.ui")

qt_add_executable(eegneo "../../resource.qrc" ${UI} ${HEADERS} ${SOURCES} ${EDFLIB_SOURCES})
target_link_libraries(eegneo PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts Qt6::Network iir::iir_static)
