#include "common.h"

int reducers, mappers, in_file_size;
char in_file[255], out_file[255];

// Puts a number in a char *
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
    //Open the configuration file
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config == NULL) {
        printf("Error on opening config file\n");
        return 1;
    }

    // Save the number of mappers and reducers
    fscanf(config, "%d", &mappers);
    fscanf(config, "%d", &reducers);

    // Save the input and output file names
    fscanf(config, "%s", in_file);
    fscanf(config, "%s", out_file);
    fclose(config);

    // Save the size of the input file
    FILE *fp = fopen(in_file, "r");
    fseek(fp, 0L, SEEK_END);
    in_file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return 0;
}

// Execute the mapper code
static int execute_mapper(MPI_Comm parentcomm, int argc, char **argv)
{
    int rank, size, block_size, *errcodes, my_reducers, diff;
    int i, fair_work, true_work, map_size, offset = 0, *reducer_off;
    char *buff;
    struct map_entry final_map[MAXIMUM_SIZE];
    MPI_File mapper_file;
    MPI_Status status;
    MPI_Datatype arraytype;
    MPI_Offset disp;
    MPI_Comm reducercomm;

    // Save the arguments (input file, number of mappers,
    // size of input file, number of reducers)
    strcpy(in_file, argv[1]);
    mappers = atoi(argv[2]);
    in_file_size = atoi(argv[3]);
    my_reducers = atoi(argv[4]);
    errcodes = (int *)malloc(my_reducers * sizeof(int));
    reducer_off = (int *)malloc(my_reducers * sizeof(int));

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);

    // Calculate how much this mapper has to process
    block_size = in_file_size / mappers;
    disp = rank * sizeof(char) * block_size;

    // Add some chars for the last mapper, if needed
    if (rank == (size - 1) && (block_size % mappers))
        block_size = block_size + in_file_size % mappers;

    // Create the buffer containgin the chunk of data
    buff = (char *)malloc(block_size * sizeof(char));
    if (!buff) {
        printf("Error on allocating reader buffer\n");
        return 1;
    }

    // Open input file and set the view
    MPI_File_open(MPI_COMM_WORLD, in_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &mapper_file);
    MPI_Type_contiguous(block_size, MPI_CHAR, &arraytype);
    MPI_Type_commit(&arraytype);
    MPI_File_set_view(mapper_file, disp, MPI_CHAR, arraytype, "native", MPI_INFO_NULL);

    // Read the designated part of the input file
    MPI_File_read(mapper_file, buff, block_size, MPI_CHAR, &status);
    MPI_File_close(&mapper_file);

    // Handle the split area between a mapper's reducers
    fair_work = block_size / my_reducers;
    offset = diff = 0;
    for (i = 0; i < my_reducers; i++) {
        true_work = fair_work;
        if (i == (my_reducers - 1))
            true_work += block_size % my_reducers;
        // Subtract the zone processed by the previous reducer
        true_work -= diff;
        diff = 0;

        // Continue to right until space
        while(!strchr(CHAR_DELIMITERS, buff[offset + fair_work + diff - 1]) 
                && (i + 1 != my_reducers))
            diff++;
        reducer_off[i] = true_work + diff;
        offset += fair_work;
    }

    // Create the reducers
    MPI_Comm_spawn( "./reducer", MPI_ARGV_NULL, my_reducers, MPI_INFO_NULL, 0, MPI_COMM_SELF, &reducercomm, errcodes);

    // Send data to the reducers (consecutive chunks of the buffer)
    offset = 0;
    for (i = 0; i < my_reducers; i++) {
        MPI_Send(&reducer_off[i], 1, MPI_INT, i, 1, reducercomm);

        MPI_Send(&buff[offset], reducer_off[i], MPI_CHAR, i, 1, reducercomm);
        offset += reducer_off[i];
    }

    // Handle split area between mappers
    struct map_entry first_word, second_word;
    first_word.word[0] = '\0';
    second_word.word[0] = '\0';
    i = 0;
    //Save the first word
    while (!strchr(CHAR_DELIMITERS, buff[i]))
        i++;
    if (i > 0) {
        strncpy(first_word.word, buff, i);
        first_word.word[i] = '\0';
    }
    //Save the last word
    i = strlen(buff) - 1;
    while (!strchr(CHAR_DELIMITERS, buff[i]))
        i--;
    if (i < (strlen(buff) - 1)) {
        strcpy(second_word.word, &buff[i + 1]);
    }

    // Create a datatype to receive the hashmap
    MPI_Type_extent(MPI_INT, &ext);
    disper[0] = 0;
    disper[1] = ext;
    MPI_Type_struct(2, blocklen, disper, types, &mapType);
    MPI_Type_commit(&mapType);

    // Wait for data from the reducers
    offset = 0;
    for (i = 0; i < my_reducers; i++) {
        // Get the size of the map from the current reducer
        MPI_Recv(&map_size, 1, MPI_INT, i, 1, reducercomm, &status);

        // Get the hashmap of the current reducer
        MPI_Recv(&final_map[offset], map_size, mapType, i, 1, reducercomm, &status);
        offset += map_size;
    }
    
    // Send the mapper's hashmap to the master process
    map_size = offset;
    MPI_Send(&map_size, 1, MPI_INT, 0, 1, parentcomm);
    MPI_Send(final_map, map_size, mapType, 0, 1, parentcomm);

    // Send the marginal words to the master
    MPI_Send(&first_word, 1, mapType, 0, 1, parentcomm);
    MPI_Send(&second_word, 1, mapType, 0, 1, parentcomm);

    return 0;
}

