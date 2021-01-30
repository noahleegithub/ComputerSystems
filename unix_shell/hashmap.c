/* From HW4, modified to map from String->String for CH01 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <bsd/string.h>
#include <string.h>
#include <assert.h>

#include "hashmap.h"


int
hash(char* key)
{
    // Hash this string.
    // Hashing is based on Java's String.hash(), but with 13 (prime #)
    // as starting hash val instead of 0.
    int hash = 13;
    for (int ii = 0; ii < strlen(key); ++ii) {
        hash = 31 * hash + key[ii];
    }
    return hash; //& hashmap->capacity - 1 to translate to index
}

hashmap_pair*
make_hashmap_pair(char* kk, char* vv, bool used, bool tomb)
{
    // Allocate memory for a hashmap_pair 
    // and return the pointer to it. kk must be a valid C string
    // of length 3.   

    hashmap_pair* pp = malloc(sizeof(hashmap_pair));
    pp->key = strdup(kk);
    pp->val = strdup(vv);
    pp->used = used;
    pp->tomb = tomb;
    return pp;
}

hashmap*
make_hashmap_presize(int nn)
{
    // Allocate memory for a hashmap and return the 
    // pointer to it.
    // hh->data starts out filled with null pairs.
    hashmap* hh = malloc(sizeof(hashmap));
    hh->size = 0;
    hh->capacity = nn;
    hh->data = malloc(hh->capacity * sizeof(hashmap_pair*));
    for (int ii = 0; ii < hh->capacity; ++ii) {
        hh->data[ii] = make_hashmap_pair("nul\0", "nul\0", false, false);
    }
    return hh;
}

hashmap*
make_hashmap()
{
    // Make a hashmap with initial capacity 4.
    return make_hashmap_presize(4);
}

void
free_hashmap_pair(hashmap_pair* pp)
{
    if (pp) {
        free(pp->key);
        free(pp->val);
        free(pp);
    }
}

void
free_hashmap(hashmap* hh)
{
    // Free all hashmap memory.
    for (int ii = 0; ii < hh->capacity; ++ii) {
        free_hashmap_pair(hh->data[ii]);
    }
    free(hh->data);
    free(hh);
}

int
hashmap_has(hashmap* hh, char* kk)
{
    // Return whether the key exists in the hashmap.
    return hashmap_get(hh, kk) != 0;
}

int
hashmap_next_insertable(hashmap* hh, char* kk)
{
    // Return the first index which contains an
    // unused pair OR a non-tomb pair with an equal key.
    int ii = hash(kk) & (hh->capacity - 1);
    hashmap_pair pair = hashmap_get_pair(hh, ii);
    while (pair.used) {
        if (!pair.tomb && strcmp(pair.key, kk) == 0) {
            return ii;
        }
        ii = (ii + 1) & (hh->capacity - 1);
        pair = hashmap_get_pair(hh, ii);
    }
    return ii;
}

char*
hashmap_get(hashmap* hh, char* kk)
{
    // Return the value associated with the given key.
    // Return -1 if the pair with the key is not in the hashmap.
    int ii = hashmap_next_insertable(hh, kk);
    if (hh->data[ii]->used) {
        return hh->data[ii]->val;
    }
    return 0;
}

void
hashmap_grow(hashmap* hh)
{
    // Double the capacity of the hashmap.
    // Reindex all pairs that are in use and are not tombs.
    // Remove all tomb pairs and decrease the size accordingly.
    int old_capacity = hh->capacity;
    hh->capacity = hh->capacity * 2;
    hashmap_pair** old_data = hh->data;
    hh->data = malloc(hh->capacity * sizeof(hashmap_pair*));

    for (int ii = 0; ii < hh->capacity; ++ii) {
       hh->data[ii] = make_hashmap_pair("nul\0", "nul\0", false, false); 
    }

    for (int ii = 0; ii < old_capacity; ++ii) {
        hashmap_pair* pp = old_data[ii];
        if (pp->tomb) {
            free_hashmap_pair(pp);
            hh->size = hh->size - 1;
        }
        else if (!pp->used) {
            free_hashmap_pair(pp);
        }
        else {
            int index = hashmap_next_insertable(hh, pp->key);
            free_hashmap_pair(hh->data[index]);
            hh->data[index] = pp;
        }
    }

    free(old_data);
}

void
hashmap_put(hashmap* hh, char* kk, char* vv)
{
    // Insert the given key, value pair into the hashmap.
    // Double the capacity if the load factor reaches 0.5.
    // Write over the existing value if the key already exists.
    if (hh->size >= (hh->capacity >> 1)) {
        hashmap_grow(hh);
    }

    int ii = hashmap_next_insertable(hh, kk);
    hashmap_pair* pp = hh->data[ii];
    if (pp->used) {
        free(pp->val);
        pp->val = strdup(vv);
    } 
    else {
        free(pp->key);
        free(pp->val);
        pp->key = strdup(kk);
        pp->val = strdup(vv);
        pp->used = true;
    }
    hh->size = hh->size + 1;
}

void
hashmap_del(hashmap* hh, char* kk)
{
    // Remove any value associated with
    // this key in the map.
    if (hashmap_has(hh, kk)) {
        hashmap_pair* del_target = hh->data[hashmap_next_insertable(hh, kk)];
        del_target->tomb = true;
    }
}

hashmap_pair
hashmap_get_pair(hashmap* hh, int ii)
{
    // Return the {k,v} pair at
    // this index in the hashmap.
    assert(ii >= 0 && ii < hh->capacity);
    return *hh->data[ii];
}

void
hashmap_dump(hashmap* hh)
{
    // Print out a listing of all pairs
    // within the hashmap.
    printf("== hashmap dump ==\n");
    for (int ii = 0; ii < hh->capacity; ++ii) {
        hashmap_pair pair = hashmap_get_pair(hh, ii);
        // prints index, key, val, used, tomb
        printf("ii=%d, kk=%s, vv=%s, used=%d, tomb=%d\n", ii, pair.key, pair.val, pair.used, pair.tomb);
    }
}
