#include "sqlconnpool.h"

using namespace std;

SqlConnPool::SqlConnPool(/* args */)
{
    useCount_ = 0;
    freeCount_ = 0;
}

// RAII机制销毁连接池
SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

// 创建静态SqlConnPool类指针
SqlConnPool *SqlConnPool::Instance(void)
{
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd,
                       const char *dbName, int connSize)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++)
    {
        MYSQL *sql = nullptr;
        // mysql初始化函数，用来初始化一个MySQL对象
        // 若传入的参数为NULL，这个函数会自动分配一个MySQL对象
        // 当调用mysql_close时，会释放这个对象
        sql = mysql_init(sql);
        if (!sql)
        {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        // sql为连接句柄，conn必须连接成功，否则无法进行sql操作
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql)
        {
            LOG_ERROR("MySql Connect error!");
        }
        // 将成功连接的sql添加到队列
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    // 信号量，数量等于MySQL的最大连接数
    // 使用信号量实现多线程争夺连接的同步机制
    // 信号量的数量等于MySQL的最大连接数，每用一个MySQL连接，信号量数量-1
    sem_init(&semId_, 0, MAX_CONN_);
}

/**
 * @brief 获取SQL连接，获取后，SQL连接池数量-1
 *
 * @return MYSQL*
 */
MYSQL *SqlConnPool::GetConn(void)
{
    MYSQL *sql = nullptr;
    if (connQue_.empty())
    {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    // 等待信号量。信号量>0，信号量数量-1，若信号量<=0，则阻塞等待
    sem_wait(&semId_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

/**
 * @brief 释放SQL连接
 *
 * @param conn SQL连接
 */
void SqlConnPool::FreeConn(MYSQL *conn)
{
    // 断言检查
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);
    // 释放信号量
    sem_post(&semId_);
}

void SqlConnPool::ClosePool(void)
{
    lock_guard<mutex> locker(mtx_);
    while (!connQue_.empty())
    {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

/**
 * @brief 获取空间的SQL连接数量
 *
 * @return int 剩余空闲SQL连接数量
 */
int SqlConnPool::GetFreeConnCount(void)
{
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}