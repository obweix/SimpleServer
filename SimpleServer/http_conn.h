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
        _DELETE,          // ��ͻ
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    /* �����ͻ�������ʱ����״̬��������״̬ */
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    /* ����������http����Ŀ��ܽ�� */
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
    /* �еĶ�ȡ״̬ */
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    int _state = 0;  //��Ϊ0, дΪ1

private:
    void init();

   
    HTTP_CODE process_read(); /* ����http���� */
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
   
    bool process_write(HTTP_CODE ret); /* ���httpӦ�� */
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
    int _checked_idx; // ��ǰ���ڷ������ַ����ڶ���������λ��
    int  _start_line; // ��ǰ���ڽ������е���ʼλ��


    int _read_idx;
    char _read_buf[READ_BUFFER_SIZE];

    int _write_idx;
    char _write_buf[WRITE_BUFFER_SIZE];

    
    CHECK_STATE _check_state;/* ��״̬��������״̬ */
   
    METHOD _method; /* ���󷽷� */
   
    char* _version; /* http Э��汾�� */
    
    char* _host; /* ������ */

    
    bool _linger;/* http�����Ƿ�Ҫ�󱣳����� */
    int _content_length;

    char* _file_address;

    int cgi;        //�Ƿ����õ�POST

    struct stat _file_stat; // Ŀ���ļ���״̬����ȡ�ļ���С����Ϣ
    char* _doc_root;
    char _real_file[FILENAME_LEN];// �ͻ��������Ŀ���ļ�������·���������ݵ���_doc_root+_url,_doc_rootʱ��վ�ĸ�Ŀ¼
    char* _url;

    std::map<std::string,std::string> _users;


    char* _uname; //�洢����ͷ����
    int _open_file_fd;
 
};