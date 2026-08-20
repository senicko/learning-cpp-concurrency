#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

int find_the_answer()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return 42;
}

void do_other_stuff() { std::cout << "doing other stuff\n"; }

struct X {
    void foo(int, std::string const&);
    std::string bar(std::string const&);
};

struct Y {
    double operator()(double);
};

// This is a toy type that can be created, can be moved, but not copied
class move_only {
public:
    move_only();

    move_only(move_only&&);
    move_only& operator=(move_only&&);

    move_only(move_only const&) = delete;
    move_only& operator=(move_only const&) = delete;

    void operator()();
};

template <> class std::packaged_task<std::string(std::vector<char>*, int)> {
public:
    template <typename Callable> explicit packaged_task(Callable&& f);
    std::future<std::string> get_future();
    void operator()(std::vector<char>*, int);
};

int main()
{
    X x;

    // This calls p->foo(42, "hello"), where p is &x
    auto f1 = std::async(&X::foo, &x, 42, "hello");
    // This creates a copy tmp of x, and calls tmp.bar("goodbye")
    auto f2 = std::async(&X::bar, x, "goodbye");

    Y y;
    auto f3 = std::async(Y(), 3.141);
    auto f4 = std::async(std::ref(y), 2.718);

    X baz(X&);
    // Calls baz on x
    auto f5 = std::async(baz, std::ref(x));
    auto f6 = std::async(move_only());

    // Specifying if async should run on its own thread

    auto f7 = std::async(std::launch::async, Y(), 1.2);
    auto f8 = std::async(std::launch::deferred, baz, std::ref(x));
    // This means that implementation can choose (default option).
    auto f9 = std::async(
        std::launch::deferred | std::launch::async, baz, std::ref(x));
    auto f10 = std::async(baz, std::ref(x));

    f8.wait(); // This invokes the deferred function

    std::future<int> the_answer = std::async(find_the_answer);
    do_other_stuff();
    std::cout << "The answer is " << the_answer.get() << "\n" << std::endl;
}
