#include <arpa/inet.h>
#include <errno.h>
#include <prodcon.h>
#include <pthread.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h> 
#include <unistd.h>

#define GO_STR "GO\r\n"
#define DONE_STR "DONE\r\n"

int count_buffer;
int cons_num, prod_num, client_num;
int prod_served, cons_served;
int total_excd_max, id_too_long;
int excd_prod_max, excd_cons_max; 

ITEM **buffer;
pthread_mutex_t item_mutex, threads_mutex;
sem_t full, empty;

typedef struct trd_inf
{
	int ssock;
	int index;
	int type;
} THREAD_INFO;

void *handleConnection( void *tid );
void threadErrorExit(int csock);
void handleProducer( int ssock, int thread_idx );
void handleConsumer( int ssock, int thread_idx );
void closeClient( int ssock, int type );
void closeProducerConnection( int psock );
void streamData( int psock, int csock, int size );

/*
**	This server can serve several clients at a time
*/
int
main( int argc, char *argv[] )
{
	char buf[ BUFSIZE ] = {'\0'};
	char		*service;
	struct 	sockaddr_in		fsin;
			socklen_t		alen;
	int			msock;
	int			ssock;
	fd_set		rfds;
	fd_set		afds;
	int			fd;
	int			nfds;
	int			cc;
	int			rport = 0;	
	int 		item_buf_size;
	time_t		curr_time;
	time_t		start_time[ MAX_CLIENTS + 3 ];

	/* Initialize counters */	
	cons_num = 0;
	prod_num = 0;
	client_num = 0;
	prod_served = 0;
	cons_served = 0;
	total_excd_max = 0;
	id_too_long = 0;
	excd_prod_max = 0;
	excd_cons_max = 0;

	switch ( argc ) 
	{
		case	2:
			/* If no port provided - let the OS choose a port and tell the user. */
			/* Initialize buffer */
			rport = 1;
			item_buf_size = atoi( argv[1] );
			break;
		case	3:
			/* If user provides a port - use it */
			/* Initialize buffer */
			service = argv[1];
			item_buf_size = atoi( argv[2] );
			break;
		default:
			fprintf( stderr, "[WARNING] usage: server [port] bufsize\n" );
			exit(-1);
	}

	buffer = malloc( item_buf_size * sizeof(ITEM *));
	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
	{
		/* Tell the user the selected port */
		fprintf( stderr,  "[INFO] server: port %d\n", rport  );
	}
	
	/* nfds is the largest monitored socket + 1 */
	nfds = msock+1;

	/* the fd_set of sockets monitored for a read activity */
	FD_ZERO(&afds);

	/* Then we put msock in the set */
	FD_SET( msock, &afds );

	/* Mutex for item writing */
	pthread_mutex_init( &item_mutex, NULL );

	/* Mutex for thread placement */
	pthread_mutex_init( &threads_mutex, NULL );

	/* Semaphores for keeping track of fullness of information buffer */
	sem_init( &full, 0, 0 );
	sem_init( &empty, 0, item_buf_size );

	/* Int to keep track of buffer idx */
	count_buffer = 0;

	fprintf( stderr,  "[INFO] Loading server.\n");

	for (;;)
	{
		memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));
		/* Timeout interval */
		struct timeval tv;
		tv.tv_sec = REJECT_TIME;
		tv.tv_usec = 0;

		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &tv) == 0)
		{
			fprintf(stderr, "Brr, I woke up\n");
			continue;
		}
		else if (cc == -1)
		{
			fprintf( stderr, "[ERROR] Server select: %s\n", strerror(errno) );
			exit(-1);
		}


		if (FD_ISSET( msock, &rfds)) 
		{
			int	ssock;

			/* we can call accept with no fear of blocking */
			alen = sizeof(fsin);
			ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
			/* Record start time */
			time( &start_time[ssock] );

			if (ssock < 0)
			{
				fprintf( stderr, "[ERROR] Accept: %s\n", strerror(errno) );
				exit(-1);
			}

			/* If a new client arrives, we must add it to our afds set */
			FD_SET( ssock, &afds );

			/* and increase the maximum, if necessary */
			if ( ssock+1 > nfds )
				nfds = ssock+1;
		}

		/* Now check all the regular sockets */
		for ( fd = 3; fd < nfds; fd++ )
		{

			if ( fd != msock && FD_ISSET(fd, &afds) )
			{
				/* Check if it is not too late to service */
				time( &curr_time );
				if ( difftime( curr_time, start_time[fd]) >= REJECT_TIME ) 
				{
					id_too_long ++;
					close(fd);		
					fprintf( stderr, "[ERROR] Client rejected. Identification takes too long.\n" );			

					/* Don't forget to stop monitoring this socket */
					FD_CLR( fd, &afds );
					FD_CLR( fd, &rfds );

					/* Decrement the count if necessary */
					if ( nfds == fd + 1 )
						nfds --;
					continue;
				}
			}

			if ( fd != msock && FD_ISSET(fd, &rfds) )
			{	

				if ( (cc = read( fd, buf, BUFSIZE )) <= 0 )
				{
					printf( "[ERROR] The client has gone.\n" );
					(void) close(fd);
				}
				else
				{
					/* Initialize a thread */
					pthread_t thread;
					int status;
					THREAD_INFO *t_inf;
					pthread_mutex_lock( &threads_mutex );

					if (client_num < MAX_CLIENTS) 
					{	
						t_inf = malloc(sizeof(THREAD_INFO));
						t_inf->ssock = fd;
						t_inf->index = client_num;
						if ( strcmp(buf, "PRODUCE\r\n") == 0 )
							t_inf->type = 1;
						else
							t_inf->type = 2;
						
						client_num ++;
						status = pthread_create( &thread, NULL, handleConnection,  (void *) t_inf );	
					}
					else
						total_excd_max ++;				

					pthread_mutex_unlock( &threads_mutex );

					if (status != 0)
					{
						fprintf( stderr,  "[ERROR] pthread_create error.\n");
						exit(-1);
					}
				}

				/* If the client has closed the connection, stop monitoring the socket */
				FD_CLR( fd, &afds );
				/* lower the max socket number if needed */
				if ( nfds == fd+1 )
					nfds --;

			}

		}

	}

}

