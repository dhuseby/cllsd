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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#ifdef USE_THREADING
#include <pthread.h>
#endif

#include "debug.h"
#include "macros.h"
#include "hashtable.h"


/* this load factor is how full the table will get before a resize is 
 * triggered */
float const default_load_factor = 0.65f;


/* this list of primes are the table size used by the hashtable.  each prime 
 * is roughly double the size of the previous prime to get amortized resize
 * operations. */
uint_t const num_primes = 30;
uint_t const hashtable_primes[] =
{
	0, 7, 13, 29, 53, 97, 193, 389, 769, 1543, 3079,
	6151, 12289, 24593, 49157,
	98317, 196613, 393241, 786433,
	1572869, 3145739, 6291469,
	12582917, 251658343, 50331653,
	100663319, 201326611,
	402563189, 805306457,
	1610612741
};

typedef struct tuple_s
{
	uint_t				hash;				/* hash value of the key */
	void *				key;				/* pointer to the key */
	void *				value;				/* pointer to the value */
} tuple_t;


/* the default key hashing function just casts the pointer value to an uint_t */
static uint_t default_key_hash(void const * const key)
{
	uint_t const hash = (uint_t)key;
	return ((hash == 0) ? hash + 1 : hash);
}

/* the default key compare function just casts the pointers to uint_t
 * and compares them.  this works with the default key hash function */
static int default_key_eq(void const * const l, void const * const r)
{
	return ((uint_t)l == (uint_t)r);
}


#ifdef USE_THREADING
void ht_lock(ht_t * const htable)
{
	CHECK_PTR(htable);

	/* lock the mutex */
	pthread_mutex_lock(&htable->lock);
}


int ht_try_lock(ht_t * const htable)
{
	CHECK_PTR_RET(htable, -1);

	/* try to lock the mutex */
	return pthread_mutex_trylock(&htable->lock);
}


void ht_unlock(ht_t * const htable)
{
	CHECK_PTR(htable);

	/* lock the mutex */
	pthread_mutex_unlock(&htable->lock);
}


pthread_mutex_t * ht_get_mutex(ht_t * const htable)
{
	CHECK_PTR_RET(htable, NULL);
	
	/* return the mutex pointer */
	return &htable->lock;	
}
#endif

/* initializes a hashtable */
void ht_initialize
(
	ht_t * const htable, 
	uint_t initial_capacity, 
	key_hash_fn khfn, 
	ht_delete_fn vdfn,
	key_eq_fn kefn,
	ht_delete_fn kdfn
)
{
	uint_t i = 0;
	CHECK_PTR(htable);

	/* zero out the memory */
	memset(htable, 0, sizeof(ht_t));

	/* set the default load factor */
	htable->load_factor = default_load_factor;

	if ( initial_capacity > 0 )
	{
		/* figure out the initial table size */
		while(((uint_t)(hashtable_primes[i] * htable->load_factor) < initial_capacity) &&
			  (i < (num_primes - 1)))
			i++;

		/* allocate room for the key/value structs */
		htable->tuples = (tuple_t*)CALLOC(hashtable_primes[i], sizeof(tuple_t));
		ASSERT(htable->tuples != NULL);
	}

	/* store the prime index */
	htable->prime_index = i;
	
	/* reset the number of tuples */
	htable->num_tuples = 0;

	/* initialize the key hashing function */
	if(khfn != NULL)
		htable->khfn = khfn;
	else
		htable->khfn = default_key_hash;

	/* initialize the value delete function */
	htable->vdfn = vdfn;
	
	/* initialize the key compare function */
	if(kefn != NULL)
		htable->kefn = kefn;
	else
		htable->kefn = default_key_eq;
		
	/* initialize the key delete function */
	htable->kdfn = kdfn;
	
	/* save the initial capacity */
	htable->initial_capacity = initial_capacity;

#ifdef USE_THREADING
	/* initialize the mutex */
	pthread_mutex_init(&htable->lock, 0);
#endif
}


/* dynamically allocates and initializes a hashtable */
/* NOTE: If NULL is passed in for the key_eq_fn function, the default
 * key compare function will be used */
