#include "simple_server.h"
#include<stdio.h>

bool g_bRun = true;
void cmdThread()
{//
	while (true)
	{
		char cmdBuf[256] = {};
		scanf_s("%s", cmdBuf,256);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令。\n");
		}
	}
}

void main()
{
	SServer server;
	server.initSocket();
	server.SBind("192.168.100.37", 4567);
	server.SListen(5);

	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.run();
		//printf("空闲时间处理其它业务..\n");
	}
	server.close();
	printf("已退出。\n");
	getchar();
}