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
#include "llsd.h"

int8_t const * const llsd_type_strings[LLSD_TYPE_COUNT] =
{
	T("UNDEF"),
	T("BOOLEAN"),
	T("INTEGER"),
	T("REAL"),
	T("UUID"),
	T("STRING"),
	T("DATE"),
	T("URI"),
	T("BINARY"),
	T("ARRAY"),
	T("MAP")
};

/* constants */
llsd_t const undefined =
{
	.type_ = LLSD_UNDEF,
	.value.bool_ = FALSE
};

uint8_t bits[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
llsd_uuid_t const zero_uuid = 
{ 
	.dyn_bits = FALSE,
	.dyn_str = FALSE,
	.bits = bits,
	.str = NULL
};

llsd_string_t const false_string = 
{
	.dyn_str = FALSE,
	.dyn_esc = FALSE,
	.key_esc = FALSE,
	.str_len = 5,
	.str = "false",
	.esc_len = 0,
	.esc = NULL
};
llsd_string_t const true_string = 
{
	.dyn_str = FALSE,
	.dyn_esc = FALSE,
	.key_esc = FALSE,
	.str_len = 4,
	.str = "true",
	.esc_len = 0,
	.esc = NULL
};

uint8_t zero_data [] = { '0' };
llsd_binary_t const false_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 1,
	.data = zero_data,
	.enc_size = 0,
	.enc = NULL
};
uint8_t one_data[] = { '1' };
llsd_binary_t const true_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 1,
	.data = one_data,
	.enc_size = 0,
	.enc = NULL
};
llsd_binary_t const empty_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 0,
	.data = NULL,
	.enc_size = 0,
	.enc = NULL
};
llsd_uri_t const empty_uri = 
{
	.dyn_uri = FALSE,
	.dyn_esc = FALSE,
	.uri_len = 0,
	.uri = "",
	.esc_len = 0,
	.esc = NULL
};
llsd_array_t const empty_array =
{
	.array = {
		.pfn = NULL,
		.num_nodes = 0,
		.buffer_size = 0,
		.data_head = -1,
		.free_head = -1,
		.node_buffer = NULL
	}
};
llsd_map_t const empty_map =
{
	.ht = {
		.khfn = NULL,
		.kefn = NULL,
		.kdfn = NULL,
		.vdfn = NULL,
		.prime_index = 0,
		.num_tuples = 0,
		.initial_capacity = 0,
		.load_factor = 0.0f,
		.tuples = NULL
	}
};

/* this function does the minimum amount of work to the l and r llsd objects
 * so that we can use standard ways of comparint the two.  so if the l object
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
			if ( ( ( l->value.uuid_.bits != NULL ) && ( r->value.uuid_.bits != NULL ) ) ||
				 ( ( l->value.uuid_.str != NULL ) && ( r->value.uuid_.bits != NULL ) ) )
				return TRUE;
			if ( l->value.uuid_.bits == NULL )
				llsd_destringify_uuid( l );
			else
				llsd_destringify_uuid( r );
			return TRUE;
		case LLSD_STRING:
			if ( ( ( l->value.string_.str != NULL ) && ( r->value.string_.str != NULL ) ) ||
				 ( ( l->value.string_.esc != NULL ) && ( r->value.string_.esc != NULL ) ) )
				return TRUE;
			if ( l->value.string_.str == NULL )
				llsd_unescape_str( l );
			else
				llsd_unescape_str( r );
			return TRUE;
		case LLSD_DATE:
			if ( ( ( l->value.date_.use_dval == TRUE ) && ( r->value.date_.use_dval == TRUE ) ) ||
				 ( ( l->value.date_.str != NULL ) && ( r->value.date_.str != NULL ) ) )
				return TRUE;
			if ( l->value.date_.use_dval != TRUE )
				llsd_destringify_date( l );
			else
				llsd_destringify_date( r );
			return TRUE;
		case LLSD_URI:
			if ( ( ( l->value.uri_.uri != NULL ) && ( r->value.uri_.uri != NULL ) ) ||
				 ( ( l->value.uri_.esc != NULL ) && ( r->value.uri_.esc != NULL ) ) )
				return TRUE;
			if ( l->value.uri_.uri == NULL )
				llsd_unescape_uri( l );
			else
				llsd_unescape_uri( r );
			return TRUE;
		case LLSD_BINARY:
			if ( ( ( l->value.binary_.data != NULL ) && ( r->value.binary_.data != NULL ) ) ||
				 ( ( l->value.binary_.enc != NULL ) && ( l->value.binary_.enc != NULL ) ) )
				return TRUE;
			if ( l->value.binary_.data == NULL )
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

static int llsd_string_eq( llsd_t * l, llsd_t * r )
{
	CHECK_RET( (l->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (r->type_ == LLSD_STRING), FALSE );

	/* do minimal work to allow use to compare str's */
	llsd_equalize( l, r );

	return ( mem_len_cmp( l->value.string_.str, l->value.string_.str_len,
						  r->value.string_.str, r->value.string_.str_len ) ||
			 mem_len_cmp( l->value.string_.esc, l->value.string_.esc_len,
						  r->value.string_.esc, r->value.string_.esc_len ) );
}

