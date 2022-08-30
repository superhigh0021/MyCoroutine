cmake_minimum_required(VERSION 3.0.0)
project(MYCOROUTINE VERSION 0.1.0)

include_directories(MYCOROUTINE ./include)
aux_source_directory(MYCOROUTINE ./src)

add_executable(MYCOROUTINE main.cpp xfiber.cpp xsocket.cpp)