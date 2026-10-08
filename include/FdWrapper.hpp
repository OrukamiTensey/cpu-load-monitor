#pragma once

#include <unistd.h>
#include <utility>
/**
 * Lightweight RAII wrapper for POSIX file descriptors.
 * Ensures strict single-ownership semantics and deterministic closure on destruction.
 */
class FdWrapper {
public:
    constexpr FdWrapper() noexcept : fd_(-1) {}
    explicit constexpr FdWrapper(int fd) noexcept : fd_(fd) {}

    ~FdWrapper() noexcept {
        reset();
    }
    // Disable copy semantics to prevent duplicate ownership of the descriptor
    FdWrapper(const FdWrapper&) = delete;
    FdWrapper& operator=(const FdWrapper&) = delete;

    // Enable move semantics for safe resource transfer
    FdWrapper(FdWrapper&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    FdWrapper& operator=(FdWrapper&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

    int get() const noexcept {
        return fd_;
    }

    bool isValid() const noexcept {
        return fd_ >= 0;
    }

private:
    int fd_;
};

/**
 * Helper to perform POSIX write and suppress unused-result compiler warnings.
 */
inline void safeWrite(int fd, const void* buf, size_t count) noexcept {
    ssize_t res = ::write(fd, buf, count);
    (void)res;
}