#include "webserver.h"
#include <iostream>

WebServer::WebServer(int port, int trigMode, int timeoutMS, bool OptLinger, int sqlPort,
                     const char *sqlUser, const char *sqlPwd, const char *dbName, int connPoolNum,
                     int threadNum, bool openLog, int logLevel, int logQueSize)
{
    port_ = port;
    openLinger_ = OptLinger;
    timeoutMS_ = timeoutMS;
    isClose_ = false;
    // 时间堆定时器智能指针，这是一个小顶堆数据结构
    timer_ = std::unique_ptr<HeapTimer>(new HeapTimer());
    // 线程池
    threadpool_ = std::unique_ptr<ThreadPool>(new ThreadPool(threadNum));
    epoller_ = std::unique_ptr<Epoller>(new Epoller());

    // getcwd获得当前终端的路径
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    // 将resources挂到字符串尾
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    // 连接本地MySQL
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);
    if (!InitSocket_())
    {
        isClose_ = true;
    }

    // 输出log日志
    if (openLog)
    {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if (isClose_)
        {
            LOG_ERROR("========= Server init error!=========");
        }
        else
        {
            LOG_INFO("========= Server init =========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger ? "true" : "false");
            // LT电平触发，是默认工作方式，在LT情况下epoll是一个效率较高的poll
            // ET边沿触发。当注册了一个EPOLLET事件时，epoll将以ET模式来操作该文件描述符，是epoll高效工作模式
            LOG_INFO("Listen Mode: %d, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET) ? "ET" : "LT",
                     (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer()
{
    // 关闭监听文件描述符
    close(listenFd_);
    isClose_ = true;
    // 释放srcDir_空间
    free(srcDir_);
    // 关闭SQL连接
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode_(int trigMode)
{
    // 监听线程EPOLLRDHUP事件
    listenEvent_ = EPOLLRDHUP;
    // 连接线程是EPOLLONESHOT|EPOLLRDHUP事件
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        // 将监听线程和连接线程的工作模式都设为ET边缘触发模式
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start(void)
{
    int timeMS = -1;
    if (!isClose_)
    {
        LOG_INFO("========= Server start =========");
    }
    while (!isClose_)
    {
        /* while循环中，基本流程是先让epoll内核态监听文件描述符，读取数据
            读取完后，往工作线程绑定OnRead_方法（此时epoll还是EPOLLIN，还没到EPOLLOUT状态）
            读取完后，自动调用Client->process()，同时修改epoll当前文件描述符监听信息
            修改后，再往工作线程绑定OnWrite_方法发送HTTP响应报文（所以说可以随时更改HTTP工作线程的绑定函数吗）
         */

        // timeous=-1表示没有事件处于阻塞状态
        if (timeoutMS_ > 0)
        {
            timeMS = timer_->GetNextTick();
        }
        // 非阻塞等待文件描述符事件
        int eventCnt = epoller_->Wait(timeMS);
        // 内核态检测到有文件描述符有事件发生
        for (int i = 0; i < eventCnt; i++)
        {
            // 处理事件
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);

            if (fd == listenFd_)
            {
                // 监听线程
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if (events & EPOLLIN)
            {
                // 有文件描述符读入数据
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                // 有文件描述符写出数据
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

/**
 * @brief 关闭HTTP连接，在epoll中删除该连接的文件描述符
 *
 * @param client HTTP连接
 */
void WebServer::CloseConn_(HttpConn *client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    // 删除文件描述符
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeoutMS_ > 0)
    {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    // 往epoller中添加文件描述符，使epoll自动监听
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    // 每一个新的连接都要设为非阻塞模式
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

/**
 * @brief 处理监听文件描述符
 *
 */
void WebServer::DealListen_(void)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    do
    {
        // 与客户端建立连接
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if (fd < 0)
        {
            return;
        }
        else if (HttpConn::userCount >= MAX_FD)
        {
            // 连接数量超过最大客户端文件描述符数量
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        // 服务器接收HTTP请求，此时与客户端浏览器建立TCP连接
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

/**
 * @brief 从HTTP连接对象中读取数据，在HTTP文件描述符有事件发生时调用，从工作线程池中获取线程
 *
 * @param client HTTP连接对象指针
 */
void WebServer::DealRead_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client)
{
    assert(client);
    if (timeoutMS_ > 0)
    {
        timer_->adjust(client->GetFd(), timeoutMS_);
    }
}

void WebServer::OnRead_(HttpConn *client)
{
    // OnRead要注意，是先有请求报文后服务器发送响应报文
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)
    {
        CloseConn_(client);
        return;
    }
    OnProcess_(client);
}

void WebServer::OnProcess_(HttpConn *client)
{
    if (client->process())
    {
        // 将该文件描述符设为EPOLLOUT状态，这样在while循环时，内核态监听到文件描述符处于EPOLLOUT
        // 之后就可以调用OnWrite_方法
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else
    {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0)
    {
        // 传输完成
        if (client->IsKeepAlive())
        {
            OnProcess_(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            // 继续传输
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

/**
 * @brief 监听文件描述符初始化
 *
 * @return true
 * @return false
 */
bool WebServer::InitSocket_(void)
{
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024)
    {
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }
    // IPv4
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = {0};
    if (openLinger_)
    {
        // 关闭，直到所剩数据发送完毕或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    // 得到监听客户端连接的文件描述符
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 设置监听文件描述符，避免监听描述符阻塞
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    // 端口复用(一般情况下都是一对一，但服务器需要一对多，所以要设SO_REUSEADDR)
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (ret == -1)
    {
        LOG_ERROR("set socket setsockopt error!");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERROR("Bind port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if (ret < 0)
    {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0)
    {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::SetFdNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
