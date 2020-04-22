#ifndef PRODCON

#define PRODCON

#define QLEN            5
#define BUFSIZE         1024
#define MAX_CLIENTS     512

int connectsock( char *host, char *service, char *protocol );
int passivesock( char *service, char *protocol, int qlen, int *rport );

typedef struct item_t
{
        int size;
        char *letters;
} ITEM;

#endif
