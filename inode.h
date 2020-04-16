#ifndef INODE_H
#define INODE_H

#include "pages.h"

typedef struct inode {
    int mode; // permission & type; zero for unused
    int size; // bytes
    int refs; // reference count
    // inode #x always uses data page #x
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
    struct timespec* time;
    struct timespec* mtime;
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode();
int inode_get_pnum(inode* node, int fpn);

#endif
