#include <atomic>

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

int main()
{
    std::atomic_flag f = ATOMIC_FLAG_INIT;
    f.clear(std::memory_order_release);
    bool x = f.test_and_set();
    return 0;
}
