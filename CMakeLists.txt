cmake_minimum_required(VERSION 3.30)
project(server C)

set(CMAKE_C_STANDARD 11)

add_executable(server main.c
        server.c
        server.h
        common.h
        common.c
        request.c
        request.h
        http_types.c
        http_types.h
        response.c
        response.h)

target_link_libraries(server ws2_32)