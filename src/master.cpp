#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

int *reducers, mappers, in_file_size;
char in_file[255], out_file[255];

int debug;

static void number_as_chars(int num, char *dest) {
    int i = 0, aux, div = 1;
    aux = num;
    while (aux > 9) {
        div *= 10;
        aux /= 10;
    }
    aux = num;
    while (div >= 1) {
        dest[i] = (aux / div) % 10 + '0';
        i++;
        div /= 10;
    }
    dest[i] = '\0';
}

// Reads the configuration file and initiates data
static int init()
{
    int i;

    //Open the configuration file
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config == NULL) {
        printf("Error on opening config file\n");
        return 1;
    }

    // Save the number of mappers and reducers
    fscanf(config, "%d", &mappers);
    reducers = (int *)malloc(mappers * sizeof(int));
    if (reducers == NULL) {
        printf("Error on alocating reducers\n");
        return 1;
    }
    for (i = 0; i < mappers; i++)
        fscanf(config,"%d", &reducers[i]);

    // Save the input and output file names
    fscanf(config, "%s", in_file);
    fscanf(config, "%s", out_file);
    fclose(config);

    // Save the size of the input file
    FILE *fp = fopen(in_file, "r");
    fseek(fp, 0L, SEEK_END);
    in_file_size = ftell(fp);
    printf("File size is %d\n", in_file_size);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return 0;
}

// Execute the mapper code
static int execute_mapper(MPI_Comm parentcomm, int argc, char **argv)
{
    int rank, size, block_size, *errcodes, my_reducers;
    int i, fair_work, true_work;
    MPI_File mapper_file;
    char *buff;
    MPI_Status status;
    MPI_Datatype arraytype;
    MPI_Offset disp;
    MPI_Comm reducercomm;
    char **args = (char **)malloc(3 * sizeof(char *));
    char reducers_arg[10], block_size_arg[10];

    // Save the arguments (input file, number of mappers, size of input file)
    strcpy(in_file, argv[1]);
    mappers = atoi(argv[2]);
    in_file_size = atoi(argv[3]);

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);

    // Calculate how much this mapper has to process
    block_size = in_file_size / mappers;
    disp = rank * sizeof(char) * block_size;
    // Add some chars for the last mapper, if needed
    if (rank == (size - 1) && (block_size % mappers))
        block_size = block_size + in_file_size % mappers;
    buff = (char *)malloc(block_size * sizeof(char));
    if (!buff) {
        printf("Error on allocating reader buffer\n");
        return 1;
    }
    printf("Mapper / size %d %d, gets b_size %d\n", rank, size, block_size);

    // Open input file and set the view
    MPI_File_open(MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &mapper_file);
    MPI_Type_contiguous(block_size, MPI_CHAR, &arraytype);
    MPI_Type_commit(&arraytype);
    MPI_File_set_view(mapper_file, disp, MPI_CHAR, arraytype, "native", MPI_INFO_NULL);

    // Read the designated part of the input file
    MPI_File_read(mapper_file, buff, block_size, MPI_CHAR, &status);
    MPI_File_close(&mapper_file);

    // Get the number of reducers to create
    MPI_Recv(&my_reducers, 1, MPI_INT, 0, 1, parentcomm, &status); 
    errcodes = (int *)malloc(my_reducers * sizeof(int));
    
    
    // Build reducer arguments
    // 0. The size of the buffer received by this mapper
    // 1. The total number of reducers for this mapper
    number_as_chars(block_size, block_size_arg);
    number_as_chars(my_reducers, reducers_arg);
    args[0] = &block_size_arg[0];
    args[1] = &reducers_arg[0];
    args[2] = NULL;

    // Create the reducers
    printf("Mapper %d before spawning, reducers: %d\n", rank, my_reducers);
    MPI_Comm_spawn( "./reducer", args, my_reducers, MPI_INFO_NULL, 0, MPI_COMM_SELF, &reducercomm, errcodes);

    // Send data to the reducers (consecutive chunks of the buffer)
    fair_work = block_size / my_reducers;
    for (i = 0; i < my_reducers; i++) {
        true_work = fair_work;
        if (i == (my_reducers - 1))
            true_work += block_size % my_reducers;
        printf("Mapper %d to %d work %d\n", rank, i, true_work);
        MPI_Send(&buff[i * fair_work], true_work, MPI_CHAR, i, 1, reducercomm);
    }

    // Wait for data from the reducers
    for (i = 0; i < my_reducers; i++) {
        MPI_Recv(&debug, 1, MPI_INT, i, 1, reducercomm, &status);
        printf("Mapper %d received STOP from %d\n", rank, i);
    }

    // Send data to the master process
    printf("Mapper %d sends STOP\n", rank);
    MPI_Send(&debug, 1, MPI_INT, 0, 1, parentcomm);

    printf("Mapper %d / %d process dies\n", rank, size);
    return 0;
}

// Executes the master process code
static int execute_master()
{
    MPI_Comm intercomm;
    MPI_Status status;
    int rank, size, i;
    char bytes_arg[100], mappers_arg[2];

    // Read the configuration file and initiate data
    if (init())
        return 1;

    int *errcodes = (int *)malloc(mappers * sizeof(int));
    if (errcodes == NULL) {
        printf("Error on alocating err codes\n");
        return 1;
    }
    printf("P creates %d mappers\n", mappers);

    //Some data (input file, mappers) will be sent as arguments
    number_as_chars(mappers, mappers_arg);
    number_as_chars(in_file_size, &bytes_arg[0]);
    char **args = (char **) malloc(5 * sizeof(char *));
    args[0] = in_file;
    args[1] = mappers_arg;
    args[2] = bytes_arg;
    args[3] = NULL;

    // Create the mappers processes
    MPI_Comm_spawn( "./master", args, mappers, MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm, errcodes );

    MPI_Comm_rank(intercomm, &rank);
    MPI_Comm_size(intercomm, &size);
    printf("P rank si size in intercom %d %d\n", rank, size);

    // Send the number of reducers for each mapper
    for (i = 0; i < mappers; i++)
        MPI_Send(&reducers[i], 1, MPI_INT, i, 1, intercomm);

    // Wait for data from the mappers
    for (i = 0; i < mappers; i++) {
        MPI_Recv(&debug, 1, MPI_INT, i, 1, intercomm, &status);
        printf("Process received STOP from mapper %d\n", i);
    }
    printf("THE MASTER %d / %d process dies\n", rank, size);
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parentcomm);

    // Distinguish the master procces and the mappers
    if (parentcomm == MPI_COMM_NULL)
        //Execute master code
        execute_master();
    else
        // Execute mapper code
        execute_mapper(parentcomm, argc, argv);

    MPI_Finalize();
    return 0;
}
