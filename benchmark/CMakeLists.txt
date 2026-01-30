# define the benchmark executable
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)