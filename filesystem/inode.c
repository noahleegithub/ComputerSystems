#include <stdint.h>

#include "inode.h"
#include "pages.h"
#include "bitmap.h"

/*
typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int pnums[5]; // direct pointers
    int iptr; // single indirect pointer
} inode;
*/

const int INODE_COUNT = 64;

void 
print_inode(inode* node)
{
    if (node) {
        printf("node: refs=%d, mode=%d, size=%d, addr=%p\n", node->refs, 
                node->mode, node->size, node);
    }
    else {
        printf("node: nul\n");
    }
}

inode* 
get_inode(int inum)
{
    uint8_t* page = pages_get_page(0);
    inode* nodes = (inode*) (page + 40);
    return &nodes[inum];
}

int 
alloc_inode()
{
    void* inbm = get_inode_bitmap();

    for (int ii = 0; ii < INODE_COUNT; ++ii) {
        if (!bitmap_get(inbm, ii)) {
            bitmap_put(inbm, ii, 1);
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
    void* inbm = get_inode_bitmap();
    bitmap_put(inbm, inum, 0);
}

int 
grow_inode(inode* node, int size)
{
    if (node) {
        node->size = node->size + size;
        return size;
    }
    return -1;
}

int 
shrink_inode(inode* node, int size)
{
    if (node) {
        node->size = node->size - size;
        return size;
    }
    return -1;
}

int 
inode_get_pnum(inode* node, int fpn)
{
    if (node) {
        return node->pnums[fpn];
    }
    return -1;
}
