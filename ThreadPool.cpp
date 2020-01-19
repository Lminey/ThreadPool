#include "stdafx.h"
#include "ThreadPool.h"



unsigned int GetMyThreadId(const thread::id & id)
{
	_Thrd_t t = *(_Thrd_t*)(char*)&id;
	unsigned int nId = t._Id;
	return nId;
}

