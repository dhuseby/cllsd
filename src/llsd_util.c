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

#include <stdarg.h>
#include <sys/types.h>
#include <sys/param.h>
#include <math.h>
#include <time.h>
#include <sys/uio.h>
#include <float.h>

#include <cutil/debug.h>
#include <cutil/macros.h>

#include "llsd.h"
#include "llsd_const.h"
#include "llsd_util.h"
#include "llsd_binary.h"
#include "llsd_xml.h"
#include "base16.h"
#include "base64.h"
#include "base85.h"

#if defined(MISSING_STRNLEN)
/* provide our own strnlen if the system doesn't */
uint32_t strnlen( uint8_t const * const s, uint32_t const n )
{
	uint8_t const * p = UT(memchr( s, 0, n ));
	return ( p ? (p - s) : n );
}
#endif

#if defined(MISSING_64BIT_ENDIAN)
uint64_t be64toh( uint64_t const be )
{
	uint8_t c;
	union 
	{
		uint64_t ull;
		uint8_t b[8];
	} x;

	x.ull = (uint64_t)0x01;
	if ( x.b[7] == (uint8_t)0x01 )
	{
		/* big endian platform, do nothing */
		return be;
	}

	/* little endian platform, swap it */
	return ( ( (uint64_t)(ntohl( (uint32_t)((be << 32) >> 32) )) << 32) | 
			    ntohl( ((uint32_t)(be >> 32)) ) );
}

uint64_t htobe64( uint64_t const h )
{
	uint8_t c;
	union 
	{
		uint64_t ull;
		uint8_t b[8];
	} x;

	x.ull = (uint64_t)0x01;
	if ( x.b[7] == (uint8_t)0x01 )
	{
		/* big endian platform, do nothing */
		return h;
	}

	/* little endian platform, swap it */
	return ( ( (uint64_t)(htonl( (uint32_t)((h << 32) >> 32) )) << 32) | 
			    htonl( ((uint32_t)(h >> 32)) ) );
}
#endif


/* this function does the minimum amount of work to the l and r llsd objects
 * so that we can use standard ways of comparing the two.  so if the l object
 * is in an escaped/encoded form and the r is not, the l one will be decoded
 * or unescaped so that l and r can be compared. */
static int llsd_equalize ( llsd_t * l, llsd_t * r )
{
	if ( llsd_get_type( l ) != llsd_get_type( r ) )
		return FALSE;

	switch( llsd_get_type( l ) )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_ARRAY:	
		case LLSD_MAP:
			return TRUE;
		case LLSD_UUID:
			if ( ( ( l->uuid_.bits != NULL ) && ( r->uuid_.bits != NULL ) ) ||
				 ( ( l->uuid_.str != NULL ) && ( r->uuid_.bits != NULL ) ) )
				return TRUE;
			if ( l->uuid_.bits == NULL )
				llsd_destringify_uuid( l );
			else
				llsd_destringify_uuid( r );
			return TRUE;
		case LLSD_STRING:
			if ( ( ( l->string_.str != NULL ) && ( r->string_.str != NULL ) ) ||
				 ( ( l->string_.esc != NULL ) && ( r->string_.esc != NULL ) ) )
				return TRUE;
			if ( l->string_.str == NULL )
				llsd_unescape_string( l );
			else
				llsd_unescape_string( r );
			return TRUE;
		case LLSD_DATE:
			if ( ( ( l->date_.use_dval != FALSE ) && ( r->date_.use_dval != FALSE ) ) ||
				 ( ( l->date_.str != NULL ) && ( r->date_.str != NULL ) ) )
				return TRUE;
			if ( l->date_.use_dval == FALSE )
				llsd_destringify_date( l );
			else
				llsd_destringify_date( r );
			return TRUE;
		case LLSD_URI:
			if ( ( ( l->uri_.uri != NULL ) && ( r->uri_.uri != NULL ) ) ||
				 ( ( l->uri_.esc != NULL ) && ( r->uri_.esc != NULL ) ) )
				return TRUE;
			if ( l->uri_.uri == NULL )
				llsd_unescape_uri( l );
			else
				llsd_unescape_uri( r );
			return TRUE;
		case LLSD_BINARY:
			if ( ( ( l->binary_.data != NULL ) && ( r->binary_.data != NULL ) ) ||
				 ( ( l->binary_.enc != NULL ) && ( l->binary_.enc != NULL ) ) )
				return TRUE;
			if ( l->binary_.data == NULL )
				llsd_decode_binary( l );
			else
				llsd_decode_binary( r );
			return TRUE;
	}

	return FALSE;

}

static int mem_len_cmp( uint8_t * l, uint32_t llen, uint8_t * r, uint32_t rlen )
{
	CHECK_PTR_RET( l, FALSE );
	CHECK_PTR_RET( r, FALSE );
	CHECK_RET( (llen == rlen), FALSE );

	return ( MEMCMP( l, r, llen ) == 0 );
}

static indent = 0;
static int llsd_string_eq( llsd_t * l, llsd_t * r )
{
	CHECK_RET( (l->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (r->type_ == LLSD_STRING), FALSE );

	/* do minimal work to allow use to compare str's */
	llsd_equalize( l, r );

	if ( (l->string_.str != NULL) && (r->string_.str != NULL) )
	{
		CHECK_RET_MSG( mem_len_cmp( l->string_.str, l->string_.str_len, r->string_.str, r->string_.str_len ), FALSE, "string mismatch\n" );
	}
	if ( (l->string_.esc != NULL) && (r->string_.esc != NULL) )
	{
		CHECK_RET_MSG( mem_len_cmp( l->string_.esc, l->string_.esc_len, r->string_.esc, r->string_.esc_len ), FALSE, "escaped string mismatch\n" );
	}

	/* null strings are equal */
	return TRUE;
}

static int llsd_uri_eq( llsd_t * l, llsd_t * r )
{
	CHECK_RET( (l->type_ == LLSD_URI), FALSE );
	CHECK_RET( (r->type_ == LLSD_URI), FALSE );

	/* do minimal work to allow use to compare str's */
	llsd_equalize( l, r );

	if ( (l->uri_.uri != NULL) && (r->uri_.uri != NULL) )
	{
		CHECK_RET_MSG( mem_len_cmp( l->uri_.uri, l->uri_.uri_len, r->uri_.uri, r->uri_.uri_len ), FALSE, "uri mismatch\n" );
	}
	if ( (l->uri_.esc != NULL) && (r->uri_.esc != NULL) )
	{
		CHECK_RET_MSG( mem_len_cmp( l->uri_.esc, l->uri_.esc_len, r->uri_.esc, r->uri_.esc_len ), FALSE, "escaped uri mistmatch\n" );
	}

	/* null uri's are equal */
	return TRUE;
}

#define FNV_PRIME (0x01000193)
uint_t fnv_key_hash(void const * const key)
{
	int i;
	llsd_t * llsd = (llsd_t*)key;
	uint_t hash = 0x811c9dc5;
	uint8_t const * p = (llsd->string_.key_esc ? 
						 (uint8_t const *)llsd->string_.esc : 
						 (uint8_t const *)llsd->string_.str);
	uint32_t const len = (llsd->string_.key_esc ?
						  llsd->string_.esc_len :
						  llsd->string_.str_len);
	CHECK_RET_MSG( (llsd->type_ == LLSD_STRING), 0 );
	for( i = 0; i < len; i++ )
	{
		hash *= FNV_PRIME;
		hash ^= p[i];
	}
	return hash;
}


int key_eq(void const * const l, void const * const r)
{
	llsd_t * llsd_l = (llsd_t *)l;
	llsd_t * llsd_r = (llsd_t *)r;
	return llsd_string_eq( llsd_l, llsd_r );
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
			if ( llsd->uuid_.dyn_str )
				FREE( llsd->uuid_.str );
			if ( llsd->uuid_.dyn_bits )
				FREE( llsd->uuid_.bits );
			break;

		case LLSD_DATE:
			if ( llsd->date_.dyn_str )
				FREE( llsd->date_.str );
			break;

		case LLSD_STRING:
			if ( llsd->string_.dyn_str )
				FREE( llsd->string_.str );
			if ( llsd->string_.dyn_esc )
				FREE( llsd->string_.esc );
			break;

		case LLSD_URI:
			if ( llsd->uri_.dyn_uri )
				FREE( llsd->uri_.uri );
			if ( llsd->uri_.dyn_esc )
				FREE( llsd->uri_.esc );
			break;

		case LLSD_BINARY:
			if ( llsd->binary_.dyn_data )
				FREE( llsd->binary_.data );
			if ( llsd->binary_.dyn_enc )
				FREE( llsd->binary_.enc );
			break;

		case LLSD_ARRAY:
			array_deinitialize( &llsd->array_.array );
			break;

		case LLSD_MAP:
			ht_deinitialize( &llsd->map_.ht );
			break;
	}
}

