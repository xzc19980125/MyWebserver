//
// Created by 69572 on 2023/3/27.
//

#include "HTTPresponse.h"
const std::unordered_map<std::string, std::string> HTTPresponse::SUFFIX_TYPE = {
        { ".html",  "text/html" },
        { ".xml",   "text/xml" },
        { ".xhtml", "application/xhtml+xml" },
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_STATUS = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_PATH = {
        { 400, "/400.html" },
        { 403, "/403.html" },
        { 404, "/404.html" },
};

HTTPresponse::HTTPresponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
}

HTTPresponse::~HTTPresponse() {
    unmapFile_();
}

//初始化函数
void HTTPresponse::init(const std::string& srcDir,std::string& path,bool isKeepAlive,int code){
    assert(srcDir != "");
    if(mmFile_){unmapFile_();}//若存在共享内存，先释放
    code_=code;
    isKeepAlive_ = isKeepAlive;
    path_=path;
    srcDir_=srcDir;
    mmFile_= nullptr;
    mmFileStat_={0};
}

//生成响应报文的主函数
void HTTPresponse::makeResponse(Buffer& buffer){
    //当请求的文件不存在(获取不到文件信息)或请求的是一个目录时，code_=404 not found
    if(stat((srcDir_+path_).data(),&mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)){
        code_ = 404;
    }
    //当文件不为其他用户可读写的权限时，code_=403 forbidde
    else if(!(mmFileStat_.st_mode & S_IROTH)){
        code_ = 403;
    }
    //上述情况没有发生，则说明正常请求文件，code_=200 ok
    else if(code_ == -1){
        code_ = 200;
    }
    //出现4XX则进行处理
    errorHTML_();
    //添加请求行
    addStateLine_(buffer);
    addResponseHeader_(buffer);
    addResponseContent_(buffer);
}

//共享内存的扫尾工作，解绑共享内存，清除指针
void HTTPresponse::unmapFile_(){
    if(mmFile_){
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

//在添加函数体时，若请求的文件打不开，返回响应的错误信息
void HTTPresponse::errorContent(Buffer& buffer,std::string message){
    std::string body;
    std::string status;
    //编写出错信息的html
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }
    else{
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buffer.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.append(body);
}


//返回文件内容
char* HTTPresponse::file(){
    return mmFile_;
}
//返回文件大小
size_t HTTPresponse::fileLen() const{
    return mmFileStat_.st_size;
}

//生成报文函数
//生成报文请求行
void HTTPresponse::addStateLine_(Buffer& buffer){
    std::string status;
    //将code_对应的请求状态填入status中
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }
    else{//若出现除了200、400、403、404的状态码，均按照400处理
        code_=400;
        status = CODE_STATUS.find(400)->second;
    }
    //组成请求头
    buffer.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}
//生成报文头
void HTTPresponse::addResponseHeader_(Buffer& buffer){
    buffer.append("Connection: ");
    if(isKeepAlive_){
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    }
    else{
        buffer.append("close\r\n");
    }
    buffer.append("Content-type: " + getFileType_() + "\r\n");
}

//生成报文的数据体
void HTTPresponse::addResponseContent_(Buffer& buffer){
    int srcFd = open((srcDir_ + path_).data(),O_RDONLY);
    if(srcFd < 0){
        errorContent(buffer,"File NotFound!");
        return;
    }
    //将srcFd文件映射到内存中
    int* mmRet = (int*)mmap(0,mmFileStat_.st_size,PROT_READ,MAP_PRIVATE,srcFd,0);
    if(*mmRet == -1){
        errorContent(buffer,"File NotFound!");
        return;
    }
    //把文件内容存入mmFile_
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buffer.append("Content-length: " + std::to_string(mmFileStat_.st_size)+"\r\n\r\n");

}

//出现4XX状态码时的函数
void HTTPresponse::errorHTML_(){
    //当出现4XX类的状态码
    if((CODE_PATH.count(code_) == 1)){
        //文件为相对应的html文件
        path_ = CODE_PATH.find(code_)->second;
        //将文件信息存入mmFileStat_
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

//添加报文头时，需要得到文件类型信息，该函数即返回该信息
std::string HTTPresponse::getFileType_(){
    //获取文件后缀
    std::string::size_type idx = path_.find_last_of('.');
    //若后缀无法解析，默认为text
    if(idx == std::string::npos){
        return "text/plain";
    }
    //取出后缀
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1){
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";

}