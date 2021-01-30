#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "slist.h"
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
        return inum;
    }
    inode* inode = get_inode(inum);
    if (st) {
        st->st_mode = inode->mode;
        st->st_size = inode->size;
        st->st_uid = getuid();
    }
    return 0;
}

int    
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    int inum = tree_lookup(path);
    inode* inode = get_inode(inum);
    void* data = pages_get_page(inode->pnums[0]);
    memcpy(buf, (void*)((uintptr_t)data + offset), size);
    return size;
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
    int inum = tree_lookup(path);
    inode* inode = get_inode(inum);
    void* data = pages_get_page(inode->pnums[0]);
    memcpy((void*)((uintptr_t)data + inode->size), 
            (void*)((uintptr_t)buf + offset), size);
    inode->size += size;
    return size;
}

int
storage_truncate(const char *path, off_t size)
{
    int inum = tree_lookup(path);
    inode* inode = get_inode(inum);
    inode->size = size;
    return size;
}

int 
storage_mknod(const char* path, int mode)
{
    int inum = alloc_inode();
    int pnum = alloc_page();
    inode* inode = get_inode(inum);
    inode->refs = 1;
    inode->mode = mode;
    inode->size = 0;
    inode->pnums[0] = pnum;
    inode->iptr = 0;

    return directory_put(get_inode(0), path, inum);
}

int  
storage_unlink(const char* path)
{
    return directory_delete(get_inode(0), path);
}

int  
storage_link(const char *from, const char *to)
{
    return -ENOSYS;
}

int  
storage_rename(const char *from, const char *to)
{
    int inum = tree_lookup(from);

    int rv = directory_delete(get_inode(0), from);
    assert(rv == 0);

    return directory_put(get_inode(0), to, inum);

 }

int  
storage_set_time(const char* path, const struct timespec ts[2])
{
    return -ENOSYS;
}

slist*
storage_list(const char* path)
{
    printf(path);
    return directory_list(path);
}
