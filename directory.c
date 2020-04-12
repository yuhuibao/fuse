
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "directory.h"
#include "pages.h"
#include "slist.h"
#include "util.h"
#include "inode.h"

#define ENT_SIZE 16

void
directory_init()
{
    inode* rn = get_inode(2);

    if (rn->mode == 0) {
        rn->size = 0;
        rn->mode = 040755;
    }
}

char*
directory_get(int ii, int pnum)
{
    char* base = pages_get_page(pnum);
    return base + ii*sizeof(dirent);
}

int
directory_lookup(inode* dd, const char* name)
{
    int pnum = dd->ptrs[0];
    int dir_count = dd->size / sizeof(dirent);
    for (int ii = 0; ii < dir_count; ++ii) {
        dirent* ent = (dirent*)directory_get(ii,pnum);
        if (ent!=NULL && streq(ent->name, name)) {
            dd = get_inode(ent->inum);
            if(dd->mode & 0x40){
                const char* dir_name = name + strlen(ent->name) + 1;
                directory_lookup(dd,dir_name);
            }else{
                return ent->inum;
            }
        }
    }
    return -ENOENT;
}

int
tree_lookup(const char* path)
{
    assert(path[0] == '/');

    if (streq(path, "/")) {
        return 1;
    }

    return directory_lookup(get_inode(2),path + 1);
}

int
directory_put(inode* dd, const char* name, int inum)
{
    int pnum = dd->ptrs[0];
    int dir_count = dd->size / sizeof(dirent);
    char* end = directory_get(dir_count,pnum);
    dirent* ent = (dirent*)end;
    strcpy(ent->name, name);
    ent->inum = inum; 

    inode* node = get_inode(inum);
    printf("+ directory_put(..., %s, %d) -> 0\n", name, inum);
    print_inode(node);

    return 0;
}

int
directory_delete(inode* dd,const char* name)
{
    printf(" + directory_delete(%s)\n", name);

    int inum = directory_lookup(dd,name);
    free_inode(inum);

    char* ent = pages_get_page(1) + inum*ENT_SIZE;
    ent[0] = 0;

    return 0;
}

slist*
directory_list(const char* path)
{
    printf("+ directory_list()\n");
    slist* ys = 0;

    const char* name = path + 1;
    inode* node = get_inode(2);
    directory_lookup(node,name);
    //after calling directory lookup node is the current directory
    int pnum = node->ptrs[0];
    int dir_count = node->size / sizeof(dirent);
    for (int ii = 0; ii < dir_count; ++ii) {
        dirent* ent = (dirent*)directory_get(ii,pnum);
        if (ent) {
            printf(" - %d: %s [%d]\n", ii, ent->name, ii);
            ys = s_cons(ent->name, ys);
        }
    }

    return ys;
}


