/* Jackson Collier, CS 494, Lab #1 */
/* This program contains methods for implementing a B+ Tree over a "jdisk", which is a UNIX file.  Each node is stored in a sector of a jdisk. 
   The values are also stored in a jdisk.  The first sector in the jdisk stores the key size, lba ("logical block address") of the root node, the 
   first free block in the tree, and the size of the jdisk file, respectively. The size of file sectors is 1K (JDISK_SECTOR_SIZE). Sectors that 
   store nodes store them in the following format:  The first byts is 0 if the node is external and 1 if it is internal.  The next byte says how 
   many keys are in the node (The number of keys that can fit in a node is (JDISK_SECTOR_SIZE - 6) / (key_size + 4), denoted MAXKEY). The next 
   MAXKEY * key_size bytes are the keys.  The last (MAXKEY + 1) * 4 bytes are the LBA's (which are LBA's of other nodes if node is internal, and
   LBA's of vals if node is external).  Methods for inserting a new key/val pair into the tree, as well as creating a tree, attaching one to an already 
   exisiting jdisk file, and for printing the tree are given below, as well as some additional methods.  However, deletion is not implemented.  
   Information about each function / procedure is given above its definition. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "jdisk.h"

/* ================================================================================= */
/* This struct represents a node in the tree */
typedef struct tnode{
    unsigned char sector[JDISK_SECTOR_SIZE];                    /* The sector */
    unsigned char nkeys;                                        /* Number of keys currently in node/sector */
    unsigned char flush;                                        /* Flush this node to disk? */
    unsigned char internal;                                        /* 1 if node/sector internal, zero otherwise */
    unsigned int lba;                                            /* This sector/nodes "logical block address" */
    unsigned char **keys;                                        /* Node's keys */
    unsigned int *lbas;                                            /* Node's vals, pointers to other blocks */
    struct tnode *parent;                                        /* Node's parent, used during splits */
} TreeNode;

/* ================================================================================= */
/* This struct represents the tree */
typedef struct{
    int key_size;                            /* Byte size of keys in the tree */
    unsigned int root_lba;                    /* The Logical block address of the root node */
    unsigned long first_free_block;            /* Logical block address of first free sector */
    void *disk;                                /* the jdisk */
    unsigned long size;                        /* size of jdisk */
    unsigned long num_lbas;                    /* size/JDISK_SECTOR_SIZE */
    int MAXKEY;                                /* keys_per_block */
    int MAXVALS;                            /* lbas_per_block */
}    B_Tree;

/* ================================================================================= */
/* A node in a "Tree Node queue", used to perform a BFS when printing the B_Tree */
typedef struct Queue_Node{
    TreeNode *tn;
    struct Queue_Node *back;
} queue_node;

/* ================================================================================= */
/* A "Tree Node queue", used to perform a BFS when printing the B_Tree */
typedef struct TN_Queue{
    struct Queue_Node *front;
    struct Queue_Node *back;
} tn_queue;

/* ================================================================================= */
/* This function appends to the "Tree Node queue" */
void append_tn_queue(tn_queue *q, queue_node *n){
    if (q->front == NULL){
        q->front = n;
        q->back = n;
    }
    else{
        n->back = NULL;
        q->back->back = n;
        q->back = n;
    }
    return;
}

/* ================================================================================= */
/* This function pops and returns (the TreeNode * field) of the front node on the "Tree Node queue" */
TreeNode *pop_tn_queue(tn_queue *q){
    TreeNode *tn;
    queue_node *qn;

    qn = q->front;
    tn = qn->tn;
    q->front = q->front->back;
    free(qn);
    return tn;
}

/* ================================================================================= */
/* This function returns 0 if the "Tree Node queue" is empty, 1 otherwise */
int TreeNode_queue_full(tn_queue *q){
    if (q->front == NULL) return 0;
    return 1;
}

/* ================================================================================= */
/* This procedure takes a pointer to a B_Tree as a paramter and flushes its key_size, root_lba, first_free
   block and size to sector zero of the jdisk, respectively. */
void write_sector_zero(B_Tree *b_tree){
    void *buf;
    
    buf = (void *) malloc(sizeof(char) * JDISK_SECTOR_SIZE);
    memcpy(buf, (void *) b_tree, sizeof(int) + sizeof(unsigned int) + sizeof(unsigned long));    
    memcpy(buf + sizeof(int) + sizeof(unsigned int) + sizeof(unsigned long), (void *) &b_tree->size, sizeof(unsigned long));    
    jdisk_write(b_tree->disk, 0, buf);
    free(buf);

    return;
}

