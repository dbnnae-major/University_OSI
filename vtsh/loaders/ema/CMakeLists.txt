cmake_minimum_required(VERSION 3.16)

project(emaload C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(emaload
    main.c
)

target_include_directories(emaload PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(emaload PRIVATE
    -O0 -Wall -Wextra -Wshadow -Wconversion -Wpedantic
)

set_target_properties(emaload PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)