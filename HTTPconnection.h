//
// Created by 69572 on 2023/3/25.
//

#ifndef CPROJECT_HTTPCONNECTION_H
#define CPROJECT_HTTPCONNECTION_H
#include<arpa/inet.h> //sockaddr_in
#include<sys/uio.h> //readv/writev
#include<iostream>
#include<sys/types.h>
#include<assert.h>

#include "Buffer.h"
#include "HTTPrequest.h"
#include "HTTPresponse.h"

class HTTPconnection {
public:
    HTTPconnection();
    ~HTTPconnection();

    //初始化HTTP链接
    void initHTTPConn(int socketFd,const sockaddr_in& addr);

    //每个连接中定义的缓冲区的读接口
    ssize_t readBuffer(int* saveErrno);
    //每个连接中定义的缓冲区的读接口
    ssize_t writeBuffer(int* saveErrno);
    //关闭连接
    void closeHTTPConn();
    //处理HTTP连接，包括request的解析以及response的生成
    bool handleHTTPConn();

    //获取IP
    const char* getIP() const;
    //获取端口
    int getPort() const;
    //获取文件描述符
    int getFd() const;
    //获取sockaddr_in结构体
    sockaddr_in getAddr() const;

    //返回写入多少字节
    int writeBytes()
    {
        return iov_[1].iov_len + iov_[0].iov_len;
    }

    //是否还是keep-alive状态
    bool isKeepAlive() const
    {
        return request_.isKeepAlive();
    }

    static bool isET;//是否是ET触发模式
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;//HTTP连接对应的文件描述符（套接字）
    struct sockaddr_in addr_;//sockaddr_in结构体
    bool isClose_;//是否关闭连接

    int iovCnt_;
    struct iovec iov_[2];
    //读写缓冲区
    Buffer readBuffer_;
    Buffer writeBuffer_;
    //请求解析、回复类对象
    HTTPrequest request_;
    HTTPresponse response_;

};


#endif //CPROJECT_HTTPCONNECTION_H
