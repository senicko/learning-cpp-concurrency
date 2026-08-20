#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <stack>
#include <string>

std::unique_lock<std::mutex> get_lock()
{
    extern std::mutex some_mutex;
    // Unique lock lock without std::defer_lock
    std::unique_lock lk(some_mutex);
    // This moves the ownership of the lock
    return lk;
}

void process_data()
{
    // Here we take the lock from get_lock()
    std::unique_lock lk(get_lock());
    std::cout << "doing something\n";
}

void get_and_process_data()
{
    extern std::mutex some_mutex;
    std::unique_lock my_lock(some_mutex);
    // some_class data_to_process = get_next_data_chunk();
    my_lock.unlock();
    // result_type = result = process(data_to_process);
    my_lock.lock();
    // write_result(data_ro_process, result);
}

class some_big_object { };

void swap(some_big_object& lhs, some_big_object& rhs);

class X {
private:
    some_big_object some_detail;
    std::mutex m;

public:
    X(some_big_object const& sd)
        : some_detail(sd)
    {
    }

    friend void swap(X& lhs, X& rhs)
    {
        if (&lhs == &rhs) {
            return;
        }

        // Leave mutexes opened with std::defer_lock. Normally it's better to
        // prefer std::lock_guard and choose std::unique_lock only if we need
        // things like deferred locking.
        std::unique_lock lock_a(lhs.m, std::defer_lock);
        std::unique_lock lock_b(rhs.m, std::defer_lock);

        // Lock these locks
        std::lock(lock_a, lock_b);

        swap(lhs.some_detail, rhs.some_detail);
    }
};

struct empty_stack : std::exception {
    const char* what() const throw();
};

template <typename T> class threadsafe_stack {
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    threadsafe_stack() { }

    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard lock(other.m);
        data = other.data;
    }

    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard lock(m);
        data.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard lock(m);

        if (data.empty()) {
            throw empty_stack();
        }

        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();

        return res;
    }

    void pop(T& value)
    {
        std::lock_guard lock(m);

        if (data.empty()) {
            throw empty_stack();
        }

        value = data.top();
        data.pop();
    }

    bool empty() const
    {
        std::lock_guard lock(m);
        return data.empty();
    }
};

class some_data {
    std::string b = "hello";

public:
    void do_something() { std::cout << "do something " << b << "\n"; }
};

class data_wrapper {
private:
    some_data data;
    std::mutex m;

public:
    template <typename Function> void process_data(Function func)
    {
        std::lock_guard l(m);
        // Pass "protected" data to user-supplied function;
        func(data);
    }
};

some_data* unprotected;

void malicious_function(some_data& protected_data)
{
    unprotected = &protected_data;
}

data_wrapper x;

void malicious_example()
{
    x.process_data(malicious_function);

    std::cout << "malicious: ";
    unprotected->do_something();
}

std::list<int> some_list;

// It's more common to group mutexes and protected data together in a
// class rather than use global variables.
std::mutex some_mutex;

void add_to_list(int new_value)
{
    // c++17 adds "called class template argument deduction" which allows
    // us to omit the template argument list
    //
    // std::lock_guard<std::mutex> guard(some_mutex);
    std::lock_guard guard(some_mutex);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find)
{
    std::lock_guard guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find)
        != some_list.end();
}

int main()
{
    malicious_example();

    // Using stack is safe only in single threaded code as we can't rely on what
    // is returned form empty() or size().

    std::stack<int> s;

    s.push(10);
    s.push(11);
    s.push(12);

    if (!s.empty()) {
        int const value = s.top();
        s.pop();
        std::cout << "doing something with value=" << value << "\n";
    }
}
