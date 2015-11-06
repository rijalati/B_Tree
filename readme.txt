**Note**: The code in b_tree.h, jdisk.h, and jdisk.c was written by Dr. James Plank of the 
University of Tennessee. The code in b_tree.c, example_creation.c, and example_tester.c is my 
own. Also note that deletions are not implemented in my b_tree, only insertions.

This repository contains the source code for a simple B+ Tree that is implemented on top of 
a Unix/Linux filesystem and is capable of functioning as a simple Database Management System. 
This was a project I did my senior year of college for Dr. Plank's Advanced Programming and 
Algorithms course at UTK. More information on the project and its specifications can be found
at: web.eecs.utk.edu/~plank/plank/classes/cs494/494/labs/Lab-1-Btree/

General information on B+ Trees can be found at: 
cis.stvincent.edu/html/tutorials/swd/btree/btree.html

The functions declared in b_tree.h define an interface for a B+ Tree ("b_tree") that is 
implemented on top of a Unix/Linux filesystem. The file jdisk.h exports a disk-like interface
on top of standard Nix files. Also note that in my B+ Tree implementation, the val for a key
in an internal node is held in the largest val pointer of the key's predecessor subtree.

The b_tree itself is stored in a file which is divided into sectors of JDISK_SECTOR_SIZE bytes. 
In my case, that is sectors of 1024 (1K) bytes. Each sector can be referred to by its 
"Logical Block Address", and sectors are labeled consecutively. For example, the first 
0 - 1023 bytes of the file correspond to sector zero, which has an LBA ("Logical 
Block Address of 0). The next 1024 - 2047 bytes in the file correspond to the sector
one, which has an LBA of 1. The next 2048 - 3071 bytes of the file correspond to 
sector two, which has an LBA of 2, etc..

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
returns the jdisk pointer for the b_tree. 

The function int b_tree_key_size(void *b_tree) takes a handle to a b_tree as an argument and
returns the key size for that b_tree. 

The function void b_tree_print_tree(void *b_tree) takes a handle to a b_tree as an argument
and prints the tree to stdout in a BFS fashion.

The structure for a b_tree file is defined as follows: The first sector (sector zero) is 
dedicated to storing information about the b_tree. The first 0-3 bytes of sector zero store
the key size of the b_tree as an integer. Bytes 4-7 of sector zero store the logical block
address (LBA) of the sector that stores the root node of the b_tree. Bytes 8-15 (or 8-11, 
if longs are 4 bytes) of sector zero store the LBA of the first "free sector" (i.e., the 
first sector that is not yet storing any data). All other sectors in the b_tree file may 
store internal nodes of the tree, external nodes of the tree, or a record for a given key
in the b_tree. 

The format of a sector storing a node in the b_tree is defined as follows: The first byte
is either a 0 or a 1, determining whether the node is external (0) or internal (1). The 
next byte tells you how many keys are in the node. The number of keys that can fit in a node
is (JDISK_SECTOR_SIZE - 6) / (key_size + 4). The key_size for a b_tree must be between 4 
and 254 bytes. So even if keys are 4 bytes long, the number of keys that can fit in a node
can be represented by an unsigned char. 

If we define MAXKEY to be (JDISK_SECTOR_SIZE - 6) / (key_size + 4), then the next 
MAXKEY * key_size bytes in a node sector are the keys for that node. The last 
(MAXKEY + 1) * 4 bytes are the LBA's, which are the pointers of the b_tree. If the 
node is internal, then the LBA's are LBA's of other sectors containing other 
nodes of the b_tree, but if the node is external then the LBA's are LBA's of 
sectors containing records/vals for keys in the b_tree. Note that if there are 
nkeys in a node, then there are (nkeys + 1) LBA's.

Note that all data written to and read from the b_tree is written/read as raw bytes.
This means that if the keys and records that are being inserted/searched for in the 
b_tree are meant to be interpreted as strings, then there is something that should
be kept in mind.

Excess bytes contained in keys and records must be handled in some way. For 
example, if your b_tree has a key_size of 40 bytes, and you wish to insert the 
key "Bugs Bunny", then your data really only requires 10 or 11 bytes (the 
length of "Bugs Bunny" + an optional NULL character). A way of dealing with 
the excess bytes should be devised (they should probably be set to the NULL 
character). For example, supposed you previously used strcpy() to copy a string 
to a 40 char buffer you are using to perform insertions/lookups on your b_tree.
Say you previously inserted "Yankee Doodle". If "Yankee Doodle" is the first 
value you've strcpy()'d to your buffer, and all the bytes in your buffer were
initially set to the NULL character, then your buffer would look like this:
"Yankee Doodle\0\0\0\0\0.......\0". If you then insert into the tree, no problem.
But now say you strcpy() "Bugs Bunny" to your buffer. Now your buffer looks like this:
"Bugs Bunny\0le\o\0\0\0......\0". You might insert this into the tree, and then later
lookup "Bugs Bunny\0\0\0.....\0" in the tree, expecting it to be there, but it would
not be. This is a subtle bug to keep in mind. The safest thing to do is set excess 
bytes in keys and vals to the NULL character when performing insertions and lookups.

I have included two example programs, example_creation.c and example_tester.c that
illustrate how one might embed b_tree functions in other C programs and use them 
to create/attach to b_tree files, and use them to store data that is meant to be 
interpreted as strings. The file names.txt contains a list of names of 100 famous
celebrities, musicians, scientists, t.v. characters, movie characters, etc... that
have been scrambled. The program example_creation.c takes the name of this file 
as a command line argument, as well as the name of a new b_tree file, and the name
of an output file. The program first creates a b_tree, which will be stored in 
a file with the name specified by the command line argument, and then generates 
100 fake names by combinging random first and last names from the names.txt 
file (in addition to a randomly generated middle initial, e.g. "A."), and also 
randomly generates fake phone number for each name. Each fake fullname and phone 
number for a key-val pair for insertion into the specified b_tree file. For example, 
say the name "Luke A.  Vader" was generated with the phone number "(555)-123-4567". 
Then ("Luke A.  Vader", "(555)-123-4567") form a key value pair and are inserted 
into the b_tree file. The program prints each insertion to the output file in the form: 
INSERT "Fullname". The file insertions.txt is an example of such a file, generated
by example_creation.c. The file phone_data.db is an example of a b_tree file formed
by inserting the names from insertions.txt into phone_data.db with randomly generated
phone numbers.

The program example_tester.c takes three command line arguments. The first is the
name of the "names file", (names.txt). The second is the name of the file containing
the insertion statements from an instance of example_creation.c (the "output file" 
from example_creation.c). The third is the name of the b_tree file that was created
in example_creation.c. The program example_tester.c then randomly does one of the 
two following things: It either generates a random name not contained in the insertions.txt
file and searches for it in the tree (it shouldn't be in there), or tries to find 
a name from the insertions.txt file in the tree (which should be in there), and then
prints its val.

I have included a makefile for building example_creation.c and example_tester.c
