#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <experimental/future>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

// The book listing treats find_the_answer as "something that returns a
// future". In the TS that is typically experimental::async, not std::async,
// because std::async returns std::future, which has no then().
std::experimental::future<int> find_the_answer()
{
    return std::experimental::async([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });
}

// Continuations receive the original future (now ready), not the raw value.
std::string find_the_question(std::experimental::future<int> answer)
{
    return "What do you get if you multiply six by nine? ("
        + std::to_string(answer.get()) + ")";
}

// Example of spawning .then() supporting features
template <typename Func>
std::experimental::future<decltype(std::declval<Func>()())> spawn_async(
    Func&& func)
{
    std::experimental::promise<decltype(std::declval<Func>()())> p;
    auto res = p.get_future();
    std::thread t([p = std::move(p), f = std::decay_t<Func>(func)]() mutable {
        try {
            p.set_value_at_thread_exit(f());
        } catch (...) {
            p.set_exception_at_thread_exit(std::current_exception());
        }
    });
    t.detach();
    return res;
}

// Example of a sequential code to process user login.
void process_login(std::string const& username, std::string const& password)
{
    try {
        user_id const id = backend.authenticate_user(username, password);
        user_data const info_to_display = backend.request_current_info(id);
        update_display(info_to_display);
    } catch (std::exception& e) {
        display_error(e);
    }
}

// Processing user login with an async task.
std::future<void> process_login_async(
    std::string const& username, std::string const& password)
{
    return std::async(std::launch::async, [=]() {
        try {
            user_id const id = backend.authenticate_user(username, password);
            user_data const info_to_display = backend.request_current_info(id);
            update_display(info_to_display);
        } catch (std::exception& e) {
            display_error(e);
        }
    });
}

// Processing user login with experimental future chains.
std::experimental::future<void> process_login_chain(
    std::string const& username, std::string const& password)
{
    return spawn_async(
        [=]() { return backend.authenticate_user(username, password); })
        .then([](std::experimental::future<user_id> id) {
            return backend.request_current_info(id.get());
        })
        .then([](std::experimental::future<user_data> info_to_display) {
            try {
                update_display(info_to_display.get())
            } catch (std::exception& e) {
                display_error(e);
            }
        });
}

void some_function() { std::cout << "some function\n"; }

struct FinalResult { };
struct ChunkResult { };
struct MyData { };

ChunkResult process_chunk(
    std::vector<MyData>::iterator first, std::vector<MyData>::iterator last)
{
    (void)first;
    (void)last;
    return { };
}

FinalResult gather_results(std::vector<ChunkResult> const& chunks)
{
    (void)chunks;
    return { };
}

// This snippet spawns asynchronous tasks and then waits for them individually.
// This is not optimal.
std::future<FinalResult> process_data(std::vector<MyData>& vec)
{
    size_t const chunk_size = 3;
    std::vector<std::future<ChunkResult>> results;

    for (auto begin = vec.begin(), end = vec.end(); begin != end;) {
        size_t const remaining_size = static_cast<size_t>(end - begin);
        size_t const this_chunk_size = std::min(remaining_size, chunk_size);
        auto const chunk_end
            = begin + static_cast<std::ptrdiff_t>(this_chunk_size);
        results.push_back(
            std::async(std::launch::async, process_chunk, begin, chunk_end));
        begin = chunk_end;
    }

    return std::async(
        std::launch::async, [all_results = std::move(results)]() mutable {
            std::vector<ChunkResult> v;
            v.reserve(all_results.size());
            for (auto& f : all_results) {
                // All futures are ready by the time execution gets there. This
                // works well when spawning multiple tasks to take advantage of
                // the available concurrency.
                v.push_back(f.get());
            }
            return gather_results(v);
        });
}

