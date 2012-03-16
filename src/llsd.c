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

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <endian.h>
#include <math.h>
#include <time.h>

#include "debug.h"
#include "macros.h"
#include "hashtable.h"
#include "array.h"
#include "llsd_const.h"
#include "llsd_binary.h"
#include "llsd_xml.h"
#include "llsd.h"
#include "base64.h"

#define FNV_PRIME (0x01000193)
static uint32_t fnv_key_hash(void const * const key)
{
	int i;
	llsd_t * llsd = (llsd_t*)key;
    uint32_t hash = 0x811c9dc5;
	uint8_t const * p = (llsd->value.string_.key_esc ? 
						 (uint8_t const *)llsd->value.string_.esc : 
						 (uint8_t const *)llsd->value.string_.str);
	CHECK_RET_MSG( (llsd->type_ == LLSD_STRING), 0 );
	while ( (*p) != '\0' )
	{
		hash *= FNV_PRIME;
		hash ^= *p++;
	}
	return hash;
}


static int key_eq(void const * const l, void const * const r)
{
	llsd_t * llsd_l = (llsd_t *)l;
	llsd_t * llsd_r = (llsd_t *)r;
	return llsd_string_eq( llsd_l, llsd_r );
}


static void llsd_initialize( llsd_t * llsd, llsd_type_t type_, ... )
{
	va_list args;
	uint8_t * p;
	int size, escaped, encoded, is_key;

	CHECK_PTR( llsd );
	
	llsd->type_ = type_;

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			break;

		case LLSD_BOOLEAN:
			va_start( args, type_ );
			llsd->value.bool_ = va_arg( args, int );
			va_end(args);
			break;

		case LLSD_INTEGER:
			va_start( args, type_ );
			llsd->value.int_ = va_arg(args, int );
			va_end(args);
			break;

		case LLSD_REAL:
			va_start( args, type_ );
			llsd->value.real_ = va_arg(args, double );
			va_end(args);
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			llsd->value.uuid_.dyn_bits = TRUE;
			llsd->value.uuid_.bits = UT(CALLOC( UUID_LEN, sizeof(uint8_t) ));
			MEMCPY( llsd->value.uuid_.bits, va_arg( args, uint8_t* ), UUID_LEN );
			va_end( args );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );
			escaped = va_arg( args, int );
			is_key = va_arg( args, int );

			if ( escaped )
			{
				llsd->value.string_.esc_len = size;
				llsd->value.string_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.string_.dyn_esc = TRUE;
				llsd->value.string_.key_esc = is_key;
				MEMCPY( llsd->value.string_.esc, p, size );
			}
			else
			{
				llsd->value.string_.str_len = size;
				llsd->value.string_.str = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.string_.dyn_str = TRUE;
				MEMCPY( llsd->value.string_.str, p, size );
			}
			va_end( args );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->value.date_.dval = va_arg( args, double );
			size = va_arg( args, int );
			if ( size > 0 )
			{
				p = va_arg( args, uint8_t* );
				llsd->value.date_.str = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.date_.dyn_str = TRUE;
				MEMCPY( llsd->value.date_.str, p, size );
			}
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );
			escaped = va_arg( args, int );

			if ( escaped )
			{
				llsd->value.uri_.esc_len = size;
				llsd->value.uri_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.uri_.dyn_esc = TRUE;
				MEMCPY( llsd->value.uri_.esc, p, size );
			}
			else
			{
				llsd->value.uri_.uri_len = size;
				llsd->value.uri_.uri = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.uri_.dyn_uri = TRUE;
				MEMCPY( llsd->value.uri_.uri, p, size );
			}
			va_end( args );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			size = va_arg( args, int );
			encoded = va_arg( args, int );
			p = va_arg( args, uint8_t* );

			if ( encoded )
			{
				llsd->value.binary_.enc_size = size;
				llsd->value.binary_.enc = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.binary_.dyn_enc = TRUE;
				MEMCPY( llsd->value.binary_.enc, p, size );
			}
			else
			{
				llsd->value.binary_.data_size = size;
				llsd->value.binary_.data = UT(CALLOC( size, sizeof(uint8_t) ));
				llsd->value.binary_.dyn_data = TRUE;
				MEMCPY( llsd->value.binary_.data, p, size );
			}
			va_end( args );
			break;

		case LLSD_ARRAY:
			array_initialize( &(llsd->value.array_.array), 
							  DEFAULT_ARRAY_CAPACITY, 
							  &llsd_delete );
			break;

		case LLSD_MAP:
			ht_initialize( &(llsd->value.map_.ht), 
						   DEFAULT_MAP_CAPACITY, 
						   &fnv_key_hash, 
						   &llsd_delete, 
						   &key_eq, 
						   &llsd_delete );
			break;
	}
}


static void llsd_deinitialize( llsd_t * llsd )
{
	CHECK_PTR( llsd );
	
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
			return;

		case LLSD_UUID:
			if ( llsd->value.uuid_.dyn_str )
				FREE( llsd->value.uuid_.str );
			break;

		case LLSD_DATE:
			if ( llsd->value.date_.dyn_str )
				FREE( llsd->value.date_.str );
			break;

		case LLSD_STRING:
			if ( llsd->value.string_.dyn_str )
				FREE( llsd->value.string_.str );
			if ( llsd->value.string_.dyn_esc )
				FREE( llsd->value.string_.esc );
			break;

		case LLSD_URI:
			if ( llsd->value.uri_.dyn_uri )
				FREE( llsd->value.uri_.uri );
			if ( llsd->value.uri_.dyn_esc )
				FREE( llsd->value.uri_.esc );
			break;

		case LLSD_BINARY:
			if ( llsd->value.binary_.dyn_data )
				FREE( llsd->value.binary_.data );
			if ( llsd->value.binary_.dyn_enc )
				FREE( llsd->value.binary_.enc );
			break;

		case LLSD_ARRAY:
			array_deinitialize( &llsd->value.array_.array );
			break;

		case LLSD_MAP:
			ht_deinitialize( &llsd->value.map_.ht );
			break;
	}
}


