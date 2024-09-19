#include "../inc/threadPool.hpp"

#include <cstdio>

using namespace frame;

ThreadPool::ThreadPool(size_t size)
    : over{false},
      semaphore{0},
      head{1},
      tail{1},
      additionalTasks{0},
      capacity{500 * size},
      buffer{new std::queue<std::function<void()>>} {
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
    while (buffer->size() > capacity) {
        head.acquire();
        if (buffer->empty()) {
            head.release();
            break;
        }
        auto task = buffer->front();
        buffer->pop();
        head.release();
        task();
        additionalTasks--;
    }
    semaphore.release(additionalTasks);
    additionalTasks = 0;
}

void ThreadPool::work() {
    while (true) {
        semaphore.acquire();
        if (over) {
            return;
        }
        head.acquire();
        if (buffer->empty()) {
            head.release();
            continue;
        }
        auto task = buffer->front();
        buffer->pop();
        head.release();
        task();
    }
}
