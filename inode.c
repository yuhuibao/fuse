
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
        //printf("ii: %d\n",bitmap_get(ibm,ii));
        if (!bitmap_get(ibm,ii)) {
            bitmap_put(ibm,ii,1);
            //printf("ii: %d\n",bitmap_get(ibm,ii));
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
    int num;
    if(num = node->ptrs[0]){
        free_page(num);
    }
    if(num = node->ptrs[1]){
        free_page(num);
    }
    if(num = node->iptr){
        free_page(num);
    }
    
    
    void* ibm = get_inode_bitmap();
    bitmap_put(ibm,inum,0);
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
    if(fpn==0){
        if(node->ptrs[0] == 0){

            node->ptrs[0] = alloc_page();
        }
        return node->ptrs[0];
    }
    if(fpn==1){
        if(node->ptrs[1] == 0){
            node->ptrs[1] = alloc_page();
        }
        return node->ptrs[1];
    }
    if(node->iptr == 0){
        node->iptr = alloc_page();
    }
    uint8_t* indirect = pages_get_page(node->iptr);
    int* arr_pnum = (int*)indirect;
    if(arr_pnum[fpn-2]==0){
        arr_pnum[fpn-2]=alloc_page();
    }
    return arr_pnum[fpn-2];
}