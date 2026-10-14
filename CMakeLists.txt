# Linux / KDE build of Calculator.
#
# The Windows app is built from src/Calculator.slnx with Visual Studio. This CMake
# project builds the portable calculation engine (src/CalcManager) together with a
# Qt 6 / Kirigami frontend (src/Calculator.Kde) for KDE Plasma and other Linux desktops.

cmake_minimum_required(VERSION 3.21)

project(Calculator VERSION 11.2607 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(CALCULATOR_BUILD_TESTS "Build the CalcManager smoke tests" ON)

if(CALCULATOR_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src/CalcManager)
add_subdirectory(src/Calculator.Kde)
