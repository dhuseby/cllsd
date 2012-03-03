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

#include "debug.h"
#include "macros.h"
#include "array.h"

/* node used in queue structure */
struct array_node_s
{
    struct array_node_s *	next;                   /* next link */
    struct array_node_s *	prev;                   /* prev link */
    void *					data;                   /* pointer to the data */
	uint32_t				dummy;
};

/* index constants */
array_itr_t const array_itr_end_t = -1;

#ifdef USE_THREADING
/*
 * This function locks the array mutex.
 */
void array_lock(array_t * const array)
{
    CHECK_PTR(array);

    /* lock the mutex */
    pthread_mutex_lock(&array->lock);
}

/*
 * This function tries to lock the mutex associated with the array.
 */
int array_try_lock(array_t * const array)
{
    CHECK_PTR_RET(array, 0);
    
    /* try to lock the mutex */
    return (pthread_mutex_trylock(&array->lock) == 0);
}

/*
 * This function unlocks the array mutex.
 */
void array_unlock(array_t * const array)
{
    CHECK_PTR(array);

    /* unlock the mutex */
    pthread_mutex_unlock(&array->lock);
}

/*
 * Returns the mutex pointer for use with condition variables.
 */
pthread_mutex_t * array_mutex(array_t * const array)
{
    CHECK_PTR_RET(array, NULL);

    /* return the mutex pointer */
    return &array->lock;
}
#endif/*USE_THREADING*/

/*
 * This function initializes the array_t structure by allocating
 * a node buffer and initializing the indexes for the data head
 * and the free list head.  It then initializes the linked lists.
 * The function pointer points to a destructor for the nodes that 
 * are going to be stored in the data structure.  If NULL is passed 
 * for the pfn parameter, then no action will be taken when the 
 * data structure is deleted.
 */
void array_initialize( array_t * const array, 
					   uint_t initial_capacity,
					   delete_fn pfn )
{
    uint_t i = 0;
    CHECK_PTR(array);

    /* zero out the memory */
    memset(array, 0, sizeof(array_t));

	/* store the delete function pointer */
	array->pfn = pfn;

    /* remember the buffer size */
    array->buffer_size = initial_capacity;

	/* remember the initial capacity */
	array->initial_capacity = initial_capacity;

    /* initialize the data list */
    array->data_head = -1;

    /* initialize the free list */
    array->free_head = 0;

    /* allocate the initial node buffer only if there is an initial capacity */
	if ( initial_capacity > 0 )
	{
		array->node_buffer = (array_node_t*)CALLOC(initial_capacity, sizeof(array_node_t));
		ASSERT(array->node_buffer != NULL);

		/* fixup the head of the free list */
		array->node_buffer[0].prev = &array->node_buffer[initial_capacity - 1];

		/* fixup the tail of the free list */
		array->node_buffer[initial_capacity - 1].next = &array->node_buffer[0];

		/* fixup the middle nodes */
		for(i = 0; i < initial_capacity; i++)
		{
			if(array->node_buffer[i].next == NULL)
				array->node_buffer[i].next = &array->node_buffer[i + 1];

			if(array->node_buffer[i].prev == NULL)
				array->node_buffer[i].prev = &array->node_buffer[i - 1];
		}
	}

#ifdef USE_THREADING
    /* initialize the mutex */
    pthread_mutex_init(&array->lock, 0);
#endif
}

/*
 * This function allocates a new array structure and
 * initializes it.
 */
array_t * array_new( uint_t initial_capacity, delete_fn pfn )
{
    array_t * array = NULL;

    /* allocate the array structure */
    array = (array_t*)MALLOC(sizeof(array_t));

    /* initialize the array */
    array_initialize( array, initial_capacity, pfn );

    /* return the new array */
    return array;
}


/*
 * This function iterates over the list of nodes passing each one to the
 * destroy function if one was provided.  If there wasn't one provided
 * then nothing will be done to clean up each node.
 */
