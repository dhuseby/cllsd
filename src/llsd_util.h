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

/* xml/binary/notation conversions */
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

#if defined(MISSING_STRNLEN)
uint32_t strnlen( uint8_t const * const s, uint32_t const n );
#endif

#if defined(MISSING_64BIT_ENDIAN)
uint64_t be64toh( uint64_t const be );
uint64_t htobe64( uint64_t const h );
#endif

#endif/*LLSD_UTIL_H*/

