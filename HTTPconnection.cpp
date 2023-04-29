//
// Created by 69572 on 2023/3/25.
//

#include "HTTPconnection.h"

const char* HTTPconnection::srcDir;
std::atomic<int> HTTPconnection::userCount;
bool HTTPconnection::isET;

HTTPconnection::HTTPconnection() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HTTPconnection::~HTTPconnection() {
    closeHTTPConn();
}

//初始化HTTP链接
void HTTPconnection::initHTTPConn(int socketFd,const sockaddr_in& addr){
    assert(socketFd > 0);
    userCount++;
    addr_ = addr;
    fd_ = socketFd;
    //读写缓冲区初始化
    writeBuffer_.initPtr();
    readBuffer_.initPtr();
    isClose_= false;
}

//每个连接中定义的缓冲区的读接口
ssize_t HTTPconnection::readBuffer(int* saveErrno){
    ssize_t len = -1;
    do{
        //将fd_指针送入readBuffer_中进行读取
        //出错则返回-1，其errno存在saveErrno中
        len = readBuffer_.readFd(fd_,saveErrno);
        if(len <= 0){
            break;
        }
    } while (isET);//当为ET触发模式时，将会进行循环读取，直到将数据读取完全
    return len;
}

//每个连接中定义的缓冲区的写接口
ssize_t HTTPconnection::writeBuffer(int* saveErrno){
    ssize_t len =-1;
    do{
        //将iov_中的数据写入fd_，iovCnt表明iovec结构体数组中元素的个数，用于指明需要写入的数据块的个数
        len = writev(fd_,iov_,iovCnt_);//第一部分为请求报文，第二部分为请求的文件内容
        if(len < 0){
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0){break;}//iov中空了，传输结束
        else if(static_cast<size_t>(len) > iov_[0].iov_len){//当能够传入的内容量大于iov_[0]的大小时
            //不仅仅发送了iov_[0]，还发送了部分iov_[1]中的内容
            //需要将iov_[1]指针指向剩余位置
            //其位置为：原本的地址+已经发送的内容-iov_[0]的总字节数
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            //其长度缩小到：原长度-已经发送的部分内容长度(这部分为：已经发送的内容-iov_[0]的总字节数)
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len){
                //对iov_[0]进行清空操作，清除其中的内容(已经写入了)
                writeBuffer_.initPtr();
                iov_[0].iov_len = 0;
            }
        }
        else{//说明iov_[0]还没有写入完全
            //将指针指向iov_[0]中还没写完的内容
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            //iov_[0]长度剪短，不需要之前写入过的内容了
            iov_[0].iov_len -= len;
            writeBuffer_.updateReadPtr(len);
        }
    }while(isET || writeBytes() > 10240);//当为ET模式或者需要写入的数据大于10240时，循环写入
    return len;
}

//关闭连接
void HTTPconnection::closeHTTPConn(){
    //释放response_中的共享内存
    response_.unmapFile_();
    //用户-1，isClose为true，关闭文件描述符
    if(isClose_ == false){
        isClose_ = true;
        userCount--;
        close(fd_);
    }
}

//处理HTTP连接，包括request的解析以及response的生成
bool HTTPconnection::handleHTTPConn(){
    //解析请求报文
    request_.init();//请求解析类初始化
    if(readBuffer_.readableBytes() <= 0){//可读取数据为0，则返回false
        return false;
    }
    else if(request_.parse(readBuffer_)){//解析请求,200则为true，4XX则为false
        //状态码为200，说明连接请求正常，正常回复
        response_.init(srcDir,request_.path(),request_.isKeepAlive(),200);
    } else{
        //出现异常，以400那一组异常类型进行回复进行回复
        std::cout<<"400!"<<std::endl;
        response_.init(srcDir,request_.path(), false,400);
    }


    //生成回复请求报文
    response_.makeResponse(writeBuffer_);//生成请求行、请求头和请求体，存入writeBuffer_
    iov_[0].iov_base = const_cast<char*>(writeBuffer_.curReadPtr());//将iov_[0]指针指向writeBuffer_的读指针位置
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_=1;
    //若正常读取到了文件，需要传输文件
    if(response_.fileLen() > 0  && response_.file()) {
        iov_[1].iov_base = response_.file();//iov_[1]的指针指向文件内容
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    return true;
}

//获取IP
const char* HTTPconnection::getIP() const{
    //inet_ntoa返回静态缓冲区的指针，多线程中不安全（考虑用ntop）
    return inet_ntoa(addr_.sin_addr);//inet_ntoa接受的是一个in_addr结构体类型，返回点分十进制
}
//获取端口
int HTTPconnection::getPort() const{
    return addr_.sin_port;
}

//获取文件描述符
int HTTPconnection::getFd() const{
    return fd_;
}

//获取sockaddr_in结构体
sockaddr_in HTTPconnection::getAddr() const{
    return addr_;
}