static int llsd_uri_eq( llsd_t * l, llsd_t * r )
{
	CHECK_RET( (l->type_ == LLSD_URI), FALSE );
	CHECK_RET( (r->type_ == LLSD_URI), FALSE );

	/* do minimal work to allow use to compare str's */
	llsd_equalize( l, r );

	return ( mem_len_cmp( l->value.uri_.uri, l->value.uri_.uri_len,
						  r->value.uri_.uri, r->value.uri_.uri_len ) ||
			 mem_len_cmp( l->value.uri_.esc, l->value.uri_.esc_len,
						  r->value.uri_.esc, r->value.uri_.esc_len ) );
}

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
		return FALSE;
	
	switch( llsd_get_type( l ) )
	{
		case LLSD_UNDEF:
			return TRUE;

		case LLSD_BOOLEAN:
			return ( llsd_as_bool( l ) == llsd_as_bool( r ) );

		case LLSD_INTEGER:
			return ( llsd_as_int( l ) == llsd_as_int( r ) );

		case LLSD_REAL:
			return ( llsd_as_real( l ) == llsd_as_real( r ) );

		case LLSD_UUID:
			llsd_equalize( l, r );
			if ( ( l->value.uuid_.bits != NULL ) && ( r->value.uuid_.bits != NULL ) )
				return ( MEMCMP( l->value.uuid_.bits, r->value.uuid_.bits, UUID_LEN ) == 0 );
			if ( ( r->value.uuid_.str != NULL ) && ( r->value.uuid_.str != NULL ) )
				return ( MEMCMP( l->value.uuid_.str, r->value.uuid_.str, UUID_STR_LEN ) == 0 );
			return FALSE;

		case LLSD_STRING:
			return llsd_string_eq( &(l->value.string_), &(r->value.string_) );

		case LLSD_DATE:
			llsd_equalize( l, r );
			if ( ( l->value.date_.use_dval == TRUE) && ( r->value.date_.use_dval == TRUE ) )
				return ( l->value.date_.dval == l->value.date_.dval );
			if ( ( l->value.date_.str != NULL ) && ( r->value.date_.str != NULL ) )
				return ( MEMCMP( l->value.date_.str, r->value.date_.str, DATE_STR_LEN ) == 0 );
			return FALSE;

		case LLSD_URI:
			return llsd_uri_eq( &(l->value.uri_), &(r->value.uri_) );

		case LLSD_BINARY:
			if ( ( l->value.binary_.data != NULL ) && ( r->value.binary_.data != NULL ) )
				return ( ( l->value.binary_.data_size == r->value.binary_.data_size ) &&
						 ( MEMCMP( l->value.binary_.data, r->value.binary_.data, l->value.binary_.data_size ) == 0 ) );
			if ( ( l->value.binary_.enc != NULL ) && ( l->value.binary_.data != NULL ) )
				return ( ( l->value.binary_.enc_size == r->value.binary_.enc_size ) &&
					     ( MEMCMP( l->value.binary_.enc, r->value.binary_.enc, l->value.binary_.enc_size) == 0 ) );
			return FALSE;

		case LLSD_ARRAY:	
			if ( array_size( &(l->value.array_.array) ) != array_size( &(r->value.array_.array) ) )
				return FALSE;

			itrl = llsd_itr_begin( l );
			itrr = llsd_itr_begin( r );
			for ( ; itrl != llsd_itr_end( l ); itrl = llsd_itr_next( l, itrl ), itrr = llsd_itr_next( r, itrr) )
			{
				llsd_itr_get( l, itrl, &vl, &kl );
				llsd_itr_get( r, itrr, &vr, &kr );
				if ( !llsd_equal( vl, vr ) )
					return FALSE;
			}
			return TRUE;

		case LLSD_MAP:
			if ( ht_size( &(l->value.map_.ht) ) != ht_size( &(r->value.map_.ht) ) )
				return FALSE;

			itrl = llsd_itr_begin( l );
			itrr = llsd_itr_begin( r );
			for ( ; itrl != llsd_itr_end( l ); itrl = llsd_itr_next( l, itrl ), itrr = llsd_itr_next( r, itrr) )
			{
				llsd_itr_get( l, itrl, &vl, &kl );
				vr = llsd_map_find( r, kl );
				if ( !llsd_equal( vl, vr ) )
					return FALSE;
			}
			return TRUE;
	}

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
			return llsd->value.bool_;
		case LLSD_INTEGER:
			if ( llsd->value.int_ != 0 )
				return TRUE;
			return FALSE;
		case LLSD_REAL:
			if ( llsd->value.real_ != 0. )
				return TRUE;
			return FALSE;
		case LLSD_UUID:
			/* destringify uuid if needed */
			llsd_destringify_uuid( llsd );
			if ( MEMCMP( llsd->value.uuid_.bits, zero_uuid.bits, UUID_LEN ) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_STRING:
			if ( ( llsd->value.string_.str_len == 0 ) && ( llsd->value.string_.enc_len == 0 ) )
				return FALSE;
			return TRUE;
		case LLSD_BINARY:
			if ( ( llsd->value.binary_.data_size == 0 ) && (llsd->value.binary_.enc_size == 0 ) )
				return FALSE;
			return TRUE;
	}
	return FALSE;
}