void llsd_delete( void * p )
{
	llsd_t * llsd = (llsd_t *)p;
	CHECK_PTR( llsd );

	/* deinitialize it */
	llsd_deinitialize( llsd );

	FREE( llsd );
}

static void llsd_initialize( llsd_t * llsd, llsd_type_t type_, ... )
{
	va_list args;
	uint8_t * p;
	int size, escaped, encoded, encoding, is_key;

	CHECK_PTR( llsd );
	
	llsd->type_ = type_;

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			break;

		case LLSD_BOOLEAN:
			va_start( args, type_ );
			llsd->bool_ = va_arg( args, int );
			va_end(args);
			break;

		case LLSD_INTEGER:
			va_start( args, type_ );
			llsd->int_.v = va_arg(args, int );
			va_end(args);
			break;

		case LLSD_REAL:
			va_start( args, type_ );
			llsd->real_.v = va_arg(args, double );
			va_end(args);
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			
			if ( p != NULL )
			{
				llsd->uuid_.dyn_bits = TRUE;
				llsd->uuid_.bits = UT(CALLOC( UUID_LEN, sizeof(uint8_t) ));
				MEMCPY( llsd->uuid_.bits, p, UUID_LEN );
			}
			else
			{
				llsd->uuid_.dyn_bits = FALSE;
				llsd->uuid_.bits = NULL;
				llsd->uuid_.dyn_str = FALSE;
				llsd->uuid_.str = NULL;
			}
			va_end( args );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );
			escaped = va_arg( args, int );
			is_key = va_arg( args, int );

			if ( size == 0 )
			{
				llsd->string_.str_len = 0;
				llsd->string_.str = UT(CALLOC( 1, sizeof(uint8_t) ));
				llsd->string_.dyn_str = FALSE;
				llsd->string_.esc_len = 0;
				llsd->string_.esc = NULL;
				llsd->string_.dyn_esc = FALSE;
			}
			else
			{
				if ( escaped )
				{
					llsd->string_.esc_len = size;
					llsd->string_.esc = UT(CALLOC( size + 1, sizeof(uint8_t) ));
					llsd->string_.dyn_esc = TRUE;
					llsd->string_.key_esc = is_key;
					MEMCPY( llsd->string_.esc, p, size );
				}
				else
				{
					llsd->string_.str_len = size;
					llsd->string_.str = UT(CALLOC( size + 1, sizeof(uint8_t) ));
					llsd->string_.dyn_str = TRUE;
					MEMCPY( llsd->string_.str, p, size );
				}
			}
			va_end( args );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->date_.dval = va_arg( args, double );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );

			if ( (p != NULL) && (size > 0) )
			{
				llsd->date_.len = size;
				llsd->date_.str = UT(CALLOC( size + 1, sizeof(uint8_t) ));
				llsd->date_.dyn_str = TRUE;
				MEMCPY( llsd->date_.str, p, size );
			}
			else
			{
				llsd->date_.use_dval = TRUE;
			}
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );
			escaped = va_arg( args, int );

			if ( size == 0 )
			{
				llsd->uri_.uri_len = 0;
				llsd->uri_.uri = UT(CALLOC( 1, sizeof(uint8_t) ));
				llsd->uri_.dyn_uri = TRUE;
				llsd->uri_.esc_len = 0;
				llsd->uri_.esc = NULL;
				llsd->uri_.dyn_esc = FALSE;
			}
			else
			{
				if ( escaped )
				{
					llsd->uri_.esc_len = size;
					llsd->uri_.esc = UT(CALLOC( size + 1, sizeof(uint8_t) ));
					llsd->uri_.dyn_esc = TRUE;
					MEMCPY( llsd->uri_.esc, p, size );
				}
				else
				{
					llsd->uri_.uri_len = size;
					llsd->uri_.uri = UT(CALLOC( size + 1, sizeof(uint8_t) ));
					llsd->uri_.dyn_uri = TRUE;
					MEMCPY( llsd->uri_.uri, p, size );
				}
			}
			va_end( args );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			size = va_arg( args, int );
			encoded = va_arg( args, int );
			encoding = va_arg( args, int );

			if ( size == 0 )
			{
				llsd->binary_.enc_size = 0;
				llsd->binary_.enc = UT(CALLOC( 1, sizeof(uint8_t) ));
				llsd->binary_.dyn_enc = TRUE;
				llsd->binary_.data_size = 0;
				llsd->binary_.data = NULL;
				llsd->binary_.dyn_data = FALSE;
				llsd->binary_.encoding = LLSD_NONE;
			}
			else
			{
				if ( encoded )
				{
					llsd->binary_.enc_size = size;
					llsd->binary_.enc = UT(CALLOC( size, sizeof(uint8_t) ));
					MEMCPY( llsd->binary_.enc, p, size );
					llsd->binary_.dyn_enc = TRUE;
					llsd->binary_.data_size = 0;
					llsd->binary_.data = NULL;
					llsd->binary_.dyn_data = FALSE;
					llsd->binary_.encoding = encoding;
				}
				else
				{
					llsd->binary_.enc_size = 0;
					llsd->binary_.enc = NULL;
					llsd->binary_.dyn_enc = FALSE;
					llsd->binary_.data_size = size;
					llsd->binary_.data = UT(CALLOC( size, sizeof(uint8_t) ));
					MEMCPY( llsd->binary_.data, p, size );
					llsd->binary_.dyn_data = TRUE;
					llsd->binary_.encoding = LLSD_NONE;
				}
			}
			va_end( args );
			break;

		case LLSD_ARRAY:
			va_start( args, type_ );
			size = va_arg( args, int );
			va_end( args );

			array_initialize( &(llsd->array_.array), 
							  size,
							  &llsd_delete );
			break;

		case LLSD_MAP:
			va_start( args, type_ );
			size = va_arg( args, int );
			va_end( args );

			ht_initialize( &(llsd->map_.ht), 
						   size,
						   &fnv_key_hash, 
						   &llsd_delete, 
						   &key_eq, 
						   &llsd_delete );
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
			a2 = va_arg( args, uint8_t* ); /* pointer to str */
			a1 = va_arg( args, int );		/* len of str date */
			llsd_initialize( llsd, type_, a3, a2, a1 );
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );	/* pointer to str */
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
			a2 = va_arg( args, uint8_t* );	/* pointer to buffer */
			a1 = va_arg( args, int );		/* size */
			a4 = va_arg( args, int );		/* encoded */
			a5 = va_arg( args, int );		/* encoding */
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1, a4, a5 );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			va_end( args );
			llsd_initialize( llsd, type_, a1 );
			break;
	}

	return llsd;
}

llsd_type_t llsd_get_type( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, LLSD_UNDEF );
	return llsd->type_;
}

int8_t const * llsd_get_type_string( llsd_type_t type_ )
{
	CHECK_RET( ((type_ >= LLSD_TYPE_FIRST) && (type_ < LLSD_TYPE_LAST)), NULL );
	return llsd_type_strings[ type_ ];
}

int8_t const * llsd_get_bin_enc_type_string( llsd_bin_enc_t enc )
{
	CHECK_RET( ((enc >= LLSD_BIN_ENC_FIRST) && (enc < LLSD_BIN_ENC_LAST)), NULL );
	return llsd_bin_enc_type_strings[ enc ];
}

