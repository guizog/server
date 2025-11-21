cmake_minimum_required(VERSION 3.30)
project(server C)

set(CMAKE_C_STANDARD 11)

add_executable(server main.c
        server.c
        server.h
        common.h
        common.c)

target_link_libraries(server ws2_32)