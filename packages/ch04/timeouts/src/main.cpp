#include <chrono>
#include <condition_variable>
#include <mutex>

std::condition_variable cv;
bool done;
std::mutex m;

// Recommended way to wait for condition variables with a time limit when not
// passign a predicate to wait.
bool wait_loop()
{
    auto const timeout
        = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    std::unique_lock lk(m);
    while (!done) {
        if (cv.wait_until(lk, timeout) == std::cv_status::timeout) {
            break;
        }
    }
    return done;
}

int main() { return 0; }
