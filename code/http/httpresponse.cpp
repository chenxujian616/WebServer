#include "httpresponse.h"

using namespace std;

// HTTP消息报头
const unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {".js", "text/javascript"},
};

// 状态码
const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

// 错误code后缀
const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse(/* args */) : code_(-1), path_(""), srcDir_(""),
                                         isKeepAlive_(false), mmFile_(nullptr), mmFileStat_({0})
{
}

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Init(const std::string &srcDir, std::string &path,
                        bool isKeepAlive, int code)
{
    assert(srcDir != "");
    if (mmFile_)
    {
        UnmapFile();
    }

    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

void HttpResponse::MakeResponse(Buffer &buff)
{
    // 判断请求的资源文件
    // stat方法获取文件属性，并把属性存到缓存中
    // S_ISDIR宏判断是否为文件。为目录则返回404错误
    // st_mode是文件对应模式，用状态码判断是否属于文件或目录
    if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode))
    {
        code_ = 404;
    }
    else if (!(mmFileStat_.st_mode & S_IROTH))
    {
        code_ = 403;
    }
    else if (code_ == -1)
    {
        code_ = 200;
    }

    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

char *HttpResponse::File(void)
{
    return mmFile_;
}

size_t HttpResponse::FileLen(void) const
{
    return mmFileStat_.st_size;
}

void HttpResponse::ErrorHtml_(void)
{
    if (CODE_PATH.count(code_) == 1)
    {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

// 响应头部状态行，HTTP版本+状态号，如HTTP/1.1 200 OK
void HttpResponse::AddStateLine_(Buffer &buff)
{
    string status;
    // 状态码存在，不存在返回400状态码
    if (CODE_STATUS.count(code_) == 1)
    {
        // 如200，status="OK"
        status = CODE_STATUS.find(code_)->second;
    }
    else
    {
        code_ = 400;
        status = CODE_STATUS.find(code_)->second;
    }
    // 添加响应报文响应行
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}
// 添加响应头部信息
void HttpResponse::AddHeader_(Buffer &buff)
{
    buff.Append("Connection: ");
    if (isKeepAlive_)
    {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

/**
 * @brief 添加响应报文响应体
 *
 * @param buff 响应体内容buff
 */
void HttpResponse::AddContent_(Buffer &buff)
{
    // 打开文件
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if (srcFd < 0)
    {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    // 将文件映射到内存提高文件的访问速度
    LOG_DEBUG("file path %s", (srcDir_ + path_).c_str());
    // mmap命令用于将一个文件或对象映射到内存，提高文件的访问速度
    // PORT_READ表示页内容可以被读取，MAP_PRIVATE表示内存区域的写入不会影响原文件
    int *mmRet = (int *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1)
    {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    mmFile_ = (char *)mmRet;
    close(srcFd);
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile(void)
{
    if (mmFile_)
    {
        // munmap用于释放由mmap创建的内存空间
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

string HttpResponse::GetFileType_(void)
{
    // 判断文件类型
    // size_type为size_t，string::npos表示没有匹配
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos)
    {
        return "text/plain";
    }
    // 获取path_的后缀
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1)
    {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

// 错误HTML代码，转化为HTML表示
void HttpResponse::ErrorContent(Buffer &buff, string message)
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1)
    {
        status = CODE_STATUS.find(code_)->second;
    }
    else
    {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebserver</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}