llsd_int_t llsd_as_int( llsd_t * llsd )
{
	static uint8_t * p = NULL;
	static llsd_int_t tmp_int;
	CHECK_PTR_RET_MSG( llsd, 0, "invalid llsd pointer\n" );

	switch( llsd->type_ )
	{
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			FAIL( "illegal conversion of %s to integer\n", llsd_get_type_string( llsd->type_ ) );
			break;
		case LLSD_UNDEF:
			return 0;
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return 0;
			return 1;
		case LLSD_INTEGER:
			return llsd->value.int_;
		case LLSD_REAL:
			if ( isnan( llsd->value.real_ ) )
			{
				FAIL( "converting NaN real to integer\n" );
			}
			else if ( isinf( llsd->value.real_ ) != 0 )
			{
				FAIL( "converting Infinite real to integer\n" );
			}
			return (llsd_int_t)lrint( llsd->value.real_ );
		case LLSD_DATE:
			/* de-stringify date if needed */
			llsd_destringify_date( llsd );
			if ( (uint64_t)llsd->value.date_.dval >= UINT32_MAX )
			{
				DEBUG( "truncating 64-bit date value to 32-bit integer...loss of data!" );
			}
			return (llsd_int_t)llsd->value.date_.dval;
		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			p = UT(CALLOC( llsd->value.string_.str_len + 1, sizeof(uint8_t) ));
			MEMCPY( p, llsd->value.string_.str_len, llsd->value.string_.str );
			tmp_int = (llsd_int_t)atoi( p );
			FREE( p );
			return tmp_int;
		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );
			if ( llsd->value.binary_.data_size < sizeof(llsd_int_t) )
			{
				return 0;
			}
			return ntohl( *((uint32_t*)llsd->value.binary_.data) );
	}
	return 0;
}

llsd_real_t llsd_as_real( llsd_t * llsd )
{
	static uint8_t * p = NULL;
	static llsd_real_t tmp_real;
	CHECK_PTR_RET_MSG( llsd, FALSE, "invalid llsd pointer\n" );

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
			if ( llsd->value.bool_ == FALSE )
				return 0.;
			return 1.;
		case LLSD_INTEGER:
			return (llsd_real_t)llsd->value.int_;
		case LLSD_REAL:
			return (llsd_real_t)llsd->value.real_;
		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			p = UT(CALLOC( llsd->value.string_.str_len + 1, sizeof(uint8_t) ));
			MEMCPY( p, llsd->value.string_.str_len, llsd->value.string_.str );
			tmp_real = (llsd_real_t)atof( p );
			FREE( p );
			return tmp_real;
		case LLSD_DATE:
			/* de-stringify date if needed */
			llsd_destringify_date( llsd );
			return (llsd_real_t)llsd->value.date_.dval;
		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );
			if ( llsd->value.binary_.data_size < sizeof(llsd_real_t) )
			{
				return 0.;
			}
			return (llsd_real_t)be64toh( *((uint64_t*)llsd->value.binary_.data) );
	}
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
			if ( llsd->value.binary_.data_size < 16 )
			{
				return zero_uuid;
			}
			MEMCPY( bits, llsd->value.binary_.data, UUID_LEN );
			return (llsd_uuid_t){ .dyn_bits = FALSE,
								  .dyn_str = FALSE,
								  .bits = bits,
								  .str = NULL };

		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			tmp_llsd.type_ = LLSD_UUID;
			tmp_llsd.value.uuid_ = { .dyn_bits = FALSE,
									 .dyn_str = FALSE,
									 .len = llsd->value.string_.str_len,
									 .str = llsd->value.string_.str,
									 .bits = NULL };
			llsd_destringify_uuid( &tmp_llsd );
			tmp_llsd.value.uuid_.len = 0;
			tmp_llsd.value.uuid_.str = NULL; /* remove our reference */
			return (llsd_uuid_t)tmp_llsd.value.uuid_;

		case LLSD_UUID:
			return (llsd_uuid_t)llsd->value.uuid_;
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
	return (llsd_array_t)llsd->value.array_;
}

