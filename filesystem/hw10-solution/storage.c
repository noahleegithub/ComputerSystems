
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

void
storage_init(const char* path)
{
    pages_init(path);
    directory_init();
}

int
storage_stat(const char* path, struct stat* st)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        printf("+ storage_stat(%s) -> %d\n", path, inum);
        return inum;
    }

    inode* node = get_inode(inum);
    printf("+ storage_stat(%s) -> 0; inode %d\n", path, inum);
    print_inode(node);

    memset(st, 0, sizeof(struct stat));
    st->st_uid   = getuid();
    st->st_mode  = node->mode;
    st->st_size  = node->size;
    st->st_nlink = 1;
    return 0;
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }
    inode* node = get_inode(inum);
    printf("+ storage_read(%s); inode %d\n", path, inum);
    print_inode(node);

    if (offset >= node->size) {
        return 0;
    }

    if (offset + size >= node->size) {
        size = node->size - offset;
    }

    int pnum = inode_get_pnum(node, 0);
    uint8_t* data = pages_get_page(pnum);
    printf(" + reading from page: %d\n", pnum);
    memcpy(buf, data + offset, size);

    return size;
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
    int trv = storage_truncate(path, offset + size);
    if (trv < 0) {
        return trv;
    }

    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    int pnum = inode_get_pnum(node, 0);
    uint8_t* data = pages_get_page(pnum);

    printf("+ writing to page: %d\n", pnum);
    memcpy(data + offset, buf, size);

    return size;
}

int
storage_truncate(const char *path, off_t size)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    node->size = size;
    return 0;
}

int
storage_mknod(const char* path, int mode)
{
    char* tmp1 = alloca(strlen(path));
    char* tmp2 = alloca(strlen(path));
    strcpy(tmp1, path);
    strcpy(tmp2, path);

    const char* name = path + 1;

    if (directory_lookup(name) != -ENOENT) {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    int    inum = alloc_inode();
    inode* node = get_inode(inum);
    node->mode = mode;
    node->size = 0;
    node->ents = 0;
    node->pnum = alloc_page();

    printf("+ mknod create %s [%04o] - #%d\n", path, mode, inum);

    return directory_put(name, inum);
}

slist*
storage_list(const char* path)
{
    return directory_list(path);
}

int
storage_unlink(const char* path)
{
    const char* name = path + 1;

    int inum = directory_lookup(name);
    free_inode(inum);
    return directory_delete(name);
}

int
storage_link(const char* from, const char* to)
{
    return -ENOENT;
}

int
storage_rename(const char* from, const char* to)
{
    int inum = directory_lookup(from + 1);
    int rv = directory_put(to + 1, inum);
    if (rv != 0) {
        return rv;
    }
   
    return directory_delete(from + 1);
}

int
storage_set_time(const char* path, const struct timespec ts[2])
{
    // Maybe we need space in a inode for timestamps.
    return 0;
}