/* ================================================================================= */
/* This procedure takes a B_Tree, TreeNode, and lba as parameters.  It then writes the node to sector
   lba in the B_Tree's jdisk file, following the protocol listed at the head of the file. */
void write_node(B_Tree *b_tree, TreeNode *node, unsigned int lba){
    void *buf;
    int i;

    buf = (void *) malloc(sizeof(char) * JDISK_SECTOR_SIZE);

    memcpy(buf, &(node->internal), 1);
    memcpy(buf + 1, &(node->nkeys), 1);

    for (i = 0; i < node->nkeys; i++){
        memcpy(buf + 2 + (i * b_tree->key_size), node->keys[i], b_tree->key_size);
    }

    for (i = 0; i < node->nkeys + 1; i++){
        memcpy(buf + ((JDISK_SECTOR_SIZE - ((b_tree->MAXKEY + 1) * 4)) + (4 * i)), &(node->lbas[i]), 4);
    }

    jdisk_write(b_tree->disk, lba, buf);
    free(buf);

    return;
}

/* ================================================================================= */
/* This function takes a filename, size, and keysize as parameters.  It creates a jdisk with the given name and size.
   It also creates a B_Tree and root node.  It flushes the B_Tree to sector 0 and writes the appropriate fields of the
   root node to sector 1.  It returns a handle on the B_Tree. */
void *b_tree_create(char *filename, long size, int key_size){
    int i;    
    B_Tree *tree;
    TreeNode *root;
    void *buf;

    /* Create B_Tree and initialize fields */
    tree = (B_Tree *) malloc(sizeof(B_Tree));
    tree->disk = jdisk_create(filename, size);
    tree->key_size = key_size;
    tree->root_lba = 1;
    tree->first_free_block = 2;
    tree->size = size;
    tree->num_lbas = size / JDISK_SECTOR_SIZE;
    tree->MAXKEY = (JDISK_SECTOR_SIZE - 6) / (key_size + 4);
    tree->MAXVALS = tree->MAXKEY + 1;

    /* Create a root node with zero keys */
    root = (TreeNode *) malloc(sizeof(TreeNode));
    root->nkeys = 0;
    root->internal = 0;
    root->lba = 1;
    root->parent = NULL;
    root->keys = (unsigned char **) malloc(sizeof(unsigned char *) * tree->MAXKEY);
    for (i = 0; i < tree->MAXKEY; i++){
        root->keys[i] = (unsigned char *) malloc(sizeof(unsigned char) * tree->key_size);
    }
    root->lbas = (unsigned int *) malloc(sizeof(unsigned int) * tree->MAXVALS);
    root->lbas[0] = 0;
    
    /* Flush new_tree and new_node to their respective sectors */
    write_sector_zero(tree);    
    write_node(tree, root, root->lba);

    return (void *) tree;
}

/* ================================================================================= */
/* This function attaches to the jdisk file "filename" and returns its B_Tree struct. */
void *b_tree_attach(char *filename){
    B_Tree *tree;                            /* The B_Tree for jdisk filename */
    void *buf;                                /* A buffer for reading sector 0 from jdisk */    
    
    /* Allocate memory for tree and a buffer for reading from sector 0 */
    tree = (B_Tree *) malloc(sizeof(B_Tree));
    buf = (void *) malloc(sizeof(char) * JDISK_SECTOR_SIZE);
    
    /* Get disk for tree and read fields stored in sector zero in b_tree */
    tree->disk = jdisk_attach(filename);
    jdisk_read(tree->disk, 0, buf);
    memcpy((void *) tree, buf, sizeof(int) + sizeof(unsigned int) + sizeof(unsigned long));
    memcpy((void *) &tree->size, buf + sizeof(int) + sizeof(unsigned int) + sizeof(unsigned long), sizeof(unsigned long));    
    tree->num_lbas = tree->size / JDISK_SECTOR_SIZE;
    tree->MAXKEY = (JDISK_SECTOR_SIZE - 6) / (tree->key_size + 4);
    tree->MAXVALS = tree->MAXKEY + 1;

    /* Free memory allocated to the buffer and return handle to tree */
    free(buf);
    return tree;
}

