#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <queue>
#include <memory>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <utility>

class ThreadPool {
public:
    // 在线程池中创建threads个工作线程
    ThreadPool(size_t threads) : stop(false)
    {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this] {
                    for(;;) {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> 
                                lock(this -> queue_mutex);

                            this -> condition.wait(lock,
                                [this] {
                                    return this -> stop || 
                                        !this -> tasks.empty();
                                });

                            if(this -> stop && tasks.empty())
                                return;

                            task = 
                                std::move(this -> tasks.front());
                            this -> tasks.pop();
                        }

                        task();
                    }
                }
            );
    }

    // 在线程池中增加线程
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = 
            typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task -> get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if(stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task] { (*task)(); });
        }

        condition.notify_one();
        return res;
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            stop = true;
        }

        condition.notify_all();

        for(std::thread & worker : workers)
            worker.join();
    }

private:
    // 需要持续追踪线程来保证可以使用join
    std::vector<std::thread> workers;
    // 任务队列
    std::queue<std::function<void()>> tasks;

    // 同步相关
    std::mutex queue_mutex; // 互斥锁
    std::condition_variable condition; // 互斥条件变量

    // 停止相关
    bool stop;
};

#endif