int llsd_get_size( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( ((llsd->type_ == LLSD_ARRAY) || (llsd->type_ == LLSD_MAP)), -1, "getting size of non-container llsd object\n" );

	if ( llsd->type_ == LLSD_ARRAY )
		return array_size( &llsd->array_.array );
	
	return ht_size( &llsd->map_.ht );
}


void llsd_array_append( llsd_t * arr, llsd_t * data )
{
	CHECK_PTR( arr );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(arr) == LLSD_ARRAY, "trying to append data to non array\n" );
	array_push_tail( &(arr->array_.array), (void*)data );
}

void llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data )
{
	CHECK_PTR( map );
	CHECK_PTR( key );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );
	if ( llsd_get_type( key ) != LLSD_STRING )
	{
		FAIL( "trying to use non-string as key\n" );
	}
	ht_add( &(map->map_.ht), (void*)key, (void*)data );
}

llsd_t * llsd_map_find_llsd( llsd_t * map, llsd_t * key )
{
	ht_itr_t itr;
	CHECK_PTR_RET( map, NULL );
	CHECK_PTR_RET( key, NULL );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );
	CHECK_MSG( llsd_get_type(key) == LLSD_STRING, "trying to use non-string as key\n" );
	
	itr = ht_find( &(map->map_.ht), (void*)key );
	CHECK_RET_MSG( itr != ht_itr_end( &(map->map_.ht) ), NULL, "failed to find key\n" );
	return (llsd_t*)ht_itr_get( &(map->map_.ht), itr, NULL );
}

llsd_t * llsd_map_find( llsd_t * map, int8_t const * const key )
{
	llsd_t t;
	ht_itr_t itr;
	CHECK_PTR_RET( map, NULL );
	CHECK_PTR_RET( key, NULL );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );

	/* wrap the key in an llsd struct */
	memset( &t, 0, sizeof( llsd_t ) );
	llsd_initialize( &t, LLSD_STRING, key, strlen( key ), FALSE, FALSE );

	itr = ht_find( &(map->map_.ht), (void*)&t );

	/* free up the memory */
	llsd_deinitialize( &t );

	CHECK_RET_MSG( itr != ht_itr_end( &(map->map_.ht) ), NULL, "failed to find key\n" );
	return (llsd_t*)ht_itr_get( &(map->map_.ht), itr, NULL );
}

