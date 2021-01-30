#ifndef INODE_H
#define INODE_H

#include "pages.h"

typedef struct inode {
    int mode; // permission & type; zero for unused
    int size; // bytes
    int pnum; // page number
    int ents; // Number of directory entries; unused for non-dirs.
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int inode_get_pnum(inode* node, int fpn);
void* inode_get_page(inode* node, int fpn);

#endif
