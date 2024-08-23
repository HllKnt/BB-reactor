#include "../inc/threadPool.hpp"

using namespace frame;

ThreadPool::ThreadPool(size_t size) : over{false}, cnt{0}, semaphore{0}, head{1}, tail{1} {
    for (int i = 0; i < size; i++) {
        threads.emplace_back(&ThreadPool::work, this);
    } 
}

ThreadPool::~ThreadPool() { drain(); }

void ThreadPool::drain() {
    over = true;
    semaphore.release(threads.size());
    for (auto& i : threads) {
        if (i.joinable()) {
            i.join();
        }
    }
}

void ThreadPool::lock() { tail.acquire(); }

void ThreadPool::unlock() { tail.release(); }

void ThreadPool::distribute() {
    semaphore.release(cnt);
    cnt = 0;
}

void ThreadPool::work() {
    while (true) {
        semaphore.acquire();
        if (over)
            return;
        head.acquire();
        auto task = tasks.front();
        tasks.pop();
        head.release();
        task();
    }
}
