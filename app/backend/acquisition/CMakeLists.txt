cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler)

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort Network Concurrent)

include_directories("inc")

file(GLOB HEADERS "../../utils/ipc.h")
file(GLOB SOURCES "src/*.cpp" "../../utils/*.cpp" "../../third/fft/*.cpp" "../../third/edf/edflib.c")

link_directories("../../third/usb" "../../third/iir/lib")
add_executable(eegneo_sampler "../../resource.qrc" ${HEADERS} ${SOURCES})
target_link_libraries(eegneo_sampler PRIVATE Qt6::Core Qt6::SerialPort Qt6::Network Qt6::Concurrent iir_static libusb)