/* ================================================================================= */
/* This function loads info into a TreeNode from a logical block address in a jdisk file. */
TreeNode *load_sector(B_Tree *b_tree, unsigned int lba){
    TreeNode *node;                        /* The new TreeNode */
    void *buf;                            /* A buffer for reading from sector/lba */
    int i;                                /* Arbitrary iteration variable */

    /* Allocate memory for buffer and node (and temporary variables) */
    node = (TreeNode *) malloc(sizeof(TreeNode));
    buf = (void *) malloc(sizeof(char) * JDISK_SECTOR_SIZE);

    /* Read sector from jdisk into buffer and into node->sector */
    jdisk_read(b_tree->disk, lba, buf);
    memcpy((void *) node->sector, buf, JDISK_SECTOR_SIZE);

    /* Read internal and nkeys from buf into node */
    memcpy(&node->internal, buf, 1);
    memcpy(&node->nkeys, buf + 1, 1);
    node->lba = lba;
    
    /* Allocate memory for node's keys and copy them from buf */
    node->keys = (unsigned char **) malloc(sizeof(unsigned char *) * b_tree->MAXKEY);
    for (i = 0; i < b_tree->MAXKEY; i++){
        node->keys[i] = (unsigned char *) malloc(sizeof(unsigned char) * b_tree->key_size);    
        if (i < node->nkeys) memcpy(node->keys[i], buf + 2 + (i * b_tree->key_size), b_tree->key_size);
    }

    /* Allocate memory for node's lbas and copy them from buf */
    node->lbas = (unsigned int *) malloc(sizeof(unsigned int) * b_tree->MAXVALS);
    for (i = 0; i < node->nkeys + 1; i++){
        memcpy(&(node->lbas[i]), buf + ((JDISK_SECTOR_SIZE - ((b_tree->MAXKEY + 1) * 4)) + (4 * i)), 4);
    }
    node->parent = NULL;

    /* Free memory allocated for buffer, tmp, and tmp_int */
    free(buf);
    return node;
}

/* ================================================================================= */
/* This procedure frees the memory allocated for a given TreeNode */
void free_node(TreeNode *node, B_Tree *b_tree){
    int i;                    /* Arbitrary iteration variable */

    for (i = 0; i < b_tree->MAXKEY; i++){
        if (node->keys[i] != NULL) free(node->keys[i]);
    }

    free(node->keys);
    free(node->lbas);
    free(node);
    return;
}

