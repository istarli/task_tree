#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
using namespace std;

#define __task__  Task* self

class Task;
class ThreadPool;
typedef function<void(Task*)> Fun;

class Task{
public:
	Task() {set(nullptr,nullptr,false);}
	Task(Fun fun) {set(fun,nullptr,false);}
	Task(Fun fun, bool suicide) {set(fun,nullptr,suicide);}
	Task(Fun fun, Task* fa) {set(fun,fa,false);}
	~Task();
	void wait();
	void run();
	void go(Fun fun,ThreadPool* pool=nullptr);
	void* const getId() {return this;}
protected:
	void set(Fun fun, Task* fa, bool suicide);
public:
	Task* father;
	bool done;
	bool suicide; // to be deleted by thread
	bool notice; // advertise done
	vector<Task*> sons;
	mutex mt;
	condition_variable cv;
private:
	Fun fun;
};

class Worker{
public:
	Worker():busy(false),th(nullptr){}
	~Worker(){
		if(th){
			th->join();
			delete th;
		}
	}
	bool isBusy() {return busy;}
	void setBusy(bool b){busy = b;}
	void setFunc(function<void(void)> f){
		th = new thread(f);
	}
private:
	bool busy;
	thread* th;
};

class ThreadPool{
public:
	ThreadPool() {set(2);}
	ThreadPool(int tNum) {set(tNum);}
	~ThreadPool();
	void commit(Task* task);
	void commit(Fun f){commit(new Task(f,true));}
	void* const getId() {return this;}
protected:
	void set(int tNum);
private:
	queue<Task*> taskQue;
	vector<Worker*> wks;
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
