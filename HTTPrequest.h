//
// Created by 69572 on 2023/3/27.
//

#ifndef CPROJECT_HTTPREQUEST_H
#define CPROJECT_HTTPREQUEST_H
#include<unordered_map>
#include<unordered_set>
#include <string>
#include <regex>

#include "Buffer.h"

class HTTPrequest {
public:
    enum PARSE_STATE{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    
    HTTPrequest(){init();};
    ~HTTPrequest()=default;
    //初始化
    void init();
    //解析请求
    bool parse(Buffer& buff);
    //返回信息
    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;

    bool isKeepAlive() const;

private:
    bool parseRequestLine_(const std::string& line);//解析请求行
    void parseRequestHeader_(const std::string& line);//解析请求头
    void parseDataBody_(const std::string& line);//解析数据体

    void parsePath_();
    void parsePost_();

    static int convertHex(char ch);
    //解析状态(解析到什么位置了)
    PARSE_STATE state_;
    //method_保存请求方式(GET/POST)
    //path_保存请求中的路径(/mix/76.html?name=kelvin&password=123456)
    //version_保存请求版本(HTTP/1.1中的1.1)
    //body_保存请求体
    std::string method_,path_,version_,body_;
    //保存请求头中的键值对
    std::unordered_map<std::string,std::string>header_;
    //保存POST请求中的键值对
    std::unordered_map<std::string,std::string>post_;
    //一些默认的HTML页面
    static const std::unordered_set<std::string>DEFAULT_HTML;

};


#endif //CPROJECT_HTTPREQUEST_H
