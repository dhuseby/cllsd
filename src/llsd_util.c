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
#include "base64.h"

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
				llsd_unescape_string( l );
			else
				llsd_unescape_string( r );
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
			return llsd_string_eq( l, r );

		case LLSD_DATE:
			llsd_equalize( l, r );
			if ( ( l->value.date_.use_dval == TRUE) && ( r->value.date_.use_dval == TRUE ) )
				return ( l->value.date_.dval == l->value.date_.dval );
			if ( ( l->value.date_.str != NULL ) && ( r->value.date_.str != NULL ) )
				return ( MEMCMP( l->value.date_.str, r->value.date_.str, DATE_STR_LEN ) == 0 );
			return FALSE;

		case LLSD_URI:
			return llsd_uri_eq( l, r );

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
			if ( ( llsd->value.string_.str_len == 0 ) && 
				 ( llsd->value.string_.esc_len == 0 ) )
				return FALSE;
			return TRUE;
		case LLSD_BINARY:
			if ( ( llsd->value.binary_.data_size == 0 ) && 
				 (llsd->value.binary_.enc_size == 0 ) )
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
			MEMCPY( p, llsd->value.string_.str, llsd->value.string_.str_len );
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
			MEMCPY( p, llsd->value.string_.str, llsd->value.string_.str_len );
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
			tmp_llsd.value.uuid_ = (llsd_uuid_t){ .dyn_bits = FALSE,
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
			tmp_llsd.value.date_ = (llsd_date_t){ .use_dval = FALSE,
												  .dyn_str = FALSE,
												  .dval = 0.0,
												  .str = llsd->value.string_.str };
			llsd_destringify_date( &tmp_llsd );
			tmp_llsd.value.date_.str = NULL; /* remove reference to str */
			return (llsd_date_t)tmp_llsd.value.date_;

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


static llsd_t * llsd_reserve_binary( uint32_t size, int encoded )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	if ( encoded )
	{
		llsd->value.binary_.enc_size = size;
		llsd->value.binary_.enc = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	else
	{
		llsd->value.binary_.data_size = size;
		llsd->value.binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_string( uint32_t size, int escaped )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_STRING;
	if ( escaped )
	{
		llsd->value.string_.esc_len = size;
		llsd->value.string_.esc = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	else
	{
		llsd->value.string_.str_len = size;
		llsd->value.string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size, int escaped )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_URI;
	if ( escaped )
	{
		llsd->value.uri_.esc_len = size;
		llsd->value.uri_.esc = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	else
	{
		llsd->value.uri_.uri_len = size;
		llsd->value.uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_date( uint32_t size, int stringified )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_DATE;
	if ( stringified )
	{
		llsd->value.date_.len = size;
		llsd->value.date_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	else
	{
		llsd->value.date_.use_dval = TRUE;
	}
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


static llsd_t * llsd_parse_notation( FILE * fin )
{
	return NULL;
}

static llsd_t * llsd_parse_xml( FILE * fin )
{
	return NULL;
}

static size_t llsd_format_xml( llsd_t * llsd, FILE * fout )
{
	return 0;
}

#define INDENT(f, x, y) (( x ) ? fprintf( f, "%*s", y, " " ) : 0)

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

static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";
static uint8_t const * const xml_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<llsd>\n";
static uint8_t const * const xml_footer = "</llsd>\n";

#define SIG_LEN (18)
llsd_t * llsd_parse( uint8_t * p, size_t len )
{
	CHECK_RET_MSG( p, NULL, "invalid buffer pointer\n" );
	CHECK_RET_MSG( len >= SIG_LEN, NULL, "not enough valid LLSD bytes\n" );

	if ( MEMCMP( p, binary_header, SIG_LEN ) == 0 )
	{
		return (llsd_t *)llsd_parse_binary( &(p[SIG_LEN]), (len - SIG_LEN) );
	}
	else
	{
		return (llsd_t *)llsd_parse_xml( p, len );
	}
}


size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout, int pretty )
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
		case LLSD_ENC_BINARY:
			s += fwrite( binary_header, sizeof(uint8_t), 18, fout );
			s += llsd_format_binary( llsd, fout );
			return s;
		case LLSD_ENC_JSON:
			FAIL( "JSON encoding not supported yet\n" );
	}
	return 0;
}

