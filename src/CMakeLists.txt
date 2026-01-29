# define the demo executable
add_executable(sanity_check main.cpp)

# it uses threads (even if single-threaded)
target_link_libraries(sanity_check PRIVATE Threads::Threads)