#pragma once

#include<vector>
#include<thread>
#include<mutex>
#include<functional>

template <typename T,typename t>
class ThreadPool
{
public:
	ThreadPool(int threadNum = 0)
	{ 
		if (0 == threadNum)
		{
			threadNum = std::thread::hardware_concurrency();
		}

		if (0 == threadNum)
		{
			threadNum = 4;
		}
		//printf("THREAD NUM:%d\n", threadNum);

		for (auto i = 0; i < threadNum; ++i)
		{
			_elementQueue.push_back(std::make_shared<T>());
		}

		_isRunning.store(true);

		for (auto i = 0; i < threadNum; ++i)
		{
			_threads.push_back(std::thread(std::mem_fn(&ThreadPool::run), this,i));
		}

	}
	~ThreadPool()
	{
		for (auto iter =  _threads.begin();iter!=_threads.end();++iter)
		{
			if (iter->joinable())
			{
				iter->join();
			}
		}
		_threads.clear();
	}

	void push(t t)
	{
		// todo:¸ÄÓÃ¶ÁÐ´Ëø
		//std::unique_lock<std::mutex> lock(_elementMutex);
		int minSizeIndex = 0;
		for (auto i = 0;i != _elementQueue.size(); i++)
		{
			if (_elementQueue[minSizeIndex]->getClientNum() > _elementQueue[i]->getClientNum())
				minSizeIndex = i;
		}
		_elementQueue[minSizeIndex]->addClient(t);
	}

	void stop()
	{
		_isRunning.store(false);
	}

private:
	void run(int idx)
	{
		printf("thread id:%d\n", std::this_thread::get_id());

		while (true == _isRunning.load())
		{
			//std::unique_lock<std::mutex> lock(_elementMutex);
			if (_elementQueue.empty())
			{
				continue;
			}
			_elementQueue[idx]->run();
		}
	}


private:
	std::mutex _elementMutex;
	std::vector<std::shared_ptr<T>> _elementQueue;

	std::vector<std::thread> _threads;

	std::atomic<bool> _isRunning = false;
};

