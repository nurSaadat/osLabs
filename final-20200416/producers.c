#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PROD_STR			"PRODUCE\r\n"
#define FIRST_PRINTABLE 	32
#define LAST_PRINTABLE		126
#define MAX_CL_NUM 			2000

void *doProducerThing(void *tid);
double poissonRandomInterarrivalDelay( double r );
void shuffle(int *array, size_t n);

char		*service;		
char		*host = "localhost";

/*
**	Producer
*/
int
main( int argc, char *argv[] )
{
	int 		num_of_prod, bad, misbehave;
	float 		rate;
	double		wait_time;
	
	/* Collect arguments */
	switch( argc ) 
	{
		case    5:
			service = argv[1];
			num_of_prod =  atoi(argv[2]);
			rate = atof(argv[3]);
			bad = atoi(argv[4]);
			break;
		case    6:
			host = argv[1];
			service = argv[2];
			num_of_prod =  atoi(argv[3]);
			rate = atof(argv[4]);
			bad = atoi(argv[5]);
			break;
		default:
			fprintf( stderr, "[WARNING] usage: producers [host] port num rate bad\n" );
			exit(-1);
	}

	/* Error checking */

	if (num_of_prod > MAX_CL_NUM)
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

	/* Calculate number of misbehaving producers */
	misbehave = (num_of_prod * bad) / 100;
	fprintf( stderr, "[INFO] [%i] misbehave number.\n",misbehave );

	/* Create an array of size num_prod
	** fill with ones for misbehave and zeros for simple ones */
	int is_bad [num_of_prod];
	int i;
	for (i = 0; i < misbehave; i++)
		is_bad[i] = 1;

	for (i = misbehave; i < num_of_prod; i++)
		is_bad[i] = 0;

	/* Shuffle array */
	shuffle(is_bad, num_of_prod);	

	int status;
	pthread_t threads [num_of_prod];
	
	for (i = 0; i < num_of_prod; i++)
	{
		/* Wait or some time, then go */
		wait_time = poissonRandomInterarrivalDelay(rate) * 1000000;
		fprintf( stderr, "[INFO] I sleep [%lf] .\n", wait_time);

		/* The usleep() function suspends execution of the calling thread for
       (at least) usec microseconds. */
		usleep(wait_time);

		status = pthread_create( &threads[i], NULL, doProducerThing, (void *) is_bad[i] );

		if (status != 0)
			exit(-1);
	}

	for ( i = 0; i < num_of_prod; i++ )	{
		pthread_join( threads[i], NULL );
	}

}

void *doProducerThing(void *tid)
{
	char		buf[BUFSIZE];
	int 		csock, cc;
	int	is_bad = (int *) tid;

	/*	Create the socket to the controller  */
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "[ERROR] [%ld] Cannot connect to server.\n", pthread_self() );
		close( csock ); 
		pthread_exit( NULL );
	}

	/* If bad, wait for some time */
	if (is_bad == 1)
	{
		sleep(SLOW_CLIENT);
		fprintf( stderr, "[INFO] [%ld] I am bad.\n", pthread_self() );
	}
	else
	{
		fprintf( stderr, "[INFO] [%ld] I am good.\n", pthread_self() );
	}
	
	/* Write to server */
	cc = write( csock, "PRODUCE\r\n", strlen("PRODUCE\r\n") );
	// fprintf( stderr, "[INFO] [%ld] Wrote to server.\n", pthread_self() );

	/* Error checking. */
	if ( cc <= 0 )
	{
		fprintf( stderr, "[ERROR] [write PRODUCE] Connection lost.\n" );
		close( csock ); 
		pthread_exit( NULL );
	}

	/* Receive message. */
	cc = read( csock, buf, BUFSIZE );
	// fprintf( stderr, "[INFO] [%ld] Got response.\n", pthread_self() );

	/* Error checking. */
	if ( cc <= 0 )
	{
		fprintf( stderr, "[ERROR] [recieve response] Connection lost.\n" );
		close( csock ); 
		pthread_exit( NULL );
	}

	/* generate random number */
	int number_of_characters = (rand() % MAX_LETTERS) + 1;

	/* send it to server */
	uint32_t temp = htonl( number_of_characters );
	cc = write( csock, &temp, 4 );
	// fprintf( stderr, "[INFO] [%ld] Sent int to server.\n", pthread_self() );

	/* Error checking. */
	if ( cc <= 0 )
	{
		fprintf( stderr, "[ERROR] [send int] Connection lost.\n" );
		close( csock ); 
		pthread_exit( NULL );
	}

	/* produce string */
	char *str = malloc(number_of_characters);

	int  i;
	for (i = 0; i < number_of_characters; i++)
		str[i] = (rand() % (LAST_PRINTABLE - FIRST_PRINTABLE + 1)) + FIRST_PRINTABLE;	

	/* send it to server */
	cc = write( csock, str, strlen(str) );

	/* Error checking. */
	if ( cc <= 0 )
	{
		fprintf( stderr, "[ERROR] [send char stream] Connection lost.\n" );
		close( csock ); 
		pthread_exit( NULL );
	}

	read( csock, buf, BUFSIZE );
	fprintf( stderr, "[INFO] [%ld] Sent char stream to server.\n", pthread_self() );	
	free(str);

	/* close connection */
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


