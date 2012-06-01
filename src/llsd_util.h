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

#ifndef LLSD_UTIL_H
#define LLSD_UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <cutil/macros.h>

#include "llsd.h"

#if defined(PORTABLE_64_BIT)
typedef uint64_t uint_t;
typedef int64_t int_t;
#elif defined(PORTABLE_32_BIT)
typedef uint32_t uint_t;
typedef int32_t int_t;
#else
#error "failed to identify if we're on a 64-bit or 32-bit platform"
#endif

/* new/delete llsd objects */
llsd_t * llsd_new( llsd_type_t type_, ... );
void llsd_delete( void * p );

/* utility macros */
#define llsd_new_empty_array() llsd_new( LLSD_ARRAY, 0 )
#define llsd_new_empty_map() llsd_new( LLSD_MAP, 0 )
#define llsd_new_array( s ) llsd_new ( LLSD_ARRAY, s )
#define llsd_new_map( s ) llsd_new ( LLSD_MAP, s )
#define llsd_new_boolean( val ) llsd_new ( LLSD_BOOLEAN, val )
#define llsd_new_integer( val ) llsd_new ( LLSD_INTEGER, val )
#define llsd_new_real( val ) llsd_new ( LLSD_REAL, val )
#define llsd_new_uuid( bits ) llsd_new ( LLSD_UUID, bits )
#define llsd_new_string( s, len, esc, key ) llsd_new( LLSD_STRING, s, len, esc, key )
#define llsd_new_uri( s, len, esc ) llsd_new( LLSD_URI, s, len, esc )
#define llsd_new_binary( p, len, enc, encoding ) llsd_new( LLSD_BINARY, p, len, enc, encoding )
#define llsd_new_date( d, s, len ) llsd_new( LLSD_DATE, d, s, len )

/* get the type of the particular object */
llsd_type_t llsd_get_type( llsd_t * llsd );
int8_t const * llsd_get_type_string( llsd_type_t type_ );
int8_t const * llsd_get_bin_enc_type_string( llsd_bin_enc_t enc );
#define llsd_is_array(x) (llsd_get_type(x) == LLSD_ARRAY)
#define llsd_is_map(x) (llsd_get_type(x) == LLSD_MAP)

/* get the size of the cotainer types */
int llsd_get_size( llsd_t * llsd );
#define llsd_is_empty(x) (llsd_get_size(x) == 0)

/* append to containers */
void llsd_array_append( llsd_t * arr, llsd_t * data );
void llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data );
llsd_t * llsd_map_find_llsd( llsd_t * map, llsd_t * key );
llsd_t * llsd_map_find( llsd_t * map, int8_t const * const key );

/* iterator type */
typedef int32_t llsd_itr_t;

/* iterator interface */
llsd_itr_t llsd_itr_begin( llsd_t * llsd );
llsd_itr_t llsd_itr_end( llsd_t * llsd );
llsd_itr_t llsd_itr_rbegin( llsd_t * llsd );
#define llsd_itr_rend(x) llsd_itr_end(x)
llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr );
llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr );

/* get the next value in the array/map */
int llsd_itr_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key );

/* compare two llsd items */
int llsd_equal( llsd_t * l, llsd_t * r );

/* type conversions */
llsd_bool_t llsd_as_bool( llsd_t * llsd );
llsd_int_t llsd_as_int( llsd_t * llsd );
llsd_real_t llsd_as_real( llsd_t * llsd );
llsd_uuid_t llsd_as_uuid( llsd_t * llsd );
llsd_string_t llsd_as_string( llsd_t * llsd );
llsd_date_t llsd_as_date( llsd_t * llsd );
llsd_uri_t llsd_as_uri( llsd_t * llsd );
llsd_binary_t llsd_as_binary( llsd_t * llsd );
llsd_array_t llsd_as_array( llsd_t * llsd );
llsd_map_t llsd_as_map( llsd_t * llsd );

/* xml/binary conversions */
int llsd_stringify_uuid( llsd_t * llsd );
int llsd_destringify_uuid( llsd_t * llsd );
int llsd_stringify_date( llsd_t * llsd );
int llsd_destringify_date( llsd_t * llsd );
int llsd_escape_string( llsd_t * llsd );
int llsd_unescape_string( llsd_t * llsd );
int llsd_esacpe_uri( llsd_t * llsd );
int llsd_unescape_uri( llsd_t * llsd );
int llsd_encode_binary( llsd_t * llsd, llsd_bin_enc_t encoding );
int llsd_decode_binary( llsd_t * llsd );

/* helper functions for map */
#define FNV_PRIME (0x01000193)
uint_t fnv_key_hash(void const * const key);
int key_eq(void const * const l, void const * const r);

/* serialize/deserialize interface */
llsd_t * llsd_parse( FILE * fin );
size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout, int pretty );

/* zero copy serialization interface */
size_t llsd_format_zero_copy( llsd_t * llsd, llsd_serializer_t fmt, struct iovec ** v, int pretty );

#endif/*LLSD_UTIL_H*/

