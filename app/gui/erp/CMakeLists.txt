cmake_minimum_required(VERSION 3.24)
project(eegneo_erp)

set(CMAKE_AUTOUIC_SEARCH_PATHS "ui")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

include_directories("inc")

file(GLOB HEADERS "inc/*.h" "${UTILS_PATH}/ipc.h")
file(GLOB SOURCES "src/*.cpp" "${UTILS_PATH}/ipc.cpp" "${UTILS_PATH}/config.cpp")
file(GLOB UI "ui/*.ui")

qt_add_executable(${PROJECT_NAME} "../../resource.qrc" ${HEADERS} ${UI} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)