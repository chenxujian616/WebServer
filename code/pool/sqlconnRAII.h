#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

/**
 * @brief ��Դ�ڶ�����ʱ��ʼ����������ʱ�ͷ�
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
    // �õ�һ������sql����
    *sql = connpool->GetConn();
    sql_ = *sql;
    connpool_ = connpool;
}

SqlConnRAII::~SqlConnRAII()
{
    if (sql_)
    {
        // �ͷ�sql���Ӽ����ź���
        connpool_->FreeConn(sql_);
    }
}

#endif