/*
**  Differentiate clients 
*/
void *handleConnection( void *tid )
{
	THREAD_INFO *t_inf = tid; 
	int ssock = t_inf->ssock;
	int thread_idx = t_inf->index;
	int type = t_inf->type;
	
	/* Free item */
	free(t_inf);

	if ( type == 1 )
		handleProducer(ssock, thread_idx);
	else if (type == 2) 
		handleConsumer(ssock, thread_idx);
	else 
		fprintf( stderr, "[ERROR] Unrecognized client type.\n");

	pthread_exit( NULL );
}

/*
**  Handle producer client
*/
void handleProducer( int ssock, int thread_idx )
{
	pthread_mutex_lock( &threads_mutex );	

	/* Kill if too much producers */
	if (prod_num >= MAX_PROD)
		closeClient( ssock , 0);

	prod_num ++;
	pthread_mutex_unlock( &threads_mutex );
	fprintf( stderr, "[INFO] [%d] Producer came.\n", thread_idx);

	/* Send GO to producer */
	int cc = write( ssock, GO_STR, strlen(GO_STR) );
	if ( cc <= 0 )
		threadErrorExit(ssock);

	int temp_size;
	uint32_t temp;
	ITEM *item = malloc( sizeof(ITEM *) );

	/* Receive INT */
	cc = read( ssock, &temp_size, 4 );
	if ( cc <= 0 )
		threadErrorExit( ssock );
	temp = ntohl( temp_size );
	item->size = temp;
	item->psd = ssock;
	fprintf( stderr, "[INFO] [%d] Read int %i.\n", thread_idx, temp);

	/* Wait for room in the buffer */
	sem_wait( &empty );
	pthread_mutex_lock( &item_mutex );
	
	/* Put the item in the next slot in the buffer */
	buffer[count_buffer] = item;
	count_buffer ++;
	prod_served ++;
	fprintf( stderr,  "[INFO] [+1] Count buffer value: %d.\n", count_buffer);

	pthread_mutex_unlock( &item_mutex );
	sem_post( &full );
	fprintf( stderr, "[INFO] [%d] Created item.\n", thread_idx);

}

