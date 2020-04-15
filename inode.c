
#include <stdint.h>

#include "pages.h"
#include "inode.h"
#include "util.h"
#include "bitmap.h"

const int INODE_COUNT = 256;

inode*
get_inode(int inum)
{
    uint8_t* base = (uint8_t*) pages_get_page(0);
    inode* nodes = (inode*)(base);
    return &(nodes[inum]);
}

int
alloc_inode()
{
    void* ibm = get_inode_bitmap();
    for (int ii = 0; ii < INODE_COUNT; ++ii) {
        printf("ii: %d\n",bitmap_get(ibm,ii));
        if (!bitmap_get(ibm,ii)) {
            bitmap_put(ibm,ii,1);
            printf("ii: %d\n",bitmap_get(ibm,ii));
            printf("+ alloc_inode() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void
free_inode(int inum)
{
    printf("+ free_inode(%d)\n", inum);

    inode* node = get_inode(inum);
    memset(node, 0, sizeof(inode));
}

void
print_inode(inode* node)
{
    if (node) {
        printf("node{mode: %04o, size: %d}\n",
               node->mode, node->size);
    }
    else {
        printf("node{null}\n");
    }
}

int
inode_get_pnum(inode* node, int fpn)
{
    if(fpn==0 || fpn==1){
        return node->ptrs[fpn];
    }
    uint8_t* indirect = pages_get_page(node->iptr);
    int* arr_pnum = (int*)indirect;
    return arr_pnum[fpn-2];
}