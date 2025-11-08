#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t n) : buf_(n), head_(0) {}
    void push(const T* data, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            buf_[head_] = data[i];
            head_ = (head_ + 1) % buf_.size();
        }
    }
    const std::vector<T>& data() const { return buf_; }
    size_t head() const { return head_; }
    size_t size() const { return buf_.size(); }
private:
    std::vector<T> buf_;
    size_t head_;
};
