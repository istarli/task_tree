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
	Task(TaskFunc fun) : fun_(fun), done_(false) {}
	~Task();
	void wait();
	void run();
	void go(TaskFunc fun,shared_ptr<ThreadPool> pool={});
	void go(function<void(void)> fun,shared_ptr<ThreadPool> pool={}) {
		go([=](__task__){fun();}, pool);
	}
	void* const getId() {return this;}
	void finish() { done_ = true; }
public:
	mutex mt;
	condition_variable cv;
	vector<shared_ptr<Task>> sons;
private:
	TaskFunc fun_;
	bool done_;
};

class Worker{
public:
	Worker() : busy(false) {}
	~Worker() {
		if(th) {
			th->join();
			th.reset();
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
	unique_ptr<thread> th;
};

class ThreadPool{
public:
	ThreadPool() {set(2);}
	ThreadPool(int tNum) {set(tNum);}
	~ThreadPool();
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
	queue<shared_ptr<Task>> taskQue;
	vector<unique_ptr<Worker>> wks;
	mutex mt;
	condition_variable cv;
	bool shutdown;
};

class Echo{
public:
	Echo():newline(true){}
	Echo(bool nl):newline(nl){}
	void operator()(string msg){
		lock_guard<mutex> ulk(mt);
		cout << msg;
		if(newline) cout << endl;
	}
private:
	bool newline;
	static mutex mt;
};

#endif
