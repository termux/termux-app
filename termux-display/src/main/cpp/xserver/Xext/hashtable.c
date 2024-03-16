#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>
#include "misc.h"
#include "hashtable.h"

/* HashResourceID */
#include "resource.h"

#define INITHASHSIZE 6
#define MAXHASHSIZE 11

struct HashTableRec {
    int             keySize;
    int             dataSize;

    int             elements;   /* number of elements inserted */
    int             bucketBits; /* number of buckets is 1 << bucketBits */
    struct xorg_list *buckets;  /* array of bucket list heads */

    HashFunc        hash;
    HashCompareFunc compare;

    void            *cdata;
};

typedef struct {
    struct xorg_list l;
    void *key;
    void *data;
} BucketRec, *BucketPtr;

HashTable
ht_create(int             keySize,
          int             dataSize,
          HashFunc        hash,
          HashCompareFunc compare,
          void            *cdata)
{
    int c;
    int numBuckets;
    HashTable ht = malloc(sizeof(struct HashTableRec));

    if (!ht) {
        return NULL;
    }

    ht->keySize = keySize;
    ht->dataSize = dataSize;
    ht->hash = hash;
    ht->compare = compare;
    ht->elements = 0;
    ht->bucketBits = INITHASHSIZE;
    numBuckets = 1 << ht->bucketBits;
    ht->buckets = xallocarray(numBuckets, sizeof(*ht->buckets));
    ht->cdata = cdata;

    if (ht->buckets) {
        for (c = 0; c < numBuckets; ++c) {
            xorg_list_init(&ht->buckets[c]);
        }
        return ht;
    } else {
        free(ht);
        return NULL;
    }
}

void
ht_destroy(HashTable ht)
{
    int c;
    BucketPtr it, tmp;
    int numBuckets = 1 << ht->bucketBits;
    for (c = 0; c < numBuckets; ++c) {
        xorg_list_for_each_entry_safe(it, tmp, &ht->buckets[c], l) {
            xorg_list_del(&it->l);
            free(it->key);
            free(it->data);
            free(it);
        }
    }
    free(ht->buckets);
    free(ht);
}

static Bool
double_size(HashTable ht)
{
    struct xorg_list *newBuckets;
    int numBuckets = 1 << ht->bucketBits;
    int newBucketBits = ht->bucketBits + 1;
    int newNumBuckets = 1 << newBucketBits;
    int c;

    newBuckets = xallocarray(newNumBuckets, sizeof(*ht->buckets));
    if (newBuckets) {
        for (c = 0; c < newNumBuckets; ++c) {
            xorg_list_init(&newBuckets[c]);
        }

        for (c = 0; c < numBuckets; ++c) {
            BucketPtr it, tmp;
            xorg_list_for_each_entry_safe(it, tmp, &ht->buckets[c], l) {
                struct xorg_list *newBucket =
                    &newBuckets[ht->hash(ht->cdata, it->key, newBucketBits)];
                xorg_list_del(&it->l);
                xorg_list_add(&it->l, newBucket);
            }
        }
        free(ht->buckets);

        ht->buckets = newBuckets;
        ht->bucketBits = newBucketBits;
        return TRUE;
    } else {
        return FALSE;
    }
}

void *
ht_add(HashTable ht, const void *key)
{
    unsigned index = ht->hash(ht->cdata, key, ht->bucketBits);
    struct xorg_list *bucket = &ht->buckets[index];
    BucketRec *elem = calloc(1, sizeof(BucketRec));
    if (!elem) {
        goto outOfMemory;
    }
    elem->key = malloc(ht->keySize);
    if (!elem->key) {
        goto outOfMemory;
    }
    /* we avoid signaling an out-of-memory error if dataSize is 0 */
    elem->data = calloc(1, ht->dataSize);
    if (ht->dataSize && !elem->data) {
        goto outOfMemory;
    }
    xorg_list_add(&elem->l, bucket);
    ++ht->elements;

    memcpy(elem->key, key, ht->keySize);

    if (ht->elements > 4 * (1 << ht->bucketBits) &&
        ht->bucketBits < MAXHASHSIZE) {
        if (!double_size(ht)) {
            --ht->elements;
            xorg_list_del(&elem->l);
            goto outOfMemory;
        }
    }

    /* if memory allocation has failed due to dataSize being 0, return
       a "dummy" pointer pointing at the of the key */
    return elem->data ? elem->data : ((char*) elem->key + ht->keySize);

 outOfMemory:
    if (elem) {
        free(elem->key);
        free(elem->data);
        free(elem);
    }

    return NULL;
}