ht_t* ht_new
(
	uint_t initial_capacity, 
	key_hash_fn khfn, 
	ht_delete_fn vdfn, 
	key_eq_fn kefn, 
	ht_delete_fn kdfn
)
{
	ht_t* htable = NULL;

	/* allocate a hashtable struct */
	DEBUG("heap allocating hash table\n");
	htable = (ht_t*)CALLOC(1, sizeof(ht_t));
	CHECK_PTR_RET(htable, NULL);

	/* initialize the hashtable */
	ht_initialize(htable, initial_capacity, khfn, vdfn, kefn, kdfn);

	/* return the new hashtable */
	return htable;
}


/* deinitialize a hashtable. */
void ht_deinitialize(ht_t * const htable)
{
	int_t ret = 0;
	int_t i = 0;
	int_t table_size = 0;
	tuple_t* tuple = NULL;
	CHECK_PTR(htable);

	if ( htable->tuples != NULL )
	{
		/* get the table size */
		table_size = hashtable_primes[htable->prime_index];

		/* get a pointer to the first tuple */
		tuple = htable->tuples;

		/* go through the tuples and free the keys and values */
		for(i = 0; i < table_size; i++)
		{
			if(tuple->hash == 0)
			{
				tuple++;
				continue;
			}

			if((tuple->key != NULL) && (htable->kdfn != NULL))
			{
				/* clean up the key */
				(*(htable->kdfn))(tuple->key);
			}
			tuple->key = NULL;

			if((tuple->value != NULL) && (htable->vdfn != NULL))
			{
				/* clean up the value */
				(*(htable->vdfn))(tuple->value);
			}
			tuple->value = NULL;

			/* move to the next tuple in the list */
			tuple++;
		}

		/* free the tuple memory */
		FREE((void*)htable->tuples);
	}

#ifdef USE_THREADING
	/* destroy the lock */
	ret = pthread_mutex_destroy(&htable->lock);
	ASSERT(ret == 0);
#endif
}


/* deinitializes and frees a hashtable allocated with ht_new() */
void ht_delete(ht_t * const htable)
{
	CHECK_PTR(htable);

	/* de-init the hashtable if needed */
	ht_deinitialize(htable);

	/* free the hashtable */
	FREE(htable);
}


/* returns the number of key/value pairs stored in the hashtable */
uint_t ht_size(ht_t * const htable)
{
	CHECK_PTR_RET(htable, 0);

	return htable->num_tuples;
}


/* returns the current fraction of how full the hash table is.
 * the range is 0.0f to 1.0f, where 1.0f is completly full */
float ht_load(ht_t * const htable)
{
	CHECK_PTR_RET(htable, -1.0f);

	/* return the number of tuples divided by the size of
	 * the hashtable. */
	return ((float)htable->num_tuples / 
			(float)hashtable_primes[htable->prime_index]);
}

/* returns the load limit that will trigger a resize when the 
 * hashtable where to exceed it during a ht_add(). */
float ht_get_resize_load_factor(ht_t const * const htable)
{
	CHECK_PTR_RET(htable, -1.0f);

	return htable->load_factor;
}

/* private function for growing the hash table.
 * NOTE: it only supports keeping the table the same size (compacting)
 * and growing the table.  this does not support shrinking the hash table. */
