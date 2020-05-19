#include <arpa/inet.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char	*service;		
char	*host = "localhost";
char    *command;
int	    csock;


void identify();

int 
main( int argc, char *argv[] )
{
    /* Collect arguments */
	switch( argc ) 
	{
		case    3:
			service = argv[1];
			command = argv[2];
			break;
		case    4:
			host = argv[1];
			service = argv[2];
			command = argv[3];
			break;
		default:
			fprintf( stderr, "[WARNING] usage: status [host] port command\n List of possible commands:\n" );
            fprintf( stderr, "• CURRCLI  – total clients currently being served \n• CURRPROD – clients who have identified themselves as producers and are currently being served\n• CURRCONS – clients who have identified themselves as consumers and are currently being served\n• TOTPROD  – who reached the point of placing their item in the item buffer\n• TOTCONS  – who reached the point of removing an item from the item buffer\n• REJMAX   – total clients rejected for exceeding client max\n• REJSLOW  – total clients rejected for taking too long to identify themselves\n• REJPROD  – total producers rejected for exceeding producer max\n• REJCONS  – total consumers rejected for exceeding producer max\n" );
			exit(-1);
	}
    
    /* Open socket */
    csock = connectsock( host, service, "tcp" );
	if ( csock == 0 )
	{
		fprintf( stderr, "[ERROR] Cannot connect to server.\n" );
		close( csock ); 
	}

    /* Send message to server */
    identify();

    /* Receive the reply */
    int cc;
    char buf[BUFSIZE];
    cc = read( csock, buf, BUFSIZE );
    // if (cc <= 0)
	// {
	// 	fprintf( stderr, "[ERROR] Couldn't receive the message from server." );
	// 	close( csock ); 
	// }

    /* Print the reply */
    fprintf( stderr, "[INFO] Server response: %s", buf);
    close( csock );
        
}

/*
    • Current clients – total clients currently being served
    • Current producers – clients who have identified themselves as producers and are currently being served
    • Current producers – clients who have identified themselves as consumers and are currently being served
    • Total producers served – who reached the point of placing their item in the item buffer
    • Total consumers served – who reached the point of removing an item from the item buffer
    • Total clients rejected for exceeding client max
    • Total clients rejected for taking too long to identify themselves
    • Total producers rejected for exceeding producer max
*/

/*
{current clients in server}\r\n
{current producers in server}\r\n
{current consumers in server}\r\n
{total producers served}\r\n
{total consumers served}\r\n
{total clients rejected for max}\r\n
{total clients rejected for slow}\r\n
{total producers rejected for max}\r\n
{total consumers rejected for max}\r\n

*/

void identify()
{
    int cc;

    if ( !strcmp( command, "STATUS/CURRCLI\r\n" ) || !strcmp( command, "STATUS/CURRCLI" ) || !strcmp( command, "CURRCLI" ) )
    {
        /* Write to server */
	    cc = write( csock, "STATUS/CURRCLI\r\n", strlen("STATUS/CURRCLI\r\n") );
    }
    else if ( !strcmp( command, "STATUS/CURRPROD\r\n" ) || !strcmp( command, "STATUS/CURRPROD" ) || !strcmp( command, "CURRPROD" ) )
    {
	    cc = write( csock, "STATUS/CURRPROD\r\n", strlen("STATUS/CURRPROD\r\n") );
    }
    else if ( !strcmp( command, "STATUS/CURRCONS\r\n" ) || !strcmp( command, "STATUS/CURRCONS" ) || !strcmp( command, "CURRCONS" ) )
    {
        cc = write( csock, "STATUS/CURRCONS\r\n", strlen("STATUS/CURRCONS\r\n") );
    }
    else if ( !strcmp( command, "STATUS/TOTPROD\r\n" ) || !strcmp( command, "STATUS/TOTPROD" ) || !strcmp( command, "TOTPROD" ) )
    {
        cc = write( csock, "STATUS/TOTPROD\r\n", strlen("STATUS/TOTPROD\r\n") );
    }
    else if ( !strcmp( command, "STATUS/TOTCONS\r\n" ) || !strcmp( command, "STATUS/TOTCONS" ) || !strcmp( command, "TOTCONS" ) )
    {
        cc = write( csock, "STATUS/TOTCONS\r\n", strlen("STATUS/TOTCONS\r\n") );
    }
    else if ( !strcmp( command, "STATUS/REJMAX\r\n" ) || !strcmp( command, "STATUS/REJMAX" ) || !strcmp( command, "REJMAX" ) )
    {
        cc = write( csock, "STATUS/REJMAX\r\n", strlen("STATUS/REJMAX\r\n") );
    }
    else if ( !strcmp( command, "STATUS/REJSLOW\r\n" ) || !strcmp( command, "STATUS/REJSLOW" ) || !strcmp( command, "REJSLOW" ) )
    {
        cc = write( csock, "STATUS/REJSLOW\r\n", strlen("STATUS/REJSLOW\r\n") );
    }
    else if ( !strcmp( command, "STATUS/REJPROD\r\n" ) || !strcmp( command, "STATUS/REJPROD" ) || !strcmp( command, "REJPROD" ) )
    {
        cc = write( csock, "STATUS/REJPROD\r\n", strlen("STATUS/REJPROD\r\n") );
    }
    else if ( !strcmp( command, "STATUS/REJCONS\r\n" ) || !strcmp( command, "STATUS/REJCONS" ) || !strcmp( command, "REJCONS" ) )
    {
        cc = write( csock, "STATUS/REJCONS\r\n", strlen("STATUS/REJCONS\r\n") );
    }
    else
    {
        fprintf(stderr, "[ERROR] Command is not supported.\n");
        close( csock ); 
        exit(-1);
    }

    if (cc <= 0)
	{
		fprintf( stderr, "[ERROR] Couldn't send message to server." );
		close( csock ); 
	}
    
}