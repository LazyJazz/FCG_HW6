cmake_minimum_required(VERSION 3.22)

project(FCG_HW2)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(LongMarch)

set(ASSETS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assets/)

add_subdirectory(code)
