/*
    和SQL相关，作用暂时不清楚
 */
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <assert.h>
// 互斥锁、信号量、线程
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

/**
 * @brief 这个类有点像线程池在这个池里面的sql句柄都已经连接上sql但还没使用
 *
 */
class SqlConnPool
{
private:
    SqlConnPool(/* args */);
    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    // 空闲sql连接队列
    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;

public:
    static SqlConnPool *Instance(void);

    MYSQL *GetConn(void);
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount(void);

    void Init(const char *host, int port, const char *user, const char *pwd,
              const char *dbName, int connSize = 10);
    void ClosePool(void);
};

#endif