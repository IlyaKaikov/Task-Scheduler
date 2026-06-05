#pragma once

#include <cstddef>

namespace mt {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);

    [[nodiscard]] std::size_t worker_count() const noexcept;

private:
    std::size_t worker_count_;
};

} // namespace mt