llsd_t * llsd_new( llsd_type_t type_, ... )
{
	va_list args;
	llsd_t * llsd;
	int a1, a4, a5;;
	uint8_t * a2;
	double a3;
	
	/* allocate the llsd object */
	llsd = (llsd_t*)CALLOC(1, sizeof(llsd_t));
	CHECK_PTR_RET_MSG( llsd, NULL, "failed to heap allocate llsd object\n" );

	switch( type_ )
	{
		case LLSD_UNDEF:
			llsd_initialize( llsd, type_ );
			break;

		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			va_end( args );
			llsd_initialize( llsd, type_, a1 );
			break;

		case LLSD_REAL:
			va_start( args, type_ );
			a3 = va_arg( args, double );
			va_end( args );
			llsd_initialize( llsd, type_, a3 );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			a3 = va_arg( args, double );	/* double date */
			a1 = va_arg( args, int );		/* len of str date */
			a2 = NULL;
			if ( a1 > 0 )
			{
				a2 = va_arg( args, uint8_t* ); /* pointer to str */
			}
			llsd_initialize( llsd, type_, a3, a1, a2 );
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );  /* pointer to str */
			a1 = va_arg( args, int );		/* length */
			a4 = va_arg( args, int );		/* escaped? */
			a5 = va_arg( args, int );		/* is used as map key */
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1, a4, a5 );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );	/* poniter to str */
			a1 = va_arg( args, int );		/* length */
			a4 = va_arg( args, int );		/* escaped? */
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1, a4 );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a1, a2 );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			llsd_initialize( llsd, type_ );
			break;
	}

	return llsd;
}

void llsd_delete( void * p )
{
	llsd_t * llsd = (llsd_t *)p;
	CHECK_PTR( llsd );

	/* deinitialize it */
	llsd_deinitialize( llsd );

	FREE( llsd );
}

llsd_type_t llsd_get_type( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, LLSD_UNDEF );
	return llsd->type_;
}

int8_t const * llsd_get_type_string( llsd_type_t type_ )
{
	CHECK_RET( ((type_ >= LLSD_TYPE_FIRST) && (type_ <= LLSD_TYPE_LAST)), NULL );
	return llsd_type_strings[ type_ ];
}

int llsd_get_size( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( ((llsd->type_ == LLSD_ARRAY) || (llsd->type_ == LLSD_MAP)), -1, "getting size of non-container llsd object\n" );

	if ( llsd->type_ == LLSD_ARRAY )
		return array_size( &llsd->value.array_.array );
	
	return ht_size( &llsd->value.map_.ht );
}


void llsd_array_append( llsd_t * arr, llsd_t * data )
{
	CHECK_PTR( arr );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(arr) == LLSD_ARRAY, "trying to append data to non array\n" );
	array_push_tail( &(arr->value.array_.array), (void*)data );
}

void llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data )
{
	CHECK_PTR( map );
	CHECK_PTR( key );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );
	CHECK_MSG( llsd_get_type(key) == LLSD_STRING, "trying to use non-string as key\n" );
	ht_add( &(map->value.map_.ht), (void*)key, (void*)data );
}

llsd_t * llsd_map_find( llsd_t * map, llsd_t * key )
{
	CHECK_PTR_RET( map, NULL );
	CHECK_PTR_RET( key, NULL );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );
	CHECK_MSG( llsd_get_type(key) == LLSD_STRING, "trying to use non-string as key\n" );
	return ht_find( &(map->value.map_.ht), (void*)key );
}

llsd_itr_t llsd_itr_begin( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, (llsd_itr_t)-1 );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_begin( &(llsd->value.array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_begin( &(llsd->value.map_.ht) );
	}
	return (llsd_itr_t)0;
}

llsd_itr_t llsd_itr_end( llsd_t * llsd )
{
	return (llsd_itr_t)-1;
}

llsd_itr_t llsd_itr_rbegin( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, (llsd_itr_t)-1 );
	
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_rbegin( &(llsd->value.array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rbegin( &(llsd->value.map_.ht) );
	}
	return (llsd_itr_t)0;
}

llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_next( &(llsd->value.array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_next( &(llsd->value.map_.ht), (ht_itr_t)itr );
	}
	return (llsd_itr_t)-1;
}

llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_rnext( &(llsd->value.array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rnext( &(llsd->value.map_.ht), (ht_itr_t)itr );
	}
	return (llsd_itr_t)-1;
}

int llsd_itr_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key )
{
	llsd_t * k;
	llsd_t * v;
	CHECK_PTR_RET( value, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( itr != -1, FALSE );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			(*value) = (llsd_t*)array_itr_get( &(llsd->value.array_.array), itr );
			(*key) = NULL;
			return TRUE;
		case LLSD_MAP:
			(*value) = (llsd_t*)ht_itr_get( &(llsd->value.map_.ht), itr, (void**)key );

			return TRUE;
	}

	/* non-container iterator just references the llsd */
	(*value) = llsd;
	(*key) = NULL;
	return TRUE;
}


