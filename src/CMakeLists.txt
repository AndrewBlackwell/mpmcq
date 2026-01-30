# define the demo executable
add_executable(correctness_test main.cpp)

# it uses threads (even if single-threaded)
target_link_libraries(correctness_test PRIVATE Threads::Threads)