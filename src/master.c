#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

int *reducers, mappers, in_file_size;
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
    fclose(config);

    FILE *fp = fopen(in_file, "r");
    fseek(fp, 0L, SEEK_END);
    in_file_size = ftell(fp);
    printf("File size is %d\n", in_file_size);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return 0;
}

// Execute the mapper code
static int execute_mapper(MPI_Comm parentcomm)
{
    int rank, size, block_size;
    MPI_File mapper_file;
    char buff[1000];
    MPI_Status status;
    MPI_Datatype arraytype;
    MPI_Offset disp;

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);
    printf("Mapper in parentcomm %d %d\n", rank, size);

    // Open input file and set the view
    MPI_File_open(MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &mapper_file);

    disp = rank * sizeof(char) * block_size;
    MPI_Type_contiguous(block_size, MPI_CHAR, &arraytype);
    MPI_Type_commit(&arraytype);
    MPI_File_set_view(mapper_file, disp, MPI_CHAR, arraytype, "native", MPI_INFO_NULL);

    MPI_File_read(mapper_file, buff, 1000, MPI_CHAR, &status);
    MPI_File_close(&mapper_file);
    printf("M %d read %s\n", rank, buff);
    return 0;
}

// Executes the master process code
static int execute_master()
{
    MPI_Comm intercomm;
    int rank, size;

    if (init())
        return 1;

    int *errcodes = (int *)malloc(mappers * sizeof(int));
    if (errcodes == NULL) {
        printf("Error on alocating err codes\n");
        return 1;
    }

    printf("P creates %d mappers\n", mappers);

    //TODO send arguments to mapper processes
    //Arguments: in_file, mappers
    //Reducer per mapper will be received by send
    char **args = (char **) malloc(3 * sizeof(char *));
    args[0] = "Hello";
    args[1] = "WORLD";

    // Create the mappers processes
    MPI_Comm_spawn( "./master", args, mappers, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes );

    MPI_Comm_rank(intercomm, &rank);
    MPI_Comm_size(intercomm, &size);
    printf("P rank si size in intercom %d %d\n", rank, size);
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;
    printf("Argc = %d\n", argc);
    int i = 0;
    for (i = 0; i < argc; i++)
        printf("Arg %d: %s ", i, argv[i]);

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
