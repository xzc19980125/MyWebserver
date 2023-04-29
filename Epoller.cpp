//
// Created by 69572 on 2023/3/25.
//

#include "Epoller.h"
Epoller::Epoller(int maxevent):epoollerFd_(epoll_create(512)),events_(maxevent)//Epoller构造函数，maxEvent表示监听就绪事件的列表大小
{
    assert(epoollerFd_ >= 0 && events_.size() > 0);
}
Epoller::~Epoller()
{
    close(epoollerFd_);
}

//上树，将fd加入epoll监听
bool Epoller::addFd(int fd,uint32_t events)
{
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0==epoll_ctl(epoollerFd_,EPOLL_CTL_ADD,fd,&ev);
}
//修改fd对应的节点
bool Epoller::modFd(int fd,uint32_t events)
{
    if(fd < 0)return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0==epoll_ctl(epoollerFd_,EPOLL_CTL_MOD,fd,&ev);
}
//下树，删除fd对应的节点
bool Epoller::delFd(int fd)
{
    if(fd < 0 ) return false;
    epoll_event ev = {0};
    return 0==epoll_ctl(epoollerFd_,EPOLL_CTL_DEL,fd,&ev);
}
//返回监听结果
int Epoller::wait(int timewait)
{
    return epoll_wait(epoollerFd_,&events_[0],static_cast<int>(events_.size()),timewait);
}

//获取fd的函数
int Epoller::getEventFd(size_t i) const
{
    assert(i < events_.size() && i>=0);
    return events_[i].data.fd;
}
//获取events的函数
uint32_t Epoller::getEvents(size_t i) const
{
    assert(i < events_.size() && i>=0);
    return events_[i].events;
}
