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

#define __task__  Task* self

class Task;
class ThreadPool;
typedef function<void(Task*)> TaskFunc;

class Task{
public:
	Task(TaskFunc fun, bool isSon=false)
		: fun_(fun), isSon_(isSon), done_(false) {}
	~Task();

	void* const getId() { return this; }
	void run();
	void wait();
	// Caution that the use of reference is necessary!
	static void release(shared_ptr<Task>&);

	void go(TaskFunc fun, shared_ptr<ThreadPool> pool={});
	void go(function<void(void)> fun, shared_ptr<ThreadPool> pool={}) {
		go([=](__task__){fun();}, pool);
	}
private:
	TaskFunc fun_;
	bool isSon_;
	bool done_;
	vector<shared_ptr<Task>> sons_;
	mutex mt;
	condition_variable cv;
};

class ThreadPool{
public:
	ThreadPool(int trNum=2);
	~ThreadPool();
	
	void* const getId() { return this; }
	void wait();

	void commit(TaskFunc f) {
		commit(make_shared<Task>(f));
	}
	void commit(function<void(void)> f) {
		commit(make_shared<Task>([=](__task__){f();}));
	}
	void commit(shared_ptr<Task> task);

	void doRightNow(TaskFunc f) {
		doRightNow(make_shared<Task>(f));
	}
	void doRightNow(function<void(void)> f) {
		doRightNow(make_shared<Task>([=](__task__){f();}));
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

class Semaphore {
public:
	explicit Semaphore(int count=0) : count_(count) {}

	void Signal() {
		unique_lock<mutex> lk(mt_);
		++count_;
		cv_.notify_one();
	}

	void Wait() {
		unique_lock<mutex> lk(mt_);
		cv_.wait(lk, [this]{ return count_ > 0; });
		--count_;
	}

private:
	int count_;
	mutex mt_;
	condition_variable cv_;
};

class Echo{
public:
	static Echo& getInstance() {
		static Echo instance;
		return instance;
	}

	template<typename T>
	void bar(T&& t){ cout << t; }

	template<typename... Args>
	void operator()(Args... args){
		lock_guard<mutex> ulk(mt);
		(cout << ... << args) << endl;
	}

private:
	Echo(){}
	Echo(const Echo&);
	Echo& operator=(const Echo&);
	static mutex mt;
};

// TODO : implement Singleton with std::do_once | unique_ptr
class Singleton{

};

#endif
