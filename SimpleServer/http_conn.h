#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib")
#endif // _WIN32

#include<string>
#include<map>

class HttpConn {
public:
	HttpConn();
	~HttpConn();

    void init(SOCKET socket);

    void closeConn(bool realColse = true);

    void process();

    bool read();

    bool write();

public:
	static const int FILENAME_LEN = 200;
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        _DELETE,          // 冲突
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    /* 解析客户端请求时，主状态机所处的状态 */
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    /* 服务器处理http请求的可能结果 */
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    /* 行的读取状态 */
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    int _state = 0;  //读为0, 写为1

private:
    void init();

   
    HTTP_CODE process_read(); /* 解析http请求 */
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
   
    bool process_write(HTTP_CODE ret); /* 填充http应答 */
    char* get_line() { return _read_buf + _start_line; }
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_len);
    bool add_content_type();
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();

private:
    SOCKET _socket;
    int _checked_idx; // 当前正在分析的字符串在读缓冲区的位置
    int  _start_line; // 当前正在解析的行的起始位置


    int _read_idx;
    char _read_buf[READ_BUFFER_SIZE];

    int _write_idx;
    char _write_buf[WRITE_BUFFER_SIZE];

    
    CHECK_STATE _check_state;/* 主状态机所处的状态 */
   
    METHOD _method; /* 请求方法 */
   
    char* _version; /* http 协议版本号 */
    
    char* _host; /* 主机名 */

    
    bool _linger;/* http请求是否要求保持连接 */
    int _content_length;

    char* _file_address;

    int cgi;        //是否启用的POST

    struct stat _file_stat; // 目标文件的状态，获取文件大小等信息
    char* _doc_root;
    char _real_file[FILENAME_LEN];// 客户端请求的目标文件的完整路径，其内容等于_doc_root+_url,_doc_root时网站的根目录
    char* _url;

    std::map<std::string,std::string> _users;


    char* _uname; //存储请求头数据
    int _open_file_fd;
 
};