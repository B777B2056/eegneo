cmake_minimum_required(VERSION 3.24)

set(3PARTY_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third")
set(UTILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/utils")

include_directories(".")

add_subdirectory("acquisition")
add_subdirectory("erp")
