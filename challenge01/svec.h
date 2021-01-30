/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
/* Modified by Noah Lee for HW4 and CH01 */

#ifndef SVEC_H
#define SVEC_H

typedef struct svec {
    int size;
    char** data;
    int capacity;
} svec;

svec* make_svec();
void  free_svec(svec* sv);

char* svec_get(svec* sv, int ii);
void  svec_put(svec* sv, int ii, char* item);

void svec_push_back(svec* sv, char* item);
void svec_swap(svec* sv, int ii, int jj);

void print_svec(svec* sv);
svec* rev_free_svec(svec* sv);

svec* copy_over(svec* sv1, svec* sv2);
void svec_remove(svec* sv, int ii);

void svec_push(svec* sv, char* item);
char* svec_peek(svec* sv);
void svec_pop(svec* sv);

#endif
