
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
root_init()
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

int
directory_init(mode_t mode)
{
    
    int inum = alloc_inode();
    inode *rn = get_inode(inum); 
    

    if (rn->mode == 0) {
        
        rn->size = 0;
        rn->mode = mode + 040000;
        rn->refs = 1;
        rn->ptrs[0] = alloc_page();
        //printf("mode for inode %d is %d\n",inum,rn->mode);
    }
    return inum;
}

dirent*
directory_get(inode* dd, int ii)
{
    char* base = pages_get_page(dd->ptrs[0]);
    return (dirent*)(base + ii*sizeof(dirent));
}

int
directory_rename(const char* from,const char* to)
{
    inode* root = get_inode(0);
    int dirent_count = root->size/32;
    for (int ii = 0; ii < dirent_count; ++ii) {
        dirent* ent = directory_get(ii);
        if (streq(ent->name, from)) {
            strcpy(ent->name, to);
            return 0;
        }
    }
    return -ENOENT;
}
int
directory_lookup(inode* dd, const char* name)
{
    //inode* root = get_inode(0);
    int dirent_count = dd->size/32;
    for (int ii = 0; ii < dirent_count; ++ii) {
        dirent* ent = directory_get(dd,ii);
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
directory_put(inode* dd, const char* name, int inum)
{
    int pnum = dd->ptrs[0];
    char* base = pages_get_page(pnum);
    //inode* root = get_inode(0);
    //add new dirent at the end
    char* new = base + dd->size/32 * sizeof(dirent);
    dirent* ent = (dirent*) new;
    strcpy(ent->name, name);
    ent->inum = inum;
    printf("+ dirent = '%s', %d\n", ent->name,ent->inum);
    dd->size += 32;

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
    inode* node = get_inode(inum);
    node->refs -= 1;
    if(node->refs == 0){
        free_inode(inum);
    }
    

    char* base = pages_get_page(1);
    inode* root = get_inode(0);
    int dirent_count = root->size/32;
    char* ent;
    for (int ii = 0; ii < dirent_count; ++ii) {
        dirent* ent = (dirent*)(base + ii*sizeof(dirent));
        if (streq(ent->name, name)) {
            printf("delete dirent %s at index %d\n",name,ii);
            ent_delete(ii,dirent_count);
            root->size -= 32;
            return 0;
        }
    }

    return -ENOENT;
}
void ent_delete(int i, int length){
    char* base = pages_get_page(1);
    dirent *ent0,*ent1;
    for(int j = i+1; j < length; j++){
        ent0 = (dirent*)(base + (j-1)*sizeof(dirent));
        ent1 = (dirent*)(base + j*sizeof(dirent));
        ent0->inum = ent1->inum;
        strcpy(ent0->name,ent1->name);
    }
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
