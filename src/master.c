#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

int *reducers, mappers, in_file_size;
char in_file[255], out_file[255];

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
static int execute_mapper(MPI_Comm parentcomm, int argc, char **argv)
{
    int rank, size, block_size;
    MPI_File mapper_file;
    char *buff;
    MPI_Status status;
    MPI_Datatype arraytype;
    MPI_Offset disp;

    // Save the arguments
    strcpy(in_file, argv[1]);
    mappers = atoi(argv[2]);
    in_file_size = atoi(argv[3]);

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);
    block_size = in_file_size / mappers;
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

    disp = rank * sizeof(char) * block_size;
    MPI_Type_contiguous(block_size, MPI_CHAR, &arraytype);
    MPI_Type_commit(&arraytype);
    MPI_File_set_view(mapper_file, disp, MPI_CHAR, arraytype, "native", MPI_INFO_NULL);

    // Read the designated part of the input file
    MPI_File_read(mapper_file, buff, block_size, MPI_CHAR, &status);
    MPI_File_close(&mapper_file);
    printf("M %d read %s\n", rank, buff);

    // Create the reducers
    return 0;
}

// Executes the master process code
static int execute_master()
{
    MPI_Comm intercomm;
    int rank, size;
    char bytes[100], c[2];

    if (init())
        return 1;

    int *errcodes = (int *)malloc(mappers * sizeof(int));
    if (errcodes == NULL) {
        printf("Error on alocating err codes\n");
        return 1;
    }
    printf("P creates %d mappers\n", mappers);

    //Some data (input file, mappers) will be sent as arguments
    char **args = (char **) malloc(5 * sizeof(char *));
    args[0] = in_file;
    number_as_chars(in_file_size, &bytes[0]);
    number_as_chars(mappers, c);
    args[1] = c;
    args[2] = bytes;



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