llsd_itr_t llsd_itr_begin( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, (llsd_itr_t)-1 );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_begin( &(llsd->array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_begin( &(llsd->map_.ht) );
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
			return (llsd_itr_t)array_itr_rbegin( &(llsd->array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rbegin( &(llsd->map_.ht) );
	}
	return (llsd_itr_t)0;
}

llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_next( &(llsd->array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_next( &(llsd->map_.ht), (ht_itr_t)itr );
	}
	return (llsd_itr_t)-1;
}

llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_rnext( &(llsd->array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rnext( &(llsd->map_.ht), (ht_itr_t)itr );
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
			(*value) = (llsd_t*)array_itr_get( &(llsd->array_.array), itr );
			(*key) = NULL;
			return TRUE;
		case LLSD_MAP:
			(*value) = (llsd_t*)ht_itr_get( &(llsd->map_.ht), itr, (void**)key );

			return TRUE;
	}

	/* non-container iterator just references the llsd */
	(*value) = llsd;
	(*key) = NULL;
	return TRUE;
}


int llsd_equal( llsd_t * l, llsd_t * r )
{
	int eq;
	llsd_itr_t itrl;
	llsd_t * kl;
	llsd_t * vl;

	llsd_itr_t itrr;
	llsd_t * kr;
	llsd_t * vr;

	if ( llsd_get_type( l ) != llsd_get_type( r ) )
	{
		WARN("type mismatch\n");
		return FALSE;
	}
	
	switch( llsd_get_type( l ) )
	{
		case LLSD_UNDEF:
			CHECK_RET_MSG( llsd_get_type( r ) == LLSD_UNDEF, FALSE, "undef mismatch\n" );
			DEBUG( "%*sUNDEF\n", indent * 4, " " );
			return TRUE;

		case LLSD_BOOLEAN:
			if ( llsd_as_bool( l ) != llsd_as_bool( r ) )
			{
				DEBUG("boolean mismatch\n");
				return FALSE;
			}
			/*CHECK_RET_MSG( llsd_as_bool( l ) == llsd_as_bool( r ), FALSE, "boolean mismatch\n" );*/
			DEBUG( "%*sBOOLEAN\n", indent * 4, " " );
			return TRUE;

		case LLSD_INTEGER:
			CHECK_RET_MSG( l->int_.v == r->int_.v, FALSE, "integer mismatch\n" );
			DEBUG( "%*sINTEGER (%d)\n", indent * 4, " ", l->int_.v );
			return TRUE;

		case LLSD_REAL:
			CHECK_RET_MSG( mem_len_cmp( llsd_as_string(l).str, llsd_as_string(l).str_len, llsd_as_string(r).str, llsd_as_string(r).str_len ), FALSE, "real mismatch\n" );

			DEBUG( "%*sREAL (%f)\n", indent * 4, " ", l->real_.v );
			return TRUE;

		case LLSD_UUID:
			llsd_equalize( l, r );
			if ( ( l->uuid_.bits != NULL ) && ( r->uuid_.bits != NULL ) )
			{
				CHECK_RET_MSG( MEMCMP( l->uuid_.bits, r->uuid_.bits, UUID_LEN ) == 0, FALSE, "uuid bits mismatch\n" );
				DEBUG( "%*sUUID BITS\n", indent * 4, " " );
				return TRUE;
			}
			if ( ( r->uuid_.str != NULL ) && ( r->uuid_.str != NULL ) )
			{
				CHECK_RET_MSG( MEMCMP( l->uuid_.str, r->uuid_.str, UUID_STR_LEN ) == 0, FALSE, "uuid string mismatch\n" );
				DEBUG( "%*sUUID STR (%*s)\n", indent * 4, " ", UUID_STR_LEN, l->uuid_.str );
				return TRUE;
			}
			DEBUG( "falling out of UUID\n" );
			return FALSE;

		case LLSD_STRING:
			if ( llsd_string_eq( l, r ) )
			{
				if ( (l->string_.str != NULL) && (r->string_.str != NULL) )
				{
					DEBUG( "%*sSTRING (%*s)\n", indent * 4, " ", l->string_.str_len, l->string_.str );
				}
				if ( (l->string_.esc != NULL) && (r->string_.esc != NULL) )
				{
					DEBUG( "%*sSTRING (%*s)\n", indent * 4, " ", l->string_.esc_len, l->string_.esc );
				}
				return TRUE;
			}
			return FALSE;

		case LLSD_DATE:
			llsd_equalize( l, r );
			if ( ( l->date_.use_dval != FALSE) && ( r->date_.use_dval != FALSE ) )
			{
				CHECK_RET_MSG( l->date_.dval == l->date_.dval, FALSE, "date dval mismatch\n" );
				DEBUG( "%*sDATE (%f)\n", indent * 4, " ", l->date_.dval );
				return TRUE;
			}
			if ( ( l->date_.str != NULL ) && ( r->date_.str != NULL ) )
			{
				CHECK_RET_MSG( MEMCMP( l->date_.str, r->date_.str, DATE_STR_LEN ) == 0, FALSE, "date string mismatch\n" );
				DEBUG( "%*sDATE STR (%*s)\n", indent * 4, " ", DATE_STR_LEN, l->date_.str );
				return TRUE;
			}
			DEBUG( "falling out of DATE\n" );
			return FALSE;

		case LLSD_URI:
			if ( llsd_uri_eq( l, r ) )
			{
				if ( (l->uri_.uri != NULL) && (r->uri_.uri != NULL) )
				{
					DEBUG( "%*sURI (%*s)\n", indent * 4, " ", l->uri_.uri_len, l->uri_.uri );
				}
				if ( (l->uri_.esc != NULL) && (r->uri_.esc != NULL) )
				{
					DEBUG( "%*sSTRING (%*s)\n", indent * 4, " ", l->uri_.esc_len, l->uri_.esc );
				}
				return TRUE;
			}
			return FALSE;

		case LLSD_BINARY:
			if ( ( l->binary_.data != NULL ) && ( r->binary_.data != NULL ) )
			{
				eq = ( ( l->binary_.data_size == r->binary_.data_size ) &&
						 ( MEMCMP( l->binary_.data, r->binary_.data, l->binary_.data_size ) == 0 ) );
				CHECK_RET_MSG( eq, FALSE, "binary data mismatch\n" );
				DEBUG( "%*sBINARY (%d)\n", indent * 4, " ", l->binary_.data_size );
				return TRUE;
			}
			if ( ( l->binary_.enc != NULL ) && ( l->binary_.data != NULL ) )
			{
				eq = ( ( l->binary_.enc_size == r->binary_.enc_size ) &&
						 ( MEMCMP( l->binary_.enc, r->binary_.enc, l->binary_.enc_size) == 0 ) );
				CHECK_RET_MSG( eq, FALSE, "encoded binary mismatch\n" );
				DEBUG( "%*sBINARY STR (%*s)\n", indent * 4, " ", l->binary_.enc_size, l->binary_.enc );
				return TRUE;
			}
			if ( (l->binary_.data == NULL) && (r->binary_.data == NULL) &&
				 (l->binary_.enc == NULL) && (r->binary_.data == NULL) &&
				 (l->binary_.data_size == 0) && (r->binary_.data_size == 0) &&
				 (l->binary_.enc_size == 0) && (r->binary_.enc_size == 0) &&
				 (l->binary_.encoding == r->binary_.encoding) )
			{
				DEBUG( "%*sBINARY STR (%*s)\n", indent * 4, " ", l->binary_.enc_size, l->binary_.enc );
				return TRUE;
			}

			DEBUG( "falling out of BINARY\n" );
			return FALSE;

		case LLSD_ARRAY:	
			CHECK_RET_MSG( array_size( &(l->array_.array) ) == array_size( &(r->array_.array) ), FALSE, "array size mismatch\n" )
			DEBUG( "%*s[[ (%d)\n", indent * 4, " ", llsd_get_size( l ) );
			indent++;

			itrl = llsd_itr_begin( l );
			itrr = llsd_itr_begin( r );
			for ( ; itrl != llsd_itr_end( l ); itrl = llsd_itr_next( l, itrl ), itrr = llsd_itr_next( r, itrr) )
			{
				llsd_itr_get( l, itrl, &vl, &kl );
				llsd_itr_get( r, itrr, &vr, &kr );
				if ( !llsd_equal( vl, vr ) )
				{
					WARN("%*sLEFT (%*s) (%s)\n", indent * 4, " ", llsd_as_string(vl).str_len, llsd_as_string(vl).str, llsd_get_type_string( llsd_get_type( vl ) ) );
					WARN("%*sRIGHT (%*s) (%s)\n", indent * 4, " ", llsd_as_string(vr).str_len, llsd_as_string(vr).str, llsd_get_type_string( llsd_get_type( vr ) ) );

					WARN("array_member mismatch\n");
					return FALSE;
				}
			}
			indent--;
			DEBUG( "%*s]] (%d)\n", indent * 4, " ", llsd_get_size( l ) );
			return TRUE;

		case LLSD_MAP:
			CHECK_RET_MSG( ht_size( &(l->map_.ht) ) == ht_size( &(r->map_.ht) ), FALSE, "map size mismatch\n" )
			DEBUG( "%*s{{ (%d)\n", indent * 4, " ", llsd_get_size( l ) );
			indent++;
			itrl = llsd_itr_begin( l );
			for ( ; itrl != llsd_itr_end( l ); itrl = llsd_itr_next( l, itrl ) )
			{
				/* get the k/v pair from the left map */
				llsd_itr_get( l, itrl, &vl, &kl );
				DEBUG("%*sKEY (%*s)\n", indent * 4, " ", kl->string_.str_len, kl->string_.str);

				/* look up the value associated with the same key in the right map */
				vr = llsd_map_find_llsd( r, kl );

				if ( vr == FALSE )
				{
					WARN( "no matching k/v pair in map\n" );
					return FALSE;
				}

				if ( !llsd_equal( vl, vr ) )
				{
					WARN("%*sLEFT (%*s)\n", indent * 4, " ", llsd_as_string(vl).str_len, llsd_as_string(vl).str );
					WARN("%*sRIGHT (%*s)\n", indent * 4, " ", llsd_as_string(vr).str_len, llsd_as_string(vr).str );

					WARN( "map member mismatch\n" );
					return FALSE;
				}
			}
			indent--;
			DEBUG( "%*s}} (%d)\n", indent * 4, " ", llsd_get_size( l ) );
			return TRUE;
	}

	DEBUG( "falling out of llsd_equal\n" );
	return FALSE;
}


llsd_bool_t llsd_as_bool( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, FALSE, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to bool\n", llsd_get_type_string( llsd->type_ ) );
			break;
		case LLSD_UNDEF:
			return FALSE;
		case LLSD_BOOLEAN:
			return llsd->bool_;
		case LLSD_INTEGER:
			if ( llsd->int_.v != 0 )
				return TRUE;
			return FALSE;
		case LLSD_REAL:
			if ( llsd->real_.v != 0. )
				return TRUE;
			return FALSE;
		case LLSD_UUID:
			/* destringify uuid if needed */
			llsd_destringify_uuid( llsd );
			if ( MEMCMP( llsd->uuid_.bits, zero_uuid.bits, UUID_LEN ) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_STRING:
			if ( ( llsd->string_.str_len == 0 ) && 
				 ( llsd->string_.esc_len == 0 ) )
				return FALSE;
			return TRUE;
		case LLSD_BINARY:
			if ( ( llsd->binary_.data_size == 0 ) && 
				 (llsd->binary_.enc_size == 0 ) )
				return FALSE;
			return TRUE;
	}
	return FALSE;
}

llsd_int_t llsd_as_int( llsd_t * llsd )
{
	static uint8_t * p = NULL;
	static llsd_int_t tmp_int;
	CHECK_PTR_RET_MSG( llsd, zero_int, "invalid llsd pointer\n" );

	switch( llsd->type_ )
	{
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to integer\n", llsd_get_type_string( llsd->type_ ) );
			break;
		case LLSD_UNDEF:
			return zero_int;
		case LLSD_BOOLEAN:
			if ( llsd->bool_ == FALSE )
				return zero_int;
			return one_int;
		case LLSD_INTEGER:
			return llsd->int_;
		case LLSD_REAL:
			if ( isnan( llsd->real_.v ) )
			{
				FAIL( "converting NaN real to integer\n" );
			}
			else if ( isinf( llsd->real_.v ) != 0 )
			{
				FAIL( "converting Infinite real to integer\n" );
			}
			return (llsd_int_t){ .v = lrint( llsd->real_.v ),
								 .be = 0 };
		case LLSD_DATE:
			/* de-stringify date if needed */
			llsd_destringify_date( llsd );
			if ( (uint64_t)llsd->date_.dval >= UINT32_MAX )
			{
				DEBUG( "truncating 64-bit date value to 32-bit integer...loss of data!" );
			}
			return (llsd_int_t){ .v = (int32_t)llsd->date_.dval,
								 .be = 0 };
		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			p = UT(CALLOC( llsd->string_.str_len + 1, sizeof(uint8_t) ));
			MEMCPY( p, llsd->string_.str, llsd->string_.str_len );
			tmp_int = (llsd_int_t){ .v = atoi( p ), .be = 0 };
			FREE( p );
			return tmp_int;
		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );
			if ( llsd->binary_.data_size < sizeof(llsd_int_t) )
			{
				return zero_int;
			}
			return (llsd_int_t){ .v = ntohl( *((uint32_t*)llsd->binary_.data) ),
								 .be = 0 };
	}
	return zero_int;
}

llsd_real_t llsd_as_real( llsd_t * llsd )
{
	static uint8_t * p = NULL;
	static llsd_real_t tmp_real;
	CHECK_PTR_RET_MSG( llsd, zero_real, "invalid llsd pointer\n" );

	switch( llsd->type_ )
	{
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to real\n", llsd_get_type_string( llsd->type_ ) );
			break;
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			if ( llsd->bool_ == FALSE )
				return zero_real;
			return one_real;
		case LLSD_INTEGER:
			return (llsd_real_t) { .v = (double)llsd->int_.v,
								   .be = 0 };
		case LLSD_REAL:
			return (llsd_real_t)llsd->real_;
		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			p = UT(CALLOC( llsd->string_.str_len + 1, sizeof(uint8_t) ));
			MEMCPY( p, llsd->string_.str, llsd->string_.str_len );
			tmp_real = (llsd_real_t){ .v = atof( p ), .be = 0 };
			FREE( p );
			return tmp_real;
		case LLSD_DATE:
			/* de-stringify date if needed */
			llsd_destringify_date( llsd );
			return (llsd_real_t){ .v = llsd->date_.dval, .be = 0 };
		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );
			if ( llsd->binary_.data_size < sizeof(llsd_real_t) )
			{
				return zero_real;
			}
			return (llsd_real_t){ .v = be64toh( *((uint64_t*)llsd->binary_.data) ),
								  .be = 0 };
	}
	return zero_real;
}

