#include"http_conn.h"

#include<stdio.h>
#include<io.h>
#include<fcntl.h>

#include "assert.h" 
#include<string>

#ifdef _WIN32
	#pragma  warning (disable:4996) 
#else
	#include<strings.h>
	#include <unistd.h>
#endif // _WIN32

//定义http响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";

HttpConn::HttpConn()
{
	_socket = INVALID_SOCKET;
}

HttpConn::~HttpConn()
{
	_socket = INVALID_SOCKET;
}

void HttpConn::init(SOCKET socket)
{
	printf("init http conn.\n");
	_socket = socket;
	_check_state = CHECK_STATE_REQUESTLINE;

	_read_idx = 0;
	_write_idx = 0;
	_checked_idx = 0;
	_start_line = 0;
	_content_length = 0;

	//root文件夹路径
	char server_path[200] = {};
	char root[6] = "/root";
	_doc_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);

	memset(_read_buf, '\0', READ_BUFFER_SIZE);
	memset(_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(_real_file, '\0', FILENAME_LEN);
}

void HttpConn::init()
{
	// 保持长连接，暂不关闭
	//_socket = INVALID_SOCKET;  
	_check_state = CHECK_STATE_REQUESTLINE;
	_read_idx = 0;
	_write_idx = 0;
	_checked_idx = 0;
	_start_line = 0;
	_content_length = 0;

	memset(_read_buf, '\0', READ_BUFFER_SIZE);
	memset(_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(_real_file, '\0', FILENAME_LEN);
}

bool HttpConn::read()
{
	assert(_socket != INVALID_SOCKET);
	if (_read_idx >= READ_BUFFER_SIZE)
	{
		return false;
	}

	//printf("read index:%d\n", _read_idx);

	int bytes_read = 0;
	//_read_idx = 0;
	while (true)
	{
		bytes_read = recv(_socket, _read_buf + _read_idx, READ_BUFFER_SIZE - _read_idx, 0);	
		if (-1 == bytes_read)
		{
			printf("errorno: %d\n",errno);
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				printf("break\n");
				break;
			}
			return false;
		}
		else if(0 == bytes_read)
		{
			return false;
		}
		_read_idx += bytes_read;
		printf("bytes_read:%d,socket:%d,read_idx:%d,\n_read_buf:\n%.*s", bytes_read, _socket, _read_idx, _read_idx, _read_buf);
		break;
	}
	return true;
}

bool HttpConn::write()
{
	int temp = 0;
	int bytes_have_send = 0;

	int bytes_to_send = _write_idx;
	if (0 == bytes_to_send)
	{
		init();
		return true;
	}

	int bytes_write = 0;
	while (true)
	{
		bytes_write = send(_socket, _write_buf + bytes_have_send, bytes_to_send - bytes_have_send, 0);

		if (-1 == bytes_write)
		{
			if (errno == EAGAIN)
			{
				return true;
			}
			return false;
		}
		else if (0 == bytes_write)
		{
			return false;
		}
		bytes_to_send -= bytes_write;
		bytes_have_send += bytes_write;
		_write_idx -= bytes_write;
		//printf("send:%s", _write_buf);
	}
	return true;
	
}

void HttpConn::closeConn(bool realColse)
{
	if (realColse && _socket != INVALID_SOCKET)
	{
		_socket = INVALID_SOCKET;
	}
}

void HttpConn::process()
{
	HttpConn::HTTP_CODE read_ret = process_read();
	if (read_ret == NO_REQUEST)
	{
		printf("client no request.");
		return;
	}
	bool write_ret = process_write(read_ret);
	if (!write_ret)
	{
		//closeConn();
	}
	printf("process return:%d\n", read_ret);
}