static int ht_grow(ht_t * const htable, uint_t new_prime_index)
{
	uint_t i = 0;
	uint_t j = 0;
	uint_t hash = 0;
	tuple_t * tuple = NULL;
	tuple_t * tuples = NULL;
	uint_t old_table_size = 0;
	uint_t new_table_size = 0;
	uint_t new_num_tuples = 0;
	CHECK_PTR_RET(htable, 0);
	CHECK_RET(htable->prime_index <= new_prime_index, 0);
	CHECK_RET(new_prime_index < num_primes, 0);

	/* get the current table size */
	old_table_size = hashtable_primes[htable->prime_index];

	/* get the new table size */
	new_table_size = hashtable_primes[new_prime_index];

	/* allocate a new tuples table */
	DEBUG("growing hash table to: %d\n", new_table_size)
	tuples = (tuple_t *)CALLOC(new_table_size, sizeof(tuple_t));
	CHECK_PTR_RET(tuples, 0);

	if ( htable->tuples != NULL )
	{
		/* go through the old table and hash the items into the new table */
		tuple = htable->tuples;
		for(i = 0; i < old_table_size; i++)
		{
			/* skip empty and erased tuples */
			if(tuple->hash == 0)
			{
				/* if the hash value is 0, the key and
				 * value pointers should be NULL */
				ASSERT(tuple->key == NULL);
				ASSERT(tuple->value == NULL);

				/* move to the next tuple in the old table */
				tuple++;
				continue;
			}

			/* handle the keys that have had their values removed */
			if((tuple->key == NULL) || (tuple->value == NULL))
			{
				/* if either key or value pointer is NULL they
				 * both should be NULL */
				ASSERT(tuple->key == NULL);
				ASSERT(tuple->value == NULL);

				/* zero out the key */
				tuple->hash = 0;

				/* move to the next tuple in the old table */
				tuple++;
				continue;
			}

			/* get the hash of the key */
			hash = tuple->hash;

			/* get the starting index of the probing */
			j = (hash % new_table_size);
			
			/* use linear probing to find a slot */
			while(tuples[j].hash != 0)
			{
				/* check for hash collisions */
				if(hash == tuples[j].hash)
				{
					/* if the hash values match then there better be 
					 * a key in the new table and the keys better not
					 * match!  this should never happen since ht_add
					 * prevents this from ever happening but it's still
					 * nice to check for it here. */
					ASSERT(tuples[j].key != NULL);
					ASSERT(!(*(htable->kefn))(tuple->key, tuples[j].key));
				}

				/* move to the next test index */
				j = ((j + 1) % new_table_size);
			}

			ASSERT(tuples[j].hash == 0);

			/* we found a slot, copy the tuple from the old table to
			 * the new one */
			tuples[j].hash = tuple->hash;
			tuples[j].key = tuple->key;
			tuples[j].value = tuple->value;
			
			/* update the count of tuples in the new table */
			new_num_tuples++;

			/* move to the next tuple */
			tuple++;
		}

		/* free the old table */
		FREE(htable->tuples);
	}

	/* store the new tuples table */
	htable->tuples = tuples;
	
	/* store the new number of tuples */
	htable->num_tuples = new_num_tuples;

	/* store the new prime index */
	htable->prime_index = new_prime_index;

	/* success */
	return 1;
}

/* determines if the hashtable needs to grow based on the number
 * of tuples and the current load factor limit for the table.  if
 * the table needs to grow, it returns the index of the first prime
 * table size that is large enough to make the hash table load 
 * factor less than the limit.	if the table does not need to grow
 * the function returns 0. */
static int ht_needs_to_grow(ht_t const * const htable, 
								uint_t new_size)
{
	uint_t new_index = 0;
	uint_t load = 0;
	CHECK_PTR_RET(htable, 0);
	CHECK_RET((((int)new_index >= 0) && (new_index < num_primes)), 0);

	/* there is no tuple table, then we always need to grow */
	if ( htable->tuples == NULL )
	{
		return 1;
	}

	/* start at the existing index */
	new_index = htable->prime_index;

	/* if the table doesn't need to grow, return 0 */
	if(new_size < (uint_t)((float)hashtable_primes[new_index] * htable->load_factor))
	{
		return 0;
	}

	/* the table needs to grow, so find the prime index for the
	 * new size */
	load = (uint_t)((float)hashtable_primes[new_index] * htable->load_factor);
	while((new_index < num_primes) && (new_size >= load))
	{
		new_index++;
		load = (uint_t)((float)hashtable_primes[new_index] * htable->load_factor);
	}

	/* can the table grow? */
	if(new_index >= num_primes)
	{
		/* AAGH! the table needs to grow but we've run out of primes */
		WARN("hashtable has run out of primes!");
		return 0;
	}

	/* the table doesn't need to grow */
	return new_index;
}

/* set the load limit that will trigger a resize.  this defaults
 * 0.65f or 65% full. */
