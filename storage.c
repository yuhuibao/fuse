
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <libgen.h>
#include <bsd/string.h>
#include <stdint.h>

#include "storage.h"
#include "slist.h"
#include "util.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"

void storage_init(const char *path, int create)
{
    //printf("storage_init(%s, %d);\n", path, create);
    pages_init(path, create);
    if (create)
    {
        root_init();
    }
}

int storage_stat(const char *path, struct stat *st)
{
    printf("+ storage_stat(%s)\n", path);
    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }

    inode *node = get_inode(inum);
    printf("+ storage_stat(%s); inode %d\n", path, inum);
    print_inode(node);
    
    memset(st, 0, sizeof(struct stat));
    st->st_uid = getuid();
    st->st_mode = node->mode;
    st->st_size = node->size;
    st->st_nlink = 1;
    
    return 0;
}

int storage_read(const char *path, char *buf, size_t size, off_t offset)
{
    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }
    inode *node = get_inode(inum);
    printf("+ storage_read(%s); inode %d\n", path, inum);
    print_inode(node);

    if (offset >= node->size)
    {
        return 0;
    }

    if (offset + size >= node->size)
    {
        size = node->size - offset;
    }
    if (size <= 4096)
    {
        int pnum = inode_get_pnum(node, 0);
        uint8_t *data = pages_get_page(pnum);
        printf(" + reading from page: %d\n", pnum);
        memcpy(buf, data + offset, size);
    }
    else
    {
        int fpn_start, fpn_end;
        fpn_start = offset / 4096;
        int pnum_start = inode_get_pnum(node, fpn_start);
        uint8_t *data_s = pages_get_page(pnum_start);
        printf(" + reading from page: %d at %d\n", fpn_start, pnum_start);
        memcpy(buf, data_s + offset % 4096, 4096 - offset % 4096);
        int num_mid = fpn_end - fpn_start - 1;
        for (int i = 0; i < num_mid; i++)
        {
            int fpn_i = fpn_start + i + 1;
            int pnum_i = inode_get_pnum(node, fpn_i);
            uint8_t *data = pages_get_page(pnum_i);
            printf(" + reading from page: %d at %d\n", fpn_i, pnum_i);
            memcpy(buf, data, 4096);
        }
        fpn_end = (offset + size) / 4096;
        int pnum_end = inode_get_pnum(node, fpn_end);
        uint8_t *data_e = pages_get_page(pnum_end);
        printf(" + reading from page: %d at %d\n", fpn_end, pnum_end);
        memcpy(buf, data_e, size - 4096 - offset % 4096 - 4096 * num_mid);
    }

    return size;
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset)
{
    int trv = storage_truncate(path, offset + size);
    if (trv < 0)
    {
        return trv;
    }

    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }

    inode *node = get_inode(inum);
    
    if (offset + size <= 4096)
    {
        int pnum = inode_get_pnum(node, 0);
        uint8_t *data = pages_get_page(pnum);
        printf(" + writing to page: %d\n", pnum);
        memcpy(data + offset, buf, size);
    }
    else
    {
        int fpn_start, fpn_end;
        fpn_start = offset / 4096;
        int pnum_start = inode_get_pnum(node, fpn_start);
        uint8_t *data_s = pages_get_page(pnum_start);
        printf(" + writing to page: %d at %d\n", fpn_start, pnum_start);
        memcpy(data_s + offset % 4096, buf, 4096 - offset % 4096);
        int num_mid = fpn_end - fpn_start - 1;
        for (int i = 0; i < num_mid; i++)
        {
            int fpn_i = fpn_start + i + 1;
            int pnum_i = inode_get_pnum(node, fpn_i);
            uint8_t *data = pages_get_page(pnum_i);
            printf(" + writing to page: %d at %d\n", fpn_i, pnum_i);
            memcpy(data, buf, 4096);
        }
        fpn_end = (offset + size) / 4096;
        int pnum_end = inode_get_pnum(node, fpn_end);
        uint8_t *data_e = pages_get_page(pnum_end);
        printf(" + writing to page: %d at %d\n", fpn_end, pnum_end);
        memcpy(data_e, buf, size - 4096 - offset % 4096 - 4096 * num_mid);
    }
    return size;
}

