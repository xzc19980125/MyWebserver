#ifndef CPROJECT_BUFFER_H
#define CPROJECT_BUFFER_H
#include<vector>
#include<iostream>
#include<cstring>
#include<atomic>
#include<unistd.h>
#include<sys/uio.h>
#include<assert.h>

class Buffer {
public:
    Buffer(int initBufferSize=1024);
    ~Buffer()=default;

    //缓冲区中可以写入的字节数
    size_t writeableBytes() const;
    //缓冲区中可以读取的字节数
    size_t readableBytes() const;
    //缓冲区中已经读取的字节数
    size_t readBytes() const;

    //获取当前读指针
    const char* curReadPtr() const;
    //获取当前写指针
    const char* curWritePtrConst() const;
    char* curWritePtr();
    //更新读指针
    void updateReadPtr(size_t len);
    void updateReadPtrUntilEnd(const char* end);//更新读指针到指定位置
    //更新写指针
    void updateWritePtr(size_t len);
    //初始化缓冲区
    void initPtr();

    //保证数据能够写入缓冲区
    void ensureWriteable(size_t len);
    //将数据写入缓冲区
    void append(const char* str,size_t len);
    void append(const std::string& str);
    void append(const void* data,size_t len);
    void append(const Buffer& buffer);

    //IO操作的读写接口
    ssize_t readFd(int fd,int* Errno);
    ssize_t writeFd(int fd,int* Errno);

    //将缓冲区的数据转化为字符串
    std::string AlltoStr();

    //test代码
    void printContent()
    {
        std::cout<<"pointer location info:"<<readPos_<<std::endl;
        for(int i=readPos_;i<= writePos_;++i)
        {
            std::cout<<buffer_[i]<<" ";
        }
        std::cout<<std::endl;
    }
private:
    //返回缓冲区指针的初始位置
    char* BeginPtr_();
    const char* BeginPtr_() const;
    //用于缓冲区扩容
    void allocateSpace(size_t len);

    std::vector<char>buffer_;
    //这里的atomic表示申请一个size_t类型的原子类型
    //对这种类型的变量进行读写操作时是原子性的，不会被其他线程所影响
    std::atomic<std::size_t>readPos_;
    std::atomic<std::size_t>writePos_;
};


#endif //CPROJECT_BUFFER_H
