//
// Created by 69572 on 2023/3/25.
//

#ifndef CPROJECT_EPOLLER_H
#define CPROJECT_EPOLLER_H

#include<sys/epoll.h>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include<vector>
#include<errno.h>
#include<stdint.h>

class Epoller {
public:
    explicit Epoller(int maxEvent=1024);//Epoller构造函数，maxEvent表示监听就绪事件的列表大小
    ~Epoller();

    //上树，将fd加入epoll监听
    bool addFd(int fd,uint32_t events);
    //修改fd对应的节点
    bool modFd(int fd,uint32_t events);
    //下树，删除fd对应的节点
    bool delFd(int fd);
    //返回监听结果
    int wait(int timewait = -1);

    //获取fd的函数
    int getEventFd(size_t i) const;
    //获取events的函数
    uint32_t getEvents(size_t i) const;

private:
    int epoollerFd_;//epfd根节点
    std::vector<struct epoll_event> events_;
};


#endif //CPROJECT_EPOLLER_H
