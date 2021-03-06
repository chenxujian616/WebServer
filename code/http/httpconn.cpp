#include "httpconn.h"
using namespace std;

const char *HttpConn::srcDir;
atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn(/* args */)
{
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::Init(int sockFd, const sockaddr_in &addr)
{
    assert(sockFd > 0);
    // 每有一个新连接，都会创建一个新的文件描述符
    userCount++;
    addr_ = addr;
    fd_ = sockFd;
    // 每init一个都会创建一个缓冲区
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close(void)
{
    response_.UnmapFile();
    if (isClose_ == false)
    {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd(void) const
{
    return fd_;
}

const char *HttpConn::GetIP(void) const
{
    // 将IP从主机转换为点分十进制的字符串形式
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::GetAddr(void) const
{
    return addr_;
}

int HttpConn::GetPort(void) const
{
    return addr_.sin_port;
}

ssize_t HttpConn::read(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0)
        {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0)
        {
            *saveErrno = errno;
            break;
        }
        // 传输结束
        if (iov_[0].iov_len + iov_[1].iov_len == 0)
        {
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len)
        {
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else
        {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process(void)
{
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_))
    {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else
    {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    // HTTP响应报文（MakeResponse没有添加响应体内容，也没有发送数据）
    response_.MakeResponse(writeBuff_);
    // 响应行、响应头的起始地址和长度
    iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    // 文件
    if (response_.FileLen() > 0 && response_.File())
    {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d to %d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}