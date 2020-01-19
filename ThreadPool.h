#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#pragma once
#include "stdafx.h"
#include <iostream>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>


template <int worker>
class ThreadPool;
template <int worker>
struct WorkerInitData
{
	int id;
	ThreadPool<worker> *pThis;
};

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

unsigned int GetMyThreadId(const thread::id & id);

//class WorkItem;
template<int worker>
class __declspec(dllexport) ThreadPool
{
public:
	ThreadPool(string strName)
	{
		this->strThreadName = strName;
		m_bQuit = false;
		m_bQuitDone = false;

		for (int i = 0; i < worker;i++)//初始化信号量数据
		{
			m_workerInitData[i].id = i;
			shared_ptr<std::mutex> mutexWork = make_shared<std::mutex>();
			shared_ptr<std::condition_variable> ocndtnWork = make_shared<std::condition_variable>();
			m_mutexMap[i] = mutexWork;
			m_cndtnMap[i] = ocndtnWork;
			m_workerInitData[i].pThis = this;

			m_vRunningWorkItem[i] = NULL;
		}

		thread thrdMain(bind(&ThreadPool::ManangerThreadProc, this,this));
		unsigned long  threadNumber = 0;
		threadNumber = GetMyThreadId(thrdMain.get_id());
		SetThreadName((DWORD)threadNumber, strThreadName.c_str());

		for (int i = 0; i < worker;i++)
		{
			char cThrdNum[10];
			/*cThrdNum=*/
			_itoa_s(i,cThrdNum, 10);
			string strThrdName = strThreadName + "_" + string(cThrdNum);
			thread thrd(bind(&ThreadPool::WorkerThreadProc, this, &m_workerInitData[i]));
			unsigned long  itemThreadNumber=0;
			itemThreadNumber = GetMyThreadId(thrd.get_id());
			SetThreadName((DWORD)itemThreadNumber, strThrdName.c_str());
			thrd.detach();
		}

		thrdMain.detach();

	}
	~ThreadPool()
	{

	}
public:
	virtual void DoWork(void * pWorkItem) = 0;

	void AddWorkItem(void * pworkitem)  // Caller new WorkItem, caller delete WorkItem after notifyEvent is triggered.
	{
		m_mutex_main.lock();
		m_queueWorkItem.push(pworkitem);
		m_mutex_main.unlock();
		m_condition_variable_main.notify_one();
		return;
	}

	bool StopThread()
	{
		m_bQuit = true;
		//stop child thread 
		for (int i = 0; i < worker;i++)
		{
			m_cndtnMap[i]->notify_one();
			m_vRunningWorkItem[i] = NULL;
		}
		Sleep(1000);
		//stop master thread
		m_condition_variable_main.notify_one();
		Sleep(1000);
		//clear worker data

		//clear mutex data
		m_mutexMap.clear();
		m_cndtnMap.clear();
		//clear queue data
		while (!m_queueWorkItem.empty())
		{	
			m_queueWorkItem.pop();
		}
		int counter = 0;
		while (true)
		{
			counter++;
			Sleep(100);

			if (m_bQuitDone == true || counter > 20)
				break;
		}
		return true;
	}

private:
	static void SetThreadName(DWORD dwThreadID, const char* threadName)
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	int ManangerThreadProc(LPVOID param)
	{
		//消费者模式
		try
		{
			ThreadPool *pThis = (ThreadPool *)param;
			unique_lock<std::mutex> lockMain(pThis->m_mutex_main);
			while (!pThis->m_bQuit)
			{
				//pThis->m_mutex_main.lock();
				int workerIndex = -1;
				if (pThis->m_queueWorkItem.size() < 1)
				{
					pThis->m_condition_variable_main.wait(lockMain);
				}
				else
				{
					for (int i = 0; i < worker; i++)
					{
						if (pThis->m_vRunningWorkItem[i] == NULL)
						{
							workerIndex = i;
							break;
						}
					}
					if (workerIndex==-1)
					{
						pThis->m_condition_variable_main.wait(lockMain);
					}
					else
					{
						void * pWorkItem = pThis->m_queueWorkItem.front();
						pThis->m_vRunningWorkItem[workerIndex] = pWorkItem;
						pThis->m_queueWorkItem.pop();
					}

				}

				//pThis->m_mutex_main.unlock();
				if (workerIndex!=-1)
				{
					pThis->m_cndtnMap[workerIndex]->notify_one();
				}
				Sleep(10);
			}

			pThis->m_bQuitDone = true;
		}
		catch (exception *p)
		{
			cout <<"error" << endl;
		}
		return 0;
	}

	int WorkerThreadProc(LPVOID param)
	{
		//线程执行
		try
		{
			WorkerInitData<worker> *pWorkerInitData = (WorkerInitData<worker> *)param;
			ThreadPool *pThis = pWorkerInitData->pThis;
			int nWorker = pWorkerInitData->id;
			while (!pThis->m_bQuit)
			{
				unique_lock<std::mutex> lock(*(pThis->m_mutexMap[nWorker]));
				if (pThis->m_vRunningWorkItem[nWorker] == NULL)
				{
					pThis->m_cndtnMap[nWorker]->wait(lock);//没任务等待
				}
				else/* (m_vRunningWorkItem[nWorker]!=NULL)*/
				{
					pThis->DoWork(this->m_vRunningWorkItem[nWorker]);
					void * pWorkItem =this->m_vRunningWorkItem[nWorker];
					pThis->m_vRunningWorkItem[nWorker] = NULL;
					delete pWorkItem;
					pWorkItem=NULL;

					pThis->m_condition_variable_main.notify_one();
				}
			}
		}
		catch (exception *p)
		{
			cout << "error" << endl;
		}
		return 0;
	}

private:
	bool m_bQuit;
	bool m_bQuitDone;
	string strThreadName;
	WorkerInitData<worker> m_workerInitData[worker];
	map<int, shared_ptr<std::mutex>> m_mutexMap;
	map<int, shared_ptr<std::condition_variable>> m_cndtnMap;
	std::condition_variable m_condition_variable_main;
	std::mutex m_mutex_main;
	queue<void *> m_queueWorkItem;
	//queue<WorkItem *> m_queueWorkItemDoing;
	void * m_vRunningWorkItem[worker];
};
#endif