#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "xmalloc.h"

/* STRUCT DEFINITIONS */
typedef struct free_ptrs {
    void* first;
    struct free_ptrs* rest;
} free_ptrs;

typedef struct span_header {
    size_t bin_size;
    pthread_mutex_t span_mutex;
    int blocks_in_use;
    free_ptrs* alloc_ptr;
    struct span_header* next_span;
    struct span_header* prev_span;
} span_header;

/* CONSTANT DEFINITIONS */
const size_t PAGE_SIZE = 4096;
const size_t SPAN_SIZE = 16 * PAGE_SIZE;
const size_t LSPAN_SIZE = 64 * PAGE_SIZE;
const size_t NUM_BINS = 16;
const long bin_sizes[16] = {16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072};

/* DATA STRUCTURES */
/* THREAD SHARED */
static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static span_header* global_free_spans = 0;

/* THREAD LOCAL */
static __thread span_header* local_free_spans = 0;
/* Thread local bin data structure. Each bin contains active spans that allow
 * for O(1) malloc and free.
 */
static __thread span_header* bins[16] = {0};

static
int
max(size_t a, size_t b)
{
    if (a > b) {
        return a;
    }
    return b;
}

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



int
compute_bin(size_t size)
{
    for (int ii = 0; ii < NUM_BINS; ++ii) {
        if (size <= bin_sizes[ii]) {
            return ii;
        }
    }
    return -1;
}

/* Iterate through spans, check if they are able to accomodate an allocation
 * of size bytes, if so, reinsert at front and return span
 */
span_header*
search_active_spans(size_t size, span_header* head) 
{
    int rv;
    for (span_header* span = head; span != NULL; span = span->next_span) {
        rv = pthread_mutex_lock(&(span->span_mutex));
        assert(rv == 0);
    }
    span_header* prev = head; 
    span_header* span = head;
    while (span != NULL) {
        
        if (span->blocks_in_use * span->bin_size + sizeof(span_header) + size
                < SPAN_SIZE) {
            if (span == head) {
                for (span_header* tmp = head; tmp != NULL; 
                        tmp = tmp->next_span) {
                    rv = pthread_mutex_unlock(&(tmp->span_mutex));
                    assert(rv == 0);
                }
                return span;
            }
            else {
                prev->next_span = span->next_span;
                if (span->next_span) {
                    span->next_span->prev_span = prev;
                }
                span->next_span = head;
                span->prev_span = 0;
                for (span_header* tmp = head; tmp != NULL; 
                        tmp = tmp->next_span) {
                    rv = pthread_mutex_unlock(&(tmp->span_mutex));
                    assert(rv == 0);
                }
                return span;
            }
        }
        prev = span;
        span = span->next_span;
    }

    for (span_header* span = head; span != NULL; span = span->next_span) {
        rv = pthread_mutex_unlock(&(span->span_mutex));
        assert(rv == 0);
    }

    return NULL;
}

void*
alloc_from_span(span_header* span)
{
    int rv = pthread_mutex_lock(&(span->span_mutex));
    assert(rv == 0);
    void* user_ptr = span->alloc_ptr->first;
    if (!span->alloc_ptr->rest) {
        span->alloc_ptr = (free_ptrs*)((char*)(span->alloc_ptr->first)
                + span->bin_size);
        span->alloc_ptr->first = (void*)(span->alloc_ptr);
        span->alloc_ptr->rest = 0;   
    }
    else {        
        span->alloc_ptr = span->alloc_ptr->rest;
    }
    span->blocks_in_use += 1;
    rv = pthread_mutex_unlock(&(span->span_mutex));
    assert(rv == 0);
    

    return user_ptr;
}


