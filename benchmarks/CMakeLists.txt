# define the benchmark executable
add_executable(telemetry_bench throughput_bench.cpp)

target_link_libraries(telemetry_bench PRIVATE Threads::Threads)