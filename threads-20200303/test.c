// Example program
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

int sum;
void *runner(void *param);

int main(int argc, char *argv[])
{
	pthread_t tid; // thread identifier
	pthread_attr_t attr; // set of attributes

	if (argc =! 2){
		fprintf(stderr, "usage: a.out <integer value>\n");
		return -1;	
	}
	if (atoi(argv[1]) < 0){
		fprintf(stderr, "%d must be >= 0\n", atoi(argv[1]));
		return -1;	
	}

	// get default attr
	pthread_attr_init(&attr);
	// create thread
	pthread_create(&tid, &attr, runner, argv[1]);
	// wait for thread to exit
	pthread_join(tid,NULL);

	printf("sum = %d\n", sum);
}

void *runner(void *param)
{
	int i, upper = atoi(param);
	sum = 0;

	for (i = 1; i <= upper; i++){
		sum += i;
	}
	pthread_exit(0);
}