int ht_set_resize_load_factor(ht_t * const htable, float load)
{
	int new_index = 0;
	CHECK_PTR_RET(htable, 0);
	CHECK_RET(load > 0.0f, 0);
	CHECK_RET(load < 1.0f, 0);

	/* adjust the load factor */
	htable->load_factor = load;
	
	/* check to see if a resize is necessary */
	if((new_index = ht_needs_to_grow(htable, htable->num_tuples)) > 0)
	{
		/* it does, so try to grow it */
		if(!ht_grow(htable, new_index))
		{
			WARN("failed to grow hashtable!");
			return 0;
		}
	}
	
	return 1;
}


/* this function will prune all empty filler nodes left over from
 * previous ht_remove calls.  the empty filler nodes left over
 * to keep the probing chains from breaking.  this function clears
 * them out by allocating a new table and rehashing the non-filler
 * nodes into it and freeing the old table.  this operation elliminates
 * the filler nodes and "compacts" the table.  use this to keep your
 * load factor from endlessly climbing and causing resizes. */
int ht_compact(ht_t * const htable)
{
	CHECK_PTR_RET(htable, 0);

	if ( htable->tuples == NULL )
	{
		return 1;
	}
	
	/* call the grow function with our current size. this will
	 * cause it to allocate a new table the same size and rehash
	 * the non-filler tuples into the new table, effectively
	 * removing them from the hashtable and reducing how full
	 * the table is. */
	if(!ht_grow(htable, htable->prime_index))
	{
		WARN("failed to compact");
		return 0;
	}
	
	return 1;
}


/* adds a key/value pair to the hashtable. */
/* NOTE: if the number of values stored in the table will exceed a load 
 * factor of 65% then the hashtable grows to make more room. */
int ht_add( ht_t * const htable, 
			void * const key,
			void * const value)
{
	uint_t hash;
	CHECK_PTR_RET(htable, 0);
	CHECK_PTR_RET(htable->khfn, 0);
	CHECK_PTR_RET(key, 0);
	CHECK_PTR_RET(value, 0);

	hash = (*(htable->khfn))(key);

	return ht_add_prehash( htable, hash, key, value );
}

int ht_add_prehash( ht_t * const htable, 
					uint_t const hash,
					void * const key,
					void * const value )
{
	int new_index = 0;
	uint_t i = 0;
	uint_t table_size = 0;
	CHECK_PTR_RET(htable, 0);
	CHECK_PTR_RET(key, 0);
	CHECK_PTR_RET(value, 0);

	/* check to see if the table needs to grow if we add one more */
	if((new_index = ht_needs_to_grow(htable, htable->num_tuples + 1)) > 0)
	{
		/* it does, so try to grow it to the new size */
		if(!ht_grow(htable, new_index))
		{
			WARN("failed to grow hashtable!");
			return 0;
		}
	}

	/* get the table size */
	table_size = hashtable_primes[htable->prime_index];

	/* get the starting index of the probing */
	i = (hash % table_size);

	/* use linear probing to find a slot */
	while(htable->tuples[i].hash != 0)
	{
		/* check for hash collisions */
		if(hash == htable->tuples[i].hash)
		{
			/* check the keys to see if they are equal */
			if((htable->tuples[i].key != NULL) &&
			   (*(htable->kefn))(key, htable->tuples[i].key))
			{
				/* there should be a value in the table because
				 * the key was not null. */
				ASSERT(htable->tuples[i].value != NULL);
				
				/* there is alread an item in the hashtable with
				 * the provided key so fail. */
				return 0;
			}
			else
			{
				/* we have a hash collision but the slot is empty
				 * so break out of here and use this spot to add
				 * the new item.  the key and value should be null. */
				 ASSERT(htable->tuples[i].key == NULL);
				 ASSERT(htable->tuples[i].value == NULL);
				 break;
			}
		}

		/* move to the next test index */
		i = ((i + 1) % table_size);
	}

	/* if we get here, we either found a slot without a hash value
	 * or we found an empty filler slot with a matching hash value. */

	/* the key and value should be NULL */
	ASSERT(htable->tuples[i].key == NULL);
	ASSERT(htable->tuples[i].value == NULL);

	if(htable->tuples[i].hash == 0)
	{
		/* we're aren't reusing an empty filler node so
		 * increment the number of tuples stored in the
		 * table so that we have an accurate load factor. */
		htable->num_tuples++;
	}

	/* we found a slot, so store the new value there. */
	htable->tuples[i].hash = hash;
	htable->tuples[i].key = key;
	htable->tuples[i].value = value;

	/* success */
	return 1;
}


