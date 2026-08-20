#include <future>
#include <iostream>
#include <latch>
#include <vector>

struct my_data { };

void foo()
{
    unsigned const thread_count = 4;
    std::latch done(thread_count);

    my_data data[thread_count];
    std::vector<std::future<void>> threads;

    for (unsigned i = 0; i < thread_count; ++i) {
        threads.push_back(std::async(std::launch::async, [&, i] {
            data[i] = { };
            done.count_down();

            // We are using latch because of this. Latch helps us synchronize
            // data preparation, but firther processing can be done in parallel.
            std::cout << "doing more stuff\n" << std::flush;
        }));
    }

    done.wait();
    std::cout << "processing data\n";
}

int main()
{
    foo();
    return 0;
}
