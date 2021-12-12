#include "simple_server.h"
#include <chrono>
#include <thread>
#include <functional>

SClient::SClient(SOCKET socket)
{
	_socket = socket;
	_httpConn.init(_socket);
}

SClient::~SClient()
{
	_socket = INVALID_SOCKET;
}

void SClient::close()
{
	if (INVALID_SOCKET != _socket) 
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		_httpConn.closeConn();
	}
}

bool SClient::read()
{
	if (0 == _httpConn._state)
	{
		if (_httpConn.read())
		{
			_httpConn.process();
			_httpConn._state = 1;
		}
		else
		{
			printf("read false.\n");
			return false;
		}
		return true;
	}
	return false;;
}

bool SClient::write()
{
	if (1 == _httpConn._state)
	{
		if (_httpConn.write())
		{
			_httpConn._state = 0;
		}
		else
		{
			printf("write false.\n");
		}
		return true;
	}
	return false;
}

SOCKET SClient::getSocket()
{
	return _socket;
}

// -------------------------------------------------------------------

TaskServer::TaskServer()
{
	_maxSocket = INVALID_SOCKET;
}

TaskServer::~TaskServer()
{
	printf("TaskServer deconstruce.");
}


void TaskServer::addClient(std::shared_ptr<SClient> spClient)
{
	_clientBuf.push(std::make_pair(spClient->getSocket(), spClient));
}

int TaskServer::getClientNum()
{
	return _mClient.size();
}

void TaskServer::run()
{
		std::map<SOCKET, std::shared_ptr<SClient>> temp;
		_clientBuf.tryPopAll(temp);
		for (auto pair : temp) {
			_mClient.insert(pair); // TODO: 优化
		}

		// sleep when has not client to accpet.
		if (_mClient.empty())
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			return;
		}

		fd_set fdRead;
		fd_set fdWrite;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		_maxSocket = _mClient.begin()->second->getSocket();
		for (auto pair : _mClient)
		{
			FD_SET(pair.second->getSocket(), &fdRead);
			FD_SET(pair.second->getSocket(), &fdWrite);
			if (_maxSocket < pair.second->getSocket())
			{
				_maxSocket = pair.second->getSocket();
			}
		}

		int readyNum = select(_maxSocket + 1, &fdRead, &fdWrite, nullptr, 0);
		//printf("ready num:%d\n", readyNum);
		if (readyNum < 0)
		{
			printf("select task end. \n");
			close();
		}
		else if (0 == readyNum)
		{
			return;
		}

#ifdef _WIN32
		//printf("fd cound:%d\n", fdRead.fd_count);
		for (unsigned int i = 0; i < fdRead.fd_count; ++i)
		{
			auto iter = _mClient.find(fdRead.fd_array[i]);
			if (iter != _mClient.end())
			{
				// reveive message frome client.
				/*char recvBuf[256] = {};
				int nLen = recv(iter->first, recvBuf, 256, 0);
				if (nLen > 0)
					printf("recvive data: %s\n", recvBuf);
				if (nLen < 0)
				{
					printf("client disconnect.\n");
					_mClient.erase(iter);
				}*/
				bool readSuccess =iter->second->read();
				if (!readSuccess)
				{
					//iter->second->close();
					_mClient.erase(iter);
				}
			}
		}

		for (unsigned int i = 0; i < fdWrite.fd_count; ++i)
		{
			auto iter = _mClient.find(fdWrite.fd_array[i]);
			if (iter != _mClient.end())
			{
				/*char msgBuf[] = "message from server.";
				int ret = send(iter->first, msgBuf, strlen(msgBuf) + 1, 0);
				if (ret < 0) 
				{
					printf("send error.\n");
				}*/
				iter->second->write();

			}
		}

