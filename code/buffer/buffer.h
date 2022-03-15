#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <assert.h>

/*
    Buffer类把内核暂时无法接收的数据先存起来
    这个类是一个Muduo Buffer数据结构
 */
class Buffer
{
private:
    char *BeginPtr_(void);
    const char *BeginPtr_(void) const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    // 可以确保所有其他线程不会在同一时间访问以下资源
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;

public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    /*
        prependable为预留空间
        readable为剩余读取空间
        writable为写入剩余空间
     */
    size_t WritableBytes(void) const;
    size_t ReadableBytes(void) const;
    size_t PrependableBytes(void) const;

    const char *Peek(void) const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    // 这里有点相当于增加readable范围
    void Retrieve(size_t len);
    void RetrieveUntil(const char *end);
    // RetrieveAll相当把整个buffer变为readable
    void RetrieveAll(void);
    std::string RetrieveAllToStr(void);

    const char *BeginWriteConst(void) const;
    char *BeginWrite(void);

    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const Buffer &buff);

    ssize_t ReadFd(int fd, int *errno);
    ssize_t WriteFd(int fd, int *errno);
};

#endif