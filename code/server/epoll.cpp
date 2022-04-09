#include "epoll.h"

// epoll_create告诉内核监听的数目为512个
Epoller::Epoller(int maxEvent) : epollFd_(epoll_create(512)), events_(maxEvent)
{
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller()
{
    // 使用完epoll后，必须调用close关闭，否则导致文件描述符fd被耗尽
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events)
{
    if (fd < 0)
    {
        return false;
    }
    // epoll事件结构体
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    // epoll_ctl是一个事件注册函数
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events)
{
    if (fd < 0)
    {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd)
{
    if (fd < 0)
    {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

// 等待文件描述符
int Epoller::Wait(int timeoutMs)
{
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}