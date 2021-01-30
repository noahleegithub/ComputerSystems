#include <errno.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "bitmap.h"
#include "directory.h"
#include "inode.h"

void 
directory_init()
{
    void* inbm = get_inode_bitmap();
    if (bitmap_get(inbm, 0) == 0) {
        int rnum = alloc_inode();
        assert(rnum == 0);
        inode* root = get_inode(rnum);
        root->refs = 1;
        root->mode = 040755;
        root->size = 0;
        root->pnums[0] = alloc_page();
    }
}

int 
directory_lookup(inode* dd, const char* name)
{
    int pnum = inode_get_pnum(dd, 0);
    void* page = pages_get_page(pnum);
    dirent* dir = (dirent*) page;
    for (int ii = 0; ii < dd->size; ++ii) {
        if (streq(dir[ii].name, name)) {
            return dir[ii].inum;
        }
    }
    return -ENOENT;
}

int 
tree_lookup(const char* path)
{
    if (streq(path, "/")) {
        return 0;
    }
    return directory_lookup(get_inode(0), path + 1);
}

int 
directory_put(inode* dd, const char* name, int inum)
{
    dd = get_inode(0);
    int pnum = inode_get_pnum(dd, 0);
    void* page = pages_get_page(pnum);
    dirent* dir = (dirent*) page;
    dirent* entry = &dir[dd->size];
    memcpy(entry->name, name + 1, strlen(name));
    entry->inum = inum;
    dd->size = dd->size + 1;
    return 0;
}

int 
directory_delete(inode* dd, const char* name)
{
    int pnum = inode_get_pnum(dd, 0);
    print_inode(dd);
    void* page = pages_get_page(pnum);
    dirent* dir = (dirent*) page;
    int compact = 0;

    for (int ii = 0; ii < dd->size; ++ii) {
        printf("%s %s\n", dir[ii].name, name);
        if (streq(dir[ii].name, name + 1)) {
            compact = 1;
            dd->size = dd->size - 1;
        }
        else {
            if (compact) {
                dir[ii - 1] = dir[ii];
            }
        }
    }

    if (compact) {
        return 0;
    }
    return -ENOENT;
}

slist* 
directory_list(const char* path)
{
   
    int inum = tree_lookup(path);
    inode* dd = get_inode(inum);
    int pnum = inode_get_pnum(dd, 0);
    printf("pnum %d\n", pnum);
    void* page = pages_get_page(1);
    dirent* dir = (dirent*) page;
    slist* list = 0;
    for (int ii = 0; ii < dd->size; ++ii) {
       printf("dlist %s\n", dir[ii].name);
       list = s_cons(dir[ii].name, list);
    }
    return list;
}

void 
print_directory(inode* dd)
{
   
    int pnum = inode_get_pnum(dd, 0);
    void* page = pages_get_page(pnum);
    dirent* dir = (dirent*) page;
  
    for (int ii = 0; ii < dd->size; ++ii) {
        printf("%s, %d\n", dir[ii].name, dir[ii].inum);       
    }
}
