#ifndef LEDCUBE_MULTITHREADING_H
#define LEDCUBE_MULTITHREADING_H

#include "PCH.h"

template<class T>
class MultithreadCommandQueue {
public:

    T& AwaitAvailable() {
        std::unique_lock lk(_mutex);
        _condition.wait(lk, [&] { return !_queue.empty(); });
        return _queue.front();
    }
    T& GetFront() const {
        std::unique_lock lk(_mutex);
        return _queue.front();    
    }
    void PopFront() {
        std::unique_lock lk(_mutex);
        _queue.pop_front();
        _condition.notify_one();
    }
    void PushBack(const T& t) {
        std::unique_lock lk(_mutex);
        _queue.push_back(t);
        _condition.notify_one();
    }
    inline bool Contains(const T& t) {
        return std::find(_queue.begin(), _queue.end(), t) == _queue.end(); 
    }
    // AwaitNotInQueue
    void AwaitProcessed(const T& t) {
        std::unique_lock lk(_mutex);
        _condition.wait(lk, [&] {
            return Contains(t);
        });
    }
    void AwaitInQueue(const T& t) {
        std::unique_lock lk(_mutex);
        _condition.wait(lk, [&] {
            return !Contains(t);
        });
    }

private:
    std::deque<T> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;
};

#endif