/* clears all key/value pairs from the hashtable and compacts it */
int ht_clear(ht_t * const htable)
{
	uint_t initial_capacity = 0;
	key_hash_fn khfn = NULL;
	ht_delete_fn vdfn = NULL;
	key_eq_fn kefn = NULL;
	ht_delete_fn kdfn = NULL;
	CHECK_PTR_RET(htable, 0);

	/* save off the htable params */
	initial_capacity = htable->initial_capacity;
	khfn = htable->khfn;
	vdfn = htable->vdfn;
	kefn = htable->kefn;
	kdfn = htable->kdfn;

	/* de-init the entire hashtable cleaning up all memory except
	 * for the htable struct itself */
	ht_deinitialize(htable);
	
	/* re-initialize the htable with the saved params */
	ht_initialize(htable, initial_capacity, khfn, vdfn, kefn, kdfn);

	return 1;
}


/* private helper function for ht_find and ht_remove */
static int ht_find_index
(
	ht_t const * const htable, 
	uint_t const hash,
	void const * const key,
	uint_t * const index
)
{
	uint_t i = 0;
	uint_t table_size = 0;
	CHECK_PTR_RET(htable, 0);
	CHECK_RET(hash != 0, 0);
	CHECK_PTR_RET(index, 0);

	if ( htable->tuples == NULL )
	{
		return 0;
	}

	/* get the table size */
	table_size = hashtable_primes[htable->prime_index];

	/* find where it should be in the table and probe that
	 * position first. */
	i = (hash % table_size);

	/* do a linear search from the probe position looking
	 * for a matching hash and key */
	while(htable->tuples[i].hash != 0)
	{
		/* check for a match */
		if((hash == htable->tuples[i].hash) &&			 /* do the hashes match? */
		   (htable->tuples[i].key != NULL) &&			 /* is there a key? */
		   (*(htable->kefn))(key, htable->tuples[i].key))/* do the keys match? */
		{
			*index = i;
			
			return 1;
		}

		/* move to the next test index */
		i = ((i + 1) % table_size);
	}

	/* if we get here we didn't find what we were looking for
	 * so fail. */
	return 0;
}


/* find a value by it's key. */
void * ht_find(ht_t const * const htable, void const * const key)
{
	uint_t hash = 0;
	CHECK_PTR_RET(htable, NULL);
	CHECK_PTR_RET(key, NULL);
	CHECK_PTR_RET(htable->khfn, NULL);

	/* get the hash of the key */
	hash = (*(htable->khfn))(key);

	return ht_find_prehash( htable, hash, key );
}

void * ht_find_prehash( ht_t const * const htable,
						uint_t const hash,
						void const * const key )
{
	uint_t i = 0;
	CHECK_PTR_RET(htable, NULL);
	CHECK_PTR_RET(key, NULL);

	/* get the index of the tuple if it is there */
	if(ht_find_index(htable, hash, key, &i))
	{
		ASSERT(hash == htable->tuples[i].hash);

		/* return the value */
		return htable->tuples[i].value;
	}

	return NULL;
}


/* remove the value associated with the key from the hashtable */
void * ht_remove(ht_t * const htable, void const * const key)
{
	uint_t hash = 0;
	CHECK_PTR_RET(htable, NULL);
	CHECK_PTR_RET(key, NULL);
	CHECK_PTR_RET(htable->khfn, NULL);

	/* get the hash of the key */
	hash = (*(htable->khfn))(key);

	return ht_remove_prehash( htable, hash, key );
}


