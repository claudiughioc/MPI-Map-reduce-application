#include "hash.h"
#include "hashtable.h"

hashtable *create_hashtable(int size)
{
        int i;
        hashtable *table = (hashtable *)malloc(sizeof(hashtable));

        if (table == NULL)
                return NULL;
        table->size = size;
        table->elements = (element **)malloc(size * sizeof(element *));
        if (table->elements == NULL) {
                free(table);
                return NULL;
        }
        for (i = 0; i < size; i++)
                table->elements[i] = NULL;
        return table;
}

int find(char *word, hashtable *table)
{
        int found = 0;
        element *curr_bucket;
        unsigned int hash_value = hash(word, table->size);

        curr_bucket = table->elements[hash_value];
        for (; curr_bucket != NULL; curr_bucket = curr_bucket->next)
                if (!strcmp(curr_bucket->string, word))
                        found = 1;
        return found;
}

int add(char *word, hashtable *table)
{
        int found;
        unsigned int hash_value;
        element *curr_bucket;
        element *new_elem = (element *)malloc(sizeof(element));

        hash_value = hash(word, table->size);
        found = find(word, table);
        if (found)
                return 0;
        if (new_elem == NULL)
                return -1;
        new_elem->string = (char *)malloc(strlen(word) + 1);
        strncpy(new_elem->string, word, strlen(word));
        new_elem->string[strlen(word)] = '\0';
        new_elem->next = NULL;

        /* Insert the new element at the end of the bucket */
        curr_bucket = table->elements[hash_value];
        if (curr_bucket == NULL)
                table->elements[hash_value] = new_elem; 
        else {
                for (; curr_bucket->next != NULL; 
                        curr_bucket = curr_bucket->next);
                curr_bucket->next = new_elem;
        }
        return 1;
}

int remove_elem(char *word, hashtable *table)
{
        unsigned int hash_value = hash(word, table->size);
        element *aux_bucket;
        element *curr_bucket = table->elements[hash_value];

        if (!strcmp(curr_bucket->string, word)) {
                table->elements[hash_value] = curr_bucket->next;
                free(curr_bucket);
                return 1;
        }
        for (; curr_bucket->next != NULL; curr_bucket = curr_bucket->next)
                if (!strcmp(curr_bucket->next->string, word)) {
                        aux_bucket = curr_bucket->next;
                        curr_bucket->next = aux_bucket->next;
                        free(aux_bucket->string);
                        free(aux_bucket);
                        return 1;
                }
        return 0;
}

void print_bucket(hashtable *table, int index, FILE *fout)
{
        element *bucket = table->elements[index];

        for(; bucket != NULL; bucket = bucket->next)
                if (fout != NULL)
                        fprintf(fout, "%s ", bucket->string);
                else    printf("%s ", bucket->string);
        if (fout != NULL)
                fprintf(fout, "\n");
        else    printf("\n");
}

void print(hashtable *table, char *fname)
{
        int n = table->size, i;
        FILE *fout = NULL;

        if (table == NULL)
                return;
        if (fname != NULL) {
                fout = fopen(fname, "a");
                if (fout == NULL)
                        printf("Eroare la deschiderea fisierului de iesire\n");
        }
        for (i = 0; i < n; i++)
                if (table->elements[i] != NULL)
                        print_bucket(table, i, fout);
        if (fname != NULL)
                fclose(fout);
}

void resize(hashtable *table, int mode)
{
        element *bucket;
        hashtable *new_table, aux;
        int new, i;

        new = table->size;
        if (mode == 0)
                new = 2 * new;
        else    new = new / 2;
        new_table = create_hashtable(new);
        for (i = 0; i < table->size; i++)
                if (table->elements[i] != NULL) {
                        bucket = table->elements[i];
                        for (; bucket != NULL; bucket = bucket->next)
                                add(bucket->string, new_table);;
                }
        aux = *table;
        *table = *new_table;
        *new_table = aux;
        clear(new_table);
}

void clear(hashtable *table)
{
        int i;
        element *curr_bucket, *aux;

        if (table == NULL)
                return;
        for (i = 0; i < table->size; i++) {
                curr_bucket = table->elements[i];
                while (curr_bucket != NULL) {
                        aux = curr_bucket;
                        curr_bucket = aux->next;
                        free(aux->string);
                        free(aux);
                }
                table->elements[i] = NULL;
        }
}
void destroy(hashtable *table)
{
        clear(table);
        free(table->elements);
        free(table);
}

