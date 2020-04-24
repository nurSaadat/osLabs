#ifndef PRODCON

#define PRODCON

#define QLEN            5
#define BUFSIZE         1024
#define MAX_CLIENTS     512
#define MAX_PROD        480
#define MAX_CON         480
#define SLOW_CLIENT     2

// Each item has a random-sized letters buffer between 1 and 1 billion.
#define MAX_LETTERS     10000

int connectsock( char *host, char *service, char *protocol );
int passivesock( char *service, char *protocol, int qlen, int *rport );

typedef struct item_t
{
	uint32_t	size;
	int		prod_sd;
} ITEM;

#endif