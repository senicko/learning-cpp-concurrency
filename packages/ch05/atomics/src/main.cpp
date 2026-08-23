#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// Dead simple spinlock mutex that's compatible with std::lock_guard<>
class spinlock_mutex {
    std::atomic_flag flag;

public:
    spinlock_mutex()
        : flag(ATOMIC_FLAG_INIT)
    {
    }

    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire))
            ;
    }

    void unlock() { flag.clear(std::memory_order_release); }
};

class foo { };

std::vector<int> data;
std::atomic<bool> data_ready(false);

void reader_thread()
{
    // This is inefficient, but it doesn't matter here.
    while (!data_ready.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "The answer = " << data[0] << "\n";
}

void writer_thread()
{
    data.push_back(42);
    data_ready = true;
}

int main()
{
    std::atomic_flag f = ATOMIC_FLAG_INIT;
    f.clear(std::memory_order_release);
    bool x = f.test_and_set();

    std::atomic<bool> b(true);
    b = false;

    bool y = b.load(std::memory_order_acquire);
    b.store(true);
    y = b.exchange(false, std::memory_order_acq_rel);

    foo some_array[5];
    std::atomic<foo*> p(some_array);

    // This adds 2 to p and returns the old value
    foo* z = p.fetch_add(2);
    assert(z == some_array);
    assert(p.load() == &some_array[2]);

    // This subtracts 1 from p and returns a new value
    z = (p -= 1);
    assert(z == &some_array[1]);
    assert(p.load() == &some_array[1]);

    return 0;
}
