#include "thread_pool.h"

mutex Echo::mt;
static Echo& echo = Echo::getInstance();

Task::~Task()
{
	cout << "to delete task (" << this << ")" << endl;
	wait();
	// Due to the memory of shared_ptr will be released automatically,
	// you don't need to call reset() explicitly here except that you want to
    // do something in destructor after your son tasks have been released.
	// Caution that using reference is necessary!
	// Otherwise the shared_ptr would be copyed and the reset() is meaningless.
	for(auto& son:sons_){
		son.reset();
	}
	if(sons_.empty())
		echo("no sons,task (",this,")");
	else
		echo("sons released,task (",this,")");
}

void Task::run()
{
	if(fun_ == nullptr)
		echo("no work assigned for task (",this,")");
	else
		fun_(this);
}

void Task::wait()
{
	for(auto son:sons_){
		son->sema_.Wait();
	}
}

void Task::go(TaskFunc fun, shared_ptr<ThreadPool> pool)
{
	auto task = make_shared<Task>(fun,true);
	sons_.push_back(task);

	if(pool){
		pool->commit(task);
	}
	else{
		// The shared_ptr is copyed into another thread.
		thread([this,task]()mutable{
			echo("task (",this,") employ new thread to do task (",task.get(),")");
			task->run();
			release(task);
		}).detach();
	}
}

void Task::release(shared_ptr<Task>& task)
{
	if(task->isSon_){
		Task* tmp = task.get();
		// To ensure that the shared_ptr object in father lives longer
		task.reset();
		tmp->sema_.Signal();
		return;
	}
	task.reset();
}

void ThreadPool::set(int thNum)
{
	shutdown = false;
	for(int i = 0; i < thNum; i++){
		unique_ptr<Worker> worker(new Worker());
		auto w = worker.get(); 
		worker->setTaskFunc([this,w]{
			while(1){
				shared_ptr<Task> task;
				{
					unique_lock<mutex> ulk(mt);
					while(!shutdown && taskQue.empty()){
						if(w->inited){
							w->inited = true;
						} else {
							w->setBusy(false);
							echo("notify");
							cv_worker.notify_one();							
						}
						cv_task.wait(ulk);
					}
					if(shutdown) return;
					w->setBusy(true);
					task = taskQue.front();
					taskQue.pop();
				}
				echo("worker (", w, ") in pool (", getId(), ") do task (", task.get(), ")");
				task->run();
				Task::release(task);
			}			
		});
		workerVec.push_back(std::move(worker));
	}
}

ThreadPool::~ThreadPool()
{
	lock_guard<mutex> lg(mt);
	shutdown = true;
	// Process remain tasks
	while(!taskQue.empty()){
		// Caution that you need to change the state to 'finish' for remain tasks,
		// otherwise it's father will be endless blocked.
		Task::release(taskQue.front());
		taskQue.pop();
	}
	cv_task.notify_all();
	// Working threads will be joined when wks is released automatically.
}

void ThreadPool::wait()
{
	unique_lock<mutex> ulk(mt);
	int cnt = taskQue.size();
	for(auto& worker : workerVec){
		if(worker->isBusy()){
			++cnt;
		}
	}
	while(cnt > 0) {
		cv_worker.wait(ulk);
		cnt--;
	}
}

void ThreadPool::commit(shared_ptr<Task> task)
{
	lock_guard<mutex> lg(mt);
	echo("pool (", this, ") push task (", task, ")");
	for(auto& worker : workerVec){
		if(!worker->isBusy()){
			taskQue.push(task);
			cv_task.notify_one();
			return;
		}
	}
	// All workers are busy now, create a new thread
	thread([this,task]()mutable{
		echo("pool (",this,") employ new thread to do task (",task, ")"); 
		task->run();
		Task::release(task);
	}).detach();
}