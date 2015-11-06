**Note**: The code in b_tree.h, jdisk.h, and jdisk.c was written by Dr. James Plank of the 
University of Tennessee. The code in b_tree.c, example_creation.c, and example_tester.c is my 
own. 

This repository contains the source code for a simple B+ Tree that is implemented on top of 
a Unix/Linux filesystem and is capable of functioning as a simple Database Management System. 
This was a project I did my senior year of college for Dr. Plank's Advanced Programming and 
Algorithms course at UTK. 

The functions declared in b_tree.h define an interface for a B+ Tree ("b_tree") that is 
implemented on top of a Unix/Linux filesystem. In my B+ Tree implementation, the val for a key
in an internal node is held in the largest val pointer of the key's predecessor subtree.

The b_tree itself is stored in a file which is divided into sectors of JDISK_SECTOR_SIZE bytes. 
In my case, that is sectors of 1024 (1K) bytes. Each sector can be referred to by its 
"Logical Block Address", and sectors are labeled consecutively. For example, the first 
0 - 1023 bytes of the file correspond to the first sector, which has an lba ("Logical 
Block Address of 0). The next 1024 - 2047 bytes in the file correspond to the second 
sector, which has an lba of 1. The next 2048 - 3071 bytes of the file correspond to 
the third sector, which has an lba of 2, etc..

The function void *b_tree_create(char *filename, long size, int key_size) allows you to create 
a b_tree. It takes the desired name of the b_tree file, its size, and key size as arguments. The 
size of the file must be a multiple of JDISK_SECTOR_SIZE bytes. The function returns a void *, 
which is your handle on the b_tree structure. Other operations, like insertion, will require this 
handle.

The function void *b_tree_attach(char *filename) allows you to attach to an already existing 
b_tree file by passing the name of the file as an argument to the function. It also returns a 
void * which is your handle on the b_tree.

The function unsigned int b_tree_insert(void *b_tree, void *key, void *record) takes a handle
to a b_tree as an argument, as well as a key (which is the key for your record/val), and a 
record. The key is a buffer of key size bytes, while record is a buffer of JDISK_SECTOR_SIZE
bytes. 

The function unsigned int b_tree_find(void *b_tree, void *key) takes a handle to a b_tree and 
a buffer to key size bytes as arguments. If the key specified exists in the tree, then the 
logical block address of the sector containing the value for the key is returned. If the 
key does not exist in the tree, then 0 is returned. 

The function takes a handle to a b_tree as an argument and void *b_tree_disk(void *b_tree) 
returns the jdisk pointer for the b_tree (jdisks explained later). 

The function int b_tree_key_size(void *b_tree) takes a handle to a b_tree as an argument and
returns the key size for that b_tree. 

The function void b_tree_print_tree(void *b_tree) takes a handle to a b_tree as an argument
and prints the tree to stdout in a BFS fashion.