llsd_map_t llsd_as_map( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, empty_map, "invalid llsd pointer\n" );
	if ( llsd->type_ != LLSD_MAP )
	{
		FAIL( "illegal conversion of %s to array\n", llsd_get_type_string( llsd->type_ ) );
		return empty_map;
	}
	return (llsd_map_t)llsd->value.map_;
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
			FAIL( "illegal conversion of %s to string\n", llsd_get_type_string( llsd->type_ ) );
			break;
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return false_string;
			return true_string;
		case LLSD_INTEGER:
			len = snprintf( tmp, 64, "%d", llsd->value.int_ );
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = strnlen( tmp, 64 ),
									.str = tmp,
									.esc_len = 0,
									.esc = NULL };
		case LLSD_REAL:
			len = snprintf( tmp, 64, "%f", llsd->value.real_ );
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
									.str = llsd->value.uuid_.str,
									.esc_len = 0,
									.esc = NULL };

		case LLSD_STRING:
			return llsd->value.string_;
		case LLSD_DATE:
			/* stringify the date if needed */
			llsd_stringify_date( llsd );
			return (llsd_string_t){ .dyn_str = FALSE,
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = DATE_STR_LEN,
									.str = llsd->value.date_.str,
									.esc_len = 0,
									.esc = NULL };
		case LLSD_URI:
			/* unescape the URI if needed */
			llsd_unescape_uri( llsd );
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = llsd->value.uri_.uri_len,
									.str = llsd->value.uri_.uri,
									.esc_len = 0,
									.esc = NULL };

		case LLSD_BINARY:
			/* decode binary if needed */
			llsd_decode_binary( llsd );
			/* TODO: check for valid UTF-8 */
			return (llsd_string_t){ .dyn_str = FALSE, 
									.dyn_esc = FALSE,
									.key_esc = FALSE,
									.str_len = llsd->value.binary_.data_size,
									.str = llsd->value.binary_.data,
									.esc_len = 0,
									.esc = NULL };
	}
}

