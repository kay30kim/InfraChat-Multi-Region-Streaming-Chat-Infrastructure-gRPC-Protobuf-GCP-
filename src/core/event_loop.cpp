#include "core/event_loop.hpp"

#include <stdexcept>

namespace infrachat::core {

EventLoop::EventLoop() = default;

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("event loop is already running");
    }

    worker_ = std::thread(&EventLoop::run, this);
}

void EventLoop::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    cv_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }
}

void EventLoop::post(Task task) {
    if (!task) {
        throw std::invalid_argument("task must not be empty");
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }

    cv_.notify_one();
}

bool EventLoop::isRunning() const {
    return running_.load();
}

void EventLoop::run() {
    while (running_.load()) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            cv_.wait(lock, [this]() {
                return !tasks_.empty() || !running_.load();
            });

            if (!running_.load() && tasks_.empty()) {
                break;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try {
            task();
        } catch (...) {
            // For now, swallow exceptions to keep the loop alive.
            // Later, this can be replaced with logger/error metrics.
        }
    }
}

}  // namespace infrachat::core