int storage_truncate(const char *path, off_t size)
{
    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }

    inode *node = get_inode(inum);
    node->size = size;
    return 0;
}

int storage_mknod(const char *path, int mode)
{
    /* char *tmp1 = alloca(strlen(path));
    char *tmp2 = alloca(strlen(path));
    strcpy(tmp1, path);
    strcpy(tmp2, path);
 */
    slist* plist = s_split(path,'/');
    
    const char *name = path + 1;

    if (directory_lookup(name) != -ENOENT)
    {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    int inum = alloc_inode();
    inode *node = get_inode(inum);
    node->mode = mode;
    node->size = 0;
    node->refs = 1;
    /* int rv = time_now(node->time);
    if(rv){
        return rv;
    }
    printf("timens([%ld, %ld; %ld %ld])\n",node->time->tv_sec,node->time->tv_nsec); */

    printf("+ mknod create %s [%04o] - #%d\n", path, mode, inum);

    return directory_put(name, inum);
}
char* search_helper(slist* plist)
{
    plist = plist->next;
    inode* parent = get_inode(0);
    inode* find;
    while(parent->mode & 0070000 == 0040000){
        int inum = directory_lookup(parent,plist->data);
        if(inum < 0){
            return parent;
        }
        find = get_inode(inum);
        parent = find;
    }
    
    
}

int storage_mkdir(const char *path, int mode)
{
    char *tmp1 = alloca(strlen(path));
    char *tmp2 = alloca(strlen(path));
    strcpy(tmp1, path);
    strcpy(tmp2, path);

    const char *name = path + 1;

    if (directory_lookup(name) != -ENOENT)
    {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    int inum = directory_init(mode);

    printf("+ mkdir create %s [%04o] - #%d\n", path, mode, inum);
    

    return directory_put(name, inum);
}

slist*
storage_list(const char *path)
{
 
    slist* s=directory_list(path);
    //printf("s is %p\n",s);
    return s;
    
}

int storage_unlink(const char *path)
{
    const char *name = path + 1;
    return directory_delete(name);
}

int storage_rmdir(const char* path){
    const char *name = path + 1;
    int inum = directory_lookup(name);
    inode* node = get_inode(inum);
    if(node->mode & 040000){
        if(node->ptrs[0]){
            return -ENOTEMPTY;
        }
    }
    return directory_delete(name);
}

int storage_link(const char *from, const char *to)
{
    int inum = directory_lookup(from);
    if(inum < 0){
        return -ENOENT;
    }
    inode* node = get_inode(inum);
    node->refs += 1;
    directory_put(to, inum);
    return 0;
}

int storage_symlink(const char* target, const char* path)
{
    int rv = storage_mknod(path,0120000);
    if(rv < 0){
        return rv;
    }
    int size = storage_write(path,target, strlen(target),0);
    printf("write size is %d\n",size);
    return 0;
}

int storage_readlink(const char* path, char* buf, size_t size)
{
    //printf("Here is storage_readlin\n");
    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }
    inode* node = get_inode(inum);
    int pnum = inode_get_pnum(node, 0);
    uint8_t *data = pages_get_page(pnum);
    printf(" + reading from page: %d\n", pnum);
    memcpy(buf, data, size);
    //buf[rv] = '\0';
    return 0;
}

int storage_chmod(const char* path, mode_t mode)
{
    int inum = tree_lookup(path);
    if (inum < 0)
    {
        return inum;
    }
    inode* node = get_inode(inum);
    printf("chmode is %04o\n",mode);
    node->mode = (node->mode & 0770000) + (mode & 0007777);
    printf("new mode is %04o\n",node->mode);
    return 0;

}
int storage_rename(const char *from, const char *to)
{
    int rv = directory_rename(from + 1, to + 1);

    return rv;
}

int storage_set_time(const char *path, const struct timespec ts[2])
{
    
    return 0;
}
