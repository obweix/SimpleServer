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
#include<thread>
#include<memory>
#include<atomic>


class SimpleClient
{
public:
	SimpleClient();

	~SimpleClient();

	void SConnect(std::string ip,unsigned int port);

	void SClose();

	void startWorkThread();

private:
	void init();

	void run();

private:
	SOCKET _socket;

	std::thread _workThread;

	std::atomic<bool> _isRunning;
};
