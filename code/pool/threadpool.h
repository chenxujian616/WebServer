/**
 * @file threadpool.h
 * @author your name (you@domain.com)
 * @brief 线程池
 * @version 0.1
 * @date 2022-03-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
// functional提供了函数类模板
#include <functional>
#include <assert.h>

class ThreadPool
{
private:
    struct Pool
    {
        std::mutex mtx;
        // 并法，条件变量
        std::condition_variable cond;
        bool isClosed;
        // std::function是函数包装器，也是一个模板类
        // tasks是一个任务队列
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;

public:
    explicit ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>())
    {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++)
        {
            // [pool]是里面lambda函数的参数，std::thread是以函数指针为参数
            std::thread([pool = pool_]
                        { std::unique_lock<std::mutex> locker(pool->mtx);
                        while(true)
                        {
                            if(!pool->tasks.empty())
                            {
                                auto task = std::move(pool->tasks.front());
                                pool->tasks.pop();
                                locker.unlock();
                                task();
                                locker.lock();
                            }
                            else if(pool->isClosed)
                            {
                                break;
                            }
                            else
                            {
                                pool->cond.wait(locker);
                            }
                                                } })
                .detach();
        }
    }
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool();

    template <class F>
    void AddTask(F &&task)
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F> task);
        }
        pool_->cond.notify_one();
    }
};

ThreadPool::~ThreadPool()
{
    if (static_cast<bool>(pool_))
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->isClosed = true;
    }
    pool_->cond.notify_all();
}

#endif