#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <prodcon.h>
#include <arpa/inet.h>

#define GO_STR "GO\r\n"
#define DONE_STR "DONE\r\n"

int count_buffer;
int cons_num, prod_num, client_num;

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

/*
**	This server can serve several clients at a time
*/
int
main( int argc, char *argv[] )
{
	char buf[BUFSIZE] = {'\0'};
	char			*service;
	struct 	sockaddr_in		fsin;
			socklen_t		alen;
	int				msock;
	int				ssock;
	fd_set			rfds;
	fd_set			afds;
	int				fd;
	int				nfds;
	int				cc;
	int				rport = 0;	
	int 			item_buf_size;
	
	cons_num = 0;
	prod_num = 0;
	client_num = 0;

	switch (argc) 
	{
		case	2:
			/* If no port provided - let the OS choose a port and tell the user. */
			/*Initialize buffer*/
			rport = 1;
			item_buf_size = atoi(argv[1]);
			break;
		case	3:
			/* If user provides a port - use it */
			/*Initialize buffer*/
			service = argv[1];
			item_buf_size = atoi(argv[2]);
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

	// Now we begin the set up for using select
	
	// nfds is the largest monitored socket + 1
	// there is only one socket, msock, so nfds is msock +1
	// Set the max file descriptor being monitored
	nfds = msock+1;

	// the variable afds is the fd_set of sockets that we want monitored for
	// a read activity
	// We initialize it to empty
	FD_ZERO(&afds);

	// Then we put msock in the set
	FD_SET( msock, &afds );

	/* mutex for item writing */
	pthread_mutex_init( &item_mutex, NULL );

	/* mutex for thread placement */
	pthread_mutex_init( &threads_mutex, NULL );

	/* semaphores for keeping track of fullness of information buffer */
	sem_init( &full, 0, 0 );
	sem_init( &empty, 0, item_buf_size );

	/* int to keep track of buffer idx */
	count_buffer = 0;

	fprintf( stderr,  "[INFO] Loading server.\n");

	for (;;)
	{
		// Since select overwrites the fd_set that we send it, 
		// we copy afds into another variable, rfds
		// Reset the file descriptors you are interested in
		memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));

		// Only waiting for sockets who are ready to read
		//  - this includes new clients arriving
		//  - this also includes the client closed the socket event
		// We pass null for the write event and exception event fd_sets
		// we pass null for the timer, because we don't want to wake early
		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,
				(struct timeval *)0) < 0)
		{
			fprintf( stderr, "server select: %s\n", strerror(errno) );
			exit(-1);
		}

		// Since we've reached here it means one or more of our sockets has something
		// that is ready to read
		// So now we have to check all the sockets in the rfds set which select uses
		// to return a;; the sockets that are ready

		// If the main socket is ready - it means a new client has arrived
		// It must be checked separately, because the action is different
		if (FD_ISSET( msock, &rfds)) 
		{
			int	ssock;

			// we can call accept with no fear of blocking
			alen = sizeof(fsin);
			ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
			if (ssock < 0)
			{
				fprintf( stderr, "accept: %s\n", strerror(errno) );
				exit(-1);
			}

			// If a new client arrives, we must add it to our afds set
			FD_SET( ssock, &afds );

			// and increase the maximum, if necessary
			if ( ssock+1 > nfds )
				nfds = ssock+1;
		}

		// Now check all the regular sockets
		for ( fd = 3; fd < nfds; fd++ )
		{
			// check every socket to see if it's in the ready set
			// But don't recheck the main socket
			if (fd != msock && FD_ISSET(fd, &rfds))
			{	
				// you can read without blocking because data is there
				// the OS has confirmed this
				//
				if ( (cc = read( fd, buf, BUFSIZE )) <= 0 )
				{
					printf( "The client has gone.\n" );
					(void) close(fd);
				}
				else
				{
					/* initialize a thread */
					pthread_t thread;
					int status;
					THREAD_INFO *t_inf;
					// fprintf( stderr,  "[INFO] Before thread lock.\n");
					pthread_mutex_lock( &threads_mutex );

					if (client_num < MAX_CLIENTS) 
					{	
						t_inf = malloc(sizeof(THREAD_INFO));
						t_inf->ssock = fd;
						t_inf->index = client_num;
						
						if ( strcmp(buf, "PRODUCE\r\n") == 0 )
						{
							t_inf->type = 1;
						}
						else
						{
							t_inf->type = 2;
						}
						
						client_num ++;
						status = pthread_create( &thread, NULL, handleConnection,  (void *) t_inf );	
					}


					pthread_mutex_unlock( &threads_mutex );
					// fprintf( stderr,  "[INFO] After thread lock.\n");

					if (status != 0)
					{
						fprintf( stderr,  "[ERROR] pthread_create error.\n");
						exit(-1);
					}
				}

				// If the client has closed the connection, we need to
				// stop monitoring the socket (remove from afds)
				FD_CLR( fd, &afds );
				// lower the max socket number if needed
				if ( nfds == fd+1 )
					nfds--;

			}

		}

	}

}

