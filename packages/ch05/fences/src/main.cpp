#include <atomic>
#include <cassert>
#include <thread>

namespace ordering_atomic {
std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_relaxed);

    // Everything below the fence don't have to synchronize. It's pure relaxed
    // ordering
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_acquire))
        ;
    // This synchronizes with the write_x_then_y thread
    std::atomic_thread_fence(std::memory_order_acquire);
    if (x.load(std::memory_order_relaxed))
        ++z;
}

void run()
{
    x = false;
    y = false;
    z = 0;

    std::thread a(write_x_then_y);
    std::thread b(read_y_then_x);

    a.join();
    b.join();

    assert(z.load() != 0);
}
}

namespace ordering_nonatomic {
bool x = false;
std::atomic<bool> y;
std::atomic<int> z;

void write_x_then_y()
{
    x = true;
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_relaxed))
        ;
    std::atomic_thread_fence(std::memory_order_acquire);
    if (x)
        ++z;
}

void run()
{
    x = false;
    y = false;
    z = 0;

    std::thread a(write_x_then_y);
    std::thread b(read_y_then_x);

    a.join();
    b.join();

    assert(z.load() != 0);
}
}

int main()
{
    ordering_atomic::run();
    ordering_nonatomic::run();
}