/* ================================================================================= */
void add_key_val(B_Tree *b_tree, TreeNode *node, void *key, int lba){
    int i;                                /* Arbitrary iteration variable */
    int j;                                /* Arbitrary iteration variable */
    int insertion_index;                /* Position we will be insertin key/val */
    unsigned char **key_array;            /* For determining median node during a split */
    unsigned int *lba_array;            /* For determining lbas during a split */
    TreeNode *right;                    /* Right node during a split */
    TreeNode *new_root;                    /* For splitting the root node */    

    /* Is the node we are trying to insert into full? */
    if (node->nkeys == b_tree->MAXKEY){ /* We need to split */
        /* Create an array to hold keys in current node (including one we are trying to insert) */
        key_array = (unsigned char **) malloc(sizeof(unsigned char *) * (b_tree->MAXKEY + 1));
        lba_array = (unsigned int *) malloc(sizeof(unsigned int) * (b_tree->MAXVALS + 1));
        
        /* Allocate memory for each key in key_array and copy each key from node->keys into key_array */
        for (i = 0; i < b_tree->MAXKEY + 1; i++){
            key_array[i] = (unsigned char *) malloc(sizeof(unsigned char) * b_tree->key_size);
            if (i < b_tree->MAXKEY){    
                memcpy((void *) key_array[i], (void *) node->keys[i], b_tree->key_size);
            }
        }

        /* Copy lbas from node over to lba_array */
        for (i = 0; i < b_tree->MAXVALS; i++){
            lba_array[i] = node->lbas[i];
        }

        /* Find spot to insert our new key into key_array */
        for (i = 0; i < b_tree->MAXKEY; i++){
            if (memcmp(key, (void *) key_array[i], b_tree->key_size) < 0){
                /* We will insert at i, and move other keys over */
                break;
            }
        }

        /* Move keys at index greater than i one to right */
        for (j = b_tree->MAXKEY; j > i; j--){
            memcpy(key_array[j], key_array[j - 1], b_tree->key_size);
        }

        /* Move lbas at index greater than i one to right */
        for (j = b_tree->MAXVALS; j > i; j--){
            lba_array[j] = lba_array[j - 1];
        }

        /* Now insert key into key_array */
        memcpy(key_array[i], key, b_tree->key_size);

        /* Get lba for new record, write the record, and insert it into lba_array */
        lba_array[i] = lba;

        /* Create right node */
        right = (TreeNode *) malloc(sizeof(TreeNode));
        right->nkeys = b_tree->MAXKEY - ((b_tree->MAXKEY + 1) / 2);
        right->internal = node->internal;
        right->lba = b_tree->first_free_block;
        right->parent = NULL;
        b_tree->first_free_block++;
        
        /* Copy keys right of median node over to right->keys */
        right->keys = (unsigned char **) malloc(sizeof(unsigned char *) * b_tree->MAXKEY);
        for (i = 0; i < b_tree->MAXKEY; i++) right->keys[i] = (unsigned char *) malloc(sizeof(unsigned char) * b_tree->key_size);
        for (i = ((b_tree->MAXKEY + 1) / 2) + 1; i < b_tree->MAXKEY + 1; i++){
            memcpy((void *) right->keys[i - (((b_tree->MAXKEY + 1) / 2) + 1)], (void *) key_array[i], b_tree->key_size);
        }

        /* Copy keys left of median node over to node->keys */
        node->nkeys = (b_tree->MAXKEY + 1) / 2;
        for (i = 0; i < (b_tree->MAXKEY + 1) / 2; i++){
            memcpy((void *) node->keys[i], (void *) key_array[i], b_tree->key_size);
        }

        /* Copy lbas left of split over to node->lbas */
        for (i = 0; i < ((b_tree->MAXKEY + 1) / 2) + 1; i++) node->lbas[i] = lba_array[i];

        /* Allocate memory for right->lbas and copy lbas right of split over to right->lbas */
        right->lbas = (unsigned int *) malloc(sizeof(unsigned int) * b_tree->MAXVALS);
        for (i = ((b_tree->MAXKEY + 1) / 2) + 1; i < b_tree->MAXVALS + 1; i++) right->lbas[i - (((b_tree->MAXKEY + 1) / 2) + 1)] = lba_array[i];

        if (node->parent == NULL){    /* We need to make a new root */
            new_root = (TreeNode *) malloc(sizeof(TreeNode));
            new_root->nkeys = 1;
            new_root->internal = 1;
            new_root->lba = b_tree->first_free_block;
            b_tree->first_free_block++;
            b_tree->root_lba = new_root->lba;
            new_root->keys = (unsigned char **) malloc(sizeof(unsigned char *) * b_tree->MAXKEY);
            for (i = 0; i < b_tree->MAXKEY; i++){
                new_root->keys[i] = (unsigned char *) malloc(sizeof(unsigned char) * b_tree->key_size);
            }
            memcpy((void *) new_root->keys[0], (void *) key_array[(b_tree->MAXKEY + 1) / 2], b_tree->key_size);
            new_root->lbas = (unsigned int *) malloc(sizeof(unsigned int) * b_tree->MAXVALS);
            new_root->lbas[0] = node->lba;
            new_root->lbas[1] = right->lba;
            new_root->parent = NULL;
            node->parent = new_root;
            right->parent = new_root;
            write_node(b_tree, node, node->lba);
            write_node(b_tree, right, right->lba);
            write_node(b_tree, new_root, new_root->lba);
            write_sector_zero(b_tree);
        }
        else{
        /* Find index of this node in parent, and change corresponding lba to right->lba */
            for (i = 0; i < node->parent->nkeys + 1; i++){
                if (node->parent->lbas[i] == node->lba){
                    node->parent->lbas[i] = right->lba;
                    break;
                }
            }
            add_key_val(b_tree, node->parent, (void *) key_array[(b_tree->MAXKEY + 1) / 2], node->lba);
            right->parent = node->parent;
            write_node(b_tree, node, node->lba);
            write_node(b_tree, right, right->lba);
        }        

        /* Free memory allocated to keys_array and lba_array. Also free right. */
        for (i = 0; i < b_tree->MAXKEY + 1; i++) free(key_array[i]);
        free(key_array);
        free(lba_array);
        free_node(right, b_tree);
    }
    else{ /* We do not need to split, we can insert normally */
        insertion_index = 0;
    
        for (i = 0; i < node->nkeys; i++){
            if (memcmp(key, (void *) node->keys[i], b_tree->key_size) < 0){
                /* We should insert key here and val here */
                break;
            }
        }

        insertion_index = i;

        /* Move keys at indices greater than insertion spot over one to right */
        for (j = node->nkeys; j > insertion_index; j--){
            memcpy((void *) node->keys[j], (void *) node->keys[j - 1], b_tree->key_size);
        }
    
        /* Move lbas at indices greater than insertion spot over one to right */
        for (j = node->nkeys + 1; j > insertion_index; j--){
            node->lbas[j] = node->lbas[j - 1];
        }
        
        /* Insert key and lba */
        memcpy((void *) node->keys[insertion_index], key, b_tree->key_size);
        node->lbas[insertion_index] = lba;
        node->nkeys++;
        write_node(b_tree, node, node->lba);
    }
    return;
}

