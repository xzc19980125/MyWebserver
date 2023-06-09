//
// Created by 69572 on 2023/3/24.
//

#include "Buffer.h"

Buffer::Buffer(int initBuffersize):buffer_(initBuffersize),readPos_(0),writePos_(0){}


//三段式存储
//|_____________________|____________________________________|________________________________________|
//BeginPtr         BeginPtr_() + readPos_               BeginPtr_() + writePos_             buffer_.size()

//缓冲区中可以写入的字节数
size_t Buffer::writeableBytes() const
{
    return buffer_.size() - writePos_;
}
//缓冲区中可以读取的字节数
size_t Buffer::readableBytes() const
{
    return writePos_ - readPos_;
}
//缓冲区中已经读取的字节数
size_t Buffer::readBytes() const
{
    return readPos_;
}

//获取当前读指针
const char* Buffer::curReadPtr() const
{
    return BeginPtr_() + readPos_;
}
//获取当前写指针
const char* Buffer::curWritePtrConst() const
{
    return BeginPtr_() + writePos_;
}
char* Buffer::curWritePtr()
{
    return BeginPtr_() + writePos_;
}
//更新读指针
void Buffer::updateReadPtr(size_t len)
{
    assert(len<=readableBytes());
    readPos_+=len;
}
void Buffer::updateReadPtrUntilEnd(const char* end)//更新读指针到指定位置
{
    assert(end>=curReadPtr());
    updateReadPtr(end - curReadPtr());
}
//更新写指针
void Buffer::updateWritePtr(size_t len)
{
    assert(len<=writeableBytes());
    writePos_ += len;
}
//初始化缓冲区
void Buffer::initPtr()
{
    bzero(&buffer_[0],buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

//保证缓冲区的大小足够写入数据
//当存储内容超过申请的空间，则需要扩容
void Buffer::ensureWriteable(size_t len)
{
    if(writeableBytes() < len)
    {
        allocateSpace(len);
    }
    assert(writeableBytes() >= len);
}
//将数据写入缓冲区
void Buffer::append(const char* str,size_t len)
{
    assert(str);
    ensureWriteable(len);
    std::copy(str,str+len,curWritePtr());
    updateWritePtr(len);
}
void Buffer::append(const std::string& str)
{
    append(str.data(),str.length());
}
void Buffer::append(const void* data,size_t len)
{
    assert(data);
    append(static_cast<const char*>(data),len);
}
void Buffer::append(const Buffer& buffer)
{
    append(buffer.curReadPtr(),buffer.readableBytes());
}

//IO操作的读接口
ssize_t Buffer::readFd(int fd,int* Errno)
{
    //设置一个较大的数组buff，来保证数据能够读完
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writeableBytes();
    //分段式写入
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    //写不完的数据先放入buff里
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd,iov,2);
    if(len < 0)//len<0，出错
    {
        *Errno=errno;
    }
    else if(static_cast<size_t>(len)<=writable)//len<=writable表示不需要buff作为缓冲区来存储
    {
        writePos_ += len;
    }
    else//需要buff作为缓冲区先进行存储，在原数组中进行扩容后再将buff中的内容存入
    {
        writePos_ = buffer_.size();
        append(buff,len-writable);
    }
    return len;
}
//IO操作的写接口
ssize_t Buffer::writeFd(int fd,int* Errno)
{
    size_t readSize = readableBytes();
    ssize_t len = write(fd,curReadPtr(),readSize);
    if(len < 0)
    {
        *Errno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

//将缓冲区的数据转化为字符串
std::string Buffer::AlltoStr()
{
    std::string str(curReadPtr(),readableBytes());
    initPtr();
    return str;
}

//返回缓冲区指针的初始位置
char* Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}
const char* Buffer::BeginPtr_() const
{
    return &*buffer_.begin();
}
//用于缓冲区扩容
void Buffer::allocateSpace(size_t len)
{
    
    if(writeableBytes()+readBytes() < len)
    {
        buffer_.resize(writePos_+len);//申请额外len的空间
    }
    else//若需要申请的空间比可写空间+已读空间(均为可写空间)小，则不需要申请，直接移动内容即可
    {
        size_t readable = readableBytes();
        std::copy(BeginPtr_() + readPos_,BeginPtr_() + writePos_,BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable==readableBytes());
    }
}