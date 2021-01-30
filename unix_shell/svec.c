/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
/* Editted by Noah Lee for HW04 and CH01*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

svec*
make_svec()
{
    // Allocate a svec with initial capacity 4 and return the pointer.
    svec* sv = malloc(sizeof(svec));
    sv->size = 0;
    sv->capacity = 4;
    sv->data = malloc(sv->capacity * sizeof(char*));
    return sv;
}

void
free_svec(svec* sv)
{
    // Free all memory for an svec.
    for (int ii = 0; ii < sv->size; ++ii) {
        free(sv->data[ii]);
    }
    free(sv->data);
    free(sv);
}

char*
svec_get(svec* sv, int ii)
{
    // Get the iith element (string) of an svec.
    assert(ii >= 0 && ii < sv->size);
    return sv->data[ii];
}

void
svec_put(svec* sv, int ii, char* item)
{
    // Insert a string into the iith space in an svec.
    assert(ii >= 0 && ii < sv->size);
    sv->data[ii] = strdup(item);
}

void
svec_push_back(svec* sv, char* item)
{
    // Double the capacity of an svec by reallocating memory.
    int ii = sv->size;

    if (sv->size == sv->capacity) {
        sv->capacity = sv->capacity * 2;
        void* tmp = realloc(sv->data, sv->capacity * sizeof(char*));
        if (tmp == NULL) {
            return; // Out of memory
        }
        sv->data = tmp;
    }
    sv->size = ii + 1;
    svec_put(sv, ii, item);
}

void
svec_swap(svec* sv, int ii, int jj)
{
    // Swap two elements ii and jj of an svec.
    char* temp_str = sv->data[ii];
    sv->data[ii] = sv->data[jj];
    sv->data[jj] = temp_str;
}

void
print_svec(svec* sv) 
{
    for (int ii = 0; ii < sv->size; ++ii) {
        printf("%s\n", svec_get(sv, ii));
    }
    printf("\n");
}

svec*
rev_free_svec(svec* sv)
{
    svec* new_sv = make_svec();
    for (int ii = sv->size - 1; ii >= 0; --ii) {
        svec_push_back(new_sv, svec_get(sv, ii));
    }
    free_svec(sv);
    return new_sv;
}

svec*
copy_over(svec* sv1, svec* sv2)
{
    // Doesn't free sv2, only copies data over.
    for (int ii = 0; ii < sv2->size; ++ii) {
        svec_push_back(sv1, svec_get(sv2, ii));
    }
    return sv1;
}

void svec_remove(svec* sv, int ii) {
    // Concurrently swaps the char* at ii to the end, frees it, then
    // reduces the size. In effect, this removes the string.
    for (int jj = ii; jj < sv->size - 1; ++jj) {
        svec_swap(sv, jj, jj + 1);
    }
    free(sv->data[sv->size - 1]);
    sv->size = sv->size - 1;
}

void svec_push(svec* sv, char* item) {
    // Adds item at the back then swaps it concurrently to the front
    svec_push_back(sv, item);
    for (int ii = sv->size - 1; ii > 0; --ii) {
        svec_swap(sv, ii, ii - 1);
    }
}

char* svec_peek(svec* sv) {
    // Alias for svec_get(sv, 0)
    return svec_get(sv, 0);
}

void svec_pop(svec* sv) {
    // Alias for svec_remove(sv, 0);
    svec_remove(sv, 0);
}