/*
**  Handle consumer client
*/
void handleConsumer( int ssock, int thread_idx )
{
	pthread_mutex_lock( &threads_mutex );

	/* Kill if too much consumers */
	if (cons_num >= MAX_CON)
		closeClient( ssock , 1);

	cons_num ++;
	pthread_mutex_unlock( &threads_mutex );
	fprintf( stderr, "[INFO] [%d] Consumer came.\n", thread_idx);

	/* Wait for items in the buffer */
	sem_wait( &full );
	pthread_mutex_lock( &item_mutex );

	/* Remove the item and update the buffer */
	ITEM *p = buffer[count_buffer-1];
	buffer[count_buffer-1] = NULL;
	count_buffer--;
	cons_served ++;

	fprintf( stderr,  "[INFO] [-1] Count buffer value: %d.\n", count_buffer);
	pthread_mutex_unlock( &item_mutex );
	sem_post( &empty );

	/* Send integer first */
	int cc;
	uint32_t temp = htonl( p->size );
	cc = write( ssock, &temp, 4);
	if ( cc <= 0 )
		threadErrorExit( ssock );
	fprintf( stderr, "[INFO] [%d] Sent %i to consumer.\n", thread_idx, p->size);

	streamData( p->psd, ssock, p->size );
	closeProducerConnection( p->psd );

	free( p );
	close( ssock );	
	pthread_mutex_lock( &threads_mutex );
	cons_num --;
	client_num --;
	pthread_mutex_unlock( &threads_mutex );
	fprintf( stderr, "[INFO] [%d] Consumer peacefully gone.\n", thread_idx);
}

/*
**  Close client
*/
void closeClient( int ssock, int type )
{
	client_num --;
	pthread_mutex_unlock( &threads_mutex );
	if (type == 0)
	{
		fprintf( stderr, "[INFO] Producer killed. Too many producers.\n");
		excd_prod_max ++;
	}
	else
	{
		fprintf( stderr, "[INFO] Consumer killed. Too many consumers.\n");
		excd_cons_max ++;
	}
		
	close( ssock );
	pthread_exit( NULL );
}

/*
** 	Exit on errors
*/
void threadErrorExit( int csock )
{
	close( csock ); 
	pthread_exit( NULL );	
}

/*
**  Consumer closes producer's connection
*/
void closeProducerConnection( int psock )
{
	write( psock, DONE_STR, strlen(DONE_STR) );

	close( psock );
	pthread_mutex_lock( &threads_mutex );
	prod_num --;
	client_num --;
	pthread_mutex_unlock( &threads_mutex );
	fprintf( stderr, "[INFO] [ ] Producer peacefully gone.\n");
}

/*
** Stream
*/
void streamData( int psock, int csock, int size )
{
	/* Send characters in chunks */
	int j, k;
	

	/* Send GO to producer */
	int cc = write( psock, GO_STR, strlen(GO_STR) );
	j = size;

	fprintf( stderr, "[INFO] [ ] Read from producer\n");

	while (j > 0)
	{
		if (j < BUFSIZE)
			k = j;
		else 
			k = BUFSIZE;
		char *str = malloc( k );
		cc = read( psock, str, k );
		cc = write( csock, str, cc );
		j = j - cc;
		free( str );
	}	

	fprintf( stderr, "[INFO] [ ] Wrote to consumer.\n");
	
	return;
}