void*
xmalloc(size_t size)
{
   // printf("malloc %ld\n", size); 
    int rv;
    size = max(size, sizeof(free_ptrs));
    if (size < bin_sizes[NUM_BINS - 1]) {
        int bin_idx = compute_bin(size);
        span_header* span = bins[bin_idx];

        span = search_active_spans(size, span);
        
        if (span != NULL) {
            // do the allocation
            return alloc_from_span(span);
        }
        else {
            // look in local and global
            if (local_free_spans) {
                printf("local: %p, %ld, %d\n", local_free_spans,
                        local_free_spans->bin_size, local_free_spans->blocks_in_use);
                span = local_free_spans;
                local_free_spans = local_free_spans->next_span;

                rv = pthread_mutex_lock(&(span->span_mutex));
              
                assert(rv == 0);
                
                span->bin_size = bin_sizes[bin_idx];
                free_ptrs* ptr = (free_ptrs*)((char*)span 
                            + sizeof(span_header)
                            + span->bin_size);
                ptr->first = (void*) ptr;
                ptr->rest = 0;

                span->alloc_ptr = ptr;

                               
                span->prev_span = 0;
                span->next_span = 0;

                span->blocks_in_use = 0;

                rv = pthread_mutex_unlock(&(span->span_mutex));
                assert(rv == 0);

                bins[bin_idx] = span;
                return alloc_from_span(span);

            }
            else {
                rv = pthread_mutex_trylock(&global_mutex);
                if (rv == 0) {
                    if (global_free_spans) {
                        span = global_free_spans;
                        global_free_spans = global_free_spans->next_span;
                        rv = pthread_mutex_unlock(&global_mutex);
                        assert(rv == 0);
                        bins[bin_idx] = span;
                        return alloc_from_span(span);
                    }
                    else {
                        rv = pthread_mutex_unlock(&global_mutex);
                        assert(rv == 0);
                        void* block = mmap(0, 2*SPAN_SIZE, 
                                PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS
                                , -1, 0);
                        assert(block != (void*) -1);
                        
                        block = (void*)((char*)block 
                                + (SPAN_SIZE - ((uintptr_t)block % SPAN_SIZE)));

                        span_header* span = (span_header*) block;
                        span->bin_size = bin_sizes[bin_idx];
                        span->blocks_in_use = 1;
                        
                        rv = pthread_mutex_init(&(span->span_mutex), 0);
                        assert(rv == 0);

                        free_ptrs* ptr = (free_ptrs*)((char*)span 
                                + sizeof(span_header)
                                + span->bin_size);
                        ptr->first = (void*) ptr;
                        ptr->rest = 0;

                        span->alloc_ptr = ptr;
                       
 
                        span->next_span = 0;
                        span->prev_span = 0;
                        bins[bin_idx] = span;
                        return (void*)((char*)(span->alloc_ptr->first) 
                                - span->bin_size);
                    }
                }
                else {
                    void* block = mmap(0, 2*SPAN_SIZE, PROT_READ|PROT_WRITE, 
                            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
                    assert(block != (void*) -1);

                    block = (void*)((char*)block 
                                + (SPAN_SIZE - ((uintptr_t)block % SPAN_SIZE)));

                    span_header* span = (span_header*) block;
                    span->bin_size = bin_sizes[bin_idx];
                    span->blocks_in_use = 1;
                    
                    rv = pthread_mutex_init(&(span->span_mutex), 0);
                    assert(rv == 0);

                    free_ptrs* ptr = (free_ptrs*)((char*)span 
                            + sizeof(span_header)
                            + span->bin_size);
                    ptr->first = (void*) ptr;
                    ptr->rest = 0;


                    span->alloc_ptr = ptr;
                    span->next_span = 0;
                    span->prev_span = 0;
                    bins[bin_idx] = span;
                    return (void*)((char*)(span->alloc_ptr->first) 
                            - span->bin_size);

                }
            }
        } 
    }
    else {
        // Large allocation, just mmap and put a size header
        size += sizeof(span_header);
        size_t num_pages = div_up(size, SPAN_SIZE);
        void* block = mmap(0, 2*num_pages*SPAN_SIZE, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        assert(block != (void*) -1);

        block = (void*)((char*)block 
                + (SPAN_SIZE - ((uintptr_t)block % SPAN_SIZE)));

        span_header* span = (span_header*) block;
        span->bin_size = num_pages * SPAN_SIZE;
        span->blocks_in_use = 1;
        span->alloc_ptr = 0;
        span->next_span = 0;
        span->prev_span = 0;
        return (void*) ((char*) block + sizeof(span_header));

    }

}

/*
 *typedef struct span_header {
    size_t bin_size;
    pthread_mutex_t span_mutex;
    int blocks_in_use;
    free_ptrs* alloc_ptr;
    struct span_header* next_span;
    struct span_header* prev_span;
} span_header;*/
void
xfree(void* item)
{
    //void* block = (void*)(char*)(((uintptr_t)item / SPAN_SIZE) * SPAN_SIZE);
      
    int rv;
    void* block = (void*)((char*)item - (char*) ((uintptr_t)item % SPAN_SIZE));
    span_header* span_info = (span_header*) block;
    
    if (span_info->bin_size > bin_sizes[NUM_BINS - 1]) {
        rv = munmap(block, span_info->bin_size);
        assert(rv == 0);
        return;
    }

    rv = pthread_mutex_lock(&(span_info->span_mutex));
    assert(rv == 0);

    span_info->blocks_in_use = span_info->blocks_in_use - 1;
    
    if (span_info->blocks_in_use == -190) { 
        printf("%p\n", span_info->prev_span);
        if (span_info->prev_span) {
            printf("%p\n", span_info->prev_span->next_span);
        }
        printf("\n");
        if (local_free_spans == 0) {
            rv = pthread_mutex_unlock(&(span_info->span_mutex));
            assert(rv == 0);
            local_free_spans = span_info;
            return;
        }
        else {
            span_info->next_span = local_free_spans;
            rv = pthread_mutex_unlock(&(span_info->span_mutex));
            assert(rv == 0);
            local_free_spans = span_info;
            return;
        }
    }

    free_ptrs* free = (free_ptrs*) item;
    free->first = (void*) free;
    free->rest = span_info->alloc_ptr;
    span_info->alloc_ptr = free;

    rv = pthread_mutex_unlock(&(span_info->span_mutex));
    assert(rv == 0);
    return;
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

    void* block = (void*)((char*)prev - (char*) ((uintptr_t)prev % SPAN_SIZE));
    span_header* span_info = (span_header*) block;
    
    if (bytes <= span_info->bin_size) {
        return prev;
    }

    void* new_ptr = xmalloc(bytes);
    new_ptr = memmove(new_ptr, prev, bytes);
    xfree(prev);
    return new_ptr;
}
