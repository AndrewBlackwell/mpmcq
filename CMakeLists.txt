cmake_minimum_required(VERSION 3.15)
project(ringbuffer LANGUAGES CXX)

# Need at least C++17 for 'std::hardware_destructive_interference_size' 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On macOS, prefer clang over gcc
if(APPLE)
    set(CMAKE_CXX_COMPILER /usr/bin/clang++)
    set(CMAKE_C_COMPILER /usr/bin/clang)
endif()

# Compiler flags for optimization and warnings
add_compile_options(-O3 -Wall -Wextra)

# Tells CMake that header files live in the "include/" folder.
include_directories(${PROJECT_SOURCE_DIR}/include)

# Threading: Even though C++ has std::thread, you explicitly need to link the system's pthread library 
# on Mac/Linux. CMake creates a "Threads::Threads" target for this.
find_package(Threads REQUIRED)

# Tells CMake to descend into source folders.
add_subdirectory(src)
