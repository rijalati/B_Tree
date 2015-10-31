#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "b_tree.h"

/* This function generates and returns a random phone number,*/
/*   in the format (555)-xxx-xxxx .                          */
char *generate_phone_number();

/* ========================================================= */
/*                  BEGINNING OF MAIN                        */
/* ========================================================= */

int main(int argc, char **argv){
    int i;                      /* Arbitrary iteration variable */
    int line_count;             /* Number of lines in file */
    int name_index;             /* Index into first and last name arrays */
    char *buf;                  /* Buffer for file input */
    char *key_buf;              /* Buffer for inserting keys into b_tree */
    char *rec_buf;              /* Buffer for inserting records into b_tree */
    char fname[100];            /* Buffer for grabbing first names */
    char lname[100];            /* Buffer for grabbing last names */
    char **first_names;         /* Array that stores first names */
    char **last_names;          /* Array that stores last names */
    void *b_tree;               /* Handle for b_tree */
    size_t BUFSIZE;             /* Size of input buffer (*buf) */
    FILE *fin;                  /* Input file structure */
    FILE *fout;                 /* Output file structure */
    
    /* Error check execution specification */
    if (argc != 4){
        printf("Usage: ./example_creation input_file b_tree_file output_file\n");
        exit(1);
    }

    /* Create 1K buffer for file input */
    BUFSIZE = 1024;
    buf = (char *) malloc(sizeof(char) * BUFSIZE);

    /* Open file for reading, intialize a counter for number of lines */
    fin = fopen(argv[1], "r");
    line_count = 0;

    /* Count number of lines in file */
    while (getline(&buf, &BUFSIZE, fin) > 0){ 
        line_count++;
    }
   
    /* Reset file pointer to beginning of file, allocate memory
        for first and last name arrays */
    lseek(fileno(fin), 0, SEEK_SET);
    first_names = (char **) malloc(sizeof(char **) * line_count);
    last_names = (char **) malloc(sizeof(char **) * line_count);

    /* Read in first and last names from file, store in corresponding
        arrays */
    name_index = 0;
    while (getline(&buf, &BUFSIZE, fin) > 0){
        sscanf(buf, "%s %s", fname, lname);
        first_names[name_index] = strdup(fname);
        last_names[name_index] = strdup(lname);
        name_index++;
    }
   
    /* Seed random number generator */
    srand(time(NULL));

    /* Allocate buffers for keys and records, initialize b_tree and 
        open output file */
    key_buf = (char *) malloc(sizeof(char) * BUFSIZE);
    rec_buf = (char *) malloc(sizeof(char) * BUFSIZE);
    b_tree = b_tree_create(argv[2], (line_count * 2) * BUFSIZE, 50);
    fout = fopen(argv[3], "w");
    
    /* Generate random names and phone numbers, store them in key_buf and
        rec_buf, respectively. Insert into b_tree, also print them to check. */
    for (i = 0; i < line_count; i++){
        /* Create key (format: "firstname middle_intial. lastname") */
        strcpy(key_buf, first_names[rand () % line_count]);
        strcat(key_buf, " ");
        key_buf[strlen(key_buf) + 2] = '\0';
        key_buf[strlen(key_buf) + 1] = '.';
        key_buf[strlen(key_buf)] = 65 + (rand() % 26);
        strcat(key_buf, " ");
        strcat(key_buf, last_names[rand() % line_count]);
        
        /* Create record by generating random phone number */
        strcpy(rec_buf, generate_phone_number());
        
        /* Insert into b_tree and print to output file */
        b_tree_insert(b_tree, key_buf, rec_buf);
        fprintf(fout, "INSERT %s %s\n", key_buf, rec_buf);
    }

    return 0;
}

/* ========================================================= */
/* This function generates and return a random phone number, */
/* in the format "(555)-xxx-xxxx".                           */
/* ========================================================= */
char *generate_phone_number(){
    int i;      /* Arbitrary iteration variable */  
    char *pn;   /* Phone number: (555)-123-4567 */

    pn = strdup("(555)-123-4567");

    for (i = 6; i < 9; i++){
        pn[i] = 48 + (rand() % 10);
    }

    for (i = 10; i < 14; i++){
        pn[i] = 48 + (rand() % 10);
    }

    return pn;
}