HttpConn::HTTP_CODE HttpConn::process_read()
{
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char* text = 0;

	while ((_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
	{
		text = get_line();
		_start_line = _checked_idx;
		//printf("%s\n", text);
		switch (_check_state)
		{
		case CHECK_STATE_REQUESTLINE:
		{
			ret = parse_request_line(text);
			if (ret == BAD_REQUEST)
				return BAD_REQUEST;
			break;
		}
		case CHECK_STATE_HEADER:
		{
			ret = parse_headers(text);
			if (ret == BAD_REQUEST)
				return BAD_REQUEST;
			else if (ret == GET_REQUEST)
			{
				return do_request();
			}
			break;
		}
		case CHECK_STATE_CONTENT:
		{
			ret = parse_content(text);
			if (ret == GET_REQUEST)
				return do_request();
			line_status = LINE_OPEN;
			break;
		}
		default:
			return INTERNAL_ERROR;
		}
	}
	return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text)
{
#ifdef _WIN32
	_url = strpbrk(text, " \t");
	if (!_url)
	{
		return BAD_REQUEST;
	}
	*_url++ = '\0';
	char* method = text;
	if (_stricmp(method, "GET") == 0)
		_method = GET;
	else if (_stricmp(method, "POST") == 0)
	{
		_method = POST;
		cgi = 1;
	}
	else
		return BAD_REQUEST;
	_url += strspn(_url, " \t");
	_version = strpbrk(_url, " \t");
	if (!_version)
		return BAD_REQUEST;
	*_version++ = '\0';
	_version += strspn(_version, " \t");
	if (_stricmp(_version, "HTTP/1.1") != 0)
		return BAD_REQUEST;
	if (_strnicmp(_url, "http://", 7) == 0)
	{
		_url += 7;
		_url = strchr(_url, '/');
	}

	if (_strnicmp(_url, "https://", 8) == 0)
	{
		_url += 8;
		_url = strchr(_url, '/');
	}

	if (!_url || _url[0] != '/')
		return BAD_REQUEST;
	//当url为/时，显示判断界面
	if (strlen(_url) == 1)
		strcat(_url, "judge.html");
	_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
#else
	_url = strpbrk(text, " \t");
	if (!_url)
	{
		return BAD_REQUEST;
	}
	*_url++ = '\0';
	char* method = text;
	if (strcasecmp(method, "GET") == 0)
		_method = GET;
	else if (strcasecmp(method, "POST") == 0)
	{
		_method = POST;
		cgi = 1;
	}
	else
		return BAD_REQUEST;
	_url += strspn(_url, " \t");
	_version = strpbrk(_url, " \t");
	if (!_version)
		return BAD_REQUEST;
	*_version++ = '\0';
	_version += strspn(_version, " \t");
	if (strcasecmp(_version, "HTTP/1.1") != 0)
		return BAD_REQUEST;
	if (strncasecmp(_url, "http://", 7) == 0)
	{
		_url += 7;
		_url = strchr(_url, '/');
	}

	if (strncasecmp(_url, "https://", 8) == 0)
	{
		_url += 8;
		_url = strchr(_url, '/');
	}

	if (!_url || _url[0] != '/')
		return BAD_REQUEST;
	//当url为/时，显示判断界面
	if (strlen(_url) == 1)
		strcat(_url, "judge.html");
	_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
#endif // _WIN32


}

HttpConn::HTTP_CODE HttpConn::parse_headers(char* text)
{
#ifdef _WIN32
	if (text[0] == '\0')
	{
		if (_content_length != 0)
		{
			_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (_strnicmp(text, "Connection:", 11) == 0)
	{
		text += 11;
		text += strspn(text, " \t");
		if (_stricmp(text, "keep-alive") == 0)
		{
			_linger = true;
		}
	}
	else if (_strnicmp(text, "Content-length:", 15) == 0)
	{
		text += 15;
		text += strspn(text, " \t");
		_content_length = atol(text);
	}
	else if (_strnicmp(text, "Host:", 5) == 0)
	{
		text += 5;
		text += strspn(text, " \t");
		_host = text;
	}
	else
	{
		//printf("oop!unknow header: %s", text);
	}
	return NO_REQUEST;
#else
	if (text[0] == '\0')
	{
		if (_content_length != 0)
		{
			_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection:", 11) == 0)
	{
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
		{
			_linger = true;
		}
	}
	else if (strncasecmp(text, "Content-length:", 15) == 0)
	{
		text += 15;
		text += strspn(text, " \t");
		_content_length = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5) == 0)
	{
		text += 5;
		text += strspn(text, " \t");
		_host = text;
	}
	else
	{
		printf("oop!unknow header: %s", text);
	}
	return NO_REQUEST;
#endif // _WIN32
}

// 没有真正解析http请求的消息体，只是判断它是否别完整得读入了
HttpConn::HTTP_CODE HttpConn::parse_content(char* text)
{
	if (_read_idx >= (_content_length + _checked_idx))
	{
		text[_content_length] = '\0';
		//POST请求中最后为输入的用户名和密码
		_uname = text;
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::do_request()
{
	strcpy(_real_file, _doc_root);
	int len = strlen(_doc_root);
	//printf("m_url:%s\n", _url);
	const char* p = strrchr(_url, '/');
	if (*(p + 1) == '0')
	{
		char* url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(url_real, "/register.html");
		strncpy(_real_file + len, url_real, strlen(url_real));

		free(url_real);
	}
	else if (*(p + 1) == '1')
	{
		char* url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(url_real, "/log.html");
		strncpy(_real_file + len, url_real, strlen(url_real));

		free(url_real);
	}
	else if (*(p + 1) == '5')
	{
		char* url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(url_real, "/picture.html");
		strncpy(_real_file + len, url_real, strlen(url_real));

		free(url_real);
	}
	else if (*(p + 1) == '6')
	{
		char* url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(url_real, "/video.html");
		strncpy(_real_file + len, url_real, strlen(url_real));

		free(url_real);
	}
	else if (*(p + 1) == '7')
	{
		char* url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(url_real, "/fans.html");
		strncpy(_real_file + len, url_real, strlen(url_real));

		free(url_real);
	}
	else
		strncpy(_real_file + len, _url, FILENAME_LEN - len - 1);

	if (stat(_real_file, &_file_stat) < 0)
		return NO_RESOURCE;

#ifdef _WIN32
	if (!(_file_stat.st_mode & S_IREAD))
		return FORBIDDEN_REQUEST;
#else
	if (!(_file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
#endif 

#ifdef _WIN32
	if (_file_stat.st_mode & S_IFDIR)
		return BAD_REQUEST;
#else
	if (S_ISDIR(_file_stat.st_mode))
		return BAD_REQUEST;
#endif 

#ifdef _WIN32
	_open_file_fd = open(_real_file, O_RDONLY);
	//_file_address = (char*)mmap(0, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	//close(fd);
	_file_address = _real_file; // TODO:建立映射区
	return FILE_REQUEST;
#else
	int fd = open(_real_file, O_RDONLY);
	_file_address = (char*)mmap(0, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
#endif // _WIN32

	
}

void HttpConn::unmap()
{
#ifdef __linux__
	if (_file_address)
	{
		munmap(m_file_address, _file_stat.st_size);
		m_file_address = 0;
	}
#endif // __linux__
}

bool HttpConn::add_response(const char* format, ...)
{
	if (_write_idx >= WRITE_BUFFER_SIZE)
		return false;
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(_write_buf + _write_idx, WRITE_BUFFER_SIZE - 1 - _write_idx, format, arg_list);
	if (len >= (WRITE_BUFFER_SIZE - 1 - _write_idx))
	{
		va_end(arg_list);
		return false;
	}
	_write_idx += len;
	va_end(arg_list);

	//printf("request:%s", _write_buf);

	return true;
}

bool HttpConn::add_content(const char* content)
{
	return add_response("%s", content);
}

bool HttpConn::add_status_line(int status, const char* title)
{
	return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConn::add_headers(int content_len)
{
	return add_content_length(content_len) && add_linger() &&
		add_blank_line();
}

bool HttpConn::add_content_type()
{
	return add_response("Content-Type:%s\r\n", "text/html");
}

bool HttpConn::add_content_length(int content_len)
{
	return add_response("Content-Length:%d\r\n", content_len);

}
bool HttpConn::add_linger()
{
	return add_response("Connection:%s\r\n", (_linger == true) ? "keep-alive" : "close");
}
bool HttpConn::add_blank_line()
{
	return add_response("%s", "\r\n");
}


bool HttpConn::process_write(HTTP_CODE ret)
{
	switch (ret)
	{
	case INTERNAL_ERROR:
	{
		add_status_line(500, error_500_title);
		add_headers(strlen(error_500_form));
		if (!add_content(error_500_form))
			return false;
		break;
	}
	case BAD_REQUEST:
	{
		add_status_line(404, error_404_title);
		add_headers(strlen(error_404_form));
		if (!add_content(error_404_form))
			return false;
		break;
	}
	case FORBIDDEN_REQUEST:
	{
		add_status_line(403, error_403_title);
		add_headers(strlen(error_403_form));
		if (!add_content(error_403_form))
			return false;
		break;
	}
	case FILE_REQUEST:
	{
		add_status_line(200, ok_200_title);
		{
			const char* ok_string = "<html><body></body></html>";
			add_headers(strlen(ok_string));
			if (!add_content(ok_string))
				return false;
		}
		break;
	}
	case NO_RESOURCE:
	{
		add_status_line(200, ok_200_title);
		{

			std::string path = "E:\\code\\cpp\\SimpleServer\\SimpleServer\\Debug\\log.html";
			struct stat stat_buf;
			int rc = stat(path.c_str(), &stat_buf);
			long file_size = rc == 0 ? stat_buf.st_size : -1;
			//printf("filesiez:%d\n", file_size);
			if (file_size != -1)
			{
				add_headers(file_size);
				char* buffer = (char*)malloc(file_size);
				FILE* fp;
				fp = fopen(path.c_str(), "rb");
				fread(buffer, file_size, 1, fp);
				if (_write_idx + file_size > WRITE_BUFFER_SIZE)
				{
					printf("write buffer too small\n");
				}
				memcpy(_write_buf + _write_idx, buffer, file_size);
				_write_idx += file_size;
				fclose(fp);
				free(buffer);
			}
			else 
			{
				const char* ok_string = "<html><body></body></html>";
				add_headers(strlen(ok_string));
				if (!add_content(ok_string))
					return false;
			}
		}
	}
	default:
		return false;
	}
	return true;
}

HttpConn::LINE_STATUS  HttpConn::parse_line()
{
	char temp;
	for (; _checked_idx < _read_idx; ++_checked_idx)
	{
		temp = _read_buf[_checked_idx];
		if (temp == '\r')
		{
			if ((_checked_idx + 1) == _read_idx)
				return LINE_OPEN;
			else if (_read_buf[_checked_idx + 1] == '\n')
			{
				_read_buf[_checked_idx++] = '\0';
				_read_buf[_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if (temp == '\n')
		{
			if (_checked_idx > 1 && _read_buf[_checked_idx - 1] == '\r')
			{
				_read_buf[_checked_idx - 1] = '\0';
				_read_buf[_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}