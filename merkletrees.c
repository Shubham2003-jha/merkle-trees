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


//update a tree node hash
//leaf or inside nodes ...
//
static int hash_node(merkle_tree *mt, size_t i) {

    if (i > (1<<mt->tree_height)-1)
        return -1;
    if (i < (1<<mt->tree_height-1)){
        if (2*i+1 <= mt->n && mt->nodes[2*i].hash && mt->nodes[2*i+1].hash) {
            char *buffer = (char *)malloc(sizeof(char *) * (2 * mt->hash_size + 1));
            memcpy(buffer, mt->nodes[2*i].hash, mt->hash_size);
            memcpy(buffer+mt->hash_size, mt->nodes[2*i+1].hash, mt->hash_size);
            if (!mt->nodes[i].hash)
                mt->nodes[i].hash = (char *)malloc(sizeof(char *) * mt->hash_size);
            mt->hash_function(buffer, 2*mt->hash_size, mt->nodes[i].hash);
            free(buffer);
        }
        else if (2*i <= mt->n && mt->nodes[2*i].hash) {
            if (!mt->nodes[i].hash)
                mt->nodes[i].hash = (char *)malloc(sizeof(char *) * mt->hash_size);
            memcpy(mt->nodes[i].hash, mt->nodes[2*i].hash, mt->hash_size);
        }
    }
    else {
        if (mt->nodes[i].data) {
            if (!mt->nodes[i].hash)
                mt->nodes[i].hash = (char *)malloc(sizeof(char *) * mt->hash_size);
            mt->hash_function(mt->nodes[i].data, mt->data_block_size, mt->nodes[i].hash);
        }
        else
            return -1;
    }
    return 0;
}

// for test use
// print a merkle tree nodes' hash
// as a list with node order
static void print_tree(merkle_tree *mt) {

    int i;
    printf("*******************************\n");
    for(i=1; i<=mt->n; i++)
        MD5Print(mt->nodes[i].hash);
    printf("*******************************\n");
    return;
}
#define TREE_HEIGHT 4 
#define BLOCK_SIZE 1024
#define DATA_BLOCKS 8 


int main()
{
    int i;
    char *data[DATA_BLOCKS], *data_copy[DATA_BLOCKS], buffer[BLOCK_SIZE];
    
    // make sure TREE_HEIGHT fits DATA_BLOCKS...
    // BLOCK_SIZE & hash_size, hash_function also needed init
    merkle_tree TREE_1 = {0, TREE_HEIGHT, MD5_DIGEST_LENGTH, BLOCK_SIZE, DATA_BLOCKS, MD5One, NULL};
    printf("Enter length of string : ");
    int x;
    scanf("%d",&x);
    getchar();
    printf("enter string: ");
    
    //gets()
    char arr[x];
    gets(arr);

    for (i=0; i<x; i++)
        buffer[i] = arr[i];
    for(i=x;i<BLOCK_SIZE;i++){
        buffer[i]='a';
    }
    for (i=0; i<DATA_BLOCKS; i++) {
        data[i] = (char *)malloc(sizeof(char) * BLOCK_SIZE);
        data_copy[i] = (char *)malloc(sizeof(char) * BLOCK_SIZE);
        memcpy(data[i], buffer, BLOCK_SIZE);
        memcpy(data_copy[i], buffer, BLOCK_SIZE);
    }

    //build tree mt_a with data
    build_tree(&TREE_1, data);
    print_tree(&TREE_1);
    freeMerkleTree(&TREE_1);
    return 0;
}