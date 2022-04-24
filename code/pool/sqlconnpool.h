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

    // 分别为最大连接数、当前连接数和剩余连接数
    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    // 空闲sql连接队列
    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;

public:
    // 返回静态SqlConnPool对象
    static SqlConnPool *Instance(void);

    /**
     * @brief 获取队列中空闲MySQL对象
     * 
     * @return MYSQL* 空闲MySQL对象指针
     */
    MYSQL *GetConn(void);
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount(void);

    /**
     * @brief SQL连接初始化
     * 
     * @param host 主机IP
     * @param port 端口号
     * @param user 用户名
     * @param pwd 用户密码
     * @param dbName 数据库名称
     * @param connSize 最大连接数量
     */
    void Init(const char *host, int port, const char *user, const char *pwd,
              const char *dbName, int connSize = 10);
    void ClosePool(void);
};

#endif