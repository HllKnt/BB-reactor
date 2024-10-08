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

    void lock();
    void unlock();
    template <typename F, typename... Args>
    auto append(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
    void distribute();
    void drain();

private:
    bool over;
    size_t capacity;
    size_t additionalTasks;
    std::vector<std::thread> threads;
    std::binary_semaphore head, tail;
    std::counting_semaphore<1> semaphore;
    std::unique_ptr<std::queue<std::function<void()>>> buffer;

    void work();
};

}  // namespace frame

namespace frame {
template <typename F, typename... Args>
auto ThreadPool::append(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    auto&& func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto&& task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    buffer->emplace([task]() { (*task)(); });
    additionalTasks++;
    return task->get_future();
}

}  // namespace frame
