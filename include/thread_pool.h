#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <memory>
using namespace std;

#define __task__ Task *self

class Task;
class ThreadPool;
typedef function<void(Task *)> TaskFunc;

class Task
{
public:
	Task(TaskFunc fun, bool isSon = false)
		: fun_(fun), isSon_(isSon), done_(false) {}
	~Task();

	void *const getId() { return this; }
	void run();
	void wait();
	// Caution that the use of reference is necessary!
	static void release(shared_ptr<Task> &);

	void go(TaskFunc fun, shared_ptr<ThreadPool> pool = {});
	void go(function<void(void)> fun, shared_ptr<ThreadPool> pool = {})
	{
		go([=](__task__) { fun(); }, pool);
	}

private:
	TaskFunc fun_;
	bool isSon_;
	bool done_;
	vector<shared_ptr<Task>> sons_;
	mutex mt;
	condition_variable cv;
};

class ThreadPool
{
public:
	ThreadPool(int trNum = 2);
	~ThreadPool();

	void *const getId() { return this; }
	void wait();

	void commit(TaskFunc f)
	{
		commit(make_shared<Task>(f));
	}
	void commit(function<void(void)> f)
	{
		commit(make_shared<Task>([=](__task__) { f(); }));
	}
	void commit(shared_ptr<Task> task);

	void doRightNow(TaskFunc f)
	{
		doRightNow(make_shared<Task>(f));
	}
	void doRightNow(function<void(void)> f)
	{
		doRightNow(make_shared<Task>([=](__task__) { f(); }));
	}
	void doRightNow(shared_ptr<Task> task);

private:
	mutex mt;
	condition_variable cv_task;
	condition_variable cv_worker;
	int threadNum;
	int busyNum;
	bool shutdown;
	queue<shared_ptr<Task>> taskQue;
	vector<thread> workerVec;
};

class Semaphore
{
public:
	explicit Semaphore(int count = 0) : count_(count) {}

	void Wait()
	{
		unique_lock<mutex> lk(mt_);
		--count_;
		if (count_ < 0)
		{
			cv_.wait(lk);
		}
	}

	void Signal()
	{
		unique_lock<mutex> lk(mt_);
		++count_;
		if (count_ <= 0)
		{
			cv_.notify_one();
		}
	}

private:
	int count_;
	mutex mt_;
	condition_variable cv_;
};

class Echo
{
public:
	static Echo &getInstance()
	{
		// C++11 requires compiler to ensure the thread-safety for internal static variables.
		static Echo instance;
		return instance;
	}

	template <typename T>
	void print(const T &&t)
	{
		cout << t;
	}

	template <typename F, typename... Args>
	void expand(const F &f, Args &&... args)
	{
		int dummy[] = {(f(std::forward<Args>(args)), 0)...};
	}

	template <typename... Args>
	void operator()(Args... args)
	{
		lock_guard<mutex> ulk(mt);
		// (cout << ... << args) << endl; // for c++17
		int dummy[] = {(print(std::forward<Args>(args)), 0)...};
		cout << endl;
	}

private:
	Echo() {}
	Echo(const Echo &);
	Echo &operator=(const Echo &);
	static mutex mt;
};

// class RWLock
// {
// public:
// 	void RLock()
// 	{
// 		unique_lock<mutex> ulk(mt_);
// 		++readers_;
// 		if (writing_)
// 		{
// 			cv_.wait(ulk);
// 		}
// 		--readers_;
// 		reading_ = true;
// 	}
// 	void RULock()
// 	{
// 		unique_lock<mutex> ulk(mt_);
// 		reading_ = false;
// 		if (writers_ > 0)
// 		{
// 			cv_.notify_one();
// 		}
// 	}
// 	void WLOCK()
// 	{
// 		unique_lock<mutex> ulk(mt_);
// 		++writers_;
// 		if (reading_ || writing_)
// 		{
// 			cv_.wait(ulk);
// 		}
// 		--writers_;
// 		writing_ = true;
// 	}
// 	void WULock()
// 	{
// 		unique_lock<mutex> ulk(mt_);
// 		writing_ = false;
// 		if (readers_ > 0 || writers_ > 0)
// 		{
// 			cv_.notify_one();
// 		}
// 	}

// private:
// 	int readers_ = 0;
// 	int writers_ = 0;
// 	bool reading_ = false;
// 	bool writing_ = false;
// 	mutex mt_;
// 	condition_variable cv_;
// };

// class RWLock
// {
// public:
// 	void RLock()
// 	{
// 		lock_guard<mutex> lkw(w_);
// 		lock_guard<mutex> lk(mt_);
// 		if (readers_ == 0)
// 		{
// 			rw_.lock();
// 		}
// 		++readers_;
// 	}
// 	void RULock()
// 	{
// 		lock_guard<mutex> lk(mt_);
// 		--readers_;
// 		if (readers_ == 0)
// 		{
// 			rw_.unlock();
// 		}
// 	}
// 	void WLock()
// 	{
// 		w_.lock();
// 		rw_.lock();
// 	}
// 	void WULock()
// 	{
// 		rw_.unlock();
// 		w_.unlock();
// 	}

// private:
// 	int readers_ = 0;
// 	mutex mt_;
// 	mutex rw_;
// 	mutex w_;
// };

// class RWLock
// {
// public:
// 	void RLock()
// 	{
// 		smt_.lock_shared();
// 	}
// 	void RULock()
// 	{
// 		smt_.unlock_shared();
// 	}
// 	void WLock()
// 	{
// 		smt_.lock();
// 	}
// 	void WULock()
// 	{
// 		smt_.unlock();
// 	}

// private:
// 	shared_mutex smt_;
// }

#endif
