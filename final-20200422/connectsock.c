/* connectsock.c - connectsock */
/* Based on Stevens */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include <netinet/in.h>

#include <netdb.h>

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */


/*------------------------------------------------------------------------
 * connectsock - allocate & connect a socket using TCP or UDP
 *------------------------------------------------------------------------
 */
int
connectsock( 
char	*host,		/* name of host to which connection is desired	*/
char	*service,	/* service associated with the desired port	*/
char	*protocol ) 	/* name of protocol to use ("tcp" or "udp")	*/
{
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/


	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

    /* Map service name to port number */
	if ( pse = getservbyname(service, protocol) )
		sin.sin_port = pse->s_port;
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
	{
		fprintf( stderr, "can't get \"%s\" service entry\n", service);
		exit(-1);
	}


    /* Map host name to IP address, allowing for dotted decimal */
	if ( phe = gethostbyname(host) )
		memcpy(phe->h_addr, (char *)&sin.sin_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
	{
		fprintf( stderr, "can't get \"%s\" host entry\n", host);
		exit(-1);
	}

    /* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(protocol)) == 0)
	{
		fprintf( stderr, "can't get \"%s\" protocol entry\n", protocol);
		exit(-1);
	}

    /* Use protocol to choose a socket type */
	if (strcmp(protocol, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

    /* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0)
	{
		fprintf( stderr, "can't create socket: %s\n", strerror(errno));
		exit(-1);
	}

    /* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		fprintf( stderr, "can't connect to %s.%s: %s\n", host, service, strerror(errno));
		exit(-1);
	}

	return s;
}
