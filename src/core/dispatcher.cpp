#include "core/dispatcher.hpp"

namespace infrachat::core {

void Dispatcher::registerHandler(MessageType type, Handler handler) {
    if (!handler) {
        throw std::invalid_argument("handler must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[type] = std::move(handler);
}

void Dispatcher::dispatch(const Message& message) const {
    Handler handler;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(message.type);
        if (it == handlers_.end()) {
            throw std::runtime_error("no handler registered for this message type");
        }
        handler = it->second;
    }

    handler(message);
}

bool Dispatcher::hasHandler(MessageType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handlers_.find(type) != handlers_.end();
}

}  // namespace infrachat::core