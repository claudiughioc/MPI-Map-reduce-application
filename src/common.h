#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <string>
#include <iostream>
#include <map>
using namespace std;

#define CHAR_DELIMITERS     " \n`\t~!\r@#$%^&*()-_=+[{]}|;:',<.>/?\"\\"
#define WORD_MAX_SIZE       50
#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

// Structure which describes a map entry
struct map_entry {
    int freq;
    char word[WORD_MAX_SIZE];
};

// Global variables needed to define a new datatype
int blocklen[2] = {1, WORD_MAX_SIZE};
MPI_Datatype mapType;
MPI_Datatype types[] = {MPI_INT, MPI_CHAR};
MPI_Aint disper[2], ext;

struct cmp : public std::binary_function<string, string, bool> {
    bool operator()(const string &lhs, const string &rhs) const {
        return strcasecmp(lhs.c_str(), rhs.c_str()) < 0 ? 1 : 0;
    }
};
