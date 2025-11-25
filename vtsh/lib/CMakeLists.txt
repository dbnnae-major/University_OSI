add_library(
    libvtsh
    STATIC
    vtsh.c
    parser.c
    runner.c
)

target_include_directories(
    libvtsh
    PUBLIC
    .
)
