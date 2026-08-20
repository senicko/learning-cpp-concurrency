#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

// Master thread pattern (snippet, not full code)
// =======================================================
// std::thread::id master_thread;

// void master_thread_pattern()
// {
//     if (std::this_thread::get_id() == master_thread) {
//         std::cout << "doing master work";
//     }
//     std::cout << "doing common work";
// }
// =======================================================

template <typename Iterator, typename T> struct accumulate_block {
    void operator()(Iterator first, Iterator last, T& result)
    {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length
        = static_cast<unsigned long>(std::distance(first, last));

    if (!length) {
        return init;
    }

    unsigned long const min_per_thread = 25;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();

    // This is integer way of ceil(length / min_per_thread).
    unsigned long const max_threads
        = (length + min_per_thread - 1) / min_per_thread;
    //  Take minimum from what computer can handle and max_threads. If we
    // don't know how many cores our machine has limit them to 2.
    unsigned long const num_threads
        = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    // We are allocating num_threads-1 so that we can reuse current thread
    // too.
    std::vector<std::thread> threads(num_threads - 1);
    std::vector<T> results(num_threads);

    Iterator block_start = first;

    for (unsigned long i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);

        // We need to pass std::ref to results[i] as threads can't return
        // values. Also std::ref is required because thread passes arguments as
        // rvalues, so only const T& is possible otherwise.
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start,
            block_end, std::ref(results[i]));

        block_start = block_end;
    }

    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);

    for (auto& entry : threads) {
        entry.join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}

struct func {
    int& i;

    func(int& i_)
        : i(i_)
    {
    }

    void operator()()
    {
        for (unsigned j = 0; j < 1'000'000; ++j) {
            std::cout << "thread " << j << "\n";
        }
    }
};

// As of c++20 there is std::jthread which kind of does what this
// joining_thread class do
class joining_thread {
    std::thread t;

public:
    joining_thread() noexcept = default;

    template <typename Callable, typename... Args>
    explicit joining_thread(Callable&& func, Args&&... args)
        : t(std::forward<Callable>(func), std::forward<Args>(args)...)
    {
    }

    explicit joining_thread(std::thread t_) noexcept
        : t(std::move(t_))
    {
    }

    joining_thread(joining_thread&& other) noexcept
        : t(std::move(other.t))
    {
    }

    joining_thread& operator=(joining_thread&& other) noexcept
    {
        if (joinable()) {
            join();
        }

        t = std::move(other.t);
        return *this;
    }

    joining_thread& operator=(std::thread other) noexcept
    {
        if (joinable()) {
            join();
        }

        t = std::move(other);
        return *this;
    }

    ~joining_thread() noexcept
    {
        if (joinable()) {
            join();
        }
    }

    void swap(joining_thread& other) noexcept { t.swap(other.t); }

    std::thread::id get_id() const noexcept { return t.get_id(); }

    bool joinable() const noexcept { return t.joinable(); }
    void join() { t.join(); }
    void detach() { t.detach(); }

    std::thread& as_thread() noexcept { return t; }
    const std::thread& as_thread() const noexcept { return t; }
};

class scoped_thread {
    std::thread t;

public:
    explicit scoped_thread(std::thread t_)
        : t(std::move(t_))
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~scoped_thread() { t.join(); }

    scoped_thread(scoped_thread const&) = delete;
    scoped_thread& operator=(scoped_thread const&) = delete;
};

void do_work(unsigned id)
{
    int a = 0;

    for (unsigned j = 0; j < 1'000'000; ++j) {
        a += 1;
    }

    std::cout << "doing work {" << id << "} = {" << a << "}\n";
}

int main()
{
    // int some_local_state = 0;
    // scoped_thread t { std::thread(func(some_local_state)) };

    // for (unsigned i = 0; i < 1'000'000; ++i) {
    //     std::cout << "main " << i << "\n";
    // }

    // std::vector<std::thread> threads;

    // for (unsigned i = 0; i < 20; ++i) {
    //     threads.emplace_back(do_work, i);
    // }

    // for (auto& entry : threads) {
    //     entry.join();
    // }

    std::vector<int> array(1'000'000);
    for (unsigned i = 0; i < array.size(); ++i) {
        array[i] = static_cast<int>(i);
    }

    int result = std::accumulate(array.begin(), array.end(), 0);
    int parallel_result = parallel_accumulate(array.begin(), array.end(), 0);

    std::cout << "result=" << result << "\nparallel_result=" << parallel_result
              << "\n";

    assert(result == parallel_result);
}
