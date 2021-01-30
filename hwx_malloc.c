#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "xmalloc.h"

typedef struct free_list_cell {
    size_t size;
    struct free_list_cell* next;
} free_list_cell;


const size_t PAGE_SIZE = 4096;
static free_list_cell* free_list = 0;
static pthread_mutex_t free_lock = PTHREAD_MUTEX_INITIALIZER;



static
size_t
div_up(size_t xx, size_t yy)
{
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;

    if (zz * yy == xx) {
        return zz;
    }
    else {
        return zz + 1;
    }
}

size_t
max(size_t a, size_t b)
{
    if (a > b) {
        return a;
    }
    return b;
}

free_list_cell*
find_first(size_t size) 
{
    free_list_cell* curr = free_list;
    free_list_cell* prev = 0;

    while (curr != 0) {
        if (curr->size >= size) {
            if (prev) {
                prev->next = curr->next;
            }
            else {
                free_list = 0;
            }
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }

    return 0;
}

void
coalesce(free_list_cell* prev, free_list_cell* next, size_t size, void* addr)
{
    if (prev == 0) {
        free_list = (free_list_cell*) addr;
        free_list->size = size;
        free_list->next = next;
    }
    else {
        prev->next = (free_list_cell*) addr;
        ((free_list_cell*)addr)->size = size;
        ((free_list_cell*)addr)->next = next;
    }

    free_list_cell* curr = (free_list_cell*) addr;

    if (prev && (char*) prev + prev->size - (char*) curr == 0) {
        prev->size = prev->size + curr->size;
        prev->next = curr->next;
        curr = prev;
    }
    if (next && (char*) next - curr->size - (char*) curr == 0) {
        curr->size = curr->size + next->size;
        curr->next = next->next;
    }
}

void
insert_free_cell(size_t size, void* addr) 
{
    free_list_cell* curr = free_list;
    free_list_cell* prev = 0;

    while (curr != 0 && (char*) curr - (char*) addr < 0) {
        prev = curr;
        curr = curr->next;
    }
    coalesce(prev, curr, size, addr);
}

void*
xmalloc(size_t size)
{
    int rv = pthread_mutex_lock(&free_lock);
    assert(rv == 0);

    size += sizeof(size_t);
    size = max(size, sizeof(free_list_cell));

    if (size < PAGE_SIZE) {

        void* block;
        size_t block_size;
        free_list_cell* free_block = find_first(size);
        if (free_block) {
            block = (void*) free_block;
            block_size = free_block->size;
        }
        else {
            block = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE,
                    MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            assert(block != (void*) -1);
            block_size = PAGE_SIZE;
        }
        
        if (block_size - size > sizeof(free_list_cell)) {
            insert_free_cell(block_size - size, (void*) ((char*) block + size));
            block_size = size;
        }

        *(size_t*)block = block_size;
        rv = pthread_mutex_unlock(&free_lock);
        assert(rv == 0);

        return (void*) ((char*) block + sizeof(size_t));
    }
    else {
        size_t num_pages = div_up(size, PAGE_SIZE);
        void* block = mmap(0, num_pages*PAGE_SIZE, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        assert(block != (void*) -1);
        *(size_t*)block = num_pages * PAGE_SIZE;

        rv = pthread_mutex_unlock(&free_lock);
        assert(rv == 0);

        return (void*) ((char*) block + sizeof(size_t));
    }

    rv = pthread_mutex_unlock(&free_lock);
    assert(rv == 0);

    return 0;
}

void
xfree(void* item)
{
    int rv = pthread_mutex_lock(&free_lock);
    assert(rv == 0);


    void* block = (void*) ((char*) item - sizeof(size_t));

    size_t block_size = *(size_t*)block;
    if (block_size < PAGE_SIZE) {
        insert_free_cell(block_size, block);
        
    }
    else {
        int rv = munmap(block, block_size);
        assert(rv == 0);
    }
    rv = pthread_mutex_unlock(&free_lock);
    assert(rv == 0);
}

void*
xrealloc(void* prev, size_t bytes)
{
    if (prev == NULL) {
        return xmalloc(bytes);
    }
    if (bytes == 0) {
        xfree(prev);
        return NULL;
    }
    
    void* old_block = (void*) ((char*) prev - sizeof(size_t));
    size_t old_size = *(size_t*) old_block;

    if (bytes + sizeof(size_t) < old_size) {
        return prev;
    }

    void* new_block = xmalloc(bytes);
    new_block = memmove(new_block, prev, bytes);
    xfree(prev);
    return new_block;
}
