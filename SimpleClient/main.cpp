#include "simple_client.h"

#define CLIENT_NUM 1000

int main()
{
	SimpleClient client[CLIENT_NUM];

	for (int i = 0; i < CLIENT_NUM; ++i)
	{
		client[i].SConnect("192.168.100.37", 4567);
		client[i].startWorkThread();
		printf("run client:%d\n", i);		
	}
	return 0;
}