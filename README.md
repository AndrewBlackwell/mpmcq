# mpmcq

## Reasoning & Design

Why does this project exist?

It started because I wanted a way to ingest telemetry (specifically distributed traces and large log chunks) whithout the ever-present "jitter" introduced by standard locking mechanisms. When you're building a distributed system, your units of work aren't usually `float` or `int`; they are 128B trace spans, 1KB log lines, or 4KB memory pages.

Most generic queues are optimized for _latency_ on small items, while this project is optimized for _throughput_ on large items.

### The Problem With `std::queue`

`std::queue` is, by default, implemented with a `std::dequeue`. If you're interested in why, [here's](https://stackoverflow.com/questions/41102465/why-does-stdqueue-use-stddequeue-as-underlying-default-container) a nice thread with some useful insight into how the implementation was decided.

As a quick refresher on `std::dequeue`, it is a map (a dynamic array, not `std::map` or `std::unordered_map`) of pointers to fixed-size arrays. This means unlike a `std::vector`, our elements are not contiguous in memory, instead it is organized in many "chunks" scattered around the heap.

Let's view this from the perspective of the CPU: when a CPU reads address $x100$, it will automatically prefetch addresses $x101-x128$ into the L1 Cache. In a fragmented `std::queue`, Node $A$ might be at address $x100$, but Node $B$ could be at $x900$. This means that when we need Node $B$, and it is not present in our L1 Cache, we will need to travel all the way to RAM to retrieve its data.

### The Problem With a `std::mutex`-Guarded Queue

This one is (hopefully) more intuitive. A Mutex is a heavy OS object. What happens when a thread tries to acquire a lock, and fails? The OS will put this thread to sleep (context switch), and waking up can take _thousands_\* of CPU cycles. If only one thread is able to acquire the lock to write to our queue at a time, it is uber slow, and it is effectively serializing our code.

\*_A full context switch is expensive because it involves saving register state, flushing TLBs (sometimes), and the scheduler deciding who runs next._

### Where Can a Lock-Free Implementation Improve Performance? (Also, Why Do We Care?)

A Queue is a FIFO data-structure by design. This means, our elements must be processed in **serial order** (1, 2, 3, ...); so why bother ditching a Mutex, or implementing parallelism anyways?

Writing to a Queue has two distinct steps:

1. Coordination: "Where is my input slot?"
2. Execution: `memcpy`\`ing my bytes into that slot

#### Here's an example which should hopefully demonstrate where the optimization occurs:

In our `std::mutex`-guarded queue , the Mutex forces both steps to be serial, resulting in the following behavior: Thread $A$ locks; Thread $A$ coordinates; Thread $A$ copies its data; Thread $A$ unlocks; Thread $B$ locks;... So, if copying takes 100$ns$, and you have 2 threads, the total time for both to complete is ~$200ns$.

In this particular lock-free implementation--which aims to solve both problems outlined above--I only serialize step 1 of the process (Coordination), while step 2 (Execution) can happen in parallel. With this idea in mind (and before we get into implementation), consider this series of events: Thread $A$ coordinates; Thread $B$ coordinates 1 nanosecond later; Threads $A$ and $B$ both copies their data in parallel; This brings our total time down to... ~$101ns$!

### A Better Way Of Transporting Large Payloads

To achieve this parallel execution, I used a Ring Buffer. A Ring Buffer is essentially just a fixed-size array that wraps around, with the unique feature that if you reach the end, you go back to the start. Because it's a single contiguous block of memory, it's extremely cache-friendly.

The core algorithm is based on Dmitry-Vyukov's MPMC Bounded Queue, which introduced the idea of a "ticketing" system (sequences) to coordinate access without locks. One specific optimization I made was choosing how to lay-out the memory. You'll often hear about **Structure of Arrays** (SOA) being faster, but for this specific use-case, I chose **Array of Structures** (AOS) to bundle the lock-free flags which coordinate access along with the data into a single struct to prevent false sharing.

Why? If the flags were packed tightly together in their own array, Thread A updating Flag 0 would invalidate the cache line for Thread B trying to read Flag 1. By bundling them, the 128-byte data payload acts as natural "padding," ensuring that every flag sits on its own cache line. This keeps the CPU cores from fighting over cache ownership.

### Benchmarks

| Payload Type | Size  | Bottleneck  | Speedup | Why?                                                                                                        |
| ------------ | ----- | ----------- | ------- | ----------------------------------------------------------------------------------------------------------- |
| Raw Signal   | 4 B   | Contention  | varies  | An atomic CAS is faster than Mutex sleep/wake, however there's no real copying cost in an int-sized payload |
| TraceSpan    | 128 B | Cache Line  | 1.18x   | Minor win. False sharing fights the parallel copy gain.                                                     |
| Log Chunk    | 1 KB  | Memory Copy | 1.71x   | Parallel copying overcomes coordination overhead.                                                           |
| Page         | 4 KB  | Memory Copy | 2.98x   | Sweet Spot. Threads copy smoothly in parallel.                                                              |
| Jumbo Frame  | 16 KB | Memory Copy | 5.58x   | Mutex is serialized; Lock-Free utilizes full bandwidth.                                                     |
| Crash Dump   | 64 KB | Memory Copy | ~9.03x  | Near-linear scaling of memory bandwidth.                                                                    |

### Conclusion

**This project doesn't aim to be the fastest queue in the world for every scenario (libraries like MoodyCamel's are fantastic for general purpose / ultra-low-latency workloads). Instead, I wrote this to learn lock free programming, and to test a hypothesis about the correlation between the "weight" of an object and the trade-offs between coordination and execution.**

I’ll be honest: lock-free programming is incredibly difficult. It is easy to write a lock-free queue that is actually slower than a mutex-guarded one (I certainly wrote a few of those while building this). For small payloads, the standard `std::mutex` is surprisingly fast and usually the right choice. But as your data grows, the cost of copying that data dominates the CPU time. That is the specific niche this project targets: using lock-free coordination to allow multiple threads to memcpy large blobs of data in parallel, maximizing memory bandwidth rather than fighting over a single lock.
