#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

typedef struct epoll_event EPOLL_EVENT;

class Epoller
{
private:
    // epollFd_本身就是一个文件描述符，用create创建epoll句柄
    int epollFd_;
    // std::vector<EPOLL_EVENT> events_;
    // 创建epoll_event结构体集合，表示内核所监听的事件
    std::vector<struct epoll_event> events_;

public:
    // explicit为显示构造函数
    /**
     * @brief 创建Epoller对象
     *
     * @param maxEvent epoll_event结构体数组数量
     */
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    // fd是文件描述符
    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;
};

#endif