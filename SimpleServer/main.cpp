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
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else {
			printf("��֧�ֵ����\n");
		}
	}
}

void main()
{
	SServer server;
	server.initSocket();
	server.SBind("192.168.100.37", 4567);
	server.SListen(5);

	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.run();
		//printf("����ʱ�䴦������ҵ��..\n");
	}
	server.close();
	printf("���˳���\n");
	getchar();
}