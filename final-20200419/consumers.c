#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <prodcon.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <math.h>

#define CONS_STR		"CONSUME\r\n"
#define MAX_CL_NUM 		2000
#define DONE_STR 		"DONE\r\n"

int connectsock( char *host, char *service, char *protocol );
void *doConsumerThing( void *tid );
double poissonRandomInterarrivalDelay( double r );
void shuffle(int *array, size_t n);
void threadErrorExit(int csock, char *message);

char		*service;		
char		*host = "localhost";	

/*
**	Consumer
*/
int
main( int argc, char *argv[] )
{
	
	int 		num_of_cons, bad, misbehave;
	float 		rate;
	double		wait_time;
	
	/* Collect arguments */
	switch( argc ) 
	{
		case    5:
			service = argv[1];
			num_of_cons =  atoi(argv[2]);
			rate = atof(argv[3]);
			bad = atoi(argv[4]);
			break;
		case    6:
			host = argv[1];
			service = argv[2];
			num_of_cons =  atoi(argv[3]);
			rate = atof(argv[4]);
			bad = atoi(argv[5]);
			break;
		default:
			fprintf( stderr, "[WARNING] usage: consumers [host] port num rate bad\n" );
			exit(-1);
	}

	/* Error checking */

	if (num_of_cons > MAX_CL_NUM)
	{
		fprintf( stderr, "[ERROR] a maximum of 2000 clients is allowed.\n" );
		exit(-1);
	}

	if (rate <= 0)
	{
		fprintf( stderr, "[ERROR] Rate should be a floating point number greater than 0.\n" );
		exit(-1);
	}

	if (bad < 0 || bad > 100)
	{
		fprintf( stderr, "[ERROR] Bad should be an integer between 0 and 100.\n" );
		exit(-1);
	}

	/* Calculate number of misbehaving consumers */
	misbehave = (num_of_cons * bad) / 100;
	fprintf( stderr, "[INFO] [%i] misbehave number.\n",misbehave );

	/* Create an array of size num_prod
	** fill with ones for misbehave and zeros for simple ones */
	int is_bad [num_of_cons];
	int i;
	for (i = 0; i < misbehave; i++)
		is_bad[i] = 1;

	for (i = misbehave; i < num_of_cons; i++)
		is_bad[i] = 0;

	/* Shuffle array */
	shuffle(is_bad, num_of_cons);	

	int status;
	pthread_t threads [num_of_cons];

	for (i = 0; i < num_of_cons; i++)
	{
		/* Wait or some time, then go */
		wait_time = poissonRandomInterarrivalDelay(1/rate) * 1000000;
		fprintf( stderr, "[INFO] I sleep [%lf] .\n", wait_time);

		/* The usleep() function suspends execution of the calling thread for
       (at least) usec microseconds. */
		usleep(wait_time);

		status = pthread_create( &threads[i], NULL, doConsumerThing, (void *) is_bad[i] );

		if (status != 0)
			exit(-1);
	}

	for ( i = 0; i < num_of_cons; i++ )	{
		pthread_join( threads[i], NULL );
	}

}

void *doConsumerThing( void *tid ) 
{
	char		buf[BUFSIZE];
	int			csock, cc;
	int	is_bad = (int *) tid;
	char		message[50];

	/*	Create the socket to the controller  */
	sprintf(message, "[ERROR] [%ld] Cannot connect to server.\n", pthread_self());
	csock = connectsock( host, service, "tcp" );
	if ( csock == 0 )
		threadErrorExit(csock, message);

	/* If bad, wait for some time */
	if (is_bad == 1)
	{
		sleep(SLOW_CLIENT);
		fprintf( stderr, "[INFO] [%ld] I am bad.\n", pthread_self() );
	}
	else
		fprintf( stderr, "[INFO] [%ld] I am good.\n", pthread_self() );

	/* Write to server */
	cc = write( csock, CONS_STR, strlen(CONS_STR) );
	// fprintf( stderr, "[INFO] [%ld] Wrote to server.\n",pthread_self() );

	/* Error checking. */
	sprintf(message, "[ERROR] [write CONSUME] Connection lost.\n");
	if ( cc <= 0 )
		threadErrorExit(csock, message);
	
	/* Receive message. */ 
	int temp;
	cc = read( csock, &temp, 4 );
	uint32_t number_of_characters = ntohl( temp );
	// fprintf( stderr, "[INFO] [%i] Received int.\n", number_of_characters );

	/* Error checking. */
	sprintf(message, "[ERROR] [read int] Connection lost.\n");
	if ( cc <= 0 )
		threadErrorExit(csock, message);

	/* allocate space for characters */
	char *str = malloc( number_of_characters );

	/* read characters */
	cc = read( csock, str, number_of_characters);

	/* Error checking. */
	sprintf(message, "[ERROR] [read stream of chars] Connection lost.\n");
	if ( cc <= 0 )
		threadErrorExit(csock, message);

	close( csock );
	// fprintf( stderr, "[INFO] [%ld] Received char stream.\n", pthread_self() );

	/* create a document */
	sprintf(buf, "files/%ld.txt", pthread_self());

	int dstf = open( buf, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG );

	if (dstf <= 0)
	{
		fprintf( stderr, "[ERROR] [%ld] Cannot create file.\n", pthread_self() );
		pthread_exit( NULL );
	}  

	write( dstf, str, number_of_characters ); 

	close(dstf);
	free(str);
	fprintf( stderr, "[INFO] [%ld] Created file.\n", pthread_self() );
	pthread_exit( NULL );
}

/*
** 	Exit on errors
*/
void threadErrorExit(int csock, char *message)
{
	fprintf( stderr, message );
	close( csock ); 
	pthread_exit( NULL );	
}

/*
**  Poisson interarrival times. Adapted from various sources
**  r = desired arrival rate
*/
double poissonRandomInterarrivalDelay( double r )
{
    return (log((double) 1.0 - 
			((double) rand())/((double) RAND_MAX)))/-r;
}

/*	
**  Arrange the N elements of ARRAY in random order.
**  Only effective if N is much smaller than RAND_MAX;
** 	if this may not be the case, use a better random
**	number generator. 
*/
void shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}