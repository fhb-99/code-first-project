#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <deque>
#include <mutex>
#include <condition_variable>


#define MAX_QUEUE_SIZE 50

template<typename T>
class FrameBuffer
{
public:
    FrameBuffer() = default;
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator= (const FrameBuffer&) = delete;

    void push(T value) {
        std::unique_lock<std::mutex> lock(mutex);

        no_full_cond.wait(lock, [this](){
            return FrameBufs.size() < MAX_QUEUE_SIZE;
        });

        FrameBufs.push_back(std::move(value));
        no_empty_cond.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        no_empty_cond.wait(lock, [this](){
            return !FrameBufs.empty();
        });
        T value = std::move(FrameBufs.front());
        FrameBufs.pop_front();
        no_full_cond.notify_one();
        return value;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex);
        if(FrameBufs.empty()) return false;
        value = std::move(FrameBufs.front());
        FrameBufs.pop_front();
        no_full_cond.notify_one();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        FrameBufs.clear();
        no_full_cond.notify_all();
    }

private:
    std::deque<T> FrameBufs;
    std::mutex mutex;
    std::condition_variable no_empty_cond;
    std::condition_variable no_full_cond;
};

#endif // FRAMEBUFFER_H
