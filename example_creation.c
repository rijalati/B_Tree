#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *generate_phone_number();

int main(int argc, char **argv){
    int i;                      /* Arbitrary iteration variable */
    int line_count;             /* Number of lines in file */
    int name_index;             /* Index into first and last name arrays */
    char *buf;                  /* Buffer for file input */
    char fname[100];            /* Buffer for grabbing first names */
    char lname[100];            /* Buffer for grabbing last names */
    char **first_names;         /* Array that stores first names */
    char **last_names;          /* Array that stores last names */
    size_t BUFSIZE;             /* Size of input buffer (*buf) */
    FILE *fin;                  /* Input file structure */
    
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
    
    srand(time(NULL));
    
    for (i = 0; i < line_count; i++){
        printf("INSERT %s %c. %s %s\n", first_names[rand() % line_count],
            65 + (rand() % 26), last_names[rand() % line_count], 
            generate_phone_number());
    }

    return 0;
}

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

