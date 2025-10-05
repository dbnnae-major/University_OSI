add_library(
    libvtsh
    STATIC
    vtsh.c
    parser.c
)

target_include_directories(
    libvtsh
    PUBLIC
    .
)