llsd_uuid_t llsd_as_uuid( llsd_t * llsd )
{
	static llsd_t tmp_llsd;
	static uint8_t bits[UUID_LEN];
	int i;

	CHECK_PTR_RET_MSG( llsd, zero_uuid, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to uuid\n", llsd_get_type_string( llsd->type_ ) );
			break;

		case LLSD_BINARY:
			/* decode binary if needed */
			llsd_decode_binary( llsd );

			/* if len < 16, return null uuid, otherwise first 16 bytes converted to uuid */
			if ( llsd->binary_.data_size < 16 )
			{
				return zero_uuid;
			}
			MEMCPY( bits, llsd->binary_.data, UUID_LEN );
			return (llsd_uuid_t){ .dyn_bits = FALSE,
								  .dyn_str = FALSE,
								  .bits = bits,
								  .str = NULL };

		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			tmp_llsd.type_ = LLSD_UUID;
			tmp_llsd.uuid_ = (llsd_uuid_t){ .dyn_bits = FALSE,
												  .dyn_str = FALSE,
												  .len = llsd->string_.str_len,
												  .str = llsd->string_.str,
												  .bits = NULL };
			llsd_destringify_uuid( &tmp_llsd );
			tmp_llsd.uuid_.len = 0;
			tmp_llsd.uuid_.str = NULL; /* remove our reference */
			return (llsd_uuid_t)tmp_llsd.uuid_;

		case LLSD_UUID:
			return (llsd_uuid_t)llsd->uuid_;
	}

	return zero_uuid;
}

llsd_array_t llsd_as_array( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, empty_array, "invalid llsd pointer\n" );
	if ( llsd->type_ != LLSD_ARRAY )
	{
		FAIL( "illegal conversion of %s to array\n", llsd_get_type_string( llsd->type_ ) );
		return empty_array;
	}
	return (llsd_array_t)llsd->array_;
}

llsd_map_t llsd_as_map( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, empty_map, "invalid llsd pointer\n" );
	if ( llsd->type_ != LLSD_MAP )
	{
		FAIL( "illegal conversion of %s to array\n", llsd_get_type_string( llsd->type_ ) );
		return empty_map;
	}
	return (llsd_map_t)llsd->map_;
}

llsd_string_t llsd_as_string( llsd_t * llsd )
{
	static int8_t tmp[64];
	int len;
	uint8_t * p;

	CHECK_PTR_RET_MSG( llsd, false_string, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_ARRAY:
		case LLSD_MAP:
			return empty_string;
		case LLSD_BOOLEAN:
			if ( llsd->bool_ == FALSE )
				return false_string;
			return true_string;
		case LLSD_INTEGER:
			len = snprintf( tmp, 64, "%d", llsd->int_.v );
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = strnlen( tmp, 64 ),
									.str = tmp,
									.esc_len = 0,
									.esc = NULL };
		case LLSD_REAL:
			len = snprintf( tmp, 64, "%f", llsd->real_.v );
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = strnlen( tmp, 64 ),
									.str = tmp,
									.esc_len = 0,
									.esc = NULL };
		case LLSD_UUID:
			/* stringify the uuid if needed */
			llsd_stringify_uuid( llsd );
			return (llsd_string_t){ .dyn_str = FALSE,
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = UUID_STR_LEN,
									.str = llsd->uuid_.str,
									.esc_len = 0,
									.esc = NULL };

		case LLSD_STRING:
			return llsd->string_;
		case LLSD_DATE:
			/* stringify the date if needed */
			llsd_stringify_date( llsd );
			return (llsd_string_t){ .dyn_str = FALSE,
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = DATE_STR_LEN,
									.str = llsd->date_.str,
									.esc_len = 0,
									.esc = NULL };
		case LLSD_URI:
			/* unescape the URI if needed */
			/*llsd_escape_uri( llsd );*/
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = llsd->uri_.uri_len,
									.str = llsd->uri_.uri,
									.esc_len = 0,
									.esc = NULL };

		case LLSD_BINARY:
			/* encode binary if needed */
			if ( llsd_encode_binary( llsd, LLSD_BASE64 ) )
			{
				return (llsd_string_t){ .dyn_str = FALSE, 
										.dyn_esc = FALSE,
										.key_esc = FALSE,
										.str_len = llsd->binary_.enc_size,
										.str = llsd->binary_.enc,
										.esc_len = 0,
										.esc = NULL };
			}
			return empty_string;
	}
}

llsd_date_t llsd_as_date( llsd_t * llsd )
{
	static llsd_t tmp_llsd;

	CHECK_PTR_RET_MSG( llsd, empty_date, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to date\n", llsd_get_type_string( llsd->type_ ) );
			break;

		case LLSD_INTEGER:
			return (llsd_date_t){ .use_dval = TRUE,
								  .dyn_str = FALSE,
								  .dval = (double)llsd->int_.v,
								  .str = NULL };

		case LLSD_REAL:
			return (llsd_date_t){ .use_dval = TRUE,
								  .dyn_str = FALSE,
								  .dval = llsd->real_.v,
								  .str = NULL };

		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );

			tmp_llsd.type_ = LLSD_DATE;
			tmp_llsd.date_ = (llsd_date_t){ .use_dval = FALSE,
												  .dyn_str = FALSE,
												  .dval = 0.0,
												  .str = llsd->string_.str };
			llsd_destringify_date( &tmp_llsd );
			tmp_llsd.date_.str = NULL; /* remove reference to str */
			return (llsd_date_t)tmp_llsd.date_;

		case LLSD_DATE:
			return llsd->date_;
	}
}

llsd_uri_t llsd_as_uri( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, empty_uri, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to uri\n", llsd_get_type_string( llsd->type_ ) );
			break;

		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			return (llsd_uri_t){ .dyn_uri = FALSE, 
								 .dyn_esc = FALSE,
								 .uri_len = llsd->string_.str_len,
								 .uri = llsd->string_.str,
								 .esc_len = 0,
								 .esc = NULL };

		case LLSD_URI:
			return llsd->uri_;

		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );

			/* TODO: check for valid UTF-8 */
			return (llsd_uri_t){ .dyn_uri = FALSE, 
								 .dyn_esc = FALSE,
								 .uri_len = llsd->binary_.data_size,
								 .uri = llsd->binary_.data,
								 .esc_len = 0,
								 .esc = NULL };
	}
}

