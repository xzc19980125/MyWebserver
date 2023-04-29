//
// Created by 69572 on 2023/3/29.
//

#ifndef CPROJECT_WEBSERVER_H
#define CPROJECT_WEBSERVER_H
#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Epoller.h"
#include "timer.h"
#include "treadPool.h"
#include "HTTPconnection.h"


class WebServer {
public:
    WebServer(int port,int trigMode,int timeoutMS,bool optLinger,int threadNum);
    ~WebServer();
    void Start();

private:
    bool InitSocket_();//初始化套接字
    void InitEventMode_(int trigMode);//事件模式的初始化
    void AddClient_(int fd,sockaddr_in addr);//添加HTTP客户端
    void closeConn_(HTTPconnection* client);//关闭HTTP客户端

    void DealListen_();//处理监听到的新连接
    //处理监听到的读写事件
    void DealWrite_(HTTPconnection* client);
    void DealRead_(HTTPconnection* client);

    //发送错误信息
    void SendError_(int fd,const char* info);
    //超时处理
    void extentTime_(HTTPconnection* client);

    void OnRead(HTTPconnection* client);
    void OnWrite(HTTPconnection* client);
    void OnProcess(HTTPconnection* client);

    static const int MAX_FD = 65536;
    //使fd非阻塞
    static int SetFdNonblock(int fd);

    int port_;//端口
    int timeoutMS_;//定时器的过期时间
    bool isClose_;//是否关闭
    int listenFd_;//监听的文件描述符
    bool openLinger_;
    char* srcDir_;//资源目录

    uint32_t listenEvent_;//监听的连接事件
    uint32_t connEvent_;//监听的读写事件

    std::unique_ptr<ThreadPool> threadpool_;//线程池
    std::unique_ptr<Epoller> epoller_;//epoll红黑树对象
    std::unordered_map<int, HTTPconnection> users_;//哈希表，保存连接客户端的信息
    std::unique_ptr<TimerManager> timer_;//定时器对象
};


#endif //CPROJECT_WEBSERVER_H