void
ht_remove(HashTable ht, const void *key)
{
    unsigned index = ht->hash(ht->cdata, key, ht->bucketBits);
    struct xorg_list *bucket = &ht->buckets[index];
    BucketPtr it;

    xorg_list_for_each_entry(it, bucket, l) {
        if (ht->compare(ht->cdata, key, it->key) == 0) {
            xorg_list_del(&it->l);
            --ht->elements;
            free(it->key);
            free(it->data);
            free(it);
            return;
        }
    }
}

void *
ht_find(HashTable ht, const void *key)
{
    unsigned index = ht->hash(ht->cdata, key, ht->bucketBits);
    struct xorg_list *bucket = &ht->buckets[index];
    BucketPtr it;

    xorg_list_for_each_entry(it, bucket, l) {
        if (ht->compare(ht->cdata, key, it->key) == 0) {
            return it->data ? it->data : ((char*) it->key + ht->keySize);
        }
    }

    return NULL;
}

void
ht_dump_distribution(HashTable ht)
{
    int c;
    int numBuckets = 1 << ht->bucketBits;
    for (c = 0; c < numBuckets; ++c) {
        BucketPtr it;
        int n = 0;

        xorg_list_for_each_entry(it, &ht->buckets[c], l) {
            ++n;
        }
        printf("%d: %d\n", c, n);
    }
}

/* Picked the function from http://burtleburtle.net/bob/hash/doobs.html by
   Bob Jenkins, which is released in public domain */
static CARD32
one_at_a_time_hash(const void *data, int len)
{
    CARD32 hash;
    int i;
    const char *key = data;
    for (hash=0, i=0; i<len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

unsigned
ht_generic_hash(void *cdata, const void *ptr, int numBits)
{
    HtGenericHashSetupPtr setup = cdata;
    return one_at_a_time_hash(ptr, setup->keySize) & ~((~0U) << numBits);
}

int
ht_generic_compare(void *cdata, const void *l, const void *r)
{
    HtGenericHashSetupPtr setup = cdata;
    return memcmp(l, r, setup->keySize);
}

unsigned
ht_resourceid_hash(void * cdata, const void * data, int numBits)
{
    const XID* idPtr = data;
    XID id = *idPtr & RESOURCE_ID_MASK;
    (void) cdata;
    return HashResourceID(id, numBits);
}

int
ht_resourceid_compare(void* cdata, const void* a, const void* b)
{
    const XID* xa = a;
    const XID* xb = b;
    (void) cdata;
    return
        *xa < *xb ? -1 :
        *xa > *xb ? 1 :
        0;
}

void
ht_dump_contents(HashTable ht,
                 void (*print_key)(void *opaque, void *key),
                 void (*print_value)(void *opaque, void *value),
                 void* opaque)
{
    int c;
    int numBuckets = 1 << ht->bucketBits;
    for (c = 0; c < numBuckets; ++c) {
        BucketPtr it;
        int n = 0;

        printf("%d: ", c);
        xorg_list_for_each_entry(it, &ht->buckets[c], l) {
            if (n > 0) {
                printf(", ");
            }
            print_key(opaque, it->key);
            printf("->");
            print_value(opaque, it->data);
            ++n;
        }
        printf("\n");
    }
}
