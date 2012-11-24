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

#define CHAR_DELIMITERS     " \n`~!@#$%^&*()-_=+[{]}|;:',<.>/?\"\\"
#define WORD_MAX_SIZE       50
#define CONFIG_FILE         "/home/claudiu/Desktop/2tema/config/config"

struct map_entry {
    int freq;
    char word[WORD_MAX_SIZE];
};
