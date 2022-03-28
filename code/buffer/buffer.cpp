#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0)
{
}

// writeable空间大小
size_t Buffer::WritableBytes(void) const
{
    return buffer_.size() - writePos_;
}

// readable空间大小
size_t Buffer::ReadableBytes(void) const
{
    // 写入位置-读取位置
    return writePos_ - readPos_;
}

// prependable预留空间大小
size_t Buffer::PrependableBytes(void) const
{
    return readPos_;
}

// Peek代表readable空间起始地址
const char *Buffer::Peek(void) const
{
    return BeginPtr_() + readPos_;
}

void Buffer::Retrieve(size_t len)
{
    // 所以prependable更像是用来回收buffer空间用的？
    assert(len <= ReadableBytes());
    readPos_ += len;
}
void Buffer::RetrieveUntil(const char *end)
{
    // 清空、回收部分readable空间
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll(void)
{
    // 将buffer内容清零，readpos和writepos置零
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}
std::string Buffer::RetrieveAllToStr(void)
{
    // 将readable空间里的内容转化为string变量，然后清空buffer
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

// 新增的写入长度，即可读空间readable增大
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
    // 每次调用Append都先检查剩余空间大小
    // 然后把readable内容放到buffer起始地址，在writepos地址开始添加新的内容
    // 并更新writepos的位置
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

// 扩充Buffer空间
void Buffer::MakeSpace_(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len)
    {
        // 这个if就真的是扩充vector的空间
        buffer_.resize(writePos_ + len + 1);
    }
    else
    {
        // prependable空间和writeable空间大小够用，先把readable空间里的内容复制到prependable中
        // 这样readPos就设为0
        size_t readable = ReadableBytes();
        // 复制readable内容到prependable中
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}

// readv函数是从文件描述符中读取数据，存到buffer中
ssize_t Buffer::ReadFd(int fd, int *saveErrno)
{
    char buff[65535];
    struct iovec iov[2];
    // 写入空间大小
    const size_t writeable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *saveErrno = errno;
        return len;
    }
    else if (static_cast<size_t>(len) <= writeable)
    {
        // HasWritten(len);
        writePos_ += len;
    }
    else
    {
        // 新增数据长度大于writeable空间
        writePos_ = buffer_.size();
        // 写入还没写入buffer的内容
        // 在执行readv时已经同步把buff内容写入buffer中，buff表示剩余未写入的内容
        Append(buff, len - writeable);
    }
    return len;
}

// write是把readable里面的内容通过socket写入文件描述符中
// 写完后要回收buffer空间，相当于是readpos增加len长度
ssize_t Buffer::WriteFd(int fd, int *saveErrno)
{
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0)
    {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}