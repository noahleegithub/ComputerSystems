#include <stdio.h>

#include "bitmap.h"
    
int 
bitmap_get(void* bm, int ii)
{
    int* bitmap = (int*) bm;
    int index = ii / sizeof(int);
    int bit = ii % sizeof(int);
    return (bitmap[index] >> bit) & 1;
}

void 
bitmap_put(void* bm, int ii, int vv)
{
    int* bitmap = (int*) bm;
    int index = ii / sizeof(int);
    int bit = ii % sizeof(int);
    bitmap[index] = bitmap[index] & ~(1 << bit);
    bitmap[index] = bitmap[index] | (vv << bit);
}

void 
bitmap_print(void* bm, int size)
{
    int* bitmap = (int*) bm;
    int arr_len = size / sizeof(int);
    for (int ii = 0; ii < arr_len; ++ii) {
        for (int jj = 0; jj < sizeof(int); ++jj) {
            printf("%d", (bitmap[ii] >> jj) & 1);
        }
    }
    printf("\n");
}

