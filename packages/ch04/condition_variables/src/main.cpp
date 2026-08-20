#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

// This is the first approach of waiting for flag. Mutex protecting the flag is
// acquired by the waiting thread. While the flag is not set, we unlock the
// mutex, wait for some time, and try to lock it again. This is quite wasteful
// as we need to run a separate thread + we disqualify the processing thread
// from locking when it actally finishes adding additional overhead.

// #include <thread>
// #include <chrono>

// bool flag;
// std::mutex m;
// int items_to_produce = 5;

// void wait_for_flag()
// {
//     std::unique_lock lk(m);
//     while (!flag) {
//         lk.unlock();
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         lk.lock();
//     }
// }

template <typename T> class threadsafe_queue {
private:
    std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue() { }

    threadsafe_queue(const threadsafe_queue& other)
    {
        std::lock_guard lk(other.mut);
        data_queue = other.data_queue;
    }

    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    void push(T new_value)
    {
        std::lock_guard lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        value = data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard lk(mut);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard lk(mut);
        return data_queue.empty();
    }
};

struct data_chunk {
    std::string type;
};

threadsafe_queue<data_chunk> data_queue;

data_chunk prepare_data() { return data_chunk { .type = "ticket" }; }
bool more_data_to_prepare() { return true; }
void process(data_chunk const& data) { std::cout << data.type << "\n"; }
bool is_last_chunk() { return true; }

void data_preparation_thread()
{
    while (more_data_to_prepare()) {
        data_chunk const data = prepare_data();
        data_queue.push(data);
    }
}

void data_processing_thread()
{
    while (true) {
        data_chunk data;
        data_queue.wait_and_pop(data);
        process(data);

        if (is_last_chunk()) {
            break;
        }
    }
}

int main() { }
