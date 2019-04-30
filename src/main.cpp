#include "thread_pool.h"
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
using namespace std;

ThreadPool tp(1);

int main(int argc,char *argv[])
{	
	Task([](__task__)
	{
		self->go([](__task__)
		{
			self->go([](__task__)
			{
				sleep(5);
				cout << "hello" << endl;
			});

			// self->wait();
			cout << "test" << endl;
		});
	}).run();

	cout << "finish" << endl;
	
	return 0;
}
