/**
 * @file blockqueue.h
 * @author your name (you@domain.com)
 * @brief 队列
 * @version 0.1
 * @date 2022-03-12
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
// 断言
#include <assert.h>
#include <sys/time.h>

#include <chrono>

template <class T>
class BlockDeque
{
private:
    // 双向队列
    std::deque<T> deq_;
    // 队列容纳能力
    size_t capacity_;
    std::mutex mtx_;
    bool isClose_;

    // 生产者消费者模型
    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;

public:
    // explicit为显示构造函数
    explicit BlockDeque(size_t maxCapacity = 1000);
    ~BlockDeque();

    void clear(void);
    bool empty(void);
    bool full(void);
    void Close(void);

    size_t size(void);
    size_t capacity(void);

    T front(void);
    T back(void);
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeOut);

    void flush(void);
};

template <class T>
BlockDeque<T>::BlockDeque(size_t maxCapacity) : capacity_(maxCapacity)
{
    assert(maxCapacity > 0);
    isClose_ = false;
}

template <class T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

// 清空线程队列
template <class T>
void BlockDeque<T>::Close(void)
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

// 同样为清空，需要在关闭前调用该函数清空数据
template <class T>
void BlockDeque<T>::flush(void)
{
    condConsumer_.notify_one();
}

template <class T>
void BlockDeque<T>::clear(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template <class T>
T BlockDeque<T>::front(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template <class T>
T BlockDeque<T>::back(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template <class T>
size_t BlockDeque<T>::size(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template <class T>
size_t BlockDeque<T>::capacity(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template <class T>
bool BlockDeque<T>::full(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template <class T>
bool BlockDeque<T>::empty(void)
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template <class T>
void BlockDeque<T>::push_back(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template <class T>
void BlockDeque<T>::push_front(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template <class T>
bool BlockDeque<T>::pop(T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    // 空队列等待
    while (deq_.empty())
    {
        condConsumer_.wait(locker);
        // 可能有其他线程导致isClose变为true
        if (isClose_ == true)
        {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template <class T>
bool BlockDeque<T>::pop(T &item, int timeOut)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty())
    {
        if (condConsumer_.wait_for(locker, std::chrono::seconds(timeOut)) ==
            std::cv_status::timeout)
        {
            return false;
        }
        if (isClose_ == true)
        {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}
#endif