#include "thread_pool.h"

mutex Echo::mt;

Task::~Task()
{
	cout << "to delete task (" << this << ")" << endl;
	wait();
	for(auto s:sons){
		delete s;
	}
	if(sons.empty())
		cout << "no sons,task (" << this << ")" << endl;
	else
		cout << "sons deleted,task (" << this << ")" << endl;
}


void Task::set(Fun fun, Task* fa, bool suicide)
{
	this->done = false;
	this->father = fa;
	this->fun = fun;
	this->suicide = suicide;
	this->notice = false;

	if(fa != nullptr){
		this->notice = true;
		this->suicide = false;
		unique_lock<mutex> ulk(fa->mt);
		cout << "push task (" << this << ") to father task (" << fa << ")" << endl;
		fa->sons.push_back(this);
	}
}

void Task::run()
{
	if(fun == nullptr)
		cout << "no work assigned for task (" << this << ")" << endl;
	else
		fun(this);
}

void Task::wait()
{
	unique_lock<mutex> ulk(mt);
	for(auto s:sons){
		cv.wait(ulk,[s]{ return s->done; });
	}
}

void Task::go(Fun fun, ThreadPool* pool)
{
	Task* t = new Task(fun,this);

	if(pool){
		pool->commit(t);
	}
	else{
		thread([t,this]{
			cout << "task ("<< this << ") employ new thread to do task (" << t << ")" << endl;
			t->run();
			lock_guard<mutex> ulk(t->father->mt);
			// t->wait();
			t->done = true;
			t->father->cv.notify_all();	
		}).detach();
	}
}

void ThreadPool::set(int thNum)
{
	shutdown = false;
	for(int i = 0; i < thNum; i++){
		Worker* w = new Worker();
		w->setFunc([this,w]{
			while(1){
				Task* task;
				{
					unique_lock<mutex> ulk(mt);
					cv.wait(ulk,[this,w]{
						// cout << "worker(" << w << ") in pool(" << getId()<< ") enter wait" << endl;
						w->setBusy(false);
						return shutdown || !taskQue.empty();
					});
					w->setBusy(true);
					if(shutdown) return;

					task = taskQue.front();
					taskQue.pop();
				}
				
				cout << "worker (" << w << ") in pool (" << getId() 
						<< ") do task (" << task << ")" << endl;
				task->run();

				if(task->notice){
					lock_guard<mutex> ulk(task->father->mt);
					// task->wait();
					task->done = true;
					task->father->cv.notify_all();
				}
				else if(task->suicide){
					cout << "task (" << task << ") suicide" << endl;
					delete task;
				}
			}			
		});
		wks.push_back(w);
	}
}

ThreadPool::~ThreadPool()
{
	shutdown = true;
	cv.notify_all();

	for(auto w:wks) delete w;

	while(!taskQue.empty()){
		Task* t = taskQue.front();
		taskQue.pop();
		if(t->suicide) delete t;
	}
}

void ThreadPool::commit(Task* task)
{
	unique_lock<mutex> ulk(mt);
	cout << "pool (" << this << ") push task (" << task << ")" << endl;
	for(auto w:wks){
		if(!w->isBusy()){
			taskQue.push(task);
			cv.notify_one();
			return;
		}
	}
	// all workers are busy now, create a new thread
	thread([task,this]{
		cout << "pool ("<< this << ") employ new thread to do task (" << task << ")" << endl; 
		task->run();
		if(task->notice){
			lock_guard<mutex> ulk(task->father->mt);
			task->done = true;
			task->father->cv.notify_all();
		}
		else if(task->suicide){
			cout << "task (" << task << ") suicide" << endl;
			delete task;
		}	
	}).detach();
}