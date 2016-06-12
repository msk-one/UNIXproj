cmake_minimum_required(VERSION 3.5)
project(UNIXproj)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -lpthread")

set(SERVER_FILES server.c server.h)
add_executable(gd_server ${SERVER_FILES})

set(CLIENT_FILES client.c client.h)
add_executable(gd_client ${CLIENT_FILES})