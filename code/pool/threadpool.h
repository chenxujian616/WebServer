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
        // 互斥锁，使用线程时上锁
        std::mutex mtx;
        // 并发，条件变量
        std::condition_variable cond;
        bool isClosed;
        // std::function是函数包装器，也是一个模板类
        // tasks是一个任务队列
        std::queue<std::function<void()>> tasks;
    };
    // 创建结构体Pool指针
    std::shared_ptr<Pool> pool_;

public:
    // explicit关键字表示显示构造函数
    // 这个类负责初始化线程池
    explicit ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>())
    {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++)
        {
            /*
            [pool]是里面lambda函数的参数，std::thread是以函数指针为参数，{}里面就是lambda函数
            最后调用detach的原因是，工作线程创建后会自动join到主线程中，但此时程序还不需要工作线程
             */
            std::thread([pool = pool_]
                        { 
                            // 上锁，防止线程之间的资源竞争，创建locker对象后即上锁
                            std::unique_lock<std::mutex> locker(pool->mtx);
                        while(true)
                        {
                            if(!pool->tasks.empty())
                            {
                                // 取出任务队列头
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
                                // 线程池为空，等待其他socket完成释放资源
                                // wait会主动调用unlock释放锁
                                pool->cond.wait(locker);
                            }
                                                } })
                .detach();
        }
    }
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool()
    {
        if (static_cast<bool>(pool_))
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->isClosed = true;
        }
        // notify_all唤醒全部等待线程
        pool_->cond.notify_all();
    }

    // 这里并不是引用的引用，而是一个引用折叠(C++没有引用的引用)
    // 引用折叠后依然是一个普通的引用或右值引用，不存在所谓的“引用的引用”
    // 引用折叠只是为了std::move、std::forward等函数服务
    template <class F>
    void AddTask(F &&task)
    {
        {
            // 把任务添加到队列中
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }
};

#endif