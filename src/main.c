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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "hashtable.h"
#include "btree.h"
#include "macros.h"

typedef struct wf_s
{
	int8_t *	buffer;
	int8_t **	words;
	int			nwords;
	int			cur;
} wf_t;

/* read an entire file into memory */
wf_t *  wf_new( int8_t const * const fname )
{
	long size;
	size_t left;
	size_t ret;
	FILE * fin;
	int i;
	int nwords;
	int8_t * pread;
	int8_t * pend;
	wf_t * wf;

	wf = CALLOC( 1, sizeof(wf_t) );
	fin = fopen( fname, "rb" );

	/* go to the end of the file to get the size */
	fseek( fin, 0L, SEEK_END );
	size = ftell( fin );

	/* go back to the beginning */
	rewind( fin );

	wf->buffer = CALLOC( size, sizeof(int8_t) );

	pread = wf->buffer;
	left = size;
	while( left > 0 )
	{
		ret = fread( pread, 1, left, fin );
		printf( "read %d bytes\n", ret );
		left -= ret;
		pread += ret;
	}
	fclose( fin );

	/* count and parse the words */
	pread = wf->buffer;
	pend = wf->buffer + size;
	nwords = 0;
	while ( pread != pend )
	{
		while( (*pread != '\r' ) && (pread != pend) )
		{
			pread++;
		}

		*pread = '\0';
		nwords++;
		pread++; /* points to \n */
		pread++; /* points to first char of next word */
	}

	i = 0;
	wf->words = CALLOC( nwords, sizeof(int8_t *) );
	wf->nwords = nwords;
	pread = wf->buffer;
	pend = wf->buffer + size;
	while ( (pread < pend) && (i < nwords) )
	{
		wf->words[i] = pread;
		while (*pread != '\0')
			pread++;

		/* advance to the start of the next word */
		pread++;
		pread++;

		i++;
	}
	wf->cur = 0;

	printf("read %d words from %s\n", wf->nwords, fname);
	return wf;
}

void wf_delete( void * p )
{
	wf_t * wf = (wf_t*)p;
	FREE(wf->words);
	FREE(wf->buffer);
	FREE(wf);
}

int8_t * wf_nextword( wf_t * const wf )
{
	int8_t * pword = 0;
	if ( wf->cur >= wf->nwords )
		return 0;

	pword = wf->words[wf->cur];
	(wf->cur)++;
	return pword;
}

void wf_reset( wf_t * const wf )
{
	wf->cur = 0;
}

#define FNV_PRIME (0x01000193)
static uint32_t fnv_key_hash(void const * const key)
{
    uint32_t hash = 0x811c9dc5;
	uint8_t const * p = (uint8_t const *)key;
	while ( (*p) != '\0' )
	{
		hash *= FNV_PRIME;
		hash ^= *p++;
	}
	return hash;
}

static int key_eq(void const * const l, void const * const r)
{
	return (strcmp( l, r ) == 0);
}

void test_hashtable( wf_t * const wf, ht_t * const ht )
{
	int32_t i;
	int8_t * pword;

	for(;;)
	{
		pword = wf_nextword( wf );
		if (pword == 0)
			break;

		/* update the word count */
		i = (int32_t)ht_find( ht, pword );
		i++;
		ht_add( ht, pword, (void*)i );
	}
}

static int str_key_cmp( void * l, void * r)
{
	return strcmp((const char *)l, (const char*)r);
}

void test_btree( wf_t * const wf, bt_t * const btree )
{
	int32_t i;
	int8_t * pword;

	for(;;)
	{
		pword = wf_nextword( wf );
		if ( pword == 0 )
			break;

		/* update the word count */
		i = (int32_t)bt_find( btree, pword );
		i++;
		bt_add( btree, pword, (void*)i );
	}
}

struct timespec diff( struct timespec start, struct timespec end )
{
	struct timespec tmp;
	if ((end.tv_nsec - start.tv_nsec) < 0)
	{
		tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
		tmp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		tmp.tv_sec = end.tv_sec - start.tv_sec;
		tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return tmp;
}

uint32_t ts_in_ms( struct timespec t )
{
	return ((t.tv_sec * 1000) + (uint32_t)(t.tv_nsec / 1000000));
}

int main(int argc, char** argv)
{
	wf_t * wf;
	ht_t * ht;
	bt_t * bt;
	struct timespec tstart;
	struct timespec tstop;

	if ( argc != 2 )
	{
		printf( "usage: %s <word file>\n", argv[0] );
		return EXIT_FAILURE;
	}


	/* set up */
	wf = wf_new( argv[1] );
	ht = ht_new( 65536, fnv_key_hash, NULL, key_eq, NULL );
	bt = bt_new( 65536, str_key_cmp, NULL, NULL );

	/* time the word count */
	clock_gettime( CLOCK_REALTIME, &tstart );
	test_hashtable( wf, ht );
	clock_gettime( CLOCK_REALTIME, &tstop );
	printf("hash table took: %d ms\n", ts_in_ms( diff( tstart, tstop ) ) );

	/* reset the file */
	wf_reset( wf );

	/* time the word count */
	clock_gettime( CLOCK_REALTIME, &tstart );
	test_hashtable( wf, ht );
	clock_gettime( CLOCK_REALTIME, &tstop );
	printf("binary tree took: %d ms\n", ts_in_ms( diff( tstart, tstop ) ) );

	/* tear down */
	bt_delete( bt );
	ht_delete( ht );
	wf_delete( wf );

	return EXIT_SUCCESS;
}