void *handleConnection( void *tid )
{
	THREAD_INFO 		*t_inf = tid; 
	int ssock = t_inf->ssock;
	int thread_idx = t_inf->index;
	int type = t_inf->type;

	/* Free item */
	free(t_inf);

	if ( type == 1 )
	{
		pthread_mutex_lock( &threads_mutex );
		
		if (prod_num >= MAX_PROD)
		{
			client_num --;
			pthread_mutex_unlock( &threads_mutex );
			fprintf( stderr, "[INFO] Producer killed. Too many producers.\n");
			close( ssock );
			pthread_exit( NULL );
		}

		prod_num += 1;
		pthread_mutex_unlock( &threads_mutex );
		
		/* producer */
		fprintf( stderr, "[INFO] [%d] Producer came.\n", thread_idx);
		write( ssock, GO_STR, strlen(GO_STR) );

		int temp_size;
		read( ssock, &temp_size, 4 );
		// fprintf( stderr, "[INFO] [%d] Read int.\n", thread_idx);

		ITEM *item = malloc( sizeof(ITEM *) );

		uint32_t temp = ntohl( temp_size );

		item->size = temp;
		item->letters = malloc(item->size);

		read( ssock, item->letters, item->size);

		/* Print characters */
		// for (i = 0; i < temp; i++)
		// 	fprintf( stderr, "%c", item->letters[i] );

		/* Wait for room in the buffer */
		sem_wait( &empty );
		pthread_mutex_lock( &item_mutex );
		
		/* Put the item in the next slot in the buffer */
		buffer[count_buffer] = item;
		count_buffer++;
		fprintf( stderr,  "[INFO] [+1] C Count %d.\n", count_buffer);

		pthread_mutex_unlock( &item_mutex );
		sem_post( &full );

		// fprintf( stderr, "[INFO] [%d] Created item.\n", thread_idx);

		write( ssock, DONE_STR, strlen(DONE_STR) );
		
		close( ssock );
		pthread_mutex_lock( &threads_mutex );
		prod_num --;
		client_num --;
		pthread_mutex_unlock( &threads_mutex );
		// fprintf( stderr, "[INFO] [%d] End working with.\n", thread_idx);

	}
	else if (type == 2) 
	{
		/* consumer */
		pthread_mutex_lock( &threads_mutex );

		if (cons_num >= MAX_CON)
		{
			client_num --;
			pthread_mutex_unlock( &threads_mutex );
			fprintf( stderr, "[INFO] Consumer killed. Too many consumers.\n");
			close( ssock );
			pthread_exit( NULL );
		}

		cons_num ++;
		pthread_mutex_unlock( &threads_mutex );
		fprintf( stderr, "[INFO] Consumer came.\n");

		/* Wait for items in the buffer */
		sem_wait( &full );
		pthread_mutex_lock( &item_mutex );

		// fprintf( stderr, "[INFO] In mutex.\n");

		/* Remove the item and update the buffer */
		ITEM *p = buffer[count_buffer-1];
		buffer[count_buffer-1] = NULL;
		count_buffer--;

		fprintf( stderr,  "[INFO] [-1] C Count %d.\n", count_buffer );

		pthread_mutex_unlock( &item_mutex );
		sem_post( &empty );

		// fprintf( stderr, "[INFO] [%d] Removed Item.\n", ssock);

		uint32_t temp = htonl( p->size );

		write( ssock, &temp, 4);
		write( ssock, p->letters, p->size );
		// fprintf( stderr, "[INFO] [%d] Sent to consumer.\n", ssock);
		
		free(p->letters);
		free(p);
		close( ssock );
		
		pthread_mutex_lock( &threads_mutex );
		cons_num --;
		client_num --;
		pthread_mutex_unlock( &threads_mutex );
		// fprintf( stderr, "[INFO] [%d] End working with.\n", thread_idx);

	}

	else 
	{
		fprintf( stderr, "[INFO] Unrecognized client type.\n");
	}

	pthread_exit( NULL );
}

