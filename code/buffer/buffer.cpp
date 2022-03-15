#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0)
{
}

// 可写入大小
size_t Buffer::WritableBytes(void) const
{
    return buffer_.size() - writePos_;
}

// 可读取大小
size_t Buffer::ReadableBytes(void) const
{
    // 写入位置-读取位置
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes(void) const
{
    return readPos_;
}

const char *Buffer::Peek(void) const
{
    return BeginPtr_() + readPos_;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    readPos_ += len;
}
void Buffer::RetrieveUntil(const char *end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll(void)
{
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}
std::string Buffer::RetrieveAllToStr(void)
{
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWriteConst(void) const
{
    return BeginPtr_() + writePos_;
}
char *Buffer::BeginWrite(void)
{
    return BeginPtr_() + writePos_;
}

void Buffer::HasWritten(size_t len)
{
    writePos_ += len;
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}
void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}
void Buffer::Append(const void *data, size_t len)
{
    assert(data);
    Append(static_cast<const char *>(data), len);
}
void Buffer::Append(const Buffer &buff)
{
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWritable(size_t len)
{
    if (WritableBytes() < len)
    {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

char *Buffer::BeginPtr_(void)
{
    // &*还是取址，把iterable变成char*
    return &*buffer_.begin();
}

const char *Buffer::BeginPtr_(void) const
{
    return &*buffer_.begin();
}

// ??
void Buffer::MakeSpace_(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len)
    {
        buffer_.resize(writePos_ + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes();
        // 复制readable内容到prependable中
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}