// This uses experimental std::experimental::when_all that waits for all the
// futures.
std::experimental::future<FinalResult> process_data(std::vector<MyData>& vec)
{
    size_t const chunk_size = 3;

    std::vector<std::experimental::future<ChunkResult>> results;

    for (auto begin = vec.begin(), end = vec.end(); begin != end;) {
        size_t const remaining_size = static_cast<size_t>(end - begin);
        size_t const this_chunk_size = std::min(remaining_size, chunk_size);
        auto const chunk_end
            = begin + static_cast<std::ptrdiff_t>(this_chunk_size);
        results.push_back(
            std::async(std::launch::async, process_chunk, begin, chunk_end));
        begin = chunk_end;
    }

    return std::experimental::when_all(results.begin(), results.end())
        .then(
            [](std::future<std::vector<std::experimental::future<ChunkResult>>>
                    ready_results) {
                std::vector<std::experimental::future<ChunkResult>> all_results
                    = ready_results.get();
                std::vector<ChunkResult> v;
                v.reserve(all_results.size());
                for (auto& f : all_results) {
                    v.push_back(f.get());
                }
                return gather_results(v);
            });
}

std::experimental::future<FinalResult> find_and_process_value(
    std::vector<MyData>& data)
{
    unsigned const concurrency = std::thread::hardware_concurrency();
    unsigned const num_tasks = (concurrency > 0) ? concurrency : 2;

    auto const chunk_size = (data.size() + num_tasks - 1) / num_tasks;
    auto chunk_begin = data.begin();

    std::vector<std::experimental::future<MyData*>> results;
    std::shared_ptr<std::atomic<bool>> done_flag
        = std::make_shared<std::atomic<bool>>(false);

    // Spawns num_tasks asynchronous tasks, each running a lambda function.
    for (unsigned i = 0; i < num_tasks; ++i) {
        auto chunk_end
            = (i < (num_tasks - 1)) ? chunk_begin + chunk_size : data.end();

        // This lambda function captures by copying, so each task has its own
        // chunk_begin, chunk_end and copy of the shared pointer.
        results.push_back(spawn_async([=] {
            for (auto entry = chunk_begin; !*done_flag && (entry != chunk_end);
                ++entry) {
                if (matches_find_criteria(*entry)) {
                    *done_flag = true;
                    return &*entry;
                }
            }

            return (MyData*)nullptr;
        }));

        chunk_begin = chunk_end;
    }

    std::shared_ptr<std::experimental::promise<FinalResult>> final_result
        = std::make_shared<std::experimental::promise<FinalResult>>();

    struct DoneCheck {
        std::shared_ptr<std::experimental::promise<FinalResult>> final_result;

        DoneCheck(std::shared_ptr<std::experimental::promise<FinalResult>>
                final_result_)
            : final_result(std::move(final_result_))
        {
        }

        // This call operator is invoked when one of the tasks is ready.
        void operator()(
            std::experimental::future<std::experimental::when_any_result<
                std::vector<std::experimental::future<MyData*>>>>
                results_param)
        {
            // Extract the ready value
            auto results = results_param.get();
            MyData* const ready_result = results.futures[results.index].get();

            if (ready_result) {
                // If result is ready, set the final value
                final_result->set_value(process_found_value(*ready_result));
            } else {
                // Otherwise drop the ready future
                results.futures.erase(results.futures.begin() + results.index);

                if (!results.futures.empty()) {
                    std::experimental::when_any(
                        results.futures.begin(), results.futures.end())
                        .then(std::move(*this));
                } else {
                    final_result->set_exception(std::make_exception_ptr(
                        std::runtime_error("Not Found")));
                }
            }
        }
    };

    // Handle the case that a task returned.
    std::experimental::when_any(results.begin(), results.end())
        .then(DoneCheck(final_result));

    return final_result->get_future();
}

int main()
{
    auto fut = find_the_answer();
    auto fut2 = fut.then(find_the_question);
    assert(!fut.valid());
    assert(fut2.valid());
    std::cout << fut2.get() << '\n';

    auto fut = spawn_async(some_function).share();
    auto fut2 = fut.then([](std::experimental::shared_future<some_data> data) {
        do_stuff(data);
    });
    auto fut3 = fut.then([](std::experimental::shared_future<some_data> data) {
        return do_other_stuff(data);
    });

    return 0;
}
