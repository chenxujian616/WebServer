#include "heaptimer.h"

HeapTimer::HeapTimer(/* args */)
{
    heap_.reserve(64);
}

HeapTimer::~HeapTimer()
{
    clear();
}

void HeapTimer::siftup_(size_t i)
{
    // 大顶堆排序
    assert(i >= 0 && i < heap_.size());
    // j是节点i的父节点
    size_t j = (i - 1) / 2;
    while (j >= 0)
    {
        if (heap_[j] < heap_[i])
        {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

bool HeapTimer::siftdown_(size_t index, size_t n)
{
    // 小顶堆排序
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());

    size_t i = index;
    // index的第一个子节点
    size_t j = 2 * i + 1;
    while (j < n)
    {
        // 节点i的右子节点
        if (j + 1 < n && heap_[j + 1] < heap_[j])
        {
            j++;
        }
        if (heap_[i] < heap_[j])
        {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = 2 * i + 1;
    }
    return i > index;
}

void HeapTimer::add(int id, int timeOut, const TimeoutCallback &cb)
{
    assert(id >= 0);
    size_t i;

    if (ref_.count(id) == 0)
    {
        // 新节点，堆尾插入，然后调整堆，排序
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeOut), cb});
        siftup_(i);
    }
    // 已有节点，调整堆
    else
    {
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeOut);
        heap_[i].cb = cb;
        if (!siftdown_(i, heap_.size()))
        {
            siftup_(i);
        }
    }
}

void HeapTimer::doWork(int id)
{
    // 删除指定id节点，并触发回调函数
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::del_(size_t index)
{
    // 删除指定位置的节点
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    // 将要删除的节点换到队尾，然后调整堆
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n)
    {
        // 将要删除的节点放在vector尾部
        SwapNode_(i, n);
        if (!siftdown_(i, n))
        {
            siftup_(i);
        }
    }
    // 删除队尾元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int newExpires)
{
    // 调整指定id的节点
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(newExpires);
    siftdown_(ref_[id], heap_.size());
}

void HeapTimer::tick(void)
{
    // 清除超时节点
    if (heap_.empty())
    {
        return;
    }
    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
        {
            break;
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop(void)
{
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear(void)
{
    // 清空vector、map容器
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick(void)
{
    tick();
    size_t res = -1;
    if (!heap_.empty())
    {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0)
        {
            res = 0;
        }
    }
    return res;
}