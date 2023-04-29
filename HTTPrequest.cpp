//
// Created by 69572 on 2023/3/27.
//

#include "HTTPrequest.h"

const std::unordered_set<std::string> HTTPrequest::DEFAULT_HTML{
        "/index", "/welcome", "/video", "/picture"};

//初始化，将string和map都清空，state_状态设置到REQUEST_LINE
void HTTPrequest::init(){
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HTTPrequest::parse(Buffer& buff){
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0){
        return false;
    }
    std::cout<<"parse buff start:"<<std::endl;
    buff.printContent();
    std::cout<<"parse buff finish:"<<std::endl;
    //循环读取，分别读取method_,path_,version_,body_中需要的内容
    while(buff.readableBytes() && state_ != FINISH){
        //查找curReadPtr到curWritePtr中是否存在CRLF，存在则返回其迭代器
        const char* lineEnd = std::search(buff.curReadPtr(),buff.curWritePtrConst(),CRLF,CRLF+2);
        //line为读指针到lineEnd的位置
        std::string line(buff.curReadPtr(),lineEnd);
        //进入状态选择
        switch (state_) {
            case REQUEST_LINE://解析请求行
                if(!parseRequestLine_(line)){
                    return false;//失败返回false
                }
                parsePath_();//解析成功则返回true，并将内容存入method_,path_,version_，设置state_=HEADERS
                break;
            case HEADERS://解析除了请求行的文件头
                parseRequestHeader_(line);//将文件头存入header_中，设置state_=BODY
                if(buff.readableBytes() <=2){//若读取结束后，剩余字节<=2，说明没有数据体了，设置状态为FINISH
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseDataBody_(line);//读取数据体，并存入post_中
                break;
            default:
                break;
        }
        //到达读指针位置，则停止
        if(lineEnd == buff.curWritePtr()){
            break;
        }
        buff.updateReadPtrUntilEnd(lineEnd+2);//跳过\r\n
    }
    return true;//成功解析，返回true
}

std::string HTTPrequest::path() const{
    return path_;
}
std::string& HTTPrequest::path(){
    return path_;
}
std::string HTTPrequest::method() const{
    return method_;
}
std::string HTTPrequest::version() const{
    return version_;
}

//返回post请求中的value
std::string HTTPrequest::getPost(const std::string& key) const{
    assert(key != "");
    if(post_.count(key)==1){
        return post_.find(key)->second;
    }
    return "";
}
std::string HTTPrequest::getPost(const char* key) const{
    assert(key != nullptr);
    if(post_.count(key) == 1){
        return post_.find(key)->second;
    }
    return "";
}

bool HTTPrequest::isKeepAlive() const{
    if(header_.count("Connection")==1){//当连接数为0时，不用保持alive
        //当键值对中Connection对应的value不为keep-alive或version不为1.1，则不用保持alive
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

//用于匹配解析第一行请求行（GET /mix/76.html?name=kelvin&password=123456 HTTP/1.1），解析成功返回true
bool HTTPrequest::parseRequestLine_(const std::string& line){//解析请求行
    //正则表达式，^表示开头，$表示结尾，[^ ]表示匹配除了" "（空格以外的所有字符）
    //*表示多次匹配，直到下一个字符为" "为止
    //()表示一个匹配组
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    //创建一个用于存储匹配结果的参数
    std::smatch subMatch;
    //下述判断中会将匹配结果放入subMatch中，subMatch[0]为匹配成功的字符串整体，subMatch[i]表示第i个匹配组的结果
    //匹配成功返回true，否则返回false
    if(std::regex_match(line,subMatch,patten)){
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;

}
//用于匹配解析文件头中除了第一行以外的所有内容
void HTTPrequest::parseRequestHeader_(const std::string& line){//解析请求头
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line,subMatch,patten)){
        header_[subMatch[1]] = subMatch[2];
    }
    else{
        state_=BODY;
    }
}

void HTTPrequest::parseDataBody_(const std::string& line){//解析数据体
    body_ = line;
    parsePost_();
    state_ = FINISH;
}

void HTTPrequest::parsePath_(){
    if(path_=="/"){
        path_="/index.html";
    }
    else{
        for(auto &item:DEFAULT_HTML){
            if(item==path_){
                path_+=".html";
                break;
            }
        }
    }
}

//解析post请求
void HTTPrequest::parsePost_(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        //请求头中的 Content-Type 是 application/x-www-form-urlencoded，说明请求中包含表单数据
        if(body_.size()==0){ return;}//数据体为空则直接返回

        std::string key,value;
        int num = 0;
        int n = body_.size();
        int i = 0,j = 0;

        for(;i<n;i++){
            char ch = body_[i];
            switch(ch){
                case '=':
                    //如果遇到字符 '='，说明此时需要提取出表单数据的键，将其存储在变量 key 中，并将变量 j 更新为当前位置的下一位
                    key = body_.substr(j,i-j);
                    j = i + 1;
                    break;
                case '+':
                    //如果遇到字符 '+'，将其替换为 ' '，因为在表单数据中，空格通常使用加号表示。
                    body_[i] = ' ';
                    break;
                case '%':
                    //如果遇到字符 '%'，说明此时需要将十六进制编码转换为 ASCII 码。
                    // 具体来说，需要将其转换为对应的十进制数，并将其表示为字符。
                    // 将转换后的字符替换原来的字符即可。
                    num = convertHex(body_[i + 1])*16+ convertHex(body_[i+2]);
                    //将两个字符表示的16进制数合并成一个10进制数，如'A'和'F'转换为10*16+15=175
                    body_[i+2] = num%10 + '0';//将5转换为对应的ascii，存入
                    body_[i+1] = num/10 + '0';//将17转换为对应的ascii，存入，则原本的'AF'变成了'175'
                    i += 2;
                    break;
                case '&':
                    //如果遇到字符 '&'，说明此时需要提取出表单数据的值，
                    //将其存储在变量 value 中，并将键值对存储在 post_ 中。
                    value = body_.substr(j,i-j);
                    j=i+1;
                    post_[key]=value;
                    break;
                default:
                    break;
            }
        }
        assert(j <= i);
        if(post_.count(key) == 0 && j < i) {
            //处理最后一个键值对
            //当key在post_中不存在并且j<i时，说明还有最后一个键值对，只在key中记录了key值
            //需要将value值也相对应地传入
            value = body_.substr(j, i - j);
            post_[key] = value;
        }
    }
}

int HTTPrequest::convertHex(char ch){//字符表示的16进制数转换为10进制数，如'A'转换为10，'a'转换为10
    if(ch >= 'A' && ch <= 'F') return ch - 'A' +10;
    if(ch >= 'a' && ch <='f') return ch - 'a' +10;
    return ch;//0-9之间，则直接返回
}
