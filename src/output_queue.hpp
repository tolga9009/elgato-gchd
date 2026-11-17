#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
class OutputQueue {
public:
    using Buffer=std::vector<unsigned char>;
    using Sink=std::function<void(Buffer &,const std::atomic<bool> *)>;
    explicit OutputQueue(Sink sink, size_t capacity=256);
    ~OutputQueue();
    bool push(Buffer buffer);
    void check();
    void finish();
    uint64_t written() const { return written_.load(); }
    uint64_t dropped() const { return dropped_.load(); }
    uint64_t droppedBytes() const { return droppedBytes_.load(); }
private:
    Sink sink_;
    size_t capacity_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<Buffer> buffers_;
    std::exception_ptr failure_;
    bool finishing_=false, done_=false;
    std::atomic<bool> cancel_{false};
    std::atomic<uint64_t> written_{0}, dropped_{0}, droppedBytes_{0};
    std::thread thread_;
    void run() noexcept;
};