llsd_date_t llsd_as_date( llsd_t * llsd )
{
	static llsd_t tmp_llsd;

	CHECK_PTR_RET_MSG( llsd, 0., "invalid llsd pointer\n" );
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
								  .dval = (double)llsd->value.int_,
								  .str = NULL };

		case LLSD_REAL:
			return (llsd_date_t){ .use_dval = TRUE,
								  .dyn_str = FALSE,
								  .dval = llsd->value.real_,
								  .str = NULL };

		case LLSD_STRING:
			/* unescape the string if needed */
			llsd_unescape_string( llsd );

			tmp_llsd.type_ = LLSD_DATE;
			tmp_llsd.value.date_ = { .use_dval = FALSE,
									 .dyn_str = FALSE,
									 .dval = 0.0,
									 .str = llsd->value.string_.str };
			llsd_destringify_date( &tmp_llsd );
			tmp_llsd.value.date_.str = NULL; /* remove reference to str */
			return (llsd_date_t)llsd_tmp.value.date_;

		case LLSD_DATE:
			return llsd->value.date_;
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
								 .uri_len = llsd->value.string_.str_len,
								 .uri = llsd->value.string_.str,
								 .esc_len = 0,
								 .esc = NULL };

		case LLSD_URI:
			return llsd->value.uri_;

		case LLSD_BINARY:
			/* decode the binary if needed */
			llsd_decode_binary( llsd );

			/* TODO: check for valid UTF-8 */
			return (llsd_uri_t){ .dyn_uri = FALSE, 
								 .dyn_esc = FALSE,
								 .uri_len = llsd->value.binary_.data_size,
								 .uri = llsd->value.binary_.data,
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
			if ( llsd->value.bool_ == FALSE )
				return false_binary;
			return true_binary;

		case LLSD_INTEGER:
			tmpl = htonl( llsd->value.int_ );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = sizeof( llsd_int_t ),
									.data = (uint8_t*)&tmpl,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_REAL:
			tmpll = htobe64( *((uint64_t*)&(llsd->value.real_)) );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = sizeof( llsd_real_t ),
									.data = (uint8_t*)&tmpll,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_UUID:
			/* de-stringify the uuid if needed */
			llsd_destringify_uuid( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = UUID_LEN,
									.data = llsd->value.uuid_.bits,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_STRING:
			/* unescape string if needed */
			llsd_unescape_string( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = llsd->value.string_.str_len,
									.data = llsd->value.string_.str,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_URI:
			/* unescape uri if needed */
			llsd_unescape_uri( llsd );
			return (llsd_binary_t){ .dyn_data = FALSE,
									.dyn_enc = FALSE,
									.encoding = LLSD_NONE,
									.data_size = llsd->value.uri_.uri_len,
									.data = llsd->value.uri_.uri,
									.enc_size = 0,
									.enc = NULL };

		case LLSD_BINARY:
			return llsd->value.binary_;
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
	CHECK_RET( (llsd->value.uuid_.len == 0), FALSE );
	CHECK_RET( (llsd->value.uuid_.str == NULL), FALSE );
	CHECK_PTR_RET( llsd->value.uuid_.bits, FALSE );

	/* allocate the string */
	llsd->value.uuid_.str = UT(CALLOC( UUID_STR_LEN, sizeof(uint8_t) ));
	llsd->value.uuid_.dyn_str = TRUE;
	llsd->value.uuid_.len = UUID_STR_LEN;

	/* convert to string and cache it */
	p = llsd->value.uuid_.bits;
	snprintf( llsd->value.uuid_.str, UUID_STR_LEN, 
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
	uint8_t * pin;
	uint8_t * pout;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_UUID), FALSE );
	CHECK_RET( (llsd->value.uuid_.len == UUID_STR_LEN), FALSE );
	CHECK_PTR_RET( llsd->value.uuid_.str, FALSE );
	CHECK_RET( (llsd->value.uuid_.bits == NULL), FALSE );

	/* check for 8-4-4-4-12 */
	for ( i = 0; i < 36; i++ )
	{
		if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
			 ( (i >= 9) && (i < 13) ) &&    /* 4 */
			 ( (i >= 14) && (i < 18) ) &&   /* 4 */
			 ( (i >= 19) && (i < 23) ) &&	/* 4 */
			 ( (i >= 24) && (i < 36) ) )	/* 12 */
		{
			if ( !isxdigit( llsd->value.uuid_.str[i] ) )
			{
				return FALSE;
			}
		}
		else if ( llsd->value.uuid_.str[i] != '-' )
		{
			return FALSE;
		}
	}

	/* allocate the bits */
	llsd->value.uuid_.bits = UT(CALLOC( UUID_LEN, sizeof(uint8_t) ));
	llsd->value.uuid_.dyn_bits = TRUE;

	/* covert to UUID */
	pin = llsd->value.uuid_.str;
	pout = llsd->value.uuid_.bits;

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
	CHECK_RET( (llsd->value.date_.len == 0), FALSE );
	CHECK_RET( (llsd->value.date_.str == NULL), FALSE );

	/* allocate the string */
	llsd->value.date_.str = CALLOC( DATE_STR_LEN, sizeof(uint8_t) );
	llsd->value.date_.dyn_str = TRUE;
	llsd->value.date_.len = DATE_STR_LEN;

	int_time = floor( llsd->value.date_.dval );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( llsd->value.date_.dval - int_time) * 1000000.0 );
	parts = *gmtime(&seconds);
	snprintf( llsd->value.date_.str, DATE_STR_LEN, 
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
	return TRUE;
}

int llsd_escape_string( llsd_t * llsd )
{
	return TRUE;
}

int llsd_unescape_string( llsd_t * llsd )
{
	return TRUE;
}

int llsd_escape_uri( llsd_t * llsd )
{
	return TRUE;
}

int llsd_unescape_uri( llsd_t * llsd )
{
	return TRUE;
}

int llsd_encode_binary( llsd_t * llsd, llsd_bin_enc_t encoding )
{
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->value.binary_.data_size != 0), FALSE );
	CHECK_PTR_RET( llsd->value.binary_.data, FALSE );
	CHECK_RET( (llsd->value.binary_.enc_size == 0), FALSE );
	CHECK_RET( (llsd->value.binary_.enc == NULL), FALSE );
	CHECK_RET( ((encoding >= LLSD_BIN_ENC_FIRST) && (encoding < LLSD_BIN_ENC_LAST)), FALSE );

	switch ( encoding )
	{
		case LLSD_BASE16:
			WARN( "base 16 encoding unimplemented\n" );
			return FALSE;
		case LLSD_BASE64:
			/* allocate enc buffer */
			llsd->value.binary_.enc_size = BASE64_LENGTH( lsd->value.binary_.data_size );
			llsd->value.binary_.enc = UT(CALLOC( llsd->value.binary_.enc_size, sizeof(uint8_t) ));
			llsd->value.binary_.dyn_enc = TRUE;
			llsd->value.binary_.encoding = LLSD_BASE64;

			/* encode the data */
			base64_encode( llsd->value.binary_.data,
						   llsd->value.binary_.data_size,
						   llsd->value.binary_.enc,
						   llsd->value.binary_.enc_size );
			break;
	}

	return TRUE;
}

/* Base64: the exact amount is 3 * inlen / 4, minus 1 if the input ends
 with "=" and minus another 1 if the input ends with "==".
 Dividing before multiplying avoids the possibility of overflow.  */
