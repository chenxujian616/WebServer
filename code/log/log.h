#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log
{
private:
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();
    void AsyncWrite_(void);

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    // path_为目录路径，suffix_为文件后缀
    const char *path_;
    const char *suffix_;

    int MAX_LINES_;
    int lineCount_;
    int toDay_;
    bool isOpen_;

    Buffer buff_;
    int level_;
    bool isAsync_;

    FILE *fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_;

public:
    // 这里没用到构造函数，而是使用自定义初始化函数
    void init(int level, const char *path = "./log",
              const char *suffix = "./log", int maxQueueCapacity = 1024);

    static Log *Instance(void);
    static void FlushLogThread(void);

    void write(int level, const char *format, ...);
    void flush(void);

    int GetLevel(void);
    void SetLevel(int level);
    bool IsOpen(void) { return isOpen_; }
};

// ##__VA_ARGS__是一个可变参数的宏
// 其含义就是参数列表中的最后一个省略号参数
/* 以下宏定义相当于debug时输出的信息 */
#define LOG_BASE(level, format, ...)                   \
    do                                                 \
    {                                                  \
        Log *log = Log::Instance();                    \
        if (log->IsOpen() && log->GetLevel() <= level) \
        {                                              \
            log->write(level, format, ##__VA_ARGS__);  \
            log->flush();                              \
        }                                              \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);
#endif