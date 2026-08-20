#include <barrier>
#include <thread>
#include <utility>
#include <vector>

struct result_chunk { };
struct result_block {
    void set_chunk(unsigned i, unsigned num_threads, result_chunk chunk);
};
struct data_chunk { };
struct data_block { };
struct data_sink {
    void write_data(result_block result);
};
struct data_source {
    bool done();
    data_block get_next_data_block() { return { }; }
};

std::vector<data_chunk> divide_into_chunks(
    data_block data, unsigned num_threads);

result_chunk process(data_chunk chunk);

void process_data(data_source& source, data_sink& sink)
{
    unsigned const concurrency = std::thread::hardware_concurrency();
    unsigned const num_threads = (concurrency > 0) ? concurrency : 2;

    std::barrier sync(num_threads);
    std::vector<std::jthread> threads(num_threads);

    std::vector<data_chunk> chunks;
    result_block result;

    for (unsigned i = 0; i < num_threads; ++i) {
        threads[i] = std::jthread([&, i] {
            while (!source.done()) {
                // This runs only on the thread with i == 0
                if (!i) {
                    data_block current_block = source.get_next_data_block();
                    chunks = divide_into_chunks(current_block, num_threads);
                }

                // These sync.arrive_and_wait() calls allow to synchronize
                // threads together, so that they align with execution on these
                // lines
                sync.arrive_and_wait();
                result.set_chunk(i, num_threads, process(chunks[i]));
                sync.arrive_and_wait();

                // This too runs only on the thread with i == 0
                if (!i) {
                    sink.write_data(std::move(result));
                }
            }
        });
    }

    // Destructor of jthread will wait for all the treads to finish.
}

void process_data_flex(data_source& source, data_sink& sink)
{
    unsigned const concurrency = std::thread::hardware_concurrency();
    unsigned const num_threads = (concurrency > 0) ? concurrency : 2;

    std::vector<data_chunk> chunks;

    auto split_source = [&] {
        if (!source.done()) {
            data_block current_block = source.get_next_data_block();
            chunks = divide_into_chunks(current_block, num_threads);
        }
    };

    split_source();
    result_block result;

    std::experimental::flex_barrier sync(num_threads, [&] {
        sink.write_data(std::move(result));
        split_source();

        // Here we can adjust number of threads.
        return -1;
    });

    std::vector<std::jthread> threads(num_threads);

    for (unsigned i = 0; i < num_threads; ++i) {
        threads[i] = std::jthread([&, i] {
            while (!source.done()) {
                result.set_chunk(i, num_threads, process(chunks[i]));
                sync.arrive_and_wait();
            }
        });
    }
}

result_chunk process(data_chunk);

int main() { return 0; }
