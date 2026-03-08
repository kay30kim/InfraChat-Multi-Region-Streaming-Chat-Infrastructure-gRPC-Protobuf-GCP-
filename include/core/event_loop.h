#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace infrachat::core {

class EventLoop {
public:
    using Task = std::function<void()>;

    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void start();
    void stop();
    void post(Task task);
    bool isRunning() const;

private:
    void run();

    std::atomic<bool> running_{false};
    std::thread worker_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
};

}  // namespace infrachat::core