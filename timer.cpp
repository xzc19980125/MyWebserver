//
// Created by 69572 on 2023/3/25.
//

#include "timer.h"

//添加定时器
void TimerManager::addTimer(int id,int timeout,const TimeoutCallBack& cb)
{
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0)//当树中不存在该节点id时，插入该节点
    {

        i = heap_.size();
        //应该是因为使用的是map，要考虑key的唯一性，使用id为key，而i为value
        ref_[id] = i;//在树的id处加入value为i（当前vector长度）的节点
        heap_.push_back({id,Clock::now()+MS(timeout),cb});//利用id、过期时间、回调函数构成一个TImerNode送入vector中
        sifup_(i);//上滤进行排序，根据i排序
    }
    else{
        i = ref_[id];//在id对应i的位置
        //修改vector中的时间和回调函数
        heap_[i].expire = Clock::now()+MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i,heap_.size())){//若未下滤，说明比子节点都大，则进行上滤
            sifup_(i);
        }
    }
}
//处理到期的定时器
void TimerManager::handle_expired_event()
{
    if(heap_.empty())//队列为空，直接返回即可
    {
        return;
    }
    //只要队列不为空，且存在过期时间小于当前时间的节点，则要一直循环
    // 将这些节点的cb都运行掉，并将节点pop掉
    while (!heap_.empty())
    {
        TimerNode node = heap_.front();//取出第一个元素（其剩余时间应当为最短的）
        //假设该节点的时间比现在的时间大，则说明还没到时间，直接break掉
        if(std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0)
        {
            break;
        }
        //否则执行该节点对应的cb并将其pop掉
        node.cb();
        pop();
    }
}
//距离下一次需要处理的定时器到来的时间
int TimerManager::getNextHandle()
{
    handle_expired_event();
    size_t res = -1;
    if(!heap_.empty())
    {
        res = std::chrono::duration_cast<MS>(heap_.front().expire - Clock::now()).count();
        if(res < 0) {res = 0;}
    }
    return res;
}

//更新时间
void TimerManager::update(int id,int timeout)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expire = Clock::now() + MS(timeout);
    siftdown_(ref_[id],heap_.size());
}

//删除指定的节点，并且触发该节点的函数
void TimerManager::work(int id)
{
    if(heap_.empty() || ref_.count(id) == 0){ //vector中不存在该节点，则return
        return;
    }
    size_t i = ref_[id];//得到该节点在vector中的位置
    TimerNode node = heap_[i];
    node.cb();//调用该节点的回调函数
    del_(i);//删除该节点
}

void TimerManager::pop()
{
    assert(!heap_.empty());
    del_(0);
}

void TimerManager::clear()
{
    ref_.clear();
    heap_.clear();
}

void TimerManager::del_(size_t index)
{
    assert(!heap_.empty() && index < heap_.size() && index >= 0);//TODO
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n)
    {
        swapNode_(i,n);//把节点i换至vector队尾
        if(!siftdown_(i,n))//对除队尾的所有元素进行一次重排
        {
            sifup_(i);
        }
    }
    //擦除树中以及vector中的该节点
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

//上滤操作
void TimerManager::sifup_(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i-1)/2;//父节点
    //当j位置节点大于i位置节点（父>子）,则交换父子节点，即子节点上移动
    //直到上移到根节点为止
    while(j >= 0){
        if(heap_[j] < heap_[i]){break;}//当j位置的节点小于i位置的节点，跳出循环
        swapNode_(i,j);//节点交换
        i = j;
        j = (i - 1)/2;//j再次成为父节点，检查是否需要继续上移
    }
}

//下滤操作
bool TimerManager::siftdown_(size_t index,size_t n)
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j =i * 2 + 1;//右子节点
    //当i位置节点大于j位置节点（父>子）,则交换父子节点，即父节点下移动
    //直到下移到最后一个节点为止
    while(j < n)
    {
        if(j + 1 < n && heap_[j+1]<heap_[j]) j++;//当左子节点大时，仅比较左子节点即可
        if(heap_[i] < heap_[j]) break;
        swapNode_(i,j);
        i = j;
        j = i*2+1;//j继续成为右子节点
    }
    return i > index;
}

//交换节点
void TimerManager::swapNode_(size_t i,size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i],heap_[j]);//交换两个节点
    //并且更新两个节点在红黑树中的位置
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}