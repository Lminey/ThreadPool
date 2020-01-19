# ThreadPool
线程池

用于多线程进行执行某项任务
例子：
class InstallManager  : public ThreadPool<5>
{
private:
	InstallManagerr(void);
public:
	static InstallManager * Instance();

	virtual void DoWork(void * pWorkItem);

private:
	static InstallManager * _instance;
	
};
通过参数来进行设置线程池中的线程数量，实现DoWork函数去执行线程的具体任务，当任务数量小于线程数量时线程会等待任务分发，
当新任务添加到线程队列里面，等待的线程会进行启动执行DoWork函数，没个线程完成会询问是否有任务执行，有任务则继续执行没有
任务将等待新的任务加入。
