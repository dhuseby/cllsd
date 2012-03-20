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

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stdint.h>

#ifndef __UINT__
#define __UINT__
#ifdef __amd64__
	typedef uint64_t uint_t;
	typedef int64_t int_t;
#else
	typedef uint32_t uint_t;
	typedef int32_t int_t;
#endif
#endif

#ifdef USE_THREADING
#include <pthread.h>
#endif

/* define the delete function ponter type */
typedef void (*delete_fn)(void*);

/* defines the array iterator type */
typedef int_t array_itr_t;

/* internal node struct */
typedef struct array_node_s array_node_t;

/* dynamic array struct */
typedef struct array_s
{
	delete_fn		pfn;					/* destruction function for each node */
	uint_t			num_nodes;				/* number of nodes in the list */
	uint_t			buffer_size;			/* number of slots in the node array */
	uint_t			initial_capacity;		/* the initial capacity value */
	int_t			data_head;				/* head node of the data circular list */
	int_t			free_head;				/* head node of the free circular list */
	array_node_t*	node_buffer;			/* buffer of nodes */
#ifdef USE_THREADING
	pthread_mutex_t lock;					/* list lock */
#endif
} array_t;

#ifdef USE_THREADING
/* threading protection */
void array_lock(array_t * const array);
int array_try_lock(array_t * const array);
void array_unlock(array_t * const array);
pthread_mutex_t * array_mutex(array_t * const array);
#endif/*USE_THREADING*/

void array_initialize( array_t * const array, uint_t initial_capacity, delete_fn dfn );
void array_deinitialize( array_t * const array );

/* array new/delete functions */
array_t * array_new( uint_t initial_capacity, delete_fn dfn );
void array_delete( array_t * const array );

/* gets the size of the array */
int_t array_size(array_t const * const array);

/* functions for getting iterators */
array_itr_t array_itr_begin(array_t const * const array);
array_itr_t array_itr_end(array_t const * const array);
#define array_itr_head(x) array_itr_begin(x)
array_itr_t array_itr_tail(array_t const * const array);
#define array_itr_rbegin(x) array_itr_tail(x)
#define array_itr_rend(x) array_itr_end(x)

/* iterator manipulation functions */
array_itr_t array_itr_next(
	array_t const * const array, 
	array_itr_t const itr);
array_itr_t array_itr_rnext(
	array_t const * const array,
	array_itr_t const itr );

/* O(1) functions for adding items to the array */
void array_push(
	array_t * const array, 
	void * const data, 
	array_itr_t const itr);
#define array_push_head(array, data) array_push(array, data, array_itr_head(array));
#define array_push_tail(array, data) array_push(array, data, array_itr_end(array));

/* O(1) functions for removing items from the array */
void* array_pop(
	array_t * const array, 
	array_itr_t const itr);
#define array_pop_head(array) array_pop(array, array_itr_head(array))
#define array_pop_tail(array) array_pop(array, array_itr_end(array))

/* functions for getting a reference to an item in the array */
void* array_itr_get(
	array_t const * const array, 
	array_itr_t const itr);
#define array_get_head(array) array_itr_get(array, array_itr_head(array))
#define array_get_tail(array) array_itr_get(array, array_itr_tail(array))

/* clear the entire array */
void array_clear(array_t * const array);


#endif/*__ARRAY_H__*/

 
