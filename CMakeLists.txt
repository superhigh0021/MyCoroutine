cmake_minimum_required(VERSION 3.0.0)
project(mycoroutine VERSION 0.1.0)

include_directories(./include)

aux_source_directory(./src SRC)

add_executable(main ${SRC})
