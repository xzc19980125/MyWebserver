//
// Created by 69572 on 2023/3/25.
//

#ifndef CPROJECT_TIMER_H
#define CPROJECT_TIMER_H
#include<queue>
#include<deque>
#include<unordered_map>
#include<ctime>
#include<chrono>
#include<functional>
#include<memory>

#include "HTTPconnection.h"

//回调函数
typedef std::function<void()> TimeoutCallBack;
//程序运行开始到现在的时间
typedef std::chrono::steady_clock Clock;
//毫秒类型
typedef std::chrono::milliseconds MS;
//单位为毫秒的时间戳类型
typedef Clock::time_point TimeStamp;

class TimerNode{
public:
    int id;//定时器编号（这里存的是文件描述符）
    TimeStamp expire;//设置过期时间，到时间了则执行该节点的回调（该时间=自定义时间+加入计时的时间戳）
    TimeoutCallBack cb;//到了设置时间则需要运行的回调函数
    bool operator<(const TimerNode& t)//需要重载符号<，比较哪个节点需要先执行
    {
        return expire<t.expire;
    }
};

class TimerManager {
    typedef std::shared_ptr<TimerNode> SP_TimerNode;
public:
    TimerManager(){heap_.reserve(64);}//初始化构造器，默认有64个定时器节点
    ~TimerManager(){clear();}

    //添加定时器
    void addTimer(int id,int timeout,const TimeoutCallBack& cb);
    //处理到期的定时器
    void handle_expired_event();
    //距离下一次需要处理的定时器到来的时间
    int getNextHandle();

    //更新时间
    void update(int id,int timeout);
    //删除指定的节点，并且触发该节点的函数
    void work(int id);

    void pop();
    void clear();

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int,size_t> ref_;//映射fd对应的定时器在heap_中的位置

    void del_(size_t i);
    //红黑树的上滤操作
    void sifup_(size_t i);
    bool siftdown_(size_t index,size_t n);
    void swapNode_(size_t i,size_t j);
};


#endif //CPROJECT_TIMER_H
