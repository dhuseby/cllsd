/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#ifndef __UINT__
#define __UINT__
	typedef uint32_t uint_t;
	typedef int32_t int_t;
#endif

/* the hashtable iterator type */
typedef int_t ht_itr_t;

/* the hash table opaque tuple type */
typedef struct tuple_s tuple_t;

/* the key hashing function type,
 * this must return a hash value > 0 because 0 is
 * reserved for internal use */
typedef uint_t (*key_hash_fn)(void const * const key);

/* define the key comparison function type,
 * it must return 1 if the keys are equal and
 * 0 if the keys are not. */
typedef int (*key_eq_fn)(void const * const l, void const * const r);

/* define the value delete function type */
typedef void (*ht_delete_fn)(void * value);

/* the hash table structure */
typedef struct ht_s
{
	key_hash_fn			khfn;				/* key hash function */
	key_eq_fn			kefn;				/* key compare function */
	ht_delete_fn		kdfn;				/* key delete function */
	ht_delete_fn		vdfn;				/* value delete function */
	uint_t				prime_index;		/* the index of the table size */
	uint_t				num_tuples;			/* number of tuples in the table */
	uint_t				initial_capacity;   /* the initial capacity value */
	float				load_factor;		/* load level that triggers resize */
	tuple_t*			tuples;				/* pointer to tuple table */
#ifdef USE_THREADING
	pthread_mutex_t		lock;				/* hashtable lock */
#endif
} ht_t;


#ifdef USE_THREADING
/* threading protection */
void ht_lock(ht_t * const htable);
int ht_try_lock(ht_t * const htable);
void ht_unlock(ht_t * const htable);
pthread_mutex_t * ht_get_mutex(ht_t * const htable);
#endif

void ht_initialize
(
	ht_t * const htable, 
	uint_t initial_capacity, 
	key_hash_fn khfn, 
	ht_delete_fn vdfn,
	key_eq_fn kefn,
	ht_delete_fn kdfn
);
void ht_deinitialize(ht_t * const htable);

/* dynamically allocates and initializes a hashtable */
/* NOTE: If NULL is passed in for the key_hash_fn function, the pointer to the 
 * key will be cast to an int32 and used as the hash value.  If NULL is passed 
 * for the delete functions, then the key/values will not be deleted when 
 * hashtableDeinitialize or hashtableDelete are called. */
ht_t* ht_new(
    uint_t initial_capacity, 
    key_hash_fn khfn,
    ht_delete_fn vdfn, 
    key_eq_fn kefn, 
    ht_delete_fn kdfn);

/* deinitializes and frees a hashtable allocated with hashtableNew() */
/* NOTE: this calls hashtableDeinitialize if there are values left in the
 * hashtable */
void ht_delete(ht_t * const htable);

/* returns the number of key/value pairs stored in the hashtable */
uint_t ht_size(ht_t * const htable);

/* returns the current fraction of how full the hash table is.
 * the range is 0.0f to 1.0f, where 1.0f is completly full */
float ht_load(ht_t * const htable);

/* returns the load limit that will trigger a resize when the 
 * hashtable where to exceed it during a hashtableAdd. */
float ht_get_resize_load_factor(ht_t const * const htable);

/* set the load limit that will trigger a resize.  this defaults
 * 0.65f or 65% full. */
int ht_set_resize_load_factor(ht_t * const htable, float load);

/* this function will prune all empty filler nodes left over from
 * previous hashtableRemove calls.  the empty filler nodes left over
 * to keep the probing chains from breaking.  this function clears
 * them out by allocating a new table and rehashing the non-filler
 * nodes into it and freeing the old table.  this operation elliminates
 * the filler nodes and "compacts" the table.  use this to keep your
 * load factor from endlessly climbing and causing resizes. */
int ht_compact(ht_t * const htable);

/* adds a key/value pair to the hashtable. */
/* NOTE: if the number of values stored in the table will exceed a load factor 
 * of 65% then the hashtable grows to make more room.
 * NOTE: the hashtable takes ownership of the key data if a key delete
 * function was given when the hashtable was created. */
int ht_add(
    ht_t * const htable, 
    void * const key, 
    void * const value);

int ht_add_prehash(
	ht_t * const htable,
	uint_t const hash,
	void * const key,
	void * const value);

/* clears all key/value pairs from the hashtable reinitializes it */
int ht_clear(ht_t * const htable);

/* find a value by it's key. this function uses the compare_keys_fn function 
 * to find the item */
void * ht_find(ht_t const * const htable, void const * const key);
void * ht_find_prehash( ht_t const * const htable, 
						uint_t const hash, 
						void const * const key );

/* remove the value associated with the key from the hashtable */
void * ht_remove(ht_t * const htable, void const * const key);
void * ht_remove_prehash( ht_t * const htable,
						  uint_t const hash,
						  void const * const key );

/* Iterator based access to the hashtable */
ht_itr_t ht_itr_begin(ht_t const * const htable);
ht_itr_t ht_itr_next(
    ht_t const * const htable, 
    ht_itr_t const itr);
ht_itr_t ht_itr_end(ht_t const * const htable);
void* ht_itr_get(
    ht_t const * const htable, 
    ht_itr_t const itr);

#endif
