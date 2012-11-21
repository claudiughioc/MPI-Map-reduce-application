#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_MAX_SIZE           20000

/* Implementation of a C Hashtable */

typedef struct list_item {
        char *string;
        struct list_item *next;
}element;

typedef struct hashtable_ {
        int size;
        element **elements;
}hashtable;


/* Create the hashtable with the given size */
hashtable *create_hashtable(int size);



/* Find a word in the hashtable
 * Return 0 or 1 if it exists
 */
int find(char *word, hashtable *table);

/* Add new word to the hashtable
 * if the element already exists return 0
 * if the new word could not be add return -1
 * if the element has been successfuly added return 1
 */
int add(char *word, hashtable *table);

/* Remove a word from the hashtable */
int remove_elem(char *word, hashtable *table);

/* Print one bucket corresponding to the index */
void print_bucket(hashtable *table, int index, FILE *fout);

/* Print the entire hashtable */
void print(hashtable *tablei, char *fname);

/* Resize the hashtable to double if mode is 0
 * or to halve if mode is different than 0
 */
void resize(hashtable *table, int mode);

/* Clears the element of the hashtable */
void clear(hashtable *table);

/* Destroys the hashtable */
void destroy(hashtable *table);
#endif
