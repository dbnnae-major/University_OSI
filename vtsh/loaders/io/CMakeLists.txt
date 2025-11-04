cmake_minimum_required(VERSION 3.16)

project(ioload C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(ioload
    main.c
    args.c
    io.c
    util.c
)

target_include_directories(ioload PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(ioload PRIVATE
    -O2 -Wall -Wextra -Wshadow -Wconversion -Wpedantic
)

set_target_properties(ioload PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)