#include "thread_pool.h"

mutex Echo::mt;

Task::~Task()
{
	cout << "to delete task (" << this << ")" << endl;
	wait();
	for(auto son:sons){
		son.reset();
	}
	if(sons.empty())
		cout << "no sons,task (" << this << ")" << endl;
	else
		cout << "sons released,task (" << this << ")" << endl;
}

void Task::run()
{
	if(fun_ == nullptr)
		cout << "no work assigned for task (" << this << ")" << endl;
	else
		fun_(this);
}

void Task::wait()
{
	for(auto son:sons){
		unique_lock<mutex> ulk(son->mt);
		while(!son->done_){
			son->cv.wait(ulk);
		}
	}
}

void Task::go(TaskFunc fun, shared_ptr<ThreadPool> pool)
{
	auto t = make_shared<Task>(fun);
	sons.push_back(t);

	if(pool){
		pool->commit(t);
	}
	else{
		thread([t,this]{
			cout << "task ("<< this << ") employ new thread to do task (" << t << ")" << endl;
			t->run();
			lock_guard<mutex> ulk(t->mt);
			t->done_ = true;
			t->cv.notify_all();	
		}).detach();
	}
}

void ThreadPool::set(int thNum)
{
	shutdown = false;
	for(int i = 0; i < thNum; i++){
		unique_ptr<Worker> wk(new Worker());
		Worker* w = wk.get();
		wk->setTaskFunc([this,w]{
			while(1){
				shared_ptr<Task> task;
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

				lock_guard<mutex> ulk(task->mt);
				task->finish();
				task->cv.notify_all();
			}			
		});
		wks.push_back(std::move(wk));
	}
}

ThreadPool::~ThreadPool()
{
	shutdown = true;
	cv.notify_all();

	for(auto& wk : wks){
		wk.reset();
	}

	while(!taskQue.empty()){
		taskQue.pop();
	}
}

void ThreadPool::commit(shared_ptr<Task> task)
{
	unique_lock<mutex> ulk(mt);
	cout << "pool (" << this << ") push task (" << task << ")" << endl;
	for(auto& wk : wks){
		if(!wk->isBusy()){
			taskQue.push(task);
			cv.notify_one();
			return;
		}
	}
	// all workers are busy now, create a new thread
	thread([task,this]{
		cout << "pool ("<< this << ") employ new thread to do task (" << task << ")" << endl; 
		task->run();
		lock_guard<mutex> ulk(task->mt);
		task->finish();
		task->cv.notify_all();
	}).detach();
}