/* ================================================================================= */
/* This function gets the lba of the value of a key in an internal node. */
unsigned int get_internal_val(B_Tree *b_tree, unsigned int lba){
    TreeNode *node;                /* The current node in the tree */
    TreeNode *prev;                /* Previous node in the traversal */
    int internal                /* Is the current node internal? */;
    int i;                        /* Arbitrary iteration variable */
    int return_lba;                /* lba of new node on path to val */

    node = NULL;
    prev = NULL;

    node = load_sector(b_tree, lba);
    while (node->internal){
        prev = node;    
        node = load_sector(b_tree, node->lbas[node->nkeys]);
        if (prev != NULL) free_node(prev, b_tree);
    }
    return_lba = node->lbas[node->nkeys];
    if (node != NULL) free_node(node, b_tree);
    return return_lba;
}

/* ================================================================================= */
/* This procedure prints the contents of a TreeNode */
void print_node(TreeNode *node){
    int i;                    /* Arbitrary iteration variable */    

    printf("LBA 0x%x --- Internal: %d --- nkeys: %d\n", node->lba, node->internal, node->nkeys);
    for (i = 0; i < node->nkeys + 1; i++){
        if (i < node->nkeys){    
            printf("     Entry %d: LBA: 0x%x --- Key: %s\n", i, node->lbas[i], (char *) node->keys[i]);
        }
        else printf("     Entry %d: LBA: 0x%x\n", i, node->lbas[i]);
    }
    return;
}

/* ================================================================================= */
/* This function uses a BFS to print the contents of the B_Tree. */
void b_tree_print_tree(void *b_tree){
    int i;                                /* Arbitrary iteration variable */    
    tn_queue *Tree_Queue;                /* A queue of TreeNodes */
    queue_node *qn;                        /* A node in the TreeNode queue */
    TreeNode *tn;                        /* An arbitrary node in the TreeNode queue */
    TreeNode *tn_child;                    /* Child of a node in the TreeNode queue */

    /* Allocate and initialize Tree_Queue */
    Tree_Queue = (tn_queue *) malloc(sizeof(tn_queue));
    Tree_Queue->front = NULL;
    Tree_Queue->back = NULL;

    /* Load the root node */
    tn = load_sector((B_Tree *) b_tree, ((B_Tree *) b_tree)->root_lba);

    /* Allocate and initialize a queue node to store b_tree->root_lba, pop it on the Tree_Queue */
    qn = (queue_node *) malloc(sizeof(queue_node));
    qn->tn = tn;
    qn->back = NULL;

    append_tn_queue(Tree_Queue, qn);
    
    /* Perform BFS to print b_tree */
    while (TreeNode_queue_full(Tree_Queue)){
        tn = pop_tn_queue(Tree_Queue);
        print_node(tn);
        printf("\n");
        if (tn->internal == 1){    
            for (i = 0; i < tn->nkeys + 1; i++){
                if (tn->lbas[i] != 0){
                    tn_child = load_sector((B_Tree *) b_tree, tn->lbas[i]);
                    qn = (queue_node *) malloc(sizeof(queue_node));
                    qn->tn = tn_child;
                    qn->back = NULL;
                    append_tn_queue(Tree_Queue, qn);
                }
            }
            
        }
        free_node(tn, (B_Tree *) b_tree);
    }
    return;
}

/* ================================================================================= */
/* This function inserts a key and record into the B_Tree.  It finds the appropriate node
   (or returns the lba of key if key is already in the tree) to insert into, and then calls
   add_key_val to perform the insertion. */
