#ifndef HASHTABLE_H
#define HASHTABLE_H 1

#include <dix-config.h>
#include <X11/Xfuncproto.h>
#include <X11/Xdefs.h>
#include "list.h"

/** @brief A hashing function.

  @param[in/out] cdata  Opaque data that can be passed to HtInit that will
                        eventually end up here
  @param[in] ptr        The data to be hashed. The size of the data, if
                        needed, can be configured via a record that can be
                        passed via cdata.
  @param[in] numBits    The number of bits this hash needs to have in the
                        resulting hash

  @return  A numBits-bit hash of the data
*/
typedef unsigned (*HashFunc)(void * cdata, const void * ptr, int numBits);

/** @brief A comparison function for hashed keys.

  @param[in/out] cdata  Opaque data that ca be passed to Htinit that will
                        eventually end up here
  @param[in] l          The left side data to be compared
  @param[in] r          The right side data to be compared

  @return -1 if l < r, 0 if l == r, 1 if l > r
*/
typedef int (*HashCompareFunc)(void * cdata, const void * l, const void * r);

struct HashTableRec;

typedef struct HashTableRec *HashTable;

/** @brief  A configuration for HtGenericHash */
typedef struct {
    int             keySize;
} HtGenericHashSetupRec, *HtGenericHashSetupPtr;

/** @brief  ht_create initializes a hash table for a certain hash table
            configuration

    @param[out] ht       The hash table structure to initialize
    @param[in] keySize   The key size in bytes
    @param[in] dataSize  The data size in bytes
    @param[in] hash      The hash function to use for hashing keys
    @param[in] compare   The comparison function for hashing keys
    @param[in] cdata     Opaque data that will be passed to hash and
                         comparison functions
*/
extern _X_EXPORT HashTable ht_create(int             keySize,
                                     int             dataSize,
                                     HashFunc        hash,
                                     HashCompareFunc compare,
                                     void            *cdata);
/** @brief  HtDestruct deinitializes the structure. It does not free the
            memory allocated to HashTableRec
*/
extern _X_EXPORT void ht_destroy(HashTable ht);

/** @brief  Adds a new key to the hash table. The key will be copied
            and a pointer to the value will be returned. The data will
            be initialized with zeroes.

  @param[in/out] ht  The hash table
  @param[key] key    The key. The contents of the key will be copied.

  @return On error NULL is returned, otherwise a pointer to the data
          associated with the newly inserted key.

  @note  If dataSize is 0, a pointer to the end of the key may be returned
         to avoid returning NULL. Obviously the data pointed cannot be
         modified, as implied by dataSize being 0.
*/
extern _X_EXPORT void *ht_add(HashTable ht, const void *key);

/** @brief  Removes a key from the hash table along with its
            associated data, which will be free'd.
*/
extern _X_EXPORT void ht_remove(HashTable ht, const void *key);

/** @brief  Finds the associated data of a key from the hash table.

   @return  If the key cannot be found, the function returns NULL.
            Otherwise it returns a pointer to the data associated
            with the key.

   @note  If dataSize == 0, this function may return NULL
          even if the key has been inserted! If dataSize == NULL,
          use HtMember instead to determine if a key has been
          inserted.
*/
extern _X_EXPORT void *ht_find(HashTable ht, const void *key);

/** @brief  A generic hash function */
extern _X_EXPORT unsigned ht_generic_hash(void *cdata,
                                          const void *ptr,
                                          int numBits);

/** @brief  A generic comparison function. It compares data byte-wise. */
extern _X_EXPORT int ht_generic_compare(void *cdata,
                                        const void *l,
                                        const void *r);

/** @brief  A debugging function that dumps the distribution of the
            hash table: for each bucket, list the number of elements
            contained within. */
extern _X_EXPORT void ht_dump_distribution(HashTable ht);

/** @brief  A debugging function that dumps the contents of the hash
            table: for each bucket, list the elements contained
            within. */
extern _X_EXPORT void ht_dump_contents(HashTable ht,
                                       void (*print_key)(void *opaque, void *key),
                                       void (*print_value)(void *opaque, void *value),
                                       void* opaque);

/** @brief  A hashing function to be used for hashing resource IDs when
            used with HashTables. It makes no use of cdata, so that can
            be NULL. It uses HashXID underneath, and should HashXID be
            unable to hash the value, it switches into using the generic
            hash function. */
extern _X_EXPORT unsigned ht_resourceid_hash(void *cdata,
                                             const void * data,
                                             int numBits);

/** @brief  A comparison function to be used for comparing resource
            IDs when used with HashTables. It makes no use of cdata,
            so that can be NULL. */
extern _X_EXPORT int ht_resourceid_compare(void *cdata,
                                           const void *a,
                                           const void *b);

#endif // HASHTABLE_H
