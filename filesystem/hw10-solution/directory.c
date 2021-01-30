
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

#define ROOT_NODE 0

void
directory_init()
{
    inode* rn = get_inode(ROOT_NODE);

    if (rn->mode == 0) {
        rn->mode = 040755;
        rn->size = 0;
        rn->ents = 0;
        rn->pnum = alloc_page();
    }
}

char*
skip_string(char* data)
{
    while (*data != 0) {
        data++;
    }
    return data + 1;
}

int
directory_lookup(const char* name)
{
    inode* rn = get_inode(ROOT_NODE);
    char* data = inode_get_page(rn, 0);
    char* text = data;

    for (int ii = 0; ii < rn->ents; ++ii) {
        //printf(" ++ lookup '%s' =? '%s' (%p)\n", name, text, text);

        if (streq(text, name)) {
            text = skip_string(text);
            int* inum = (int*)(text);
            return *inum;
        }

        text = skip_string(text);
        text += 4;
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
    inode* rn = get_inode(ROOT_NODE);
    int nlen = strlen(name) + 1;
    if (rn->size + nlen + sizeof(inum) > 4096) {
        return -ENOSPC;
    }

    char* data = inode_get_page(rn, 0);
    memcpy(data + rn->size, name, nlen);
    printf("+ dirent = '%s'\n", name);
    rn->size += nlen;

    memcpy(data + rn->size, &inum, sizeof(inum));
    rn->size += sizeof(inum);

    rn->ents += 1;

    printf("+ directory_put(..., %s, %d) -> 0\n", name, inum);
    print_inode(rn);

    return 0;
}

int
directory_delete(const char* name)
{
    printf(" + directory_delete(%s)\n", name);
    inode* rn = get_inode(ROOT_NODE);
    char* data = inode_get_page(rn, 0);
    char* text = data;
    char* eend = 0;

    for (int ii = 0; ii < rn->ents; ++ii) {
        if (streq(text, name)) {
            goto delete_found;
        }

        text = skip_string(text);
        text += 4;
    }

    return -ENOENT;

delete_found:
    eend = skip_string(text);
    int inum = *((int*)eend);
    eend += sizeof(int);

    int epos = (int)(eend - data);
    memmove(text, eend, rn->size - epos);

    int elen = (int)(eend - text);
    rn->size -= elen;
    rn->ents -= 1;

    return 0;
}

slist*
directory_list()
{
    inode* rn = get_inode(ROOT_NODE);
    char* data = inode_get_page(rn, 0);
    char* text = data;

    printf("+ directory_list()\n");
    slist* ys = 0;

    for (int ii = 0; ii < rn->ents; ++ii) {
        char* name = text;
        text = skip_string(text);
        int pnum = *((int*) text);
        text += sizeof(int);

        printf(" - %d: %s [%d]\n", ii, name, pnum);
        ys = s_cons(name, ys);
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
