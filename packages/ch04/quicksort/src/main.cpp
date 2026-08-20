#include <future>
#include <list>
#include <thread>
#include <type_traits>

template <typename T> std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty()) {
        return input;
    }

    // Remove last element from input
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    // Pivot is now the first element of the result list
    T const& pivot = *result.begin();

    // Partition the input array and find the divide point (first element that
    // is NOT less than the pivot)
    auto divide_point = std::partition(
        input.begin(), input.end(), [&](T const& t) { return t < pivot; });

    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);

    return result;
}

template <typename T> std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty()) {
        return input;
    }

    std::list<T> result;

    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();

    auto divide_point = std::partition(
        input.begin(), input.end(), [&](T const& t) { return t < pivot; });

    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));
    auto new_higher(parallel_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());

    return result;
}

template <typename F, typename A>
std::future<std::invoke_result_t<F, A&&>> spawn_task(F&& f, A&& a)
{
    using result_type = std::invoke_result_t<F, A&&>;
    std::packaged_task<result_type(A&&)> task(std::move(f));
    std::future<result_type> res(task.get_future());
    std::thread t(std::move(task), std::move(a));
    t.detach();
    return res;
}

int main() { return 0; }
