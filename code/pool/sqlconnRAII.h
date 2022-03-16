#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

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
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool);
    ~SqlConnRAII();
};

SqlConnRAII::SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
{
    assert(sql);
    // 得到一个空闲sql连接
    *sql = connpool->GetConn();
    sql_ = *sql;
    connpool_ = connpool;
}

SqlConnRAII::~SqlConnRAII()
{
    if (sql_)
    {
        // 释放sql连接及其信号量
        connpool_->FreeConn(sql_);
    }
}

#endif