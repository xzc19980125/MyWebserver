//
// Created by 69572 on 2023/3/27.
//

#ifndef CPROJECT_HTTPRESPONSE_H
#define CPROJECT_HTTPRESPONSE_H
#include <unordered_map>
#include <fcntl.h>  //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap,munmap
#include <assert.h>

#include "Buffer.h"


class HTTPresponse {
public:
    HTTPresponse();
    ~HTTPresponse();

    //初始化函数
    void init(const std::string& srcDir,std::string& path,bool isKeepAlive=false,int code=-1);

    //生成响应报文的主函数
    void makeResponse(Buffer& buffer);

    //共享内存的扫尾函数
    void unmapFile_();

    //在添加函数体时，若请求的文件打不开，返回响应的错误信息
    void errorContent(Buffer& buffer,std::string message);

    //返回文件信息
    char* file();
    size_t fileLen() const;
    //返回状态码
    int code() const {return code_;}

private:
    //生成报文函数
    //生成报文请求行
    void addStateLine_(Buffer& buffer);
    //生成报文头
    void addResponseHeader_(Buffer& buffer);
    //生成报文的数据体
    void addResponseContent_(Buffer& buffer);

    //出现4XX状态码时的函数
    void errorHTML_();

    //添加报文头时，需要得到文件类型信息，该函数即返回该信息
    std::string getFileType_();

    int code_;//HTTP状态
    bool isKeepAlive_;//HTTP是否还处于KeepAlive状态

    std::string path_;//解析到请求的相对路径
    std::string srcDir_;//表示根目录路径

    //共享内存信息
    char* mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string,std::string> SUFFIX_TYPE;//后缀名到文件类型的映射
    static const std::unordered_map<int,std::string> CODE_STATUS;//状态码到响应状态的映射
    static const std::unordered_map<int,std::string> CODE_PATH;//状态码到响应文件路径的映射

};


#endif //CPROJECT_HTTPRESPONSE_H
