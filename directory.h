// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME 28

#include "slist.h"
#include "pages.h"
#include "inode.h"

typedef struct dirent {
    char name[DIR_NAME];
    int  inum;
    
} dirent;

void directory_init();
int directory_lookup( const char* name);
int tree_lookup(const char* path);
int directory_put(const char* name, int inum);
int directory_delete(const char* name);
slist* directory_list();
void print_directory(inode* dd);
dirent* directory_get(int ii);
void ent_delete(int i, int length);
int directory_rename(const char* from,const char* to);
#endif

