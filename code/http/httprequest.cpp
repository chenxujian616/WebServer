#include "httprequest.h"
using namespace std;

HttpRequest::HttpRequest(/* args */)
{
    Init();
}

/**
 * @brief HTML页面后缀，默认是/welcome（严格说并不是后缀，而是资源所在的路由）
 *
 */
const unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/pitcure",
};

const unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::Init(void)
{
    method_ = path_ = version_ = body_ = "";
    // 默认是请求行状态
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

/**
 * @brief 检查keepalive状态
 *
 * @return true
 * @return false
 */
bool HttpRequest::IsKeepAlive(void) const
{
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n";
    // 请求报文空
    if (buff.ReadableBytes() <= 0)
    {
        return false;
    }
    // 从缓存空间中获取数据内容
    while (buff.ReadableBytes() && state_ != FINISH)
    {
        // 寻找\r\n的地址，并把地址赋值给缓存区中
        const char *lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        // line是从每一行首项元素开始到\r\n的字符串
        string line(buff.Peek(), lineEnd);
        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line))
            {
                return false;
            }
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2)
            {
                // 请求体没有内容，只有\r\n
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }

        if (lineEnd == buff.BeginWrite())
        {
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_(void)
{
    // 添加后缀，默认资源是/index.html
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string &line)
{
    // 正则表达式匹配
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten))
    {
        // 得到头部、URL和HTTP版本号，并把状态更改为headers状态
        // subMatch[0]是line
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string &line)
{
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten))
    {
        // subMatch[1]的key，subMatch[2]是value
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string &line)
{
    body_ = line;
    std::cout << line << std::endl;
    // 获取请求体内容
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

// 把英文字符变成10进制数
int HttpRequest::ConvertHex(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a' + 10;
    }
    return ch;
}

void HttpRequest::ParsePost_(void)
{
    // 不是说用POST方法连接数据库，而是浏览器用POST方法发送用户名和密码，服务器做匹配
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_))
        {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1)
            {
                // tag=1表示登录
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin))
                {
                    path_ = "/welcome.html";
                }
                else
                {
                    path_ = "/error.html";
                }
            }
        }
    }
}

/*
    编码目的在于让程序能够处理URL中的特殊符号
    https://www.jianshu.com/p/7aa8821c29c1

    第二个功能是从请求体内容中分离POST请求内容
 */
void HttpRequest::ParseFromUrlencoded_(void)
{
    if (body_.size() == 0)
    {
        return;
    }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    /*
        服务器程序接收的是URL字符串，而URL字符串有下面的特殊字符
        这段for循环就是为了解码，处理HTTP字符，将特殊字符转换为ASCII码
    */

    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            // HTTP传输中，使用+替代空格，因为空格对URL来说是一个非法字符
            body_[i] = ' ';
            break;
        case '%':
            num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    // 就是说key一定会有？
    if (post_.count(key) == 0 && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin)
{
    if (name == "" || pwd == "")
    {
        return false;
    }
    LOG_INFO("Veriry name:%s pwd:%s", name, pwd);
    // 获取空闲sql连接
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    // MYSQL_FIELD包含关于字段的信息，如字段名、类型和大小
    MYSQL_FIELD *fields = nullptr;
    // MYSQL_RES表示返回行的查询结果(select,show等)
    MYSQL_RES *res = nullptr;

    if (!isLogin)
    {
        flag = true;
    }
    // 查询用户及密码
    snprintf(order, 256,
             "select username, password from user where username='%s' limit 1", name.c_str());
    LOG_DEBUG("%s", order);

    // 查询语句，查询成功返回0
    if (mysql_query(sql, order))
    {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        // 注册行为，且用户名未被使用
        if (isLogin)
        {
            if (pwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("password error!");
            }
        }
        else
        {
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    // 注册行为
    if (!isLogin && flag == true)
    {
        LOG_DEBUG("register!");
        bzero(order, 256);
        // 向user表插入新数据，不需要加分号
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')",
                 name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }

    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify success!!");
    return flag;
}

string HttpRequest::path(void) const
{
    return path_;
}

string &HttpRequest::path(void)
{
    return path_;
}

string HttpRequest::method(void) const
{
    return method_;
}

string HttpRequest::version(void) const
{
    return version_;
}

string HttpRequest::GetPost(const string &key) const
{
    assert(key != "");
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}