#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

int *reducers, mappers;
char in_file[255], out_file[255];

// Reads the configuration file and initiates data
static int init()
{
    int i;
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config == NULL) {
        printf("Error on opening config file\n");
        return 1;
    }
    fscanf(config, "%d", &mappers);
    reducers = (int *)malloc(mappers * sizeof(int));
    if (reducers == NULL) {
        printf("Error on alocating reducers\n");
        return 1;
    }
    for (i = 0; i < mappers; i++)
        fscanf(config,"%d", &reducers[i]);

    fscanf(config, "%s", in_file);
    fscanf(config, "%s", out_file);
    return 0;
}

// Execute the mapper code
static int execute_mapper(MPI_Comm parentcomm)
{
    int rank, size;
    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);
    printf("Mapper in parentcomm %d %d\n", rank, size);
    return 0;
}

// Executes the master process code
static int execute_master()
{
    MPI_Comm intercomm;
    int rank, size;

    int *errcodes = (int *)malloc(mappers * sizeof(int));
    if (errcodes == NULL) {
        printf("Error on alocating err codes\n");
        return 1;
    }

    printf("P creates %d mappers\n", mappers);
    // Create the mappers processes
    MPI_Comm_spawn( "./master", MPI_ARGV_NULL, mappers, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes );

    MPI_Comm_rank(intercomm, &rank);
    MPI_Comm_size(intercomm, &size);
    printf("P rank si size in intercom %d %d\n", rank, size);
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;

    if (init())
        return 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parentcomm);

    // Distinguish the master procces and the mappers
    if (parentcomm == MPI_COMM_NULL)
        //Execute master code
        execute_master();
    else
        // Execute mapper code
        execute_mapper(parentcomm);

    MPI_Finalize();
    return 0;
}
