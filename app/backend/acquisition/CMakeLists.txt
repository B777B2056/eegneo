cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler)

add_definitions(-D_CMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")
# message( status "this var key = ${CMAKE_SOURCE_DIR}.")

set(PY_ROOT_PATH "E:/anaconda3/envs/qtcpp_env")

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort Network)

include_directories("inc")
include_directories("${PY_ROOT_PATH}/include")

file(GLOB SOURCES "src/*.cpp" "${UTILS_PATH}/*.cpp" "${3PARTY_ROOT_PATH}/fft/*.cpp" "${3PARTY_ROOT_PATH}/edf/edflib.c")

link_directories("${3PARTY_ROOT_PATH}/usb" "${3PARTY_ROOT_PATH}/iir/lib" "${PY_ROOT_PATH}/libs")
add_executable(eegneo_sampler "../../resource.qrc" ${SOURCES})
target_link_libraries(eegneo_sampler PRIVATE Qt6::Core Qt6::SerialPort Qt6::Network iir_static libusb python310)
