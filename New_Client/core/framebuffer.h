#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class FrameBuffer
{
public:
    FrameBuffer() = default;
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator= (const FrameBuffer&) = delete;

    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex);
        FrameBufs.push_back(std::move(value));
        cond.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this](){
            return !FrameBufs.empty();
        });
        T value = std::move(FrameBufs.front());
        FrameBufs.pop_front();
        return value;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex);
        if(FrameBufs.empty()) return false;
        value = std::move(FrameBufs.front());
        FrameBufs.pop_front();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        FrameBufs.clear();
    }

private:
    std::deque<T> FrameBufs;
    std::mutex mutex;
    std::condition_variable cond;
};

#endif // FRAMEBUFFER_H
