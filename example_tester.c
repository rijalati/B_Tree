#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "b_tree.h"

int main(int argc, char **argv){
    int line_count;         /* Number of lines in insertion file */ 
    int name_index;         /* Index into fnames and lnames */
    size_t BUFSIZE;         /* 1k */
    char *input_buf;        /* 1k buffer for reading input */
    char *first_buf;        /* For reading first name from input file */
    char *middle_buf;       /* For reading middle initial and period from input file */
    char *last_buf;         /* For reading last name from input file */
    char *phone_buf;        /* For reading phone number from input file */
    char **fnames;          /* Array of first names from the name file */
    char **lnames;          /* Array of last names from the name file */
    char **names;           /* Array of names read from input file */
    char **phone_numbers;   /* Array of corresponding phone numbers for names */
    FILE *fin;              /* File structure for input file */
   
    /* Allocate memory for buffers */
    BUFSIZE = 1024;
    input_buf = (char *) malloc(sizeof(char) * BUFSIZE);
    first_buf = (char *) malloc(sizeof(char) * 128);
    middle_buf = (char *) malloc(sizeof(char) * 128);
    last_buf = (char *) malloc(sizeof(char) * 128);
    phone_buf = (char *) malloc(sizeof(char) * 128);

    /* Make sure executable was called correctly */
    if (argc != 4){
        printf("Usage: ./example_tester name_file insertion_file b_tree_file\n");
        exit(1);
    }
    
    /* Open names file for reading */
    fin = fopen(argv[1], "r");
   
    /* Count number of lines in names file */
    line_count = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        line_count++;
    }

    /* Reset file pointer for names file to beginning of file and 
        allocate memory for first and last name arrays */
    lseek(fileno(fin), 0, SEEK_SET);
    fnames = (char **) malloc(sizeof(char *) * line_count);
    lnames = (char **) malloc(sizeof(char *) * line_count);

    name_index = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        sscanf(input_buf, "%s %s", first_buf, last_buf);
        fnames[name_index] = strdup(first_buf);
        lnames[name_index] = strdup(last_buf);
        name_index++;
    }
        
    return 0;
}
