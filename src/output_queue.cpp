#include "output_queue.hpp"
#include "output_io.hpp"
#include <chrono>
#include <stdexcept>
OutputQueue::OutputQueue(Sink sink, size_t capacity): sink_(std::move(sink)), capacity_(capacity) {
    if (!capacity_) throw std::invalid_argument("Output queue capacity must be positive");
    thread_=std::thread(&OutputQueue::run,this);
}
OutputQueue::~OutputQueue() { try { finish(); } catch (...) {} }
void OutputQueue::check() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (failure_) std::rethrow_exception(failure_);
}
bool OutputQueue::push(Buffer buffer) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (failure_) std::rethrow_exception(failure_);
    if (finishing_) throw std::logic_error("Output queue is closed");
    if (buffers_.size()>=capacity_) { ++dropped_; droppedBytes_+=buffer.size(); return false; }
    buffers_.push_back(std::move(buffer)); ready_.notify_one(); return true;
}
void OutputQueue::run() noexcept {
    for (;;) {
        Buffer buffer;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock,[&] { return finishing_ || !buffers_.empty(); });
            if (buffers_.empty()) break;
            buffer=std::move(buffers_.front()); buffers_.pop_front();
        }
        try { sink_(buffer,&cancel_); written_+=buffer.size(); }
        catch (const output_cancelled &) { ++dropped_; droppedBytes_+=buffer.size(); break; }
        catch (...) {
            std::lock_guard<std::mutex> lock(mutex_);
            failure_=std::current_exception(); ++dropped_; droppedBytes_+=buffer.size(); break;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &buffer: buffers_) { ++dropped_; droppedBytes_+=buffer.size(); }
        buffers_.clear(); done_=true;
    }
    ready_.notify_all();
}
void OutputQueue::finish() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        finishing_=true; ready_.notify_all();
        if (!ready_.wait_for(lock,std::chrono::seconds(2),[&] { return done_; })) cancel_=true;
    }
    if (thread_.joinable()) thread_.join();
    check();
}
