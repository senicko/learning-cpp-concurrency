#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// When using sequential memory ordering operations have a strict order. In this
// case it's impossible for the assert to fire, as it's guaranteed that we
// increment z.
namespace sequential_example {
std::atomic<bool> x, y;
std::atomic<int> z;

void write_x() { x.store(true, std::memory_order_seq_cst); }
void write_y() { y.store(true, std::memory_order_seq_cst); }

void read_x_then_y()
{
    while (!x.load(std::memory_order_seq_cst))
        ;

    if (y.load(std::memory_order_seq_cst))
        ++z;
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_seq_cst))
        ;

    if (x.load(std::memory_order_seq_cst)) {
        ++z;
    }
}

void run()
{
    x = false;
    y = false;
    z = 0;

    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);

    a.join();
    b.join();
    c.join();
    d.join();

    // This never fires, as it's guaranteed for z to be = 1
    assert(z.load() != 0);
}
}

// Using relaxed ordering doesn't guaratee anything. Threads don't need to agree
// on operation order at all. Assert can fire in that case.
namespace relaxed_ordering_example {
std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_relaxed))
        ;

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

namespace relaxed_ordering_example_two {
std::atomic<int> x(0), y(0), z(0);
std::atomic<bool> go(false);

unsigned const loop_count = 10;

struct read_values {
    int x, y, z;
};

read_values values1[loop_count];
read_values values2[loop_count];
read_values values3[loop_count];
read_values values4[loop_count];
read_values values5[loop_count];

void read_and_inc_vals(std::atomic<int>* var_to_inc, read_values* values)
{
    while (!go) {
        std::this_thread::yield();
    }

    for (unsigned i = 0; i < loop_count; ++i) {
        values[i].x = x.load(std::memory_order_relaxed);
        values[i].y = y.load(std::memory_order_relaxed);
        values[i].z = z.load(std::memory_order_relaxed);

        var_to_inc->store(static_cast<int>(i + 1), std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

void read_vals(read_values* values)
{
    while (!go) {
        std::this_thread::yield();
    }

    for (unsigned i = 0; i < loop_count; ++i) {
        values[i].x = x.load(std::memory_order_relaxed);
        values[i].y = y.load(std::memory_order_relaxed);
        values[i].z = z.load(std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

void print(read_values* values)
{
    for (unsigned i = 0; i < loop_count; ++i) {
        if (i)
            std::cout << ",";

        std::cout << "(" << values[i].x << "," << values[i].y << ","
                  << values[i].z << ")";
    }

    std::cout << "\n";
}

void run()
{
    std::thread t1(read_and_inc_vals, &x, values1);
    std::thread t2(read_and_inc_vals, &y, values2);
    std::thread t3(read_and_inc_vals, &z, values3);

    std::thread t4(read_vals, values4);
    std::thread t5(read_vals, values5);

    // This signals to start execution of main loop
    go = true;

    t5.join();
    t4.join();
    t3.join();
    t2.join();
    t1.join();

    // Here we can see that each thread has it's own idea of what happened when.
    // Only updates of the variables controlled by each thread have a clear
    // order ON THAT THREAD only. So first thread has x increasing by one.
    print(values1);
    print(values2);
    print(values3);
    print(values4);
    print(values5);

    // Example output:
    // (0,0,0),(1,0,5),(2,0,6),(3,0,7),(4,0,8),(5,10,10),(6,10,10),(7,10,10),(8,10,10),(9,10,10)
    // (5,0,9),(5,1,9),(5,2,9),(5,3,9),(5,4,9),(5,5,9),(5,6,10),(5,7,10),(5,8,10),(5,9,10)
    // (1,0,0),(1,0,1),(1,0,2),(1,0,3),(1,0,4),(2,0,5),(3,0,6),(4,0,7),(5,0,8),(5,6,9)
    // (0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(5,8,10),(5,10,10)
    // (1,0,0),(5,0,9),(5,8,10),(10,10,10),(10,10,10),(10,10,10),(10,10,10),(10,10,10),(10,10,10),(10,10,10)
}
}

namespace acquire_release_ordering_example {
// Release operations synchronize with acquire operations that read the values
// written

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x() { x.store(true, std::memory_order_release); }

void write_y() { y.store(true, std::memory_order_release); }

void read_x_then_y()
{
    while (!x.load(std::memory_order_acquire))
        ;

    if (y.load(std::memory_order_acquire))
        ++z;
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_acquire))
        ;

    if (x.load(std::memory_order_acquire))
        ++z;
}

void run()
{
    x = false;
    y = false;
    z = 0;

    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);

    a.join();
    b.join();
    c.join();
    d.join();

    // This still can fire, acquire-release doesn't solve this case. Writes to x
    // and y are still on separate threads so there is no order between threads
    // setting x and y.
    assert(z.load() != 0);
}
}

namespace acquire_release_ordering_example_two {
std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);

    // Because this store uses order_release, it synchronizes with the
    // order_acquire memory ordering on the read_y_then_x thread. This imposes
    // an ordering between stores to x and y, as they run on the same thread
    // (operations on the same thread are sequential)
    y.store(true, std::memory_order_release);
}

void read_y_then_x()
{
    while (!y.load(std::memory_order_acquire))
        ;

    // Here, x always loads true, because store to x happens before the store to
    // y, and both threads are synchronized thanks to the used orderings
    if (x.load(std::memory_order_relaxed))
        ++z;
}
}

namespace transitive_sync_with_acq_rel_ordering_example {
std::atomic<int> data[5];
std::atomic<bool> sync1(false), sync2(false);

void thread_1()
{
    data[0].store(42, std::memory_order_relaxed);
    data[1].store(97, std::memory_order_relaxed);
    data[2].store(17, std::memory_order_relaxed);
    data[3].store(-141, std::memory_order_relaxed);
    data[4].store(2003, std::memory_order_relaxed);
    sync1.store(true, std::memory_order_release);
}

// This thread allows for transitive sync between thread 1 and 3. Happens-before
// is transitive.
void thread_2()
{
    while (!sync1.load(std::memory_order_acquire))
        ;
    sync2.store(true, std::memory_order_release);
}

void thread_3()
{
    while (!sync2.load(std::memory_order_acquire))
        ;
    assert(data[0].load(std::memory_order_relaxed) == 42);
    assert(data[1].load(std::memory_order_relaxed) == 97);
    assert(data[2].load(std::memory_order_relaxed) == 17);
    assert(data[3].load(std::memory_order_relaxed) == -141);
    assert(data[4].load(std::memory_order_relaxed) == 2003);
}
}

// We can use a single read-write-modify operation for sync on int atomic.
namespace transitive_sync_with_acq_rel_ordering_example_two {
std::atomic<int> sync(0);

void thread_1()
{
    // ...
    sync.store(1, std::memory_order_release);
}

void thread_2()
{
    int expected = 1;

    // We choose std::memory_order_acq_rel semantics because that's what we want
    while (
        !sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
        expected = 1;
}

void thread_3()
{
    while (sync.load(std::memory_order_acquire) < 2)
        ;
}
}

namespace consume_ordering_for_data_sync {
struct X {
    int i;
    std::string s;
};

std::atomic<X*> p;
std::atomic<int> a;

void create_x()
{
    X* x = new X;
    x->i = 42;
    x->s = "hello";

    a.store(99, std::memory_order_relaxed);
    p.store(x, std::memory_order_release);
}

void use_x()
{
    X* x;
    while (!(x = p.load(std::memory_order_consume)))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Because we have used memory_order_consume, these asserts won't fire
    // because x members are synchronized.
    assert(x->i == 42);
    assert(x->s == "hello");

    // This may or may not fire, as a is not synchronized!
    assert(a.load(std::memory_order_relaxed) == 99);
}

void run()
{
    std::thread t1(create_x);
    std::thread t2(use_x);

    t1.join();
    t2.join();
}

int global_data[] = { 0, /* ... (data) ... */ };
std::atomic<int> index;

void do_something_with(int value);

void f()
{
    int i = index.load(std::memory_order_consume);
    do_something_with(global_data[std::kill_dependency(i)]);
}
}

int main() { return 0; }
