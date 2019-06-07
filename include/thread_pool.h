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

class Semaphore {
public:
	explicit Semaphore(int count=0) : count_(count) {
	}

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

class Task{
public:
	Task(TaskFunc fun, bool isSon=false)
		: fun_(fun), isSon_(isSon), sema_(0) {}
	~Task();

	void run();
	void wait();
	// Caution that the use of reference is necessary!
	static void release(shared_ptr<Task>&);
	void* const getId() { return this; }

	void go(TaskFunc fun, shared_ptr<ThreadPool> pool={});
	void go(function<void(void)> fun, shared_ptr<ThreadPool> pool={}) {
		go([=](__task__){fun();}, pool);
	}
private:
	TaskFunc fun_;
	bool isSon_;
	Semaphore sema_;
	vector<shared_ptr<Task>> sons_;
};

class Worker{
public:
	friend class ThreadPool;
	Worker() : busy(false),inited(false) {}
	~Worker() {
		if(th) {
			th->join();
		}		
	}
	bool isBusy() { return busy; }
	void setBusy(bool b) { busy = b; }
	void setTaskFunc(function<void(void)> f){
		unique_ptr<thread> tmp(new thread(f)); 
		swap(th, tmp);
	}
private:
	bool busy;
	bool inited;
	unique_ptr<thread> th;
};

class ThreadPool{
public:
	ThreadPool() {set(2);}
	ThreadPool(int tNum) {set(tNum);}
	~ThreadPool();
	void wait();
	void commit(shared_ptr<Task> task);
	void commit(TaskFunc f){
		commit(make_shared<Task>(f));
	}
	void commit(function<void(void)> f) {
		commit(make_shared<Task>([=](__task__){f();}));
	}
	void* const getId() {return this;}
protected:
	void set(int tNum);
private:
	mutex mt;
	condition_variable cv_task;
	condition_variable cv_worker;
	bool shutdown;
	queue<shared_ptr<Task>> taskQue;
	// Use unique_ptr to ensure that the worker is only exist in wks
	vector<unique_ptr<Worker>> workerVec;
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
