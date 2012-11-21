#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HASHTABLE_SIZE      300000

int debug;

static void build_hashtable(char *buff, int size, int *table)
{
}

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;
    int rank, size, i, block_size, coworkers;
    int my_size;
    char *buff;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parentcomm);
    if (parentcomm == MPI_COMM_NULL) {
        printf("This reducer should have a parent");
        return 1;
    }

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);
    block_size = atoi(argv[1]);
    coworkers = atoi(argv[2]);

    //Calculate how much I have to process
    my_size = block_size / coworkers;
    if (rank + 1 == size)
        my_size += block_size % coworkers;
    buff = (char *)malloc((my_size + 1) * sizeof(char));

    // Now wait for the data from my mapper...
    MPI_Recv(buff, my_size, MPI_CHAR, 0, 1, parentcomm, &status);
    printf("Reducer %d of %d, received %d\n", rank, size, my_size);

    
    MPI_Finalize();
    printf("Moare reducer %d din %d\n", rank, size);
    return 0;
}
