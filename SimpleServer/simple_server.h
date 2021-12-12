#pragma once

#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS

	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")

#endif // _WIN32

#include<string>
#include<vector>
#include<map>
#include<memory>
#include<mutex>
#include<condition_variable>
#include<atomic>

#include"thread_pool.h"
#include "http_conn.h"

class SClient
{
public:
	SClient(SOCKET socket = INVALID_SOCKET);

	~SClient();

	void close();

	//bool onWork();

	bool read();

	bool write();

	SOCKET getSocket();

private:
	SOCKET _socket;
	HttpConn _httpConn;
};

/**
*  客户端socket缓存，线程安全
*/
class SClientQueue
{
public:
	void push(std::pair<SOCKET, std::shared_ptr<SClient>> client)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_mClient.insert(client);
		_dataCond.notify_one();
	}

	void waitAndPopAll(std::map<SOCKET, std::shared_ptr<SClient>>& out)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_dataCond.wait(lock, [this] {
			printf("wait.\n");
			return !_mClient.empty(); 
		});
		out = _mClient; 
		_mClient.clear();
	}

	void tryPopAll(std::map<SOCKET, std::shared_ptr<SClient>>& out)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (_mClient.empty()) {
			return;
		}
		out = _mClient; 
		_mClient.clear();
	}


	void notifyOne()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_mClient.clear();
		_mClient.insert(std::make_pair(INVALID_SOCKET, std::shared_ptr<SClient>()));
		_dataCond.notify_one();
	}
	
private:
	std::map<SOCKET, std::shared_ptr<SClient>> _mClient;
	std::mutex _mutex;
	std::condition_variable _dataCond;
};

class TaskServer
{
public:
	TaskServer();

	~TaskServer();

	void addClient(std::shared_ptr<SClient> spClient);

	int getClientNum();

	void run();

	void close();

private:
	std::map<SOCKET,std::shared_ptr<SClient>> _mClient;

	SClientQueue _clientBuf;

	SOCKET _maxSocket;
};

class SServer
{
public:
	SServer();

	SOCKET initSocket();

	bool SBind(std::string ip,unsigned short port);

	int SListen(int n);

	void startTaskServer(int cellSServerNum = -1);

	void run();

	bool isRunning();

	void close();

	void SAccept();

	//void addNew2TaskServer(SClient client);

private:
	SOCKET _socket;

	//std::shared_ptr<TaskServer> _taskServer;
	ThreadPool<TaskServer, std::shared_ptr<SClient>> _pool;
};