llsd_binary_t llsd_as_binary( llsd_t * llsd )
{
	static uint8_t tmp[8];
	static uint32_t tmpl;
	static uint64_t tmpll;

	CHECK_PTR_RET_MSG( llsd, empty_binary, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_DATE:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to binary\n", llsd_get_type_string( llsd->type_ ) );
			break;

		case LLSD_BOOLEAN:
			if ( llsd->bool_ == FALSE )
				return false_binary;
			return true_binary;

		case LLSD_INTEGER:
			llsd->int_.be = htonl( llsd->int_.v );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = sizeof( llsd_int_t ),
									.data = (uint8_t*)&(llsd->int_.be),
									.enc_size = 0,
									.enc = NULL };

		case LLSD_REAL:
			llsd->real_.be = htobe64( *((uint64_t*)&(llsd->real_)) );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = sizeof( llsd_real_t ),
									.data = (uint8_t*)&(llsd->real_.be),
									.enc_size = 0,
									.enc = NULL };

		case LLSD_UUID:
			/* de-stringify the uuid if needed */
			llsd_destringify_uuid( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = UUID_LEN,
									.data = llsd->uuid_.bits,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_STRING:
			/* unescape string if needed */
			llsd_unescape_string( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = llsd->string_.str_len,
									.data = llsd->string_.str,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_URI:
			/* unescape uri if needed */
			llsd_unescape_uri( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = llsd->uri_.uri_len,
									.data = llsd->uri_.uri,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_BINARY:
			return llsd->binary_;
	}
}


static uint8_t hex_to_byte( uint8_t hi, uint8_t lo )
{
	uint8_t v = 0;
	if ( isdigit(hi) && isdigit(lo) )
	{
		return ( ((hi << 4) & (0xF0)) | (0x0F & lo) );
	}
	else if ( isdigit(hi) && isxdigit(lo) )
	{
		return ( ((hi << 4) & 0xF0) | (0x0F & (10 + (tolower(lo) - 'a'))) );
	}
	else if ( isxdigit(hi) && isdigit(lo) )
	{
		return ( (((10 + (tolower(hi) - 'a')) << 4) & 0xF0) | (0x0F & lo) );
	}
	else if ( isxdigit(hi) && isxdigit(lo) )
	{
		return ( (((10 + (tolower(hi) - 'a')) << 4) & 0xF0) | (0x0F & (10 + (tolower(lo) - 'a'))) );
	}
}


int llsd_stringify_uuid( llsd_t * llsd )
{
	uint8_t * p;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_UUID), FALSE );
	CHECK_RET( (llsd->uuid_.len == 0), FALSE );
	CHECK_RET( (llsd->uuid_.str == NULL), FALSE );
	CHECK_PTR_RET( llsd->uuid_.bits, FALSE );

	/* allocate the string */
	llsd->uuid_.str = UT(CALLOC( UUID_STR_LEN + 1, sizeof(uint8_t) ));
	llsd->uuid_.dyn_str = TRUE;
	llsd->uuid_.len = UUID_STR_LEN + 1;

	/* convert to string and cache it */
	p = llsd->uuid_.bits;
	snprintf( llsd->uuid_.str, UUID_STR_LEN + 1, 
			  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			  p[0], p[1], p[2], p[3], 
			  p[4], p[5], 
			  p[6], p[7], 
			  p[8], p[9], 
			  p[10], p[11], p[12], p[13], p[14], p[15] );

	return TRUE;
}

/* A valid 8-4-4-4-12 string UUID is converted to a binary UUID */
int llsd_destringify_uuid( llsd_t * llsd )
{
	int i;
	uint8_t * pin;
	uint8_t * pout;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_UUID), FALSE );
	CHECK_RET( (llsd->uuid_.len == UUID_STR_LEN), FALSE );
	CHECK_PTR_RET( llsd->uuid_.str, FALSE );
	CHECK_RET( (llsd->uuid_.bits == NULL), FALSE );

	/* check for 8-4-4-4-12 */
	for ( i = 0; i < 36; i++ )
	{
		if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
			 ( (i >= 9) && (i < 13) ) &&	/* 4 */
			 ( (i >= 14) && (i < 18) ) &&	/* 4 */
			 ( (i >= 19) && (i < 23) ) &&	/* 4 */
			 ( (i >= 24) && (i < 36) ) )	/* 12 */
		{
			if ( !isxdigit( llsd->uuid_.str[i] ) )
			{
				return FALSE;
			}
		}
		else if ( llsd->uuid_.str[i] != '-' )
		{
			return FALSE;
		}
	}

	/* allocate the bits */
	llsd->uuid_.bits = UT(CALLOC( UUID_LEN, sizeof(uint8_t) ));
	llsd->uuid_.dyn_bits = TRUE;

	/* covert to UUID */
	pin = llsd->uuid_.str;
	pout = llsd->uuid_.bits;

	/* 8 */
	pout[0] = hex_to_byte( pin[0], pin[1] );
	pout[1] = hex_to_byte( pin[2], pin[3] );
	pout[2] = hex_to_byte( pin[4], pin[5] );
	pout[3] = hex_to_byte( pin[6], pin[7] );

	/* 4 */
	pout[4] = hex_to_byte( pin[9], pin[10] );
	pout[5] = hex_to_byte( pin[11], pin[12] );

	/* 4 */
	pout[6] = hex_to_byte( pin[14], pin[15] );
	pout[7] = hex_to_byte( pin[16], pin[17] );

	/* 4 */
	pout[8] = hex_to_byte( pin[19], pin[20] );
	pout[9] = hex_to_byte( pin[21], pin[22] );

	/* 12 */
	pout[10] = hex_to_byte( pin[24], pin[25] );
	pout[11] = hex_to_byte( pin[26], pin[27] );
	pout[12] = hex_to_byte( pin[28], pin[29] );
	pout[13] = hex_to_byte( pin[30], pin[31] );
	pout[14] = hex_to_byte( pin[32], pin[33] );
	pout[15] = hex_to_byte( pin[34], pin[35] );

	return TRUE;
}

int llsd_stringify_date( llsd_t * llsd )
{
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_DATE), FALSE );
	CHECK_RET( (llsd->date_.len == 0), FALSE );
	CHECK_RET( (llsd->date_.str == NULL), FALSE );

	/* allocate the string */
	llsd->date_.str = UT(CALLOC( DATE_STR_LEN + 1, sizeof(uint8_t) ));
	llsd->date_.dyn_str = TRUE;
	llsd->date_.len = DATE_STR_LEN + 1;

	int_time = floor( llsd->date_.dval );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( llsd->date_.dval - int_time) * 1000000.0 );
	parts = *gmtime(&seconds);
	snprintf( llsd->date_.str, DATE_STR_LEN + 1, 
			  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			  parts.tm_year + 1900,
			  parts.tm_mon + 1,
			  parts.tm_mday,
			  parts.tm_hour,
			  parts.tm_min,
			  parts.tm_sec,
			  ((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
	return TRUE;
}

int llsd_destringify_date( llsd_t * llsd )
{
	int useconds;
	struct tm parts;
	time_t mktimeEpoch;
	double seconds;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_DATE), FALSE );
	CHECK_RET( (llsd->date_.len == DATE_STR_LEN), FALSE );
	CHECK_RET( llsd->date_.str, FALSE );

	MEMSET( &parts, 0, sizeof(struct tm) );
	mktimeEpoch = mktime(&parts);

	sscanf( llsd->date_.str, 
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			&parts.tm_year,
			&parts.tm_mon,
			&parts.tm_mday,
			&parts.tm_hour,
			&parts.tm_min,
			&parts.tm_sec,
			&useconds );

	/* adjust a few things */
	parts.tm_year -= 1900;
	parts.tm_mon -= 1;

	/* convert to seconds */
	seconds = (double)(mktime(&parts) - mktimeEpoch);

	/* add in the miliseconds */
	seconds += ((double)useconds * 1000.0);

	/* store it in the date */
	llsd->date_.dval = seconds;
	llsd->date_.use_dval = TRUE;

	return TRUE;
}

static uint32_t llsd_escaped_string_len( llsd_t * llsd )
{
	uint32_t len = 0;
#if 0
	int i;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_STRING), 0 );
	CHECK_RET( (llsd->string_.str_len > 0), 0 );
	CHECK_PTR_RET( llsd->string_.str, 0 );
	CHECK_RET( (llsd->string_.esc_len == 0), 0 );
	CHECK_RET( (llsd->string_.esc == NULL), 0 );

	for( i = 0; i < llsd->string_.str_len; i++ )
	{
		len += notation_esc_len[llsd->string_.str[i]];
	}
#endif
	return len;
}

