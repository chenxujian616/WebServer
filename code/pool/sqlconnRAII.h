#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

/**
 * @brief RAII机制是一种C++内存资源管理机制
 * 其做法是在使用一个对象时，在构造函数中获取相应资源
 * 在对象生命周期中使用这些资源，控制对这些资源的访问和使用
 * 在对象析构时释放资源
 *
 */

/**
 * @brief 资源在对象构造时初始化，在析构时释放
 *
 */
class SqlConnRAII
{
private:
    MYSQL *sql_;
    SqlConnPool *connpool_;

public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
    {
        assert(sql);
        // 得到一个空闲sql连接
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    ~SqlConnRAII()
    {
        if (sql_)
        {
            // 释放sql连接及其信号量
            connpool_->FreeConn(sql_);
        }
    }
};

#endif