void array_deinitialize(array_t * const array)
{
    int_t ret = 0;
    uint_t i = 0;
    array_node_t* node = NULL;
    CHECK_PTR(array);

    /* iterate over the list calling the destructor for each node */
    if((array->pfn != NULL) && (array->node_buffer != NULL))
    {
        node = array->node_buffer;
        for(i = 0; i < array->num_nodes; i++)
        {
            /* if there is data, free it */
            if((node != NULL) && (node->data != NULL))
            {
                /* free up the data */
                (*(array->pfn))(node->data);

                node->data = NULL;
            }

            /* move to the next node in the list */
            node++;
        }
    }

    /* free up the node buffer */
    FREE(array->node_buffer);
    array->node_buffer = NULL;

#ifdef USE_THREADING
    /* destroy the lock */
    ret = pthread_mutex_destroy(&array->lock);
    ASSERT(ret == 0);
#endif

    /* invalidate the delete function */
    array->pfn = NULL;

    /* invalidate the two list heads */
    array->data_head = -1;
    array->free_head = -1;

    /* clear out the counts */
    array->num_nodes = 0;
    array->buffer_size = 0;
}


/* this frees an allocated array structure */
void array_delete(array_t * const array)
{
    CHECK_PTR(array);

    /* deinit the array */
    array_deinitialize(array);

    /* free the array */
    FREE(array);
}


/* get the size of the array in a thread-safe way */
int_t array_size(array_t const * const array)
{
    CHECK_PTR_RET(array, -1);

    return (int_t)array->num_nodes;
}


/* 
 * this function doubles the node buffer size and moves the nodes from the old
 * buffer to the new buffer, compacting them at the same time
 */
static int array_grow(array_t * const array)
{
    uint_t new_free_head = 0;
    uint_t i = 0;
    uint_t old_size = 0;
    uint_t new_size = 0;
    array_node_t* old_itr = NULL;
    array_node_t* new_buffer = NULL;
    CHECK_PTR_RET(array, 0);

    /* get the old size */
    old_size = array->buffer_size;

    /* get the new size */
	if ( old_size == 0 )
	{
		new_size = 16;
	}
	else if ( old_size >= 256 )
	{
		new_size = (old_size + 256);
	}
	else
	{
		new_size = (old_size << 1);
	}

    /* allocate the new buffer */
    new_buffer = CALLOC(new_size, sizeof(array_node_t));
    CHECK_RET(new_buffer != NULL, 0);

    /* copy the existing nodes to the new new buffer if needed */
    if(array_size(array) > 0)
    {
        /* move the new free head to the node after the end of the data list */
        new_free_head = array_size(array);

        /* fix up the head of the data list */
        new_buffer[0].prev = &new_buffer[new_free_head - 1];

        /* fix up the tail of the data list */
        new_buffer[new_free_head - 1].next = &new_buffer[0];

        /* fix up the middle nodes */
        old_itr = &array->node_buffer[array->data_head];
        for(i = 0; i < new_free_head; i++)
        {
            /* set up the next link if it isn't already */
            if(new_buffer[i].next == NULL)
                new_buffer[i].next = &new_buffer[i + 1];

            /* set up the prev link if it isn't already */
            if(new_buffer[i].prev == NULL)
                new_buffer[i].prev = &new_buffer[i - 1];

            /* copy the data pointer */
            new_buffer[i].data = old_itr->data;

            /* move to the next node in the old data list */
            old_itr = old_itr->next;
        }

        /* fix up the head of the data list index */
        array->data_head = 0;

        /* fix up the head of the free list index */
        array->free_head = (int_t)new_free_head;
    }

    /* free up the old buffer */
    FREE(array->node_buffer);

    /* store the new buffer pointer */
    array->node_buffer = new_buffer;
    
    /* store the new size */
    array->buffer_size = new_size;

    /* fix up the head of the free list */
    array->node_buffer[array->free_head].prev = &array->node_buffer[array->buffer_size - 1];

    /* fix up the tail of the free list */
    array->node_buffer[array->buffer_size - 1].next = &array->node_buffer[array->free_head];
    
    /* fix up the middle free nodes */
    for(i = array->free_head; i < array->buffer_size; i++)
    {
        /* initialize the prev pointer if needed */
        if(array->node_buffer[i].prev == NULL)
            array->node_buffer[i].prev = &array->node_buffer[i - 1];

        /* initialize the next pointer if needed */
        if(array->node_buffer[i].next == NULL)
            array->node_buffer[i].next = &array->node_buffer[i + 1];

        /* initialize the data pointer */
        array->node_buffer[i].data = NULL;
    }

    return 1;
}