int llsd_escape_string( llsd_t * llsd )
{
#if 0
	int i;
	uint8_t c;
	uint8_t * p;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (llsd->string_.str_len > 0), FALSE );
	CHECK_PTR_RET( llsd->string_.str, FALSE );
	CHECK_RET( (llsd->string_.esc_len == 0), FALSE );
	CHECK_RET( (llsd->string_.esc == NULL), FALSE );

	size = llsd_escaped_string_len( llsd );
	llsd->string_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->string_.esc, FALSE );
	llsd->string_.dyn_esc = TRUE;
	llsd->string_.esc_len = size;

	p = llsd->string_.esc;
	for( i = 0; i < llsd->string_.str_len; i++)
	{
		c = llsd->string_.str[i];
		MEMCPY( p, notation_esc_chars[c], notation_esc_len[c] );
		p += notation_esc_len[c];

		CHECK_RET( (p <= (llsd->string_.esc + llsd->string_.esc_len)), FALSE );
	}
#endif
	return TRUE;
}

static uint32_t llsd_unescaped_string_len( llsd_t * llsd )
{
	uint32_t len = 0;
#if 0
	uint8_t* p;
	int i;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_STRING), 0 );
	CHECK_RET( (llsd->string_.esc_len > 0), 0 );
	CHECK_PTR_RET( llsd->string_.esc, 0 );
	CHECK_RET( (llsd->string_.str_len == 0), 0 );
	CHECK_RET( (llsd->string_.str == NULL), 0 );

	p = llsd->string_.esc;
	while( p < (llsd->string_.esc + llsd->string_.esc_len) )
	{
		/* skip over the logical character */
		if ( p[0]  == '\\' )
		{
			if ( p[1] == 'x' )
			{
				CHECK_RET( ( isxdigit( p[2] ) && isxdigit( p[3] ) ), 0 );
				p += 4;
			}
			else
			{
				p += 2;
			}
		}
		else
		{
			p++;
		}
		/* increment the unescaped length */
		len += 1;
	}
#endif
	return len;
}

int llsd_unescape_string( llsd_t * llsd )
{
#if 0
	int i;
	uint8_t * p;
	uint8_t * e;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (llsd->string_.esc_len > 0), FALSE );
	CHECK_PTR_RET( llsd->string_.esc, FALSE );
	CHECK_RET( (llsd->string_.str_len == 0), FALSE );
	CHECK_RET( (llsd->string_.str == NULL), FALSE );

	size = llsd_unescaped_string_len( llsd );
	CHECK_RET( (size > 0), FALSE );
	llsd->string_.str = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->string_.str, FALSE );
	llsd->string_.dyn_str = TRUE;
	llsd->string_.str_len = size;

	p = llsd->string_.str;
	e = llsd->string_.esc;
	while( e < (llsd->string_.esc + llsd->string_.esc_len) )
	{
		if ( e[0] == '\\' )
		{
			switch ( e[1] )
			{
				case 'x':
					if ( isxdigit( e[2] ) && isxdigit( e[3] ) )
					{
						*p = hex_to_byte( e[2], e[3] );
						p++;
					}
					else
					{
						WARN( "found invalid escape sequence in string\n" );
						return FALSE;
					}
					e += 4;
					break;
				case 'a':
					*p = 0x7;
					p++;
					e += 2;
					break;
				case 'b':
					*p = 0x8;
					p++;
					e += 2;
					break;
				case 'f':
					*p = 0xc;
					p++;
					e += 2;
					break;
				case 'n':
					*p = 0xa;
					p++;
					e += 2;
					break;
				case 'r':
					*p = 0xd;
					p++;
					e += 2;
					break;
				case 't':
					*p = 0x9;
					p++;
					e += 2;
					break;
				case 'v':
					*p = 0xb;
					p++;
					e += 2;
					break;
				case '"':
				case '\'':
				case '\\':
					*p = e[1];
					p++;
					e += 2;
					break;
			}
		}
		else
		{
			*p = *e;
			e++;
		}
	}
#endif
	return TRUE;
}

#define URL_ENCODED_CHAR( x ) ( ( x <= 0x1F ) || \
								( x >= 0x7F ) || \
								( x == ' '  ) || \
								( x == '\'' ) || \
								( x == '"'  ) || \
								( x == '<'  ) || \
								( x == '>'  ) || \
								( x == '%'  ) || \
								( x == '{'  ) || \
								( x == '}'  ) || \
								( x == '|'  ) || \
								( x == '\\' ) || \
								( x == '^'  ) || \
								( x == '['  ) || \
								( x == ']'  ) || \
								( x == '`'  ) )

static uint32_t llsd_escaped_uri_len( llsd_t * llsd )
{
	uint8_t * p;
	uint32_t len = 0;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_URI), 0 );
	CHECK_RET( (llsd->uri_.uri_len > 0), 0 );
	CHECK_PTR_RET( llsd->uri_.uri, 0 );
	CHECK_RET( (llsd->uri_.esc_len == 0), 0 );
	CHECK_RET( (llsd->uri_.esc == NULL), 0 );

	p = llsd->uri_.uri;
	while( p < (llsd->uri_.uri + llsd->uri_.uri_len) )
	{
		if ( URL_ENCODED_CHAR( *p ) )
		{
			len += 3;
		}
		else
		{
			len++;
		}
		p++;
	}
	return len;
}

int llsd_escape_uri( llsd_t * llsd )
{
	int i;
	uint8_t c;
	uint8_t * p;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_URI), FALSE );
	CHECK_RET( (llsd->uri_.uri_len > 0), FALSE );
	CHECK_PTR_RET( llsd->uri_.uri, FALSE );
	CHECK_RET( (llsd->uri_.esc_len == 0), FALSE );
	CHECK_RET( (llsd->uri_.esc == NULL), FALSE );

	size = llsd_escaped_uri_len( llsd );
	llsd->uri_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->uri_.esc, FALSE );
	llsd->uri_.dyn_esc = TRUE;
	llsd->uri_.esc_len = size;

	p = llsd->uri_.esc;
	for( i = 0; i < llsd->uri_.uri_len; i++)
	{
		c = llsd->uri_.uri[i];
		if ( URL_ENCODED_CHAR( c ) )
		{
			sprintf( p, "%%02x", c );
			p += 3;
		}
		else
		{
			*p = c;
			p++;
		}
	}
	return TRUE;
}

int llsd_unescape_uri( llsd_t * llsd )
{
	/* TODO */
	return TRUE;
}

int llsd_encode_binary( llsd_t * llsd, llsd_bin_enc_t encoding )
{
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->binary_.data_size != 0), FALSE );
	CHECK_PTR_RET( llsd->binary_.data, FALSE );
	CHECK_RET( (llsd->binary_.enc_size == 0), FALSE );
	CHECK_RET( (llsd->binary_.enc == NULL), FALSE );
	CHECK_RET( ((encoding >= LLSD_BIN_ENC_FIRST) && (encoding < LLSD_BIN_ENC_LAST)), FALSE );

	switch ( encoding )
	{
		case LLSD_NONE:
			if ( llsd->binary_.data != NULL )
			{
				WARN( "LLSD_NONE encoding with non-null data buffer\n" );
			}
			if ( llsd->binary_.data_size > 0 )
			{
				WARN( "LLSD_NONE encoding with non-zero data size\n" );
			}
			return FALSE;
		case LLSD_BASE16:
			/* allocate enc buffer */
			llsd->binary_.enc_size = BASE16_LENGTH( llsd->binary_.data_size );
			llsd->binary_.enc = UT(CALLOC( llsd->binary_.enc_size, sizeof(uint8_t) ));
			llsd->binary_.dyn_enc = TRUE;
			llsd->binary_.encoding = LLSD_BASE16;

			/* encode the data */
			base16_encode( llsd->binary_.data,
						   llsd->binary_.data_size,
						   llsd->binary_.enc,
						   &(llsd->binary_.enc_size) );
			break;
		case LLSD_BASE64:
			/* allocate enc buffer */
			llsd->binary_.enc_size = BASE64_LENGTH( llsd->binary_.data_size );
			llsd->binary_.enc = UT(CALLOC( llsd->binary_.enc_size, sizeof(uint8_t) ));
			llsd->binary_.dyn_enc = TRUE;
			llsd->binary_.encoding = LLSD_BASE64;

			/* encode the data */
			base64_encode( llsd->binary_.data,
						   llsd->binary_.data_size,
						   llsd->binary_.enc,
						   &(llsd->binary_.enc_size) );
			break;
		case LLSD_BASE85:
			/* allocate enc buffer */
			llsd->binary_.enc_size = BASE85_LENGTH( llsd->binary_.data_size );
			llsd->binary_.enc = UT(CALLOC( llsd->binary_.enc_size, sizeof(uint8_t) ));
			llsd->binary_.dyn_enc = TRUE;
			llsd->binary_.encoding = LLSD_BASE85;

			/* encode the data */
			base85_encode( llsd->binary_.data,
						   llsd->binary_.data_size,
						   llsd->binary_.enc,
						   &(llsd->binary_.enc_size) );
			break;
		default:
			WARN( "unknown binary encoding (%d)\n", (int)encoding );
			return FALSE;
	}

	return TRUE;
}

