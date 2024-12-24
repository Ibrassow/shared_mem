cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)

project(SH VERSION 1.0 LANGUAGES CXX)




# Set up a preprocessor definition 
add_compile_definitions(WRITING_FREQ=2)
add_compile_definitions(READING_FREQ=1)



# Find the required packages
set(SPDLOG_FMT_EXTERNAL OFF)
find_package(spdlog REQUIRED)

# Add the executables
add_executable(reader reader.cpp)
add_executable(writer writer.cpp)

# Link libraries to the executables
target_link_libraries(reader
    PRIVATE
    spdlog::spdlog
    rt
)

target_link_libraries(writer
    PRIVATE
    spdlog::spdlog
    rt
)

# Add compiler warnings (as errors) and other warning flags
target_compile_options(reader PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
)

target_compile_options(writer PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
)

