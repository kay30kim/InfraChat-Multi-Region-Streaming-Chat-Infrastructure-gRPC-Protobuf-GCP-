#pragma once

#include <functional>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

#include "core/message.hpp"

namespace infrachat::core {

class Dispatcher {
public:
    using Handler = std::function<void(const Message&)>;

    Dispatcher() = default;
    ~Dispatcher() = default;

    void registerHandler(MessageType type, Handler handler);
    void dispatch(const Message& message) const;
    bool hasHandler(MessageType type) const;

private:
    struct EnumClassHash {
        template <typename T>
        std::size_t operator()(T t) const {
            return static_cast<std::size_t>(t);
        }
    };

    mutable std::mutex mutex_;
    std::unordered_map<MessageType, Handler, EnumClassHash> handlers_;
};

}  // namespace infrachat::core