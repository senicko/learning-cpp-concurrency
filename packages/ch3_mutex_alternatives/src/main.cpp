#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

class dns_entry { };

// Example of a simple and naive DNS cache using shared mutexes.
class dns_cache {
    std::map<std::string, dns_entry> entries;

    // This needs to be marked mutable to allow locking inside member functions.
    mutable std::shared_mutex entry_mutex;

public:
    dns_entry find_entry(std::string const& domain) const
    {
        std::shared_lock<std::shared_mutex> lk(entry_mutex);
        std::map<std::string, dns_entry>::const_iterator const it
            = entries.find(domain);
        return (it == entries.end()) ? dns_entry() : it->second;
    }

    void update_or_add_entry(
        std::string const& domain, dns_entry const& dns_details)
    {
        std::lock_guard<std::shared_mutex> lk(entry_mutex);
        entries[domain] = dns_details;
    }
};

struct data_packet {
    std::string payload;
};

struct connection_info {
    std::string host;
};

struct connection_handle {
    std::string host;

    void send_data(data_packet const& data)
    {
        std::cout << "send to " << host << ": " << data.payload << '\n';
    }

    data_packet receive_data()
    {
        std::cout << "receive from " << host << '\n';
        return data_packet { .payload = "data" };
    }
};

connection_handle connect_to(connection_info const& info)
{
    return connection_handle { .host = info.host };
}

// This is an example of lazy one-time connection setup inside a class. While we
// could protect handle with mutex, it's much better to use std::call_once
class X {
private:
    connection_info info;
    connection_handle handle;
    std::once_flag connection_init_flag;

    // This is the function that will be called exactly once using
    // std::call_once.
    void open_connection() { handle = connect_to(info); }

public:
    X(connection_info const& info_)
        : info(info_)
    {
    }

    void send_data(data_packet const& data)
    {
        std::call_once(connection_init_flag, &X::open_connection, this);
        handle.send_data(data);
    }

    data_packet receive_data()
    {
        std::call_once(connection_init_flag, &X::open_connection, this);
        return handle.receive_data();
    }
};

struct some_resource {
    std::string name;

    void do_something()
    {
        std::cout << "doing something with resource=" << name << "\n";
    }
};

std::shared_ptr<some_resource> resource_ptr;

// std::once_flag stores neccessary initialization synchronization data. Each
// instance of std::once_flag corresponds to a different initialization.
std::once_flag resource_flag;

void init_resource()
{
    resource_ptr.reset(new some_resource { .name = "resource" });
}

void foo()
{
    // This makes sure initialization gets called exactly once.
    std::call_once(resource_flag, init_resource);
    resource_ptr->do_something();
}

int main()
{
    std::cout << "hello world\n";
    foo();
}
