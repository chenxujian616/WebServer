#include "heaptimer.h"

HeapTimer::HeapTimer(/* args */)
{
    // 分配64个sizeof(TimerNode)的空间给vector
    heap_.reserve(64);
}

HeapTimer::~HeapTimer()
{
    clear();
}

void HeapTimer::siftup_(size_t i)
{
    // 小根堆节点上移方法
    assert(i >= 0 && i < heap_.size());
    // j是节点i的父节点
    size_t j = (i - 1) / 2;
    while (j >= 0)
    {
        // <生效是因为在struct做了重载
        if (heap_[j] < heap_[i])
        {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

/**
 * @brief 交换堆节点方法
 *
 * @param i 子节点
 * @param j 父节点
 */
void HeapTimer::SwapNode_(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

/**
 * @brief 小根堆节点下移方法
 *
 * @param index 需要调整的节点索引
 * @param n 小根堆的size，即最后一个元素，不参与堆节点调整
 * @return true 调整后的i比调整前的i大，返回true
 * @return false 调整后的i比调整前的i小，返回false，然后上移调整后的节点
 */
bool HeapTimer::siftdown_(size_t index, size_t n)
{
    // 小顶堆排序，下移
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

/**
 * @brief 添加新节点到小根堆中，时间到了自动调用callback函数
 *
 * @param id 文件描述符
 * @param timeOut 过期时间
 * @param cb 回调函数
 */
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
        // 修改已有节点的到期时间和回调函数
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
    // 修改新的到期时间后，新时间必然大于旧时间，故要把当前id的节点下移
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

/**
 * @brief 清除到期节点，这个节点必然是小顶堆的根节点，故索引为0
 *
 */
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