#else
		for (auto pair : _mClient) 
		{
			std::vector<SOCKET> disconnectedSockets;
			if (FD_ISSET(iter->first, &fdRead))
			{
				// reveive message frome client.
				char recvBuf[256] = {};
				int nLen = recv(iter->first, recvBuf, 256, 0);
				if (nLen > 0)
					printf("recvive data: %d.\n", recvBuf);
				if (nLen < 0)
				{
					printf("client disconnect.\n");
					disconnectedSockets.push(iter->first);
				}

			}
			for (auto socket : disconnectedSockets)
			{
				_mClient.erase(socket);
			}
		}
#endif // _WIN32
}


void TaskServer::close()
{
	for (auto pair : _mClient) 
	{
		pair.second->close();
	}
	_mClient.clear();
	//_clientBuf.notifyOne();
}


// -------------------------------------------------------------------

SServer::SServer()
{
	_socket = INVALID_SOCKET;
	//_taskServer = std::make_shared<TaskServer>();
}

SOCKET SServer::initSocket()
{
#ifdef _WIN32
	//启动Windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	if (INVALID_SOCKET != _socket)
	{
		printf("close old socket: %d. @[%s]\n", (int)_socket, __FUNCTION__);
		this->close();
	}
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _socket)
	{
		printf("error,init socket fail. @[%s]\n", __FUNCTION__);
	}
	else {
		printf("init socket successfully. @[%s]\n", __FUNCTION__);
	}
	return _socket;
}

bool SServer::SBind(std::string ip, unsigned short port)
{
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
	if (!ip.empty()) {
		_sin.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	}
	else {
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
	}
#else
	if (!ip.empty()) {
		_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	else {
		_sin.sin_addr.s_addr = INADDR_ANY;
	}
#endif
	int ret = bind(_socket, (sockaddr*)&_sin, sizeof(_sin));
	if (SOCKET_ERROR == ret)
	{
		printf("fail,bind port[%d] fail.\n", port);
	}
	else {
		printf("bind port[%d] successfully.\n", port);
	}
	return ret;
}

int SServer::SListen(int n)
{
	int ret = listen(_socket, n);
	if (SOCKET_ERROR == ret)
	{
		printf("fail,listening socket:[%d] fail.\n", _socket);
	}
	else {
		printf("listening socket:[%d].\n", _socket);
	}
	return ret;
}

void SServer::startTaskServer(int cellSServerNum)
{
	//_taskServer->startTaskThread();
}

void SServer::run()
{
	fd_set fdRead;
	FD_ZERO(&fdRead);
	FD_SET(_socket, &fdRead);

	if (isRunning())
	{

		int numReady;
		timeval t = { 0,10 };
		numReady = select(_socket + 1, &fdRead, 0, 0, &t);
		if (numReady < 0)
		{
			printf("select return negative.");
		}

		//判断描述符（socket）是否在集合中
		if (FD_ISSET(_socket, &fdRead))
		{
			FD_CLR(_socket, &fdRead);
			SAccept();
		}

	}
}

bool  SServer::isRunning()
{
	return _socket != INVALID_SOCKET;
}


void SServer::SAccept()
{
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
	cSock = accept(_socket, (sockaddr*)&clientAddr, &nAddrLen);
#else
	cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
	if (INVALID_SOCKET == cSock)
	{
		printf("fial,invalid socket socket=<%d>.\n", (int)_socket);
	}
	else
	{
		//addClient2XServer(new ClientSocket(cSock));
		//_taskServer->addClient(cSock, std::make_shared<SClient>(cSock));
		_pool.push(std::make_shared<SClient>(cSock));
		printf("new client.\n");
	}
	//return cSock;
}

void SServer::close()
{
	if (_socket != INVALID_SOCKET)
	{
#ifdef _WIN32
		//_taskServer->close();
		_pool.stop();

		//关闭套节字closesocket
		closesocket(_socket);
		//清除Windows socket环境
		WSACleanup();
		//_socket = INVALID_SOCKET;
#else
		//关闭套节字closesocket
		close(_socket);
#endif
	}
}