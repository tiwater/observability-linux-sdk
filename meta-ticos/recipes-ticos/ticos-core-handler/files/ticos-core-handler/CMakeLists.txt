cmake_minimum_required(VERSION 3.16)

project(ticos-core-handler)

add_executable(ticos-core-handler
    ticos-core-handler.c
)

target_compile_options(ticos-core-handler PRIVATE -O3 -Wall -Wpedantic -Wextra -Werror)

install(PROGRAMS ${CMAKE_BINARY_DIR}/ticos-core-handler TYPE SBIN)
