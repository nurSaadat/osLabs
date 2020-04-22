#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define BUFSIZE		4096

int connectsock( char *host, char *service, char *protocol );

/*
**	Client
*/
int
main( int argc, char *argv[] )
{
	char		buf[BUFSIZE];
	char		*service;		
	char		*host = "localhost";
	int		cc;
	int		csock;
	
	switch( argc ) 
	{
		case    2:
			service = argv[1];
			break;
		case    3:
			host = argv[1];
			service = argv[2];
			break;
		default:
			fprintf( stderr, "usage: chat [host] port\n" );
			exit(-1);
	}

	/*	Create the socket to the controller  */
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	printf( "The server is ready, please start sending to the server.\n" );
	printf( "Type q or Q to quit.\n" );
	fflush( stdout );

	// 	Start the loop
	while ( fgets( buf, BUFSIZE, stdin ) != NULL )
	{
		// If user types 'q' or 'Q', end the connection
		if ( buf[0] == 'q' || buf[0] == 'Q' )
		{
			break;
		}
		// Send to the server
		if ( write( csock, buf, strlen(buf) ) < 0 )
		{
			fprintf( stderr, "client write: %s\n", strerror(errno) );
			exit( -1 );
		}	
		// Read the echo and print it out to the screen
		if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
                {
                	printf( "The server has gone.\n" );
                        close(csock);
                        break;
                }
                else
                {
                        buf[cc] = '\0';
                        printf( "%s\n", buf );
		}
	}
	close( csock );

}


