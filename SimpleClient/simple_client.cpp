
#include"simple_client.h"
#include<functional>

SimpleClient::SimpleClient()
{
	_isRunning.store(false);

	_socket = INVALID_SOCKET;

}

SimpleClient::~SimpleClient()
{
	if (_workThread.joinable())
		_workThread.join();

	_socket = INVALID_SOCKET;
}


void SimpleClient::init()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _socket)
	{
		printf("invalid socket.");
	}
}

void SimpleClient::SConnect(std::string ip, unsigned int port)
{

	init();

	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);
	_sin.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	int ret = connect(_socket, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("connect error.");
	}
}

void SimpleClient::SClose()
{
	_isRunning.store(false);
	closesocket(_socket);
	WSACleanup();
}

void SimpleClient::run()
{
	while (_isRunning.load())
	{
		/*char msgBuf[] = "hello,I am Client.";
		int ret = send(_socket, msgBuf, strlen(msgBuf) + 1, 0);
		printf("send return: %d\n", ret);
		Sleep(1000);*/

		fd_set fdRead, fdWrite;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_SET(_socket, &fdRead);
		FD_SET(_socket, &fdWrite);
		timeval t = { 0,1 };
		int readyNum = 0;
		readyNum = select(_socket + 1, &fdRead, &fdWrite, nullptr, &t);
		if (readyNum < 0)
		{
			printf("error,select exit.\n");
		}

		if (FD_ISSET(_socket, &fdRead))
		{
			char recvBuf[256] = {};
			int nLen = recv(_socket, recvBuf, 256, 0);
			if (nLen > 0)
				printf("recvive data: %s\n", recvBuf);
			if (nLen < 0)
			{
				printf("server close.\n");
				SClose();
			}
		}

		if (FD_ISSET(_socket, &fdWrite))
		{
			char msgBuf[] = "message from client.";
			int ret = send(_socket, msgBuf, strlen(msgBuf) + 1, 0);
			//printf("send return: %d\n", ret);
			Sleep(1000);
		}

	}
}

void SimpleClient::startWorkThread()
{
	_isRunning.store(true);
	_workThread = std::thread(std::mem_fn(&SimpleClient::run), this);
}