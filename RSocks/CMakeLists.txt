cmake_minimum_required(VERSION 3.14)
project(RSocks)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost 1.40.0 REQUIRED system)

add_library(RSocks INTERFACE)