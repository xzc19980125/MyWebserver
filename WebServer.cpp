//
// Created by 69572 on 2023/3/29.
//

#include "WebServer.h"

WebServer::WebServer(
        int port,int trigMode,int timeoutMS,bool optLinger,int threadNum):
        port_(port),openLinger_(optLinger),timeoutMS_(timeoutMS),isClose_(false),
        timer_(new TimerManager()),threadpool_(new ThreadPool(threadNum)),epoller_(new Epoller())
        //创建了一个定时器对象、一个线程池对象、一个epoll反应堆对象
{
    //获取工作目录
    srcDir_ = getcwd(nullptr,256);//获取当前目录，nullptr表示不指定返回内容的缓存区，256表示缓冲区大小
    assert(srcDir_);
    strncat(srcDir_,"/resources/",16);

    HTTPconnection::userCount = 0;//初始化连接用户数量为0
    HTTPconnection::srcDir = srcDir_;//将工作目录传入HTTPconnection

    InitEventMode_(trigMode);//设置读写事件和监听事件的模式(在这里可以设置为ET模式)
    if(!InitSocket_()){isClose_ = true;}//初始化套接字，若初始化失败，则将isClose设置为true
}


WebServer::~WebServer(){
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::Start(){
    int timeMS = -1;
    if(!isClose_){std::cout<<"=================Server Start!================="<<std::endl;}
    while(!isClose_)
    {
        if(timeoutMS_>0)//若设置的定时器时间大于0
        {
            timeMS=timer_->getNextHandle();
        }
        int eventCnt=epoller_->wait(timeMS);//监听事件，等待事件为timeMS
        for(int i=0;i<eventCnt;++i)//处理响应事件
        {
            int fd=epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);

            if(fd==listenFd_)//触发监听事件
            {
                DealListen_();//处理监听事件
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))//对端断连请求
            {
                assert(users_.count(fd)>0);
                closeConn_(&users_[fd]);//关闭套接字
            }
            else if(events & EPOLLIN)//触发读事件
            {
                assert(users_.count(fd)>0);
                DealRead_(&users_[fd]);//处理读事件
            }
            else if(events & EPOLLOUT)//触发写事件
            {
                assert(users_.count(fd)>0);
                DealWrite_(&users_[fd]);//处理写事件
            }
            else
            {
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}

//初始化套接字
bool WebServer::InitSocket_(){
    int ret;
    //创建sockaddr_in结构体
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024){
        std::cout<<"Port:"<<port_<<"error!"<<std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    //创建socket文件
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0){
        std::cout<<"Create socket error!"<<" "<<port_<<std::endl;
        return false;
    }

    //设置SO_LINGER 选项，
    //当openLinger_为0时，optLinger为默认0，此时，关闭连接，内核会立即返回，而未发送的数据将被丢弃。
    //当openLinger_为1时，optLinger.l_onoff = 1;optLinger.l_linger = 1;
    // 此时，关闭连接，内核会等待一段时间（即 l_linger 指定的时间）来尝试将未发送的数据发送出去，超时后立即返回

    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        std::cout<<"Init linger error!"<<std::endl;
        return false;
    }

    //设置端口复用，
    int optval = 1;
    // SOL_SOCKET表示设置套接字级别的选项，SO_REUSEADDR表示设置为端口复用
    //(const void*)&optval和sizeof(int)表示设置该标志位为多少，以及设置参数的长度
    ret = setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int));
    if(ret == -1){
        std::cout<<"set socket setsockopt error!"<<std::endl;
        close(listenFd_);
        return false;
    }
    //绑定
    ret = bind(listenFd_,(struct sockaddr*)&addr,sizeof(addr));
    if(ret < 0){
        std::cout<<"Bind Port:"<<port_<<"error!"<<std::endl;
        close(listenFd_);
        return false;
    }

    //监听
    ret = listen(listenFd_,6);
    if(ret < 0){
        std::cout<<"Listen Port:"<<port_<<"error!"<<std::endl;
        close(listenFd_);
        return false;
    }

    //将监听用的lfd上树,监听其读事件
    ret = epoller_->addFd(listenFd_,listenEvent_|EPOLLIN);
    if(ret == 0){
        std::cout<<"add listen error!"<<std::endl;
        close(listenFd_);
        return false;
    }
    //设置lfd的文件描述符非阻塞
    SetFdNonblock(listenFd_);
    std::cout<<"Server Port:"<<port_<<std::endl;
    return true;
}