static uint32_t llsd_decoded_binary_len( llsd_t * llsd, llsd_bin_enc_t encoding )
{
	uint32_t len = 0;
	uint32_t size = 0;
	CHECK_PTR_RET( llsd, -1 );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), -1 );
	CHECK_RET( (llsd->value.binary_.enc_size != 0), -1 );
	CHECK_RET_PTR( llsd->value.binary_.enc, -1 );

	size = llsd->value.binary_.data_size;

	switch ( encoding )
	{
		case LLSD_BASE16:
			break;
		case LLSD_BASE64:
			len = 3 * (size / 4) + 2;
			if ( llsd->value.binary_.data[ size - 1 ] == '=' )
				len--;
			if ( llsd->value.binary_.data[ size - 2 ] == '=' )
				len--;
			return len;
	}
	return -1;
}

int llsd_decode_binary( llsd_t * llsd )
{
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->value.binary_.data_size == 0), FALSE );
	CHECK_PTR( (llsd->value.binary_.data == NULL), FALSE );
	CHECK_RET( (llsd->value.binary_.enc_size != 0), FALSE );
	CHECK_RET_PTR( llsd->value.binary_.enc, FALSE );
	CHECK_RET( ((llsd->value.binary_.encoding >= LLSD_BIN_ENC_FIRST) && (llsd->value.binary_.encoding < LLSD_BIN_ENC_LAST)), FALSE );

	switch ( llsd->value.binary_.encoding )
	{
		case LLSD_BASE16:
			break;
		case LLSD_BASE64:
			/* figure out the size of the needed buffer */
			size = llsd_decoded_binary_len( llsd );
			CHECK_RET( (size > -1), FALSE );

			/* allocate the data buffer */
			llsd->value.binary_.data = UT(CALLOC( size, sizeof(uint8_t) ));
			llsd->value.binary_.dyn_data = TRUE;

			/* decode the base64 data */
			base64_decode( llsd->value.binary_.enc,
						   llsd->value.binary_.enc_size,
						   llsd->value.binary_.data,
						   &(llsd->value.binary_.data_size) );
			CHECK_RET( (llsd->value.binary_.data_size == size), FALSE );
			break;
	}

	return TRUE;
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


