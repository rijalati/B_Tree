#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "b_tree.h"

void generate_random_name(char *name_buf, char **fnames, char **lnames, int line_count, int ksize);

/* ==================*/
/* BEGINNING OF MAIN */
/* ==================*/

int main(int argc, char **argv){
    int i;                  /* Arbitrary iteration variable */ 
    int line_count_nf;      /* Number of lines in name file */ 
    int line_count_if;      /* Number of lines in insertion file */
    int name_index;         /* Index into fnames and lnames */
    int insertion_index;    /* Index into names and phone numbers */
    int name_found;         /* Boolean: Used to generate random names not in insertion file */
    int ksize;              /* Key size for b_tree file */
    unsigned int key_lba;   /* Logical Block Address for a given key */
    size_t BUFSIZE;         /* 1k */
    char middle_initial[2]; /* Contains some capital letter, followed by a period, e.g. "A." */ 
    char *input_buf;        /* 1k buffer for reading file input */
    char *garbage_buf;      /* Reads INSERTION from insertion file */
    char *first_buf;        /* For reading first name from insertions file */
    char *middle_buf;       /* For reading middle initial and period from insertions file */
    char *last_buf;         /* For reading last name from input file */
    char *name_buf;         /* For reading names from insertions file */
    char *phone_buf;        /* For reading phone number from insertions file */
    char *key_buf;          /* Buffer for b_tree key lookups */
    char **fnames;          /* Array of first names from the name file */
    char **lnames;          /* Array of last names from the name file */
    char **names;           /* Array of names read from input file */
    char **phone_numbers;   /* Array of corresponding phone numbers for names */
    void *b_tree;           /* Handle on b_tree for b_tree_file */
    FILE *fin;              /* File structure for input file */
   
    /* Allocate memory for buffers */
    BUFSIZE = 1024;
    garbage_buf = (char *) malloc(sizeof(char) * 128);
    input_buf = (char *) malloc(sizeof(char) * BUFSIZE);
    first_buf = (char *) malloc(sizeof(char) * 128);
    middle_buf = (char *) malloc(sizeof(char) * 128);
    last_buf = (char *) malloc(sizeof(char) * 128);
    name_buf = (char *) malloc(sizeof(char) * 128);
    phone_buf = (char *) malloc(sizeof(char) * 128);

    if (argc != 4){
        printf("Usage: ./example_tester name_file insertion_file b_tree_file\n");
        exit(1);
    }
    
    /* Open names file for reading */
    fin = fopen(argv[1], "r");
   
    /* Count number of lines in names file */
    line_count_nf = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        line_count_nf++;
    }

    /* Reset file pointer for names file to beginning of file and 
        allocate memory for first and last name arrays */
    lseek(fileno(fin), 0, SEEK_SET);
    fnames = (char **) malloc(sizeof(char *) * line_count_nf);
    lnames = (char **) malloc(sizeof(char *) * line_count_nf);
    
    /* Read in first and last names from names file and store in 
        first and last name arrays */
    name_index = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        sscanf(input_buf, "%s %s", first_buf, last_buf);
        fnames[name_index] = strdup(first_buf);
        lnames[name_index] = strdup(last_buf);
        name_index++;
    }

    /* Close names file */
    close(fin);

    /* Open insertion file for reading */
    fin = fopen(argv[2], "r");

    /* Count number of lines in insertion file */
    line_count_if = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        line_count_if++;
    }

    /* Reset file pointer for insertion file to beginning of file and 
        allocate memory for names and phone_numbers arrays */
    lseek(fileno(fin), 0, SEEK_SET);
    names = (char **) malloc(sizeof(char *) * line_count_if);
    phone_numbers = (char **) malloc(sizeof(char *) * line_count_if);

    /* Read in names and phone numbers from insertion file and store
        in names and phone numbers arrays */
    insertion_index = 0;
    while (getline(&input_buf, &BUFSIZE, fin) > 0){
        sscanf(input_buf, "%s %s %s %s %s", garbage_buf, first_buf,
            middle_buf, last_buf, phone_buf);
        
        /* Form name */
        strcpy(name_buf, first_buf);
        strcat(name_buf, " ");
        strcat(name_buf, middle_buf);
        strcat(name_buf, " "); 
        strcat(name_buf, last_buf);

        /* Copy name and phone number to arrays */
        names[insertion_index] = strdup(name_buf);
        phone_numbers[insertion_index] = strdup(phone_buf);
        insertion_index++;
    }
    
    /* Seed random number generator, attach to b_tree file and 
        allocate key buffer */
    srand(time(NULL));
    b_tree = b_tree_attach(argv[3]);
    ksize = b_tree_key_size(b_tree);
    key_buf = (char *) malloc(sizeof(char) * ksize);

    name_index = 0;
    while (name_index < line_count_if){
        if (rand() % 2 == 0){
            /* Generate random phone number (That is not in insertion file) */ 
            do{
                name_found = 0;
                generate_random_name((char *) key_buf, fnames, lnames, line_count_nf, ksize);
                for (i = 0; i < line_count_if; i++){
                    if (strcmp(key_buf, names[i]) == 0){
                        name_found = 1;
                        break;
                    }
                }
            } while (name_found);

            /* Perform lookup of name_buf in b_tree file */
            key_lba = b_tree_find(b_tree, key_buf);

            printf("FIND %s (Should not be in b_tree) \n", key_buf);
            printf("Result --- Return LBA: %d\n\n", key_lba);
        } else{
            /* Pick name and phone_number from data in insertions file */
            for (i = 0; i < ksize; i++){
                ((char *) key_buf)[i] = '\0';
            } 

            strcpy(key_buf, names[name_index]);

            /* Perform lookup of name_buf in b_tree file */
            key_lba = b_tree_find(b_tree, key_buf);
            printf("FIND %s (Should be in b_tree, index: %d)\n", key_buf, name_index);
            printf("Result --- Return LBA: %d\n\n", key_lba);
            name_index++;
        }
    }
    return 0;
}

/* ============================================================ */
/* This procedure generates a random name by randomly selecting */
/* a first name, middle initial, and last name. The first       */
/* and last name are chosen from the names in the name file.    */
/* It takes the buffer for holding the randomly generated       */
/* name, first and last name arrays as arguments, and the line  */
/* count for the name file. The procedure then fills in         */
/* name_buf with the randomly generated phone number.           */
/* ============================================================ */

void generate_random_name(char *name_buf, char **fnames, char **lnames, int line_count, int ksize){
    int i;                      /* Arbitrary iteration variable */
    char middle_initial[3];     /* For forming random middle initials, e.g. "A." */
    
    for (i = 0; i < ksize; i++){
        name_buf[i] = '\0';
    }

    strcpy(name_buf, fnames[rand() % line_count]);
    strcat(name_buf, " ");
    middle_initial[0] = 65 + (rand() % 26);
    middle_initial[1] = '.';
    middle_initial[2] = '\0';
    strcat(name_buf, middle_initial);
    strcat(name_buf, " ");
    strcat(name_buf, lnames[rand() % line_count]);

    return;
}

