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
    } while (isET);
    return len;
}

//每个连接中定义的缓冲区的写接口
ssize_t HTTPconnection::writeBuffer(int* saveErrno){
    ssize_t len =-1;
    do{
        //将iov_中的数据写入fd_，iovCnt表明iovec结构体数组中元素的个数，用于指明需要写入的数据块的个数
        len = writev(fd_,iov_,iovCnt_);
        if(len < 0){
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0){break;}//iov中空了，传输结束
        else if(static_cast<size_t>(len) > iov_[0].iov_len){//当需要传入的内容量大于iov_[0]的大小时
            //由于仅使用iov_[0]无法完整发送数据
            //将把iov_[1]的指针向后移动，为iov_[0]腾出更大的空间。需要调整其base指针到正确的位置
            //其位置为：字节的地址+需要发送的完整字节数-iov_[0]已经发送的字节数
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            //其长度缩小到：原长度-未发送内容长度
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len){
                //检查是否还有未发送的内容，若有
                //将数据进行清空操作
                writeBuffer_.initPtr();
                iov_[0].iov_len = 0;
            }
        }
        else{
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.updateReadPtr(len);
        }
    }while(isET || writeBytes() > 10240);
    return len;
}

//关闭连接
void HTTPconnection::closeHTTPConn(){
    response_.unmapFile_();
    if(isClose_ == false){
        isClose_ = true;
        userCount--;
        close(fd_);
    }
}

//处理HTTP连接，包括request的解析以及response的生成
bool HTTPconnection::handleHTTPConn(){
    request_.init();//请求解析类初始化
    if(readBuffer_.readableBytes() <= 0){//读取数据为0，则返回false
        return false;
    }
    else if(request_.parse(readBuffer_)){//解析请求,200则为true，4XX则为false
        response_.init(srcDir,request_.path(),request_.isKeepAlive(),200);//初始化回应类
    } else{
        std::cout<<"400!"<<std::endl;
        response_.init(srcDir,request_.path(), false,400);
    }

    response_.makeResponse(writeBuffer_);//生成请求行和请求头，存入writeBuffer_

    iov_[0].iov_base = const_cast<char*>(writeBuffer_.curReadPtr());//将iov_[0]指针指向writeBuffer_的读指针位置
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_=1;
    //若正常读取到了文件，存在数据体
    if(response_.fileLen() > 0  && response_.file()) {
        iov_[1].iov_base = response_.file();//iov_[1]的指针指向数据体内容
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