#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
// 正则表达式头文件
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

/**
 * @brief 这个类用于处理HTTP请求，并不是说服务器发送一个HTTP请求到另一个服务器
 *
 */
class HttpRequest
{
public:
    // 请求报文状态，分别是请求行、请求头、请求体
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

private:
    // 以下方法，都是用于处理HTTP请求报文
    /**
     * @brief 处理、解析请求行
     *
     * @param line 请求报文
     * @return true 请求行处理解析成功
     * @return false 请求行处理解析失败
     */
    bool ParseRequestLine_(const std::string &line);
    /**
     * @brief 处理、解析请求头
     *
     * @param line 请求报文
     */
    void ParseHeader_(const std::string &line);
    /**
     * @brief 处理、解析请求体
     *
     * @param line 请求报文
     */
    void ParseBody_(const std::string &line);

    void ParsePath_(void);
    void ParsePost_(void);
    void ParseFromUrlencoded_(void);

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

    // 用一个状态机来表示HTTP的状态？
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConvertHex(char ch);

public:
    HttpRequest(/* args */);
    ~HttpRequest() = default;

    void Init(void);
    bool parse(Buffer &buff);

    std::string path(void) const;
    std::string &path(void);
    std::string method(void) const;
    std::string version(void) const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;

    bool IsKeepAlive(void) const;
};

#endif