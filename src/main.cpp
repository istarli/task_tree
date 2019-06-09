#include "thread_pool.h"
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
using namespace std;

shared_ptr<ThreadPool> tp = make_shared<ThreadPool>(2);
static Echo& echo = Echo::getInstance();

Semaphore sema(0);

int main(int argc,char *argv[])
{	
	// Example for Semaphore
	echo("Example for Semaphore:");
	tp->commit([]{
		sema.Wait();
		echo("world");
	});
	tp->commit([]{
		echo("hello");
		sleep(3);
		sema.Signal();
	});
	tp->doRightNow([]{
		echo("urgent");
	});
	tp->wait();

	// Example for task tree
	echo("Example for task tree");
	Task([](__task__)
	{
		self->go([](__task__)
		{
			self->go([]
			{
				sleep(5);
				echo("son-son-1 task");
			});
			self->go([]
			{
				echo("son-son-2 task");
			});
			self->wait();

			echo("son-task");
		});
		
		echo("origin-task");
	}).run();

	echo("finish",1,"haha");

	return 0;
}
