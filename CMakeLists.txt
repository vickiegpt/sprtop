cmake_minimum_required(VERSION 3.16)

project(sprtop_core)

# We need C++23 for std::views::enumerate, std::format, std::ranges.
# Force C++23 flag globally so pybind11 sees it and doesn't override.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b")
find_package(Threads REQUIRED)
find_package(pybind11 REQUIRED CONFIG)
pybind11_add_module(sprtop_core lib/sprtop_core.cpp)
target_link_libraries(sprtop_core PRIVATE pybind11::module Threads::Threads)
