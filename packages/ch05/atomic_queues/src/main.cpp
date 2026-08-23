#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> queue_data;
std::atomic<int> number_of_elements;

void process(int value) { std::cout << "processing=" << value << "\n"; }

void populate_queue()
{
    unsigned const number_of_items = 20;
    queue_data.clear();
    for (unsigned i = 0; i < number_of_items; ++i) {
        queue_data.push_back(static_cast<int>(i));
    }

    number_of_elements.store(number_of_items, std::memory_order_release);
}

void consume_queue_items()
{
    while (true) {
        int item_index;
        if ((item_index
                = number_of_elements.fetch_sub(1, std::memory_order_acquire))
            <= 0) {
            // Wait for more items
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        process(queue_data[static_cast<unsigned>(item_index - 1)]);
    }
}

int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);

    a.join();
    b.join();
    c.join();
}
