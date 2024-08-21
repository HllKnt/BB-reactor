#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <queue>
#include <semaphore>

namespace frame {
class ThreadPool
{
public:
    ThreadPool(size_t size);
    ~ThreadPool();

    void drain();
    void lock();
    void unlock();
    void distribute();
    template <typename F, typename... Args>
    auto append(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

private:
    bool over;
    size_t cnt;
    std::vector<std::thread> threads;
    std::binary_semaphore head, tail;
    std::counting_semaphore<1> semaphore;
    std::queue<std::function<void()>> tasks;

    void work();
};

}  // namespace frame

namespace frame {
template <typename F, typename... Args>
auto ThreadPool::append(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    cnt++;
    auto&& func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto&& task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    tasks.emplace([task]() { (*task)(); });
    return task->get_future();
}

}  // namespace frame