static array_node_t* array_get_free_node(array_t * const array)
{
    array_node_t* node = NULL;
    array_node_t* new_free_head = NULL;
    CHECK_PTR_RET(array, NULL);

    if((array->num_nodes + 1) >= array->buffer_size)
    {
        /* we need to grow the buffer */
        if(!array_grow(array))
        {
            WARN("failed to grow the array!");
            return NULL;
        }
    }

    /* remove the node at the head of the free list */
    node = &array->node_buffer[array->free_head];

    /* get the pointer to the new free head */
    new_free_head = node->next;

    /* remove the old head */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;

    /* calculate the new free head index */
    array->free_head = (int_t)((uint_t)new_free_head - (uint_t)array->node_buffer) / sizeof(array_node_t);

    /* return the old free head */
    return node;
}


static void array_put_free_node(
    array_t * const array, 
    array_node_t* const node)
{
    array_node_t* old_free_head = NULL;
    CHECK_PTR(array);
    CHECK_PTR(array);

    /* clear the data pointer */
    node->data = NULL;

    /* get the old free head */
    old_free_head = &array->node_buffer[array->free_head];

    /* hook in the new head */
    node->prev = old_free_head->prev;
    node->prev->next = node;
    node->next = old_free_head;
    node->next->prev = node;

    /* calculate the new free head index */
    array->free_head = (int_t)((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t);
}

/* get an iterator to the start of the array */
array_itr_t array_itr_begin(array_t const * const array)
{
    CHECK_PTR_RET(array, array_itr_end_t);
    CHECK_RET(array_size(array) > 0, array_itr_end_t);

    return array->data_head;
}

/* get an iterator to the end of the array */
array_itr_t array_itr_end(array_t const * const array)
{
    return array_itr_end_t;
}

/* get an iterator to the last item in the array */
array_itr_t array_itr_tail(array_t const * const array)
{
	array_node_t* tail = NULL;
	
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	
	/* get a pointer to the tail */
	tail = array->node_buffer[array->data_head].prev;
	
	/* calculate return the iterator pointed to the tail item */
	return (array_itr_t)(((uint_t)tail - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

/* iterator based access to the array elements */
array_itr_t array_itr_next(
    array_t const * const array, 
    array_itr_t const itr)
{
    array_node_t* node = NULL;
	
    CHECK_PTR_RET(array, array_itr_end_t);
    CHECK_RET(array_size(array) > 0, array_itr_end_t);
    CHECK_RET(itr != array_itr_end_t, array_itr_end_t);

    /* get a pointer to the node the iterator points to */
    node = array->node_buffer[itr].next;

    /* check that we got a valid node pointer */
    CHECK_RET(node != NULL, array_itr_end_t);

    /* check to see if we've gone full circle */
    if(node == &array->node_buffer[array->data_head])
    {
        /* yep so return -1 */
        return array_itr_end_t;
    }

    /* check that the node is still in the data list */
    CHECK_RET(node->data != NULL, array_itr_end_t);

    /* return the index of the node */
    return (array_itr_t)(((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

array_itr_t array_itr_rnext(
    array_t const * const array, 
    array_itr_t const itr)
{
    array_node_t* node = NULL;
	
    CHECK_PTR_RET(array, array_itr_end_t);
    CHECK_RET(array_size(array) > 0, array_itr_end_t);
    CHECK_RET(itr != array_itr_end_t, array_itr_end_t);

    /* get a pointer to the node the iterator points to */
    node = &(array->node_buffer[itr]);

    /* check that we got a valid node pointer */
    CHECK_RET(node != NULL, array_itr_end_t);

    /* check to see if we've gone full circle */
    if(node == &array->node_buffer[array->data_head])
    {
        /* yep so return -1 */
        return array_itr_end_t;
    }

	/* now get the previous node pointer */
	node = array->node_buffer[itr].prev;

    /* check that we got a valid node pointer */
    CHECK_RET(node != NULL, array_itr_end_t);

    /* check that the node is still in the data list */
    CHECK_RET(node->data != NULL, array_itr_end_t);

    /* return the index of the node */
    return (array_itr_t)(((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

/* 
 * push a piece of data into the list at the iterator position
 * ahead of the piece of data already at that position.  To
 * push a piece of data at the tail so that it is the last item
 * in the array, you must use the iterator returned from the
 * array_itr_end() function.
 */
void array_push(
    array_t * const array, 
    void * const data, 
    array_itr_t const itr)
{
    array_node_t * node = NULL;
    array_itr_t const i = ((itr == array_itr_end(array)) ? array->data_head : itr);
    CHECK_PTR(array);
    CHECK_PTR(data);
	CHECK(array_size(array) > 0);
	
    /* a node from the free list */
    node = array_get_free_node(array);
    ASSERT(node != NULL);
	
    /* store the data pointer */
    node->data = data;
	
    /* hook the new node into the list where the iterator specifies */	
    if(array_size(array) > 0)
    {
        node->prev = array->node_buffer[i].prev;
        node->prev->next = node;
        node->next = &array->node_buffer[i];
        node->next->prev = node;
    }
    else
    {
        node->prev = node;
        node->next = node;
    }
	
    /* 
     * if there already is a node in the list and they were pushing
     * onto the tail, recalculate the new data head index. otherwise
     * always update the data head index
     */
    if(array_size(array) > 0)
    {
        if(itr != array_itr_end(array))
            array->data_head = (int_t)((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t);
    }
    else
    {
        array->data_head = (int_t)((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t);
    }

    /* we added an item to the list */
    array->num_nodes++;
}


/* pop the data at the location specified by the iterator */
void * array_pop(
    array_t * const array, 
    array_itr_t const itr)
{
    void * ret = NULL;
    array_node_t * next = NULL;
    array_node_t * node = NULL;
	array_itr_t const i = ((itr == array_itr_end(array)) ? array->data_head : itr);
    CHECK_PTR_RET(array, NULL);
    CHECK_RET(array_size(array) > 0, NULL);
    
    /* get a pointer to the node */
    node = &array->node_buffer[i];
	
	/* bail if the node is no longer in the data list */
    CHECK_RET(node->data != NULL, NULL);
	
	/* remember the next node */
	if(itr == (array_itr_t)array->data_head)
		next = node->next;

    /* unhook it from the list */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;

    /* get the pointer to return */
    ret = node->data;

    /* put the node back on the free list */
    array_put_free_node(array, node);

    /* we removed an item to the list */
    array->num_nodes--;
	
    /* 
     * handle the special case where the iterator pointed to the head
     * by adjusting the head index correctly
     */
    if(array->num_nodes == 0)
    {
        /* the list is empty so reset the head index */
        array->data_head = array_itr_end(array);
    }
    else
    {
        /* calculate the new data head index */
        array->data_head = (int_t)((uint_t)next - (uint_t)array->node_buffer) / sizeof(array_node_t);
    }
    
    return ret;
}

/* get a pointer to the data at the location specified by the iterator */
void* array_itr_get(
    array_t const * const array, 
    array_itr_t const itr)
{
    CHECK_PTR_RET(array, NULL);
    CHECK_RET(array_size(array) > 0, NULL);
	CHECK_RET(itr != array_itr_end(array), NULL);

    /* return the pointer to the head's data */
    return array->node_buffer[itr].data;
}

/* clear the contents of the array */
void array_clear(array_t * const array)
{
	uint_t initial_capacity = 0;
    delete_fn pfn = NULL;
    CHECK_PTR(array);
    
    /* store the existing params */
    pfn = array->pfn;
	initial_capacity = array->initial_capacity;
    
    /* de-init the array and clean up all the memory except for
     * the array struct itself */
    array_deinitialize( array );
    
    /* clear out the array struct */
    memset(array, 0, sizeof(array_t));

    /* reinitialize the array with the saved params */
    array_initialize( array, initial_capacity, pfn );
}

