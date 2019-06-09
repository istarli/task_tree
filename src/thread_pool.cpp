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
	for(auto& son:sons_){
		unique_lock<mutex> ulk(son->mt);
		while(!son->done_){
			son->cv.wait(ulk);
		}
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
	task->done_ = true;
	if(task->isSon_){
		Task* tmp = task.get();
		// To ensure that the task object in it's father lives longer
		task.reset();
		tmp->cv.notify_all();
		return;
	}
	// To ensure that the task object is released before the thread re-enter the loop.
	task.reset();
}

ThreadPool::ThreadPool(int thNum) :
	threadNum(thNum), 
	busyNum(thNum), 
	shutdown(false)
{
	for(int i = 0; i < threadNum; i++){
		workerVec.push_back(std::move(thread([this]{
			while(1){
				shared_ptr<Task> task;
				{
					unique_lock<mutex> ulk(mt);
					--busyNum;	
					while(!shutdown && taskQue.empty()){
						if(0 == busyNum){
							// Consider that ThreadPool::wait() may be called in multiply threads,
							// using notify_all() here is necessary. 
							cv_worker.notify_all();
						}
						cv_task.wait(ulk);
					}
					if(shutdown) {
						return;
					}
					task = taskQue.front();
					taskQue.pop();
				}
				echo("worker (", std::this_thread::get_id(), ") in pool (", getId(), ") do task (", task.get(), ")");
				task->run();
				Task::release(task);
			}			
		})));
	}
	// Ensure that all threads are in waiting mode (busyNum is 0) after constructor
	wait();
}

ThreadPool::~ThreadPool()
{
	{
		lock_guard<mutex> lg(mt);
		shutdown = true;
		// Process remain tasks
		while(!taskQue.empty()){
			// Caution that you need to change the state to 'finish' for remain tasks,
			// otherwise it's father will be endless blocked.
			Task::release(taskQue.front());
			taskQue.pop();
			echo("here");
		}
		cv_task.notify_all();
	}
	// Release the lock and waiting for threads's exiting.
	for(auto& th : workerVec) {
		th.join();
	}
}

void ThreadPool::wait()
{
	unique_lock<mutex> ulk(mt);
	cv_worker.wait(ulk,[this]{
		return 0 == busyNum || shutdown;
	});
}

/*
	Just push the task into taskQue.
	Caution:
		Deadlock maybe accure! The remained tasks in taskQue will not be executed if 
	all threads are busy waiting these tasks to be finished. 
*/
void ThreadPool::commit(shared_ptr<Task> task)
{
	lock_guard<mutex> lg(mt);
	echo("pool (", this, ") push task (", task, ")");
	taskQue.push(task);
	++busyNum;
	cv_task.notify_one();
}

/*
	Do the task right now.
	If all threads are busy now, create a new one.
	No deadlock will be caused.
*/
void ThreadPool::doRightNow(shared_ptr<Task> task)
{
	lock_guard<mutex> lg(mt);
	++busyNum;
	if(busyNum < threadNum){
		taskQue.push(task);
		cv_task.notify_one();
	} else {
		// All workers are busy now, create a new thread
		thread([this,task]()mutable{
			echo("pool (",this,") employ new thread to do task (",task, ")");
			task->run();
			Task::release(task);
			lock_guard<mutex> lg(mt);
			--busyNum;
			if(busyNum == 0){
				cv_worker.notify_all();
			}
		}).detach();
	}
}