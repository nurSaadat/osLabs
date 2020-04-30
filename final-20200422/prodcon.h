#ifndef PRODCON

#define PRODCON

#define QLEN            5
#define BUFSIZE         2048

// The maximum total simultaneous clients allowed
#define MAX_CLIENTS     512

// The maximum simultaneous producer clients allowed
#define MAX_PROD    	480

// The maximum simultaneous consumer clients allowed
#define MAX_CON     	480

// Some clients may wait (seconds) before introducing themseleves
#define SLOW_CLIENT     3

// Slow clients may be rejected if they do not respond in (seconds)
#define REJECT_TIME     2

// Results to be recorded in the txt files by consumers
#define BYTE_ERROR	"ERROR: bytes"
#define SUCCESS		"SUCCESS: bytes"
#define REJECT		"ERROR: REJECTED"

// Each item consists of random number of bytes between 1 and MAX_LETTERS.
#define MAX_LETTERS     10000

// function prototypes
int connectsock( char *host, char *service, char *protocol );
int passivesock( char *service, char *protocol, int qlen, int *rport );

// This item struct will work for all versions
typedef struct item_t
{
	uint32_t	size;
	int			psd;
    char		*letters;
} ITEM;

#endif
