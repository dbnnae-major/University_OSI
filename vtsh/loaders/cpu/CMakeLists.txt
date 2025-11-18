cmake_minimum_required(VERSION 3.16)

project(cpuload C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(cpuload
    main.c
)

target_include_directories(cpuload PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cpuload PRIVATE
    -O0 -Wall -Wextra -Wshadow -Wconversion -Wpedantic
)

set_target_properties(cpuload PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
