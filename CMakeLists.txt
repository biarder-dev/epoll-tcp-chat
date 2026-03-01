cmake_minimum_required(VERSION 3.10)
project(tcp_chat C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)
set(SOURCES
    src/main.c
)
add_executable(tcp_chat ${SOURCES})

target_compile_definitions(tcp_chat PRIVATE _XOPEN_SOURCE=500)
target_compile_options(tcp_chat PRIVATE -Wall -Wextra -O2)
