#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

std::thread v()
{
    void some_function();
    return std::thread(some_function);
}

void thread_move_example()
{
    std::thread t1([] { std::cout << "hello from t1"; });
    std::thread t2 = std::move(t1);

    t1 = std::thread([] { std::cout << "hello from t2"; });
    std::thread t3;

    t3 = std::move(t2);

    // This will crash as t1 is still managing a thread.
    // t1 = std::move(t3);
}

struct BigObject {
    std::string title;
    std::string body;

    BigObject(std::string title, std::string body)
        : title(title)
        , body(body)
    {
    }
};

void process_big_object(std::unique_ptr<BigObject> big_object)
{
    std::cout << big_object->title << "\n" << big_object->body << "\n";
}

void handle_big_object()
{
    std::unique_ptr<BigObject> b
        = std::make_unique<BigObject>("Book Title", "Book Contents");

    std::thread t(process_big_object, std::move(b));

    t.join();
}

struct Widget {
    std::string hello;
};

void update_data_for_widget(Widget& widget) { widget.hello = "sranie"; }

void oops_again()
{
    Widget widget = { .hello = "bez srania" };
    std::cout << widget.hello << "\n";
    std::thread t(update_data_for_widget, std::ref(widget));
    t.join();
    std::cout << widget.hello << "\n";
}

struct func {
    int& m_i;

    func(int& i_)
        : m_i(i_)
    {
    }

    void operator()() const
    {
        for (unsigned j = 0; j < 100000; ++j) {
            std::cout << "doing something" << m_i << "\n";
        }
    }
};

// std::thread can be initialized with anything callable,
// so for example with this class that overloads () operator.
class background_task {
public:
    void operator()() const
    {
        std::cout << "hello from concurrent class execution\n";
    }
};

class thread_guard {
    std::thread& m_t;

public:
    explicit thread_guard(std::thread& t_)
        : m_t(t_)
    {
    }

    ~thread_guard()
    {
        if (m_t.joinable()) {
            m_t.join();
        }
    }

    // this is c++ way of doing no-copy
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

void hello() { std::cout << "hello concurrent\n"; }

void f1()
{
    int some_local_state = 0;

    func my_func(some_local_state);

    std::thread t(my_func);
    thread_guard g(t);
}

void f()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);

    try {

    } catch (...) {
        t.join();
        throw;
    }

    t.join();
}

void oops()
{
    int some_local_state = 0;
    func my_func(some_local_state);

    std::thread t4(my_func);

    // t4.detach();   // This causes undefined behaviour.
    t4.join(); // this causes the whole main thread to do nothing.
}

int main()
{
    thread_move_example();
    oops();
    handle_big_object();

    std::thread t1(hello);
    std::thread t2 { background_task() };
    std::thread t3([] { std::cout << "Hello from subcalled threads?\n"; });

    t1.join();
    t2.join();
    t3.join();

    std::thread t4([] { std::cout << "Detached thread!\n"; });
    t4.detach();
    assert(!t4.joinable());

    oops_again();
}
