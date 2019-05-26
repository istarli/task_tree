#include "thread_pool.h"
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
using namespace std;

shared_ptr<ThreadPool> tp = make_shared<ThreadPool>(5);

int main(int argc,char *argv[])
{	
	tp->commit([]{
		sleep(5);
		cout << "hello world" << endl;
	});

	Task([](__task__)
	{
		self->go([](__task__)
		{
			self->go([]
			{
				sleep(2);
				cout << "hello" << endl;
			}, tp);

			// self->wait();
			cout << "test" << endl;
		}, tp);
	}).run();

	cout << "finish" << endl;
	
	return 0;
}
