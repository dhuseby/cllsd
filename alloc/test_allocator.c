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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <debug.h>
#include <macros.h>
#include "test_allocator.h"

typedef struct tag_s
{
	uint32_t size: 31;
	uint32_t in_use: 1;
	uint8_t  data[0];

} tag_t;

/* minimum block size in bytes */
#define MIN_BLOCK_SIZE ( 16 )

/* pointer to the allocator state */
static void * buffer = NULL;
static uint8_t * pool = NULL;
static size_t pool_size = 0;
static size_t in_use = 0;


static int update_block( tag_t * block, size_t size, int in_use )
{
	uint8_t * p = NULL;
	tag_t * tag_head = NULL;
	tag_t * tag_tail = NULL;
	tag_t * tag2_head = NULL;
	tag_t * tag2_tail = NULL;
	size_t bsize = 0;

	CHECK_PTR_RET( pool, FALSE );
	CHECK_PTR_RET( block, FALSE );

	/* the size needs to be a multiple of 4 in bytes */
	bsize = ((size + 3) & ~0x3);

	/* then the block size (bsize) must be at least MIN_BLOCK_SIZE */
	bsize = ((bsize + (2 * sizeof(tag_t))) < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : (bsize + (2 * sizeof(tag_t)));

	/* now the bsize must be a multiple of 16 bytes */
	bsize = ((bsize + 15) & ~0xF);

	/* get the pointer to the tag */
	tag_head = block;  /* this will be sizeof(tag_t) less than a 16-byte aligned address */
	tag_tail = (tag_t*)((void*)tag_head + (tag_head->size - sizeof(tag_t)));

	/* sanity check */
	CHECK_RET_MSG( (((uint32_t)&(tag_head->data[0]) & (uint32_t)0xF) == 0), FALSE, "block isn't 16-byte aligned\n" );

	/* SANITY CHECK */
	CHECK_RET( (tag_head->size == tag_tail->size), FALSE );
	CHECK_RET( (tag_head->in_use == tag_tail->in_use), FALSE );

	if ( in_use )
	{
		/* make sure the selected block is not in use */
		CHECK_RET( !tag_head->in_use, FALSE );
		
		/* make sure the block is large enough */
		CHECK_RET( (tag_head->size >= bsize), FALSE );

		if ( (tag_head->size - bsize) > MIN_BLOCK_SIZE )
		{
			/* we need to subdivide the current block into two */

			/* get the pointer to the tag at the start of the new empty block */
			tag2_head = ((void*)tag_head) + bsize;
			tag2_tail = ((void*)tag_head) + (tag_head->size - sizeof(tag_t));
			tag_tail = (tag_t*)((void*)tag2_head - sizeof(tag_t));

			/* mark the second block as not in use */
			tag2_head->in_use = tag2_tail->in_use = FALSE;

			/* calculate the size of the new block */
			tag2_head->size = tag2_tail->size = tag_head->size - bsize;

			/* update the size of the old block */
			tag_head->size = tag_tail->size = bsize;

			/* sanity check */
			CHECK_RET_MSG( (((uint32_t)&(tag_head->data[0]) & (uint32_t)0xF) == 0), FALSE, "new trailing block isn't 16-byte aligned\n" );
		}

		/* update total in_use count */
		in_use += bsize;
	
		/* mark the block as in-use */
		tag_head->in_use = tag_tail->in_use = TRUE;

		return TRUE;
	}
	else
	{
		/* make sure the selected block is in use */
		CHECK_RET( tag_head->in_use, FALSE );

		/* mark the block as not in use */
		tag_head->in_use = tag_tail->in_use = FALSE;

		/* get the pointer to the previous block */
		tag2_tail = (tag_t*)((void *)tag_head - sizeof(tag_t));

		if ( !tag2_tail->in_use )
		{
			/* coalesce the previous block with this block */
		
			/* get a pointer to the previous block's head tag */
			tag2_head = (tag_t*)((void *)tag_head - tag2_tail->size);

			/* sanity check */
			CHECK_RET_MSG( (tag2_head->size == tag2_tail->size), FALSE, "bad sizes when coalescing preceding block\n" );
			CHECK_RET_MSG( (tag2_head->in_use == tag2_tail->in_use), FALSE, "bad in_use flags when coalescing preceding block\n" );

			/* update it's size */
			tag2_head->size += tag_head->size;

			/* adjust the tail of the coalesced block */
			tag_tail->size = tag2_head->size;

			/* update pointers */
			tag_head = tag2_head;
		}

		/* get the pointer to the block after */
		tag2_head = (tag_t*)((void *)tag_tail + sizeof(tag_t));

		if ( !tag2_head->in_use )
		{
			/* coalesce the following block with this block */

			/* get a pointer to the following block's tail */
			tag2_tail = (tag_t*)((void*)tag2_head + (tag2_head->size - sizeof(tag_t)));

			/* sanity check */
			CHECK_RET_MSG( (tag2_head->size == tag2_tail->size), FALSE, "bad sizes when coalescing following block\n" );
			CHECK_RET_MSG( (tag2_head->in_use == tag2_tail->in_use), FALSE, "bad in_use flags when coalescing following block\n" );

			/* update size for new coalesced block */
			tag_head->size += tag2_head->size;

			/* update tail of new coalesced block */
			tag2_tail->size = tag_head->size;

			/* update pointers */
			tag_tail = tag2_tail;
		}

		/* one last sanity check */
		CHECK_RET_MSG( (tag_head->size == tag_tail->size), FALSE, "coalesced block had mismatched sizes\n" );
		CHECK_RET_MSG( (tag_head->in_use == tag_tail->in_use), FALSE, "coalesced block had mismatched in_use flags\n" );

		/* update total in_use count */
		in_use -= tag_head->size;

		return TRUE;
	}

	return FALSE;
}

static void * get_mem( size_t size )
{
	tag_t * tag = NULL;
	CHECK_RET( pool, NULL );

	/* do a slow linear search for the first block with enough mem */
	tag = (tag_t*)&(pool[0]);
	while( (void*)tag < (void*)&(pool[pool_size - sizeof(tag_t)]) )
	{
		if ( tag->in_use )
		{
			tag = (tag_t*)((void*)tag + tag->size);
			continue;
		}

		if ( tag->size > (size + (2* sizeof(tag_t))) )
		{
			if( update_block( tag, size, TRUE ) )
			{
				/* return the pointer to the memory block */
				return (void*)&(tag->data[0]);
			}
			else
			{
				DEBUG("failed to find a block large enough\n");
				return NULL;
			}
		}
	}
}

static void put_mem( void * p )
{
	tag_t * tag = NULL;
	CHECK_PTR( p );

	/* get the tag pointer */
	tag = (tag_t*)(p - sizeof(tag_t));

	if ( !update_block( tag, tag->size, FALSE ) )
	{
		DEBUG( "failed to return block at %p\n", (void*)tag );
	}
}


void init_alloc( size_t psize )
{
	tag_t * tag = NULL;
	uint8_t * p = NULL;
	CHECK( psize > 0 );

	/* allocate the buffer */
	buffer = calloc( 1, psize + 15 );
	CHECK_PTR_MSG( buffer, "failed to allocate memory pool\n" );

	/* get a 16-byte aligned pool address */
	pool = (uint8_t*)((uint32_t)(buffer + 15) & ~(uint32_t)0xF);


	/* calculate the last 16-byte aligned address in the buffer */
	p = pool + psize;
	p = (uint8_t*)((uint32_t)p & ~(uint32_t)0xF);

	/* calculate pool size from difference in pointers */
	pool_size = (size_t)(p - pool);
	in_use = 0;

	/* initialize the tags in the pool */

	/* init the leading guard tag */
	tag = (tag_t*)&(pool[0]);
	tag->size = 16 - sizeof(tag_t);
	tag->in_use = TRUE;
	tag = (tag_t*)((void*)tag + (tag->size - sizeof(tag_t)));
	tag->size = 16 - sizeof(tag_t);
	tag->in_use = TRUE;

	/* init the trailing guard tag */
	tag = (tag_t*)&(pool[pool_size - (16 + sizeof(tag_t))]);
	tag->size = 16 + sizeof(tag_t);
	tag->in_use = TRUE;
	tag = (tag_t*)((void*)tag + 16);
	tag->size = 16 + sizeof(tag_t);
	tag->in_use = TRUE;

	/* update the pool size */
	pool_size -= 32;

	/* now the init the one block for the rest of the space */
	tag = (tag_t*)&(pool[(16 - sizeof(tag_t))]);  /* this makes the tag->data 16-byte aligned */
	tag->size = pool_size;
	tag->in_use = FALSE;
	tag = (tag_t*)((void*)tag + (tag->size - sizeof(tag_t)));
	tag->size = pool_size;
	tag->in_use = FALSE;
}

void deinit_alloc( void )
{
	free( buffer );
	buffer = NULL;
	pool = NULL;
}

size_t get_in_use( void )
{
	CHECK_PTR_RET( pool, 0 );
	return in_use;
}

void dump_blocks( void )
{
	tag_t * tag = NULL;
	CHECK( pool );

	/* do a slow linear search for the first block with enough mem */
	tag = (tag_t*)&(pool[sizeof(tag_t)]);
	while( (void*)tag < (void*)&(pool[pool_size - sizeof(tag_t)]) )
	{
		LOG( "%p -- size: %d, in_use: %s\n", (void*)tag, tag->size, (tag->in_use ? "TRUE" : "FALSE") );

		/* move to the next block */
		tag = (tag_t*)((void*)tag + tag->size);
	}
}

void * calloc_( size_t nmemb, size_t size )
{
	void * p;

	/* try to get a block of memory */
	p = get_mem( nmemb * size );
	CHECK_PTR_RET( p, NULL );

	/* now zero it ouc */
	memset( p, 0, nmemb * size );

	return p;
}

void * malloc_( size_t size )
{
	return get_mem( size );
}

void free_( void * ptr )
{
	return put_mem( ptr );
}

void * realloc_( void * ptr, size_t size )
{
	/* TODO */
}

uint8_t * strdup_( uint8_t const * str )
{
	size_t len = 0;
	void * p = NULL;
	CHECK_PTR_RET( str, NULL );

	/* calculate the string length */
	len = strlen( str ) + 1;

	/* get a new block of memory */
	p = get_mem( len );

	/* copy the string over */
	memcpy( p, str, len );

	return p;
}


