#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_ROW_COL 1024

int **result, **m1, **m2, THREADS, ROWS, COLS;

double getTime();
void *matrixMultiplication( void *tid );

int main(int argc, char* argv[])
{

	struct timeval tv;
	time_t curtime;
	int i, j, k, status, val1, val2;

	if (argc != 5){
		fprintf(stderr, "%s", "[ERROR] Arguments must be in form mm ROWS COLS val1 val2\n");
		exit(-1);
	}

	ROWS = atoi(argv[1]);
	COLS = atoi(argv[2]);
	val1 = atoi(argv[3]);
	val2 = atoi(argv[4]);

	if (ROWS > MAX_ROW_COL || COLS > MAX_ROW_COL){
		fprintf(stderr, "%s", "[ERROR] Maximum number of columns or ROWS handled is 1024\n");
		exit(-1);
	}

	// // generate matrices
	m1 = malloc(ROWS * sizeof(int *));
	for(i = 0; i < ROWS; i++)
		m1[i] = malloc(COLS * sizeof(int));

	for (int i = 0; i < ROWS; i++){
		for (int j = 0; j < COLS; j++){
			m1[i][j] = val1;
		}
	}

	m2 = malloc(COLS * sizeof(int *));
	for(i = 0; i < COLS; i++)
		m2[i] = malloc(ROWS * sizeof(int));

	for (int i = 0; i < COLS; i++){
		for (int j = 0; j < ROWS; j++){
			m2[i][j] = val2;
		}
	}

	// // print result matrix
	// printf("1 matrix:\n");
	// for (i = 0; i < ROWS; i++) {
	// 	for(j = 0; j < COLS; j++) {
	// 		printf(" %d ", m1[i][j]);
	// 	}
	// 	printf("\n");
	// }

	// 	printf("2 matrix:\n");
	// for (i = 0; i < COLS; i++) {
	// 	for(j = 0; j < ROWS; j++) {
	// 		printf(" %d ", m2[i][j]);
	// 	}
	// 	printf("\n");
	// }

	result = malloc(ROWS * sizeof(int *));
	for(i = 0; i < ROWS; i++)
		result[i] = malloc(ROWS * sizeof(int));

	double start_time = getTime();

	// simple sequential algorithm 
	for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < ROWS; ++j) {
            for (int k = 0; k < COLS; ++k) {
                result[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }

 //    printf("Resulting matrix:\n");
	// for (i = 0; i < ROWS; i++) {
	// 	for(j = 0; j < ROWS; j++) {
	// 		printf(" %d ", result[i][j]);
	// 	}
	// 	printf("\n");
	// }

	fprintf(stderr, "%f\n", getTime() - start_time);

	// reset result matrix
	for (int j = 0; j < ROWS; ++j) {
        for (int k = 0; k < ROWS; ++k) {
            result[j][k] = 0;
        }
    }

    // treads implementation
    THREADS = ROWS;
    pthread_t threads[THREADS];
	int index[THREADS];
	start_time = getTime();

	for (i = 0; i < THREADS; i++) {
		index[i] = i;
		status = pthread_create(&threads[i], NULL,
									matrixMultiplication, (void *) index[i]);
		if (status != 0)
			exit(-1);
	}

	for ( i = 0; i < THREADS; i++ )	{
		pthread_join( threads[i], NULL );
	}

	fprintf(stderr, "%f\n", getTime() - start_time);

	// print result matrix
	for ( i = 0; i < ROWS; i++ )
	{
	     for ( j = 0; j < ROWS; j++ )
	          printf( " %d ", result[i][j] );
	     printf( "\n" );
	}

	free(result);
	free(m1);
	free(m2);
	pthread_exit(0);

}

void *matrixMultiplication( void *tid )
{
	int id = (int *) tid;
	int j, k;

    for (int j = 0; j < ROWS; ++j) {
        for (int k = 0; k < COLS; ++k) {
            result[id][j] += m1[id][k] * m2[k][j];
        }
    }
}

double getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time_res = tv.tv_sec + (1.0/1000000) * tv.tv_usec;
    return time_res;
}

