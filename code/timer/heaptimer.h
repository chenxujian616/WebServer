#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>

#include "../log/log.h"

// 函数对象容器模板，这样回调的参数就可以是一个函数(传统方法函数指针也行)
typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallback cb;
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};
/**
 * @brief 时间堆定时器
 * 只有时间最短的定时器会首先被触发，因此可以采用最小堆
 * 按时间排序，堆顶的元素就是时间最短的定时器，故只需要判断堆顶元素是否被触发
 *
 */
class HeapTimer
{
private:
    void del_(size_t index);
    void siftup_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;

public:
    HeapTimer(/* args */);
    ~HeapTimer();

    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeoutCallback &cb);
    void doWork(int id);
    void clear(void);
    void tick(void);
    void pop(void);
    int GetNextTick(void);
};

#endif