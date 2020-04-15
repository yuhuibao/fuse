
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
#include "bitmap.h"

#define DIRENT_COUNT_MAX 128

void
directory_init()
{
    //inode for root is 0=inum
    int inum = alloc_inode();
    inode *rn = get_inode(inum); 
    

    if (rn->mode == 0) {
        
        rn->size = 0;
        rn->mode = 040755;
        rn->ptrs[0] = 1;
        void* pbm = get_pages_bitmap();
        bitmap_put(pbm,1,1);
        //printf("mode for inode %d is %d\n",inum,rn->mode);
    }
}

dirent*
directory_get(int ii)
{
    char* base = pages_get_page(1);
    return (dirent*)(base + ii*sizeof(dirent));
}

int
directory_lookup(const char* name)
{
    inode* root = get_inode(0);
    int dirent_count = root->size/32;
    for (int ii = 0; ii < dirent_count; ++ii) {
        dirent* ent = directory_get(ii);
        if (streq(ent->name, name)) {
            return ent->inum;
        }
    }
    return -ENOENT;
}

int
tree_lookup(const char* path)
{
    assert(path[0] == '/');

    if (streq(path, "/")) {
        return 0;
    } 

    return directory_lookup(path + 1);
}

int
directory_put(const char* name, int inum)
{
    char* base = pages_get_page(1);
    inode* root = get_inode(0);
    //add new dirent at the end
    char* new = base + root->size/32 * sizeof(dirent);
    dirent* ent = (dirent*) new;
    strcpy(ent->name, name);
    ent->inum = inum;
    printf("+ dirent = '%s', %d\n", ent->name,ent->inum);
    root->size += 32;

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

    char* ent = pages_get_page(1) + inum*16;
    ent[0] = 0;

    return 0;
}

slist*
directory_list()
{
    printf("+ directory_list()\n");
    slist* ys = 0;
    inode* root = get_inode(0);
    int dirent_count = root->size/32;
    for (int ii = 0; ii < dirent_count; ++ii) {
        dirent* ent = directory_get(ii);
        if (ent && ent->name[0]) {
            printf(" - %d: %s [%d]\n", ii, ent->name, ii);
            ys = s_cons(ent->name, ys);
            
        }
    }
    printf("ys is %p\n",ys);
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
