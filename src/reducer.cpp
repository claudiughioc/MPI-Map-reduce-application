#include "common.h"

// Builds a hashtable which associates a word with its frequency
static void build_hashtable(char *buff, int size, map<string, int, cmp> &table)
{
    char delimiter[] = CHAR_DELIMITERS;
    char *word, *context;

    // Make a copy of the input buffer
    char *inputCopy = (char*) calloc(size + 1, sizeof(char));
    strncpy(inputCopy, buff, size);

    // Parse the string looking for words
    word = strtok_r(inputCopy, delimiter, &context);
    table[word] += 1;
    while(true) {
        word = strtok_r(NULL, delimiter, &context);
        if (word == NULL)
            break;
        table[word] += 1;
    }
}

int main(int argc, char **argv)
{
    MPI_Comm parentcomm;
    int rank, size, i = 0, my_size;
    long map_size;
    char *buff;
    MPI_Status status;
    map<string, int, cmp>::iterator iter;
    map<string, int, cmp> stringCounts;
    struct map_entry *hashmap;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parentcomm);
    if (parentcomm == MPI_COMM_NULL) {
        printf("This reducer should have a parent");
        return 1;
    }

    MPI_Comm_rank(parentcomm, &rank);
    MPI_Comm_size(parentcomm, &size);

    //Calculate how much I have to process
    MPI_Recv(&my_size, 1, MPI_INT, 0, 1, parentcomm, &status);
    buff = (char *)malloc((my_size + 1) * sizeof(char));
    printf("Reducer %d of %d, received size %d\n", rank, size, my_size);

    // Now wait for the data from my mapper...
    MPI_Recv(buff, my_size, MPI_CHAR, 0, 1, parentcomm, &status);
    buff[my_size] = '\0';
    //printf("Reducer %d of %d, received %d\n", rank, size, my_size);

    //Build the hashtable with words and their frequency
    build_hashtable(buff, strlen(buff), stringCounts);
    //printf("Reducer %d of %d, built the HT\n", rank, size);
    map_size = stringCounts.size();
    hashmap = (struct map_entry*)malloc(map_size * sizeof(struct map_entry));
    for (iter = stringCounts.begin(); iter != stringCounts.end(); iter++) {
        strcpy(hashmap[i].word, iter->first.c_str());
        hashmap[i].word[iter->first.size()] = '\0';
        hashmap[i].freq = iter->second;
        i++;
    }

    // Create a new datatype
    MPI_Type_extent(MPI_INT, &ext);
    disper[0] = 0;
    disper[1] = ext;
    MPI_Type_struct(2, blocklen, disper, types, &mapType);
    MPI_Type_commit(&mapType);

    // Send data back to the mapper
    //printf("Reducer %d of %d, sent map_size %d\n", rank, size, map_size);
    MPI_Send(&map_size, 1, MPI_INT, 0, 1, parentcomm);
    MPI_Send(hashmap, map_size, mapType, 0, 1, parentcomm);

    MPI_Finalize();
    //printf("Moare reducer %d din %d\n", rank, size);

    return 0;
}