void * ht_remove_prehash( ht_t * const htable,
						  uint_t const hash,
						  void const * const key )
{
	uint_t i = 0;
	void * value = NULL;
	CHECK_PTR_RET(htable, NULL);
	CHECK_PTR_RET(key, NULL);
	CHECK_PTR_RET(htable->khfn, NULL);

	/* get the index of the tuple if it is there */
	if(ht_find_index(htable, hash, key, &i))
	{
		ASSERT(hash == htable->tuples[i].hash);

		/* return the value to the caller...they're responsible for 
		 * the memory now */
		value = htable->tuples[i].value;

		/* NOTE: clear the key and value but keep the hash there 
		 * so that probe chains don't break.  the hash tuples 
		 * with a hash but no key or value will be pruned/freed
		 * at the next resize. */

		/* clean up the key memory */
		if((htable->tuples[i].key != NULL) && (htable->kdfn != NULL))
		{
			/* clean up the key */
			(*(htable->kdfn))(htable->tuples[i].key);
		}

		/* set the key and value pointers to NULL */
		htable->tuples[i].key = NULL;
		htable->tuples[i].value = NULL;

		/* NOTE: don't decrement the number of tuples
		 * in the hashtable so that the emtpy filler
		 * nodes are counted in the load factor calculation. */

		/* return the value */
		return value;
	}

	/* we didn't find it so return NULL */
	return NULL;
}


ht_itr_t ht_itr_begin(ht_t const * const htable)
{
	CHECK_PTR_RET(htable, -1);
	CHECK_RET( htable->tuples != NULL, -1 );

	/* start at 0 and find the first real, non-filler tuple */
	return ht_itr_next(htable, (ht_itr_t)-1);
}

ht_itr_t ht_itr_end(ht_t const * const htable)
{
	CHECK_PTR_RET(htable, -1);
	return -1;
}

ht_itr_t ht_itr_rbegin(ht_t const * const htable)
{
	CHECK_PTR_RET(htable, -1);
	CHECK_RET( htable->tuples != NULL, -1 );

	/* start at hashtable_primes[htable->prime_index] and search down to find
	 * the first non-filler tuple */
	return ht_itr_rnext(htable, (ht_itr_t)(hashtable_primes[htable->prime_index] - 1));
}

ht_itr_t ht_itr_next(ht_t const * const htable, ht_itr_t const itr)
{
	int_t i = itr;
	CHECK_PTR_RET(htable, -1);
	CHECK_RET( htable->tuples != NULL, -1 );

	/* if they aren't at the beginning or end, increment the itr so that it moves to the
	 * next item in the hashtable instead of matching the same one always.
	 */
	if((i >= -1) && (i < (int_t)hashtable_primes[htable->prime_index]))
		i++;

	if(i >= (int_t)hashtable_primes[htable->prime_index])
		return ht_itr_end(htable);

	for(; i < (int_t)hashtable_primes[htable->prime_index]; i++)
	{
		/* if is is a live tuple, return the itr to that */
		if((htable->tuples[i].hash != 0) &&
		   (htable->tuples[i].key != NULL) &&
		   (htable->tuples[i].value != NULL))
			return i;
	}

	return -1;
}

ht_itr_t ht_itr_rnext(ht_t const * const htable, ht_itr_t const itr)
{
	int_t i = itr;
	CHECK_PTR_RET(htable, -1);
	CHECK_RET( htable->tuples != NULL, -1 );

	if(i < (int_t)0)
		return ht_itr_rend(htable);

	for(; i >= (int_t)0; i--)
	{
		/* if is is a live tuple, return the itr to that */
		if((htable->tuples[i].hash != 0) &&
		   (htable->tuples[i].key != NULL) &&
		   (htable->tuples[i].value != NULL))
			return i;
	}

	return -1;
}

void* ht_itr_get(ht_t const * const htable, ht_itr_t const itr, void** key)
{
	CHECK_PTR_RET(htable, NULL);
	CHECK_RET(itr != -1, NULL);
	CHECK_RET(itr < (int_t)hashtable_primes[htable->prime_index], NULL);
	CHECK_RET( htable->tuples != NULL, NULL );

	if((htable->tuples[itr].hash == 0) ||
	   (htable->tuples[itr].key == NULL) ||
	   (htable->tuples[itr].value == NULL))
		return NULL;

	if ( key != NULL )
	{
		(*key) = htable->tuples[itr].key;
	}

	/* return the value at the iterator location in the tuple list */
	return htable->tuples[itr].value;
}

