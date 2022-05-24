#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
// stat.h是linux系统用于定义文件状态，并获取文件属性
// stat在终端上有相应的命令
#include <sys/stat.h>
// 使用mman.h里的mmap内存映射文件方法
#include <sys/mman.h>

#include "../buffer/buffer.h"
#include "../log/log.h"

// 和httprequest处理请求报文不同，httpresponse创建响应报文

class HttpResponse
{
private:
    void AddStateLine_(Buffer &buff);
    void AddHeader_(Buffer &buff);
    void AddContent_(Buffer &buff);

    void ErrorHtml_(void);
    std::string GetFileType_(void);

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    // 文件在内存中的起始地址
    char *mmFile_;
    // 文件属性
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

public:
    HttpResponse(/* args */);
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path,
              bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer &buff);
    void UnmapFile(void);
    char *File(void);
    size_t FileLen(void) const;
    void ErrorContent(Buffer &buff, std::string message);
    int Code(void) const { return code_; };
};

#endif