#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoll.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer
{
private:
    // 初始化socket
    bool InitSocket_(void);
    void InitEventMode_(int trigMode);

    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_(void);
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    void SendError_(int fd, const char *info);
    void ExtentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess_(HttpConn *client);

    // 最大连接数
    static const int MAX_FD = 65535;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    // 监听的文件描述符
    int listenFd_;
    char *srcDir_;

    // 监听事件、连接事件
    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;

public:
    /**
     * @brief Construct a new Web Server object
     *
     * @param port 端口号
     * @param trigMode 触发模式，示例使用ET，作用未明
     * @param timeoutMS
     * @param OptLinger
     * @param sqlPort MySql端口
     * @param sqlUser MySql用户名
     * @param sqlPwd MySql用户密码
     * @param dbName 数据库名称
     * @param connPoolNum 连接池数量
     * @param threadNum 线程池数量
     * @param openLog 日志开关
     * @param logLevel 日志等级
     * @param logQueSize 日志异步队列容量
     */
    WebServer(int port, int trigMode, int timeoutMS, bool OptLinger, int sqlPort,
              const char *sqlUser, const char *sqlPwd, const char *dbName, int connPoolNum,
              int threadNum, bool openLog, int logLevel, int logQueSize);
    ~WebServer();

    void Start(void);
};

#endif