// Converts a string to lower case
static void to_lower_case(const char *str, char *dest)
{
    int i = 0;
    while(str[i]) {
        dest[i] = tolower(str[i]);
        i++;
    }
    dest[i] = '\0';
}

// Sorts the hashmap and prints it to the output file
static void format_and_print(map<string, int, cmp> table)
{
    unsigned int i;
    char dest[WORD_MAX_SIZE];
    std::vector<mypair> myvec(table.begin(), table.end());

    // Sort using a comparation function
    std::sort(myvec.begin(), myvec.end(), IntCmp());

    // Open output file and write the results
    FILE *fout = fopen(out_file, "w");
    if (!fout) {
        printf("Error opening output file\n");
        return;
    }
    for (i = 0; i < myvec.size(); i++) {
        to_lower_case(myvec[i].first.c_str(), dest);
        fprintf(fout, "%s\t%d\n", dest, myvec[i].second);
    }
    fclose(fout);
}

// Executes the master process code
static int execute_master()
{
    MPI_Comm intercomm;
    MPI_Status status;
    int rank, size, i, j, map_size, offset = 0;;
    char bytes_arg[100], mappers_arg[2], reducers_arg[2];
    struct map_entry final_map[MAXIMUM_SIZE], curr_first_word,
                     curr_second_word, second_word;
    map<string, int, cmp> stringCounts;

    // Read the configuration file and initiate data
    if (init())
        return 1;

    int *errcodes = (int *)malloc(mappers * sizeof(int));
    if (errcodes == NULL) {
        printf("Error on alocating err codes\n");
        return 1;
    }

    //Some data (input file, mappers) will be sent as arguments
    number_as_chars(mappers, mappers_arg);
    number_as_chars(in_file_size, &bytes_arg[0]);
    number_as_chars(reducers, reducers_arg);
    char **args = (char **) malloc(6 * sizeof(char *));
    args[0] = in_file;
    args[1] = mappers_arg;
    args[2] = bytes_arg;
    args[3] = reducers_arg;
    args[4] = NULL;

    // Create the mappers processes
    MPI_Comm_spawn( "./master", args, mappers, MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm, errcodes );

    MPI_Comm_rank(intercomm, &rank);
    MPI_Comm_size(intercomm, &size);

    // Create the new datatype to receive the hashmap
    MPI_Type_extent(MPI_INT, &ext);
    disper[0] = 0;
    disper[1] = ext;
    MPI_Type_struct(2, blocklen, disper, types, &mapType);
    MPI_Type_commit(&mapType);

    // Wait for data from the mappers
    for (i = 0; i < mappers; i++) {
        MPI_Recv(&map_size, 1, MPI_INT, i, 1, intercomm, &status);

        // Get the hashmap of the current mapper
        MPI_Recv(&final_map[offset], map_size, mapType, i, 1, intercomm, &status);
        offset += map_size;
    }

    // Create the final map
    for (j = 0; j < offset; j++)
        stringCounts[final_map[j].word] += final_map[j].freq;

    // Get the marginal words from the mappers
    for (i = 0; i < mappers; i++) {
        MPI_Recv(&curr_first_word, 1, mapType, i, 1, intercomm, &status);
        MPI_Recv(&curr_second_word, 1, mapType, i, 1, intercomm, &status);

        if (i == 0) {
            second_word = curr_second_word;
            continue;
        }
        // Remove the word parts from the hash and add the good word
        if (second_word.word[0] != '\0' && curr_first_word.word[0] != '\0') {
            if (stringCounts[second_word.word] > 1)
                stringCounts[second_word.word] -= 1;
            else
                stringCounts.erase(second_word.word);

            if (stringCounts[curr_first_word.word] > 1)
                stringCounts[curr_first_word.word] -= 1;
            else
                stringCounts.erase(curr_first_word.word);
            strcat(second_word.word, curr_first_word.word);
            stringCounts[second_word.word] += 1;
        }
        second_word = curr_second_word;
    }

    format_and_print(stringCounts);

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
