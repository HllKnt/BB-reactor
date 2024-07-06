#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <semaphore>

namespace localSocket {
class ThreadPool
{
public:
    explicit ThreadPool(size_t size) : state(creation), semaphore(0) {
        threads.reserve(size);
        for (int i = 0; i < size; i++) threads.emplace_back(&ThreadPool::work, this);
    }

    ~ThreadPool() {
        drain();
        for (auto& i : threads) i.join();
    }

    void drain() {
        state = destruction;
        for (auto& i : threads) semaphore.release();
    }

    template <typename F, typename... Args>
    auto append(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        auto&& func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        auto res = task->get_future();
        std::unique_lock<std::mutex> lock{tail};
        tasks.emplace([task]() { (*task)(); });
        lock.unlock();
        semaphore.release();
        return res;
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex tail, head;
    std::counting_semaphore<1> semaphore;
    enum { creation, destruction } state;

    void work() {
        while (state != destruction) {
            semaphore.acquire();
            std::unique_lock<std::mutex> lock{head};
            if (tasks.empty()) continue;
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();
            task();
        }
    }
};

}  // namespace threadPool

#endif  // !THREADPOOL_H
