
#include <stdint.h>
#include <assert.h>

#include "pages.h"
#include "inode.h"
#include "util.h"

const int INODE_COUNT = 64;

inode*
get_inode(int inum)
{
    assert(inum < INODE_COUNT);
    uint8_t* base = (uint8_t*) pages_get_page(0);
    inode* nodes = (inode*)(base);
    return &(nodes[inum]);
}

int
alloc_inode()
{
    for (int ii = 0; ii < INODE_COUNT; ++ii) {
        inode* node = get_inode(ii);
        if (node->mode == 0) {
            memset(node, 0, sizeof(inode));
            node->mode = 010644;
            node->size = 0;
            node->ents = 0;
            node->pnum = alloc_page();
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
    free_page(node->pnum);
    memset(node, 0, sizeof(inode));
}

void
print_inode(inode* node)
{
    if (node) {
        printf("node{mode: %04o, size: %d, ents: %d, inum: %d}\n",
               node->mode, node->size, node->ents, node->pnum);
    }
    else {
        printf("node{null}\n");
    }
}

// fpn is file page number; unused for hw10
int
inode_get_pnum(inode* node, int _fpn)
{
    return node->pnum;
}

void*
inode_get_page(inode* node, int fpn)
{
    return pages_get_page(inode_get_pnum(node, fpn));
}
