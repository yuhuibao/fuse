
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
        if (streq(ent->name, name)) {
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
    ent->name = name;
    ent->inum = inum; 

    inode* node = get_inode(inum);
    printf("+ directory_put(..., %s, %d) -> 0\n", name, inum);
    print_inode(node);

    return 0;
}

int
directory_delete(const char* name)
{
    printf(" + directory_delete(%s)\n", name);

    int inum = directory_lookup(name);
    free_inode(inum);

    char* ent = pages_get_page(1) + inum*ENT_SIZE;
    ent[0] = 0;

    return 0;
}

slist*
directory_list()
{
    printf("+ directory_list()\n");
    slist* ys = 0;

    for (int ii = 0; ii < 256; ++ii) {
        char* ent = directory_get(ii);
        if (ent[0]) {
            printf(" - %d: %s [%d]\n", ii, ent, ii);
            ys = s_cons(ent, ys);
        }
    }

    return ys;
}

void
print_directory(inode* dd)
{
    printf("Contents:\n");
    slist* items = directory_list(dd);
    for (slist* xs = items; xs != 0; xs = xs->next) {
        printf("- %s\n", xs->data);
    }
    printf("(end of contents)\n");
    s_free(items);
}