unsigned int b_tree_insert(void *b_tree, void *key, void *record){
    int i;                                        /* Arbitrary iteration variable */
    TreeNode *node;                                /* Node currently being processed */
    TreeNode *prev;                                /* current_node's parent */
    unsigned long lba;                            /* lba of key, to be returned */

    /* Make sure node's in tree are large enough to store key/val pairs. */
    if (((B_Tree *) b_tree)->first_free_block >= ((B_Tree *) b_tree)->size / JDISK_SECTOR_SIZE) return 0;

    /* Load root node and set internal to be true */
    node = load_sector((B_Tree *) b_tree, ((B_Tree *) b_tree)->root_lba);
    while (node->internal){
        for (i = 0; i < node->nkeys + 1; i++){
            if (i == node->nkeys){
                prev = node;    
                node = load_sector((B_Tree *) b_tree, node->lbas[i]);
                node->parent = prev;
                break;
            }
            if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) < 0){
                prev = node;    
                node = load_sector((B_Tree *) b_tree, node->lbas[i]);
                node->parent = prev;    
                break;
            }
            if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) == 0){
                lba = get_internal_val((B_Tree *) b_tree, node->lbas[i]);
                jdisk_write(((B_Tree *) b_tree)->disk, lba, record);
                /* Free memory allocated in search */
                while (node != NULL){
                    prev = node->parent;
                    free_node(node, (B_Tree *) b_tree);
                    node = prev;
                }
                return lba;
            }
        }
    }

    /* We have reached an external node. If this node has a key that equals our key, return its lba. */
    for (i = 0; i < node->nkeys; i++){
        if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) == 0){
            jdisk_write(((B_Tree *) b_tree)->disk, node->lbas[i], record);
            lba = node->lbas[i];
            /* Free memory allocated in search */
            while (node != NULL){
                prev = node;
                node = node->parent;
                free_node(prev, (B_Tree *) b_tree);
            }
            return lba;
        }
    }

    /* Add new key/val pair to current node */    
    lba = ((B_Tree *) b_tree)->first_free_block;
    ((B_Tree *) b_tree)->first_free_block++;
    jdisk_write(((B_Tree *) b_tree)->disk, lba, record);
    add_key_val((B_Tree *) b_tree, node, key, lba);    

    /* Rewrite sector containing info about b_tree */    
    write_sector_zero((B_Tree *) b_tree);

    /* Free memory allocated in search. */
    while (node != NULL){
        prev = node;
        node = node->parent;
        free_node(prev, (B_Tree *) b_tree);
    }

    return lba;
}

/* ================================================================================= */
/* This function returns the lba of a key in the B_Tree. If the key does not exist, it
   returns 0. */
unsigned int b_tree_find(void *b_tree, void *key){    
    int i;                                /* Arbitrary iteration variable */
    int val;                            /* Lba to be returned */
    TreeNode *node;                        /* For traversing the tree */
    TreeNode *prev;                        /* Previous node in our search path. For memory management. */

    /* Load root node and set internal to be true. Begin search of b_tree. */
    node = load_sector((B_Tree *) b_tree, ((B_Tree *) b_tree)->root_lba);
    while (node->internal){
        for (i = 0; i < node->nkeys + 1; i++){
            /* Our key is greater than any key in current node. Load rightmost lba */
            if (i == node->nkeys){    
                prev = node;
                node = load_sector((B_Tree *) b_tree, node->lbas[i]);
                free_node(prev, (B_Tree *) b_tree);
                break;
            } /* Our key is less than current key in node, load lba paired with current key in node */
            if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) < 0){ 
                prev = node;
                node = load_sector((B_Tree *) b_tree, node->lbas[i]);
                free_node(prev, (B_Tree *) b_tree);
                break;
            } /* Our key is equivalent to current key in node, return this keys value. */
            if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) == 0){
                val = get_internal_val((B_Tree *) b_tree, node->lbas[i]);
                free_node(node, (B_Tree *) b_tree);
                return val;
            }
        }
    }

    /* Our search has hit an external node. Return lba of key if key is in external node, otherwise return 0. */
    for (i = 0; i < node->nkeys; i++){
        if (memcmp(key, (void *) node->keys[i], ((B_Tree *) b_tree)->key_size) == 0){
            val = node->lbas[i];
            free_node(node, (B_Tree *) b_tree);
            return val;
        }
    }
    return 0;
}

/* ================================================================================= */
/* This function returns the jdisk pointer for the B_Tree. */
void *b_tree_disk(void *b_tree){
    return ((B_Tree *) b_tree)->disk;
}

/* ================================================================================= */
/* This function returns the size of keys in the B_Tree. */
int b_tree_key_size(void *b_tree){
    return ((B_Tree *) b_tree)->key_size;
}
