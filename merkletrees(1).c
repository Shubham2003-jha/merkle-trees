#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.c"



typedef int (*Hash_Function) (unsigned char *, unsigned int, unsigned char *);

typedef struct { 
    char *hash;
    char *data;
} merkle_tree_node;

typedef struct {
    size_t n;
    size_t tree_height;
    size_t hash_size;
    size_t data_block_size;
    size_t data_blocks;
    Hash_Function hash_function;
    merkle_tree_node *nodes;
} merkle_tree;

int build_tree(merkle_tree *mt, char **data);
int tree_cmp(merkle_tree *a, merkle_tree *b, size_t i);
int set_tree_data(merkle_tree *mt, size_t i, char *data);
void freeMerkleTree(merkle_tree *mt);



static int hash_node(merkle_tree *mt, size_t i);
static void print_tree(merkle_tree *mt);

//build a merkle tree with settings in 'mt'
//use data blocks
int build_tree(merkle_tree *mt, char *data[]) {

    if (mt->data_blocks > (1 << (mt->tree_height - 1)))
        return -1;
    int i, leaf_start;
    leaf_start = (1 << (mt->tree_height - 1));
    mt->n = leaf_start + mt->data_blocks - 1;
    mt->nodes = (merkle_tree_node *)malloc(sizeof(merkle_tree_node) * (mt->n + 1));
    for (i = leaf_start; i <= mt->n; i++) {
        mt->nodes[i].data = data[i-leaf_start];
        mt->nodes[i].hash = NULL;
        if (hash_node(mt, i) == -1)
            return -1;
    }
    for (i = leaf_start - 1; i > 0; i--) {
        mt->nodes[i].hash = NULL;
        if (hash_node(mt, i) == -1)
            return -1;
    }
    return 0;
}

//compare two merkle trees from node i
//make sure the two trees in same height
//return different data block number
//if no differnece return 0
int tree_cmp(merkle_tree *a, merkle_tree *b, size_t i) {

    int cmp;
    if (i > (1<<a->tree_height)-1)
        return -1;
    if (memcmp(a->nodes[i].hash, b->nodes[i].hash, a->hash_size) != 0) {
        if (i<<1 > (1<<a->tree_height)-1)
            return i - (1 << (a->tree_height - 1)) + 1;
        else {
            cmp = tree_cmp(a, b, i<<1);
            if (cmp == 0)
                return tree_cmp(a, b, (i<<1)+1);
            else
                return cmp;
        }
    }
    else
        return 0;
}

// set tree data with specific block number
//
int set_tree_data(merkle_tree *mt, size_t block_num, char *data) {

    if (block_num > mt->data_blocks)
        return -1;
    size_t i = (1 << (mt->tree_height - 1)) + block_num - 1;
    if (mt->nodes[i].data)
        free(mt->nodes[i].data);
    mt->nodes[i].data = data;
    if (hash_node(mt, i) == -1)
        return -1;
    for (i>>=1; i>0; i>>=1)
        if (hash_node(mt, i) == -1)
            return -1;
    return 0;
}

//free the Merkle Tree Object...
//
void freeMerkleTree(merkle_tree *mt) {

    int i;
    if (!mt)
        return;
    if (mt->nodes) {
        for (i=1; i<=mt->n; i++)
            if(mt->nodes[i].hash)
                free(mt->nodes[i].hash);
        free(mt->nodes);
    }
    return;
}
