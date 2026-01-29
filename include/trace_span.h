#pragma once

#include <cstdint>
#include <new> //hardware_desrtuctive_intererference_size

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 128;
#endif

struct alignas(hardware_destructive_interference_size) TraceSpan
{
    uint64_t trace_id_high;
    uint64_t trace_id_low;
    uint64_t span_id;
    uint64_t parent_id;
    uint64_t start_ns;
    uint64_t duration_ns;
    uint32_t flags;
    // alignas to ensure that no two Spans share a cache line
};
