#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;
    int rank, size, i, block_size, coworkers;
    int my_size, source;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parentcomm);
    if (parentcomm == MPI_COMM_NULL) {
        printf("This reducer should have a parent");
        return 1;
    }

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);

    printf("Reducer %d of %d\n", rank, size);
    block_size = atoi(argv[1]);
    coworkers = atoi(argv[2]);

    //Calculate how much I have to process
    my_size = block_size / coworkers;
    if (rank + 1 == size)
        my_size += block_size % coworkers;

    // Now wait for the data from my mapper...

    MPI_Finalize();
    return 0;
}