/* Base64: the exact amount is 3 * inlen / 4, minus 1 if the input ends
 with "=" and minus another 1 if the input ends with "==".
 Dividing before multiplying avoids the possibility of overflow.  */
static uint32_t llsd_decoded_binary_len( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, -1 );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), -1 );
	CHECK_RET( (llsd->binary_.enc_size != 0), -1 );
	CHECK_PTR_RET( llsd->binary_.enc, -1 );
	CHECK_RET( ((llsd->binary_.encoding >= LLSD_BIN_ENC_FIRST) && (llsd->binary_.encoding < LLSD_BIN_ENC_LAST)), -1 );

	switch ( llsd->binary_.encoding )
	{
		case LLSD_NONE:
			break;
		case LLSD_BASE16:
			return base16_decoded_len( llsd->binary_.data, llsd->binary_.data_size );
		case LLSD_BASE64:
			return base64_decoded_len( llsd->binary_.data, llsd->binary_.data_size );
		case LLSD_BASE85:
			return base85_decoded_len( llsd->binary_.data, llsd->binary_.data_size );
		default:
			break;
	}
	return 0;
}

int llsd_decode_binary( llsd_t * llsd )
{
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->binary_.data_size == 0), FALSE );
	CHECK_RET( (llsd->binary_.data == NULL), FALSE );
	CHECK_RET( (llsd->binary_.enc_size != 0), FALSE );
	CHECK_PTR_RET( llsd->binary_.enc, FALSE );
	CHECK_RET( ((llsd->binary_.encoding >= LLSD_BIN_ENC_FIRST) && (llsd->binary_.encoding < LLSD_BIN_ENC_LAST)), FALSE );
	
	/* figure out the size of the needed buffer */
	size = llsd_decoded_binary_len( llsd );
	CHECK_RET( (size > -1), FALSE );

	/* allocate the data buffer */
	llsd->binary_.data = UT(CALLOC( size, sizeof(uint8_t) ));
	llsd->binary_.dyn_data = TRUE;

	switch ( llsd->binary_.encoding )
	{
		case LLSD_NONE:
			break;

		case LLSD_BASE16:
			/* decode the base16 data */
			base16_decode( llsd->binary_.enc,
						   llsd->binary_.enc_size,
						   llsd->binary_.data,
						   &(llsd->binary_.data_size) );
			break;

		case LLSD_BASE64:
			/* decode the base64 data */
			base64_decode( llsd->binary_.enc,
						   llsd->binary_.enc_size,
						   llsd->binary_.data,
						   &(llsd->binary_.data_size) );
			break;

		case LLSD_BASE85:
			/* decode the base64 data */
			base85_decode( llsd->binary_.enc,
						   llsd->binary_.enc_size,
						   llsd->binary_.data,
						   &(llsd->binary_.data_size) );
			break;
	}
	
	CHECK_RET( (llsd->binary_.data_size == size), FALSE );

	return TRUE;
}


static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";
static uint8_t const * const xml_signature = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
static uint8_t const * const xml_header = "<llsd>\n";
static uint8_t const * const xml_footer = "</llsd>\n";

#define XML_SIG_LEN (38)
#define XML_HEADER_LEN (6)
#define XML_FOOTER_LEN (7)

#define SIG_LEN (18)
llsd_t * llsd_parse( FILE *fin )
{
	size_t ret;
	uint8_t sig[SIG_LEN];
	CHECK_RET_MSG( fin, NULL, "invalid file pointer\n" );
	ret = fread( sig, sizeof(uint8_t), SIG_LEN, fin );
	CHECK_MSG( ret == SIG_LEN, "failed to read signature from LLSD file\n" );
	if ( memcmp( sig, binary_header, SIG_LEN ) == 0 )
	{
		return llsd_parse_binary( fin );
	}
	else
	{
		rewind( fin );
		return llsd_parse_xml( fin );
	}
}


size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout, int pretty )
{
	size_t s = 0;
	unsigned long start = ftell( fout );

	CHECK_PTR( llsd );
	CHECK_PTR( fout );

	switch ( fmt )
	{
		case LLSD_ENC_XML:
			s += fwrite( xml_signature, sizeof(uint8_t), XML_SIG_LEN + (pretty ? 1 : 0), fout );
			DEBUG( "XML SIG %lu - %lu\n", start, ftell( fout ) - 1 );
			start = ftell( fout );
			s += fwrite( xml_header, sizeof(uint8_t), XML_HEADER_LEN + (pretty ? 1 : 0), fout );
			DEBUG( "START LLSD %lu - %lu\n", start, ftell( fout ) - 1 );
			s += llsd_format_xml( llsd, fout, pretty );
			start = ftell( fout );
			s += fwrite( xml_footer, sizeof(uint8_t), XML_FOOTER_LEN + (pretty ? 1 : 0), fout );
			DEBUG( "END LLSD %lu - %lu\n", start, ftell( fout ) - 1 );
			return s;
		case LLSD_ENC_BINARY:
			s += fwrite( binary_header, sizeof(uint8_t), 18, fout );
			s += llsd_format_binary( llsd, fout );
			return s;
	}
	return 0;
}

static size_t llsd_get_zero_copy_size( llsd_t * llsd, llsd_serializer_t fmt, int pretty )
{
	size_t s = 0;

	CHECK_PTR_RET( llsd, 0 );

	switch ( fmt )
	{
		case LLSD_ENC_XML:
			s += 3; /* XML signature, header and footer */
			s += llsd_get_xml_zero_copy_size( llsd, pretty );
			return s;
		case LLSD_ENC_BINARY:
			s += 1; /* binary header */
			s += llsd_get_binary_zero_copy_size( llsd );
			return s;
	}
	return 0;
}

size_t llsd_format_zero_copy( llsd_t * llsd, llsd_serializer_t fmt, struct iovec ** v, int pretty )
{
	size_t s = 0;

	CHECK_PTR_RET( llsd, 0 );
	CHECK_PTR_RET( v, 0 );

	/* get the total number of iovec structs needed */
	s = llsd_get_zero_copy_size( llsd, fmt, pretty );

	/* allocate a buffer for the iovec structs */
	(*v) = (struct iovec*)CALLOC( s, sizeof(struct iovec) );

	switch ( fmt )
	{
		case LLSD_ENC_XML:
			(*v)[0].iov_base = (void*)xml_signature;
			(*v)[0].iov_len = XML_SIG_LEN + (pretty ? 1 : 0);
			(*v)[1].iov_base = (void*)xml_header;
			(*v)[1].iov_len = XML_HEADER_LEN + (pretty ? 1 : 0);

			/* add in the iovec structs for the llsd */
			s = 2;
			s += llsd_format_xml_zero_copy( llsd, &((*v)[s]), pretty );

			(*v)[s].iov_base = (void*)xml_footer;
			(*v)[s].iov_len = XML_FOOTER_LEN + (pretty ? 1 : 0);
			s++;
			return s;
		case LLSD_ENC_BINARY:
			/* add in the header */
			(*v)[0].iov_base = (void*)binary_header;
			(*v)[0].iov_len = SIG_LEN;

			/* add in the iovec structs for the llsd */
			s = 1;
			s += llsd_format_binary_zero_copy( llsd, &((*v)[s]) );

			return s;
	}
	return 0;
}