static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	llsd->value.binary_.dyn = FALSE;
	llsd->value.binary_.data_size = size;
	llsd->value.binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_string( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_STRING;
	llsd->value.string_.str_len = size;
	llsd->value.string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_URI;
	llsd->value.uri_.uri_len = size;
	llsd->value.uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_array( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_ARRAY;
	array_initialize( &(llsd->value.array_.array), size, &llsd_delete );
	return llsd;
}

static llsd_t * llsd_reserve_map( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_MAP;
	ht_initialize( &(llsd->value.map_.ht), 
				   size, 
				   &fnv_key_hash, 
				   &llsd_delete, 
				   &key_eq, 
				   &llsd_delete );
	return llsd;
}

static int indent = 0;

static llsd_t * llsd_parse_binary( FILE * fin )
{
	int i;
	unsigned long start = ftell( fin );
	uint8_t p;
	uint32_t t1;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	uint16_t t3[UUID_LEN];
	llsd_t * llsd;
	llsd_t * key;

	CHECK_PTR_RET( fin, NULL );

	while( !feof( fin ) )
	{
		/* read the type marker */
		fread( &p, sizeof(uint8_t), 1, fin );

		switch( p )
		{
			default:
				WARN( "invalid type character %c at offset %lu\n", p, ftell( fin ) );
				break;
			case '!':
				WARN( "%*sUNDEF %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_UNDEF );
			case '1':
				WARN( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_BOOLEAN, TRUE );
			case '0':
				WARN( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_BOOLEAN, FALSE );
			case 'i':
				fread( &t1, sizeof(uint32_t), 1, fin );
				WARN( "%*sINTEGER %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_INTEGER, ntohl( t1 ) );
			case 'r':
				fread( &t2, sizeof(double), 1, fin );
				t2.ull = be64toh( t2.ull );
				WARN( "%*sREAL %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_REAL, t2.d );
			case 'u':
				fread( t3, sizeof(uint8_t), UUID_LEN, fin );
				WARN( "%*sUUID %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_UUID, t3 );
			case 'b':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_binary( t1 );
				fread( llsd->value.binary_.data, sizeof(uint8_t), t1, fin );
				WARN( "%*sBINARY %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd;
			case 's':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_string( t1 );
				fread( llsd->value.string_.str, sizeof(uint8_t), t1, fin );
				WARN( "%*sSTRING %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd;
			case 'l':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_uri( t1 );
				fread( llsd->value.uri_.uri, sizeof(uint8_t), t1, fin );
				WARN( "%*sURI %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd;
			case 'd':
				fread( &t2, sizeof(double), 1, fin );
				t2.ull = be64toh( t2.ull );
				WARN( "%*sDATE %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd_new( LLSD_DATE, t2.d );
			case '[':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				WARN( "%*s[[ (%d)\n", indent, " ", t1 );
				indent += 4;
				llsd = llsd_reserve_array( t1 );
				for ( i = 0; i < t1; ++i )
				{
					/* parse and append the array member */
					llsd_array_append( llsd, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != ']' )
				{
					FAIL( "array didn't end with ']'\n" );
				}
				indent -= 4;
				WARN( "%*s]] ARRAY %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd;
			case '{':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				WARN( "%*s{{ (%d)\n", indent, " ", t1 );
				indent += 4;
				llsd = llsd_reserve_map( t1 );
				for ( i = 0; i < t1; ++i )
				{
					/* parse the key */
					key = llsd_parse_binary( fin );
					if ( llsd_get_type( key ) != LLSD_STRING )
					{
						FAIL( "key is not an LLSD_STRING\n" );
					}

					/* parse and append the key/value pair */
					llsd_map_insert( llsd, key, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != '}' )
				{
					FAIL( "map didn't end with '}'\n" );
				}
				indent -= 4;
				WARN( "%*s}} MAP %lu - %lu\n", indent, " ", start, ftell( fin ) - 1 );
				return llsd;
		}
	}
}

static llsd_t * llsd_parse_notation( FILE * fin )
{
	return NULL;
}

static llsd_t * llsd_parse_xml( FILE * fin )
{
	return NULL;
}

static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";
static uint8_t const * const notation_header = "<?llsd/notation?>\n";
static uint8_t const * const xml_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<llsd>\n";
static uint8_t const * const xml_footer = "</llsd>\n";

#define SIG_LEN (18)
llsd_t * llsd_parse( FILE *fin )
{
	uint8_t sig[SIG_LEN];

	CHECK_RET_MSG( fin, NULL, "invalid file pointer\n" );

	fread( sig, sizeof(uint8_t), SIG_LEN, fin );
	if ( MEMCMP( sig, binary_header, SIG_LEN ) == 0 )
	{
		return llsd_parse_binary( fin );
	}
	else if ( MEMCMP( sig, notation_header, SIG_LEN ) == 0 )
	{
		return llsd_parse_notation( fin );
	}
	else
	{
		rewind( fin );
		return llsd_parse_xml( fin );
	}
}

static size_t llsd_format_xml( llsd_t * llsd, FILE * fout )
{
	return 0;
}

#define INDENT(x, y) (( x ) ? fprintf( "%*s", y, " " ) : 0)


static size_t llsd_format_notation( llsd_t * llsd, FILE * fout, int pretty )
{
	static uint8_t buf[128];
	int i;
	uint8_t * str

	size_t num = 0;
	unsigned long start = ftell( fout );
	uint32_t s;
	uint8_t p;

	llsd_itr_t itr;
	llsd_t *k, *v;

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "!" );
			WARN( "%*sUNDEF %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "%c", ( llsd_as_bool( llsd ) == TRUE ) ? '1' : '0' );
			WARN( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_INTEGER:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "i%s", llsd_as_string( llsd ) );
			WARN( "%*sINTEGER %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_REAL:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "r%s", llsd_as_string( llsd ) );
			WARN( "%*sREAL %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_UUID:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "u%s", llsd_as_string( llsd ) );
			WARN( "%*sUUID %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_STRING:
			num += INDENT( pretty, indent );
			llsd_escape_string( llsd );
			num += fprintf( fout, "s\"%s\"", llsd_as_string( llsd ).esc );
			WARN( "%*sSTRING %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_DATE:
			num += INDENT( pretty, indent );
			num += fprintf( fout, "d\"%s\"", llsd_as_string( llsd ).str );
			WARN( "%*sDATE %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_URI:
			num += INDENT( pretty, indent );
			llsd_escape_uri( llsd );
			num += fprintf( fout, "l\"%s\"", llsd_as_string( llsd ).esc );
			WARN( "%*sURI %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BINARY:
			num += INDENT( pretty, indent );
			llsd_encode_binary( llsd, LLSD_BASE64 );
			num += fprintf( fout, "b64\"%s\"", llsd_as_binary( llsd ).enc );
			WARN( "%*sBINARY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_ARRAY:
			s = array_size( &(llsd->value.array_.array) );
			WARN( "%*s[[ (%d)\n", indent, " ", s );

			num += INDENT( pretty, indent );
			indent += 2;
			num += fprintf( fout, "[%s", (((s > 1) && pretty) ? "\n" : "") );

			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				if ( itr != llsd_itr_begin( llsd ) )
				{
					num += fprintf( fout, ",%s", (pretty ? "\n": "") );
				}

				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				num += llsd_format_notation( v, fout );
			}

			indent -= 2;

			if ( ( s > 1 ) && pretty )
			{
				num += fprintf( fout, "\n%*s]\n", indent, " " );
			}
			else
			{
				num += fprintf( fout, "]" );
			}

			WARN( "%*s]] ARRAY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_MAP:
			s = ht_size( &(llsd->value.map_.ht) );
			WARN( "%*s{{ (%d)\n", indent, " ", s );

			num += INDENT( pretty, indent );
			indent += 2;
			num += fprintf( fout, "{%s", (((s > 1) && pretty) ? "\n" : "") );

			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				if ( itr != llsd_itr_begin(llsd) )
				{
					num += fprintf( fout, ",%s", (pretty ? "\n" : "") );
				}

				llsd_itr_get( llsd, itr, &v, &k );
				num += llsd_format_notation( k, fout );
				p = ':';
				num += fwrite( &p, sizeof(uint8_t), 1, fout );
				num += llsd_format_notation( v, fout );
			}

			indent -= 2;
			if ( ( s > 1 ) && pretty )
			{
				num += fprintf( fout, "\n%*s}\n", indent, " " );
			}
			else
			{
				num += fprintf( fout, "}" );
			}

			WARN( "%*s}} MAP %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
	}
	return num;
}

static size_t llsd_format_binary( llsd_t * llsd, FILE * fout )
{
	size_t num = 0;
	unsigned long start = ftell( fout );
	uint32_t s;
	uint8_t p;
	uint32_t t1;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	uint16_t t3[UUID_LEN];

	llsd_itr_t itr;
	llsd_t *k, *v;

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
			p = '!';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			WARN( "%*sUNDEF %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
			p = ( llsd_as_bool( llsd ) == TRUE ) ? '1' : '0';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			WARN( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_INTEGER:
			p = 'i';
			t1 = htonl( llsd_as_int( llsd ) );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			WARN( "%*sINTEGER %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_REAL:
			p = 'r';
			t2.d = llsd_as_real( llsd );
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			WARN( "%*sREAL %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_UUID:
			p = 'u';
			/* de-stringify the uuid if needed */
			llsd_destringify_uuid( llsd );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += fwrite( &(llsd->value.uuid_.bits[0]), sizeof(uint8_t), UUID_LEN, fout );
			WARN( "%*sUUID %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_STRING:
			p = 's';
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			s = llsd->value.string_.str_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd_as_string( llsd ).str, sizeof(uint8_t), s, fout );
			WARN( "%*sSTRING %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_DATE:
			p = 'd';
			/* de-stringify date if needed */
			llsd_destringify_date( llsd );
			t2.d = llsd->value.date_.dval;
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			WARN( "%*sDATE %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_URI:
			p = 'l';
			/* unescape uri if needed */
			llsd_unescape_uri( llsd );
			s = llsd->value.uri_.uri_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.uri_.uri, sizeof(uint8_t), s, fout );
			WARN( "%*sURI %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BINARY:
			p = 'b';
			/* decode binary if needed */
			llsd_decode_binary( llsd );
			s = llsd->value.binary_.data_size;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.binary_.data, sizeof(uint8_t), s, fout );
			WARN( "%*sBINARY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_ARRAY:
			p = '[';
			s = array_size( &(llsd->value.array_.array) );
			WARN( "%*s[[ (%d)\n", indent, " ", s );
			indent += 4;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );

			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				num += llsd_format_binary( v, fout );
			}

			p = ']';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			indent -= 4;
			WARN( "%*s]] ARRAY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_MAP:
			p = '{';
			s = ht_size( &(llsd->value.map_.ht) );
			WARN( "%*s{{ (%d)\n", indent, " ", s );
			indent += 4;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );

			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				num += llsd_format_binary( k, fout );
				num += llsd_format_binary( v, fout );
			}

			p = '}';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			indent -= 4;
			WARN( "%*s}} MAP %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
	}
	return num;
}

size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout )
{
	size_t s = 0;

	CHECK_PTR( llsd );
	CHECK_PTR( fout );

	switch ( fmt )
	{
		case LLSD_ENC_XML:
			s += fwrite( xml_header, sizeof(uint8_t), 46, fout );
			s += llsd_format_xml( llsd, fout );
			s += fwrite( xml_footer, sizeof(uint8_t), 8, fout );
			return s;
		case LLSD_ENC_NOTATION:
			s += fwrite( notation_header, sizeof(uint8_t), 18, fout );
			s += llsd_format_notation( llsd, fout );
			return s;
		case LLSD_ENC_BINARY:
			s += fwrite( binary_header, sizeof(uint8_t), 18, fout );
			s += llsd_format_binary( llsd, fout );
			return s;
		case LLSD_ENC_JSON:
			FAIL( "JSON encoding not supported yet\n" );
	}
	return 0;
}

