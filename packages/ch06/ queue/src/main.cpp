#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace threadsafe_queue_naive {
template <typename T> class threadsafe_queue {
private:
    mutable std::mutex m;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue() { }

    void push(T new_value)
    {
        std::lock_guard l(m);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock l(m);
        data_cond.wait(l, [this] { return !data_queue.empty(); });
        value = std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock l(m);
        data_cond.wait(l, [this] { return !data_queue.empty(); });
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard l(m);

        if (data_queue.empty()) {
            return false;
        }

        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard l(m);

        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard l(m);
        return data_queue.empty();
    }
};
}

namespace thread_safe_queue_improved {
template <typename T> class threadsafe_queue {
private:
    mutable std::mutex m;
    std::condition_variable data_cond;

    // Store std::shared_pointer<T> instead of T directly
    std::queue<std::shared_ptr<T>> data_queue;

public:
    threadsafe_queue() { }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> l(m);
        data_cond.wait(l, [this] { return !data_queue.empty(); });
        value = std::move(*data_queue.front()); // Dereference the pointer
        data_queue.pop();
    }

    bool try_pop(T& value)
    {
        std::lock_guard l(m);
        if (data_queue.empty()) {
            return false;
        }
        value = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock l(m);
        data_cond.wait(l, [this] { return !data_queue.empty(); });
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard l(m);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    void push(T new_value)
    {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        std::lock_guard l(m);
        data_queue.push(data);
        data_cond.notify_one();
    }

    bool empty()
    {
        std::lock_guard l(m);
        return data_queue.empty();
    }
};
}

namespace ll_queue_naive {
template <typename T> class queue {
private:
    struct node {
        T data;
        // Notice the use of std::unique_ptr here
        std::unique_ptr<node> next;
        node(T data_)
            : data(std::move(data_)) { };
    };

    std::unique_ptr<node> head;
    node* tail;

public:
    queue()
        : tail(nullptr)
    {
    }

    // Delete copy constructors
    queue(const queue& other) = delete;
    queue& operator=(const queue& other) = delete;

    std::shared_ptr<T> try_pop()
    {
        if (!head) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> const res(
            std::make_shared<T>(std::move(head->data)));
        std::unique_ptr<node> const old_head = std::move(head);
        head = std::move(old_head->next);

        if (!head) {
            tail = nullptr;
        }

        return res;
    }

    void push(T new_value)
    {
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node* const new_tail = p.get();

        if (tail) {
            tail->next = std::move(p);
        } else {
            head = std::move(p);
        }

        tail = new_tail;
    }
};
}

namespace ll_queue_with_dummy {
template <typename T> class queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::unique_ptr<node> head;
    node* tail;

public:
    queue()
        : head(new node)
        , tail(head.get())
    {
    }

    queue(const queue& other) = delete;
    queue& operator=(const queue& other) = delete;

    std::shared_ptr<T> try_pop()
    {
        // Note that tail is used only here, so the lock required for thread
        // safe operation is short-lived
        if (head.get() == tail) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> const res(head->data);
        std::unique_ptr<node> old_head = std::move(head);
        // We know that next is not NULL as the if above early returns
        head = std::move(old_head->next);

        return res;
    }

    // Push now accesses only tail, so we don't have to lock both on tail and
    // head as before in case of threadsafe implementation
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        tail->data = new_data;
        node* const new_tail = p.get();
        tail->next = std::move(p);
        tail = new_tail;
    }
};
}

namespace ll_queue_threadsafe {
template <typename T> class threadsafe_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::mutex head_mutex;
    std::unique_ptr<node> head;

    std::mutex tail_mutex;
    node* tail;

    node* get_tail()
    {
        std::lock_guard tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head()
    {
        std::lock_guard head_lock(head_mutex);

        if (head.get() == get_tail()) {
            return nullptr;
        }

        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue()
        : head(new node)
        , tail(head.get())
    {
    }

    threadsafe_queue(const threadsafe_queue& other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::make_shared<T>();
    }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node* const new_tail = p.get();
        std::lock_guard tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }
};
}

namespace ll_queue_threadsafe_with_cond_var {
template <typename T> class threadsafe_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    mutable std::mutex head_mutex;
    mutable std::unique_ptr<node> head;

    std::mutex tail_mutex;
    node* tail;

    std::condition_variable data_cond;

    node* get_tail()
    {
        std::lock_guard tail_lock(tail_mutex);
        return tail;
    }

    /// Modified the list to remote the head item.
    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    /// Wait for the queue to have some data to pop.
    std::unique_lock<std::mutex> wait_for_data()
    {
        std::unique_lock head_lock(head_mutex);
        data_cond.wait(head_lock, [&] { return head.get() != get_tail(); });

        // return std::move(head_lock);
        // We can return head_lock directly thanks to copy elision
        return head_lock;
    }

    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock head_lock(wait_for_data());
        std::unique_ptr<node> old_head = pop_head();
        value = std::move(*old_head->data);
        return old_head;
    }

    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }

public:
    threadsafe_queue()
        : head(new node)
        , tail(head.get())
    {
    }

    threadsafe_queue(const threadsafe_queue& other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);

        {
            std::lock_guard tail_lock(tail_mutex);
            tail->data = new_data;
            node* const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }

        data_cond.notify_one();
    }

    bool empty() const
    {
        std::lock_guard head_lock(head_mutex);
        return head.get() == get_tail();
    }
};
}

int main() { return 0; }
