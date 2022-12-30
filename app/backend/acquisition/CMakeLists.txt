cmake_minimum_required(VERSION 3.24)
project(eegneo_sampler)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${Python3_ROOT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${Python3_ROOT_DIR}")

add_definitions(-D_Python3_ROOT_DIR=\"${Python3_ROOT_DIR}\")
add_definitions(-D_PYSCRIPT_MODULE_PATH=\"${CMAKE_SOURCE_DIR}/app/scripts/\")

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core SerialPort Network)

include_directories("inc")

file(GLOB SOURCES "src/*.cpp" "${UTILS_PATH}/*.cpp" "${3PARTY_ROOT_PATH}/fft/*.cpp" "${3PARTY_ROOT_PATH}/edf/edflib.c")

link_directories("${3PARTY_ROOT_PATH}/usb" "${3PARTY_ROOT_PATH}/iir/lib")
add_executable(${PROJECT_NAME} "../../resource.qrc" ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::SerialPort Qt6::Network iir_static_d libusb ${Python3_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::SerialPort Qt6::Network iir_static libusb ${Python3_LIBRARIES})
endif()
