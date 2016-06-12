cmake_minimum_required(VERSION 3.5)
project(UNIXproj)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -lpthread")

find_package(Threads)

set(SERVER_FILES server.c server.h)
add_executable(gd_server ${SERVER_FILES})
target_link_libraries(gd_server ${CMAKE_THREAD_LIBS_INIT})

set(CLIENT_FILES client.c client.h)
add_executable(gd_client ${CLIENT_FILES})
target_link_libraries(gd_client ${CMAKE_THREAD_LIBS_INIT})