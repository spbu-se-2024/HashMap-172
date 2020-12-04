file(GLOB_RECURSE C_SOURCES ./*.c)

add_executable(word_counter ${C_SOURCES} status.h)

target_compile_options(word_counter PRIVATE -Wall -Wextra -pedantic)