//事件模式的初始化
void WebServer::InitEventMode_(int trigMode){
    //标志位表示对端连接关闭或者半关闭（即写半部关闭）
    //即收到了FIN包，表示数据传输已经结束，不会再有数据传输过来，但是还可以继续发送数据到对端，只是对端不再接收。
    listenEvent_=EPOLLRDHUP;
    //EPOLLONESHOT避免竞态条件,标志位用于告诉内核在事件发生后只监听一次该文件描述符的事件
    //如果一个线程或进程处理完某个文件描述符后，
    //没有重新使用epoll_ctl()函数修改该文件描述符的状态，那么该文件描述符将会一直处于EPOLLONESHOT状态，
    //导致其他线程或进程无法处理该文件描述符
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch(trigMode)
    {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HTTPconnection::isET = (connEvent_ & EPOLLET);
}

//处理监听到的新连接
void WebServer::DealListen_(){
    //创建sockaddr_in结构体
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listenFd_,(struct sockaddr*)&addr,&len);
        if(fd <= 0){ return;}
        else if(HTTPconnection::userCount >= MAX_FD){//客户端超过限制，回复busy
            SendError_(fd,"Server busy!");
            return;
        }
        AddClient_(fd,addr);//添加客户端
    }while(listenEvent_ & EPOLLET);
}
//添加客户端
void WebServer::AddClient_(int fd,sockaddr_in addr){
    assert(fd>0);
    users_[fd].initHTTPConn(fd,addr);//利用fd和addr来初始化HTTPconnection类型
    if(timeoutMS_>0)
    {
        //若timeoutMS_设置大于0，则设置定时器，绑定删除事件
        timer_->addTimer(fd,timeoutMS_,std::bind(&WebServer::closeConn_,this,&users_[fd]));
    }
    epoller_->addFd(fd,EPOLLIN | connEvent_);//将新事件上树
    SetFdNonblock(fd);//设置新事件为非阻塞事件
}

//处理监听到的读写事件
void WebServer::DealRead_(HTTPconnection* client){
    assert(client);
    extentTime_(client);
    //需要bind成员函数时，第一个参数需要为该类的对象，这里用this
    threadpool_->submit(std::bind(&WebServer::OnRead,this,client));
}
void WebServer::DealWrite_(HTTPconnection* client){
    assert(client);
    extentTime_(client);
    threadpool_->submit(std::bind(&WebServer::OnWrite,this,client));
}



//读操作
void WebServer::OnRead(HTTPconnection* client){
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->readBuffer(&readErrno);
    if(ret<=0 && readErrno != EAGAIN){//没有阻塞，并且ret<0，说明读取完毕
        closeConn_(client);
        return;
    }
    OnProcess(client);
}
//写操作
void WebServer::OnWrite(HTTPconnection* client){
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeBuffer(&writeErrno);
    if(client->writeBytes() == 0){//写入完毕
        if(client->isKeepAlive()){
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0){
        if(writeErrno == EAGAIN){
            epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}

//切换读写监听状态
void WebServer::OnProcess(HTTPconnection* client)
{
    if(client->handleHTTPConn()){//当client正在回复请求
        epoller_->modFd(client->getFd(),connEvent_ | EPOLLOUT);//设置为监听写事件
    }
    else{
        epoller_->modFd(client->getFd(),connEvent_ | EPOLLIN);
    }
}

//超时处理
void WebServer::extentTime_(HTTPconnection* client){
    assert(client);
    if(timeoutMS_>0){
        timer_->update(client->getFd(),timeoutMS_);
    }
}

//关闭连接
void WebServer::closeConn_(HTTPconnection* client){
    assert(client);
    epoller_->delFd(client->getFd());//从节点上删除
    client->closeHTTPConn();//关闭HTTP连接
}

//发送错误信息
void WebServer::SendError_(int fd,const char* info){
    assert(fd>0);
    int ret = send(fd,info,strlen(info),0);
    close(fd);
}

//使fd非阻塞
int WebServer::SetFdNonblock(int fd){
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
