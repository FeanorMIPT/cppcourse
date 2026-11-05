cmake_minimum_required(VERSION 3.30)
project(shared_ptr)

set(CMAKE_CXX_STANDARD 20)

add_executable(shared_ptr smart_pointers.h smartpointers_test.cpp)
