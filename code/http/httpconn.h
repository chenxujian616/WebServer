/**
 * @file httpconn.h
 * @author your name (you@domain.com)
 * @brief HTTP连接头文件
 * @version 0.1
 * @date 2022-03-15
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
// readv，writev方法声明
#include <sys/uio.h>
/*
    arpa/inet.h里面定义了网络操作方法
    详见网址https://pubs.opengroup.org/onlinepubs/7908799/xns/arpainet.h.html
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    /* iovCnt_、iov_都是用于writev函数，用于在一次函数调用中写多个非连续缓冲区 */
    int iovCnt_;
    // 向量元素
    struct iovec iov_[2];

    // 读缓冲区
    Buffer readBuff_;
    // 写缓冲区
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;

public:
    HttpConn(/* args */);
    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);
    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);

    void Close(void);
    int GetFd(void) const;
    int GetPort(void) const;

    const char *GetIP(void) const;
    sockaddr_in GetAddr(void) const;

    bool process(void);
    int ToWriteBytes(void)
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }
    bool IsKeepAlive(void) const
    {
        return request_.IsKeepAlive();
    }

    // ET模式？
    static bool isET;
    // resources目录
    static const char *srcDir;
    // 用户数量
    static std::atomic<int> userCount;
};

#endif