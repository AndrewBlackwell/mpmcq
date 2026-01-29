# mpmcq

## Introduction

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

In our `std::mutex`-guarded queue , the Mutex forces both steps to be serial, resulting in the following behavior: Thread $A$ locks; Thread $A$ coordinates; Thread $A$ copies its data; Thread $A$ unlocks; Thread $B$ locks;... So, if copying takes 100$ns$, and you have 2 threads, the total time for both to complete is ~200$ns$.

In this particular lock-free implementation--which aims to solve both problems outlined above--I only serialize step 1 of the process (Coordination), while step 2 (Execution) can happen in parallel. With this idea in mind (and before we get into implementation), consider this series of events: Thread $A$ coordinates; Thread $B$ coordinates 1 nanosecond later; Threads $A$ and $B$ both copies their data in parallel; This brings our total time down to... ~101$ns$!
