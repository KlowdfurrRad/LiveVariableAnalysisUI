cmake_minimum_required(VERSION 3.12)
project(Live_Pass)


set(CMAKE_CXX_STANDARD 17)


find_package(LLVM 20.1.8 REQUIRED CONFIG)
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(analysis)
