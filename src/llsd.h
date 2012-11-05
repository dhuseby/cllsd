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

#ifndef LLSD_H
#define LLSD_H

#include <cutil/macros.h>
#include <cutil/list.h>
#include <cutil/hashtable.h>

/* different sig lengths */

typedef enum llsd_type_e
{
	LLSD_UNDEF,
	LLSD_BOOLEAN,
	LLSD_INTEGER,
	LLSD_REAL,
	LLSD_UUID,
	LLSD_STRING,
	LLSD_DATE,
	LLSD_URI,
	LLSD_BINARY,
	LLSD_ARRAY,
	LLSD_MAP,

	LLSD_TYPE_LAST,
	LLSD_TYPE_FIRST = LLSD_UNDEF,
	LLSD_TYPE_COUNT = LLSD_TYPE_LAST - LLSD_TYPE_FIRST,
	LLSD_TYPE_INVALID,

	LLSD_KEY, /* type of LLSD key tag in XML */
	LLSD_LLSD /* type of LLSD tag in XML */

} llsd_type_t;
#define IS_VALID_LLSD_TYPE( x ) ((x >= LLSD_TYPE_FIRST) && (x < LLSD_TYPE_LAST))

extern int8_t const * const llsd_type_strings[LLSD_TYPE_COUNT];

#define TYPE_TO_STRING( t ) (((t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST)) ? \
							 llsd_type_strings[t] : T("INVALID") )

typedef enum llsd_serializer_s
{
	LLSD_ENC_XML,
	LLSD_ENC_BINARY,
	LLSD_ENC_NOTATION,
	LLSD_ENC_JSON,

	LLSD_ENC_LAST,
	LLSD_ENC_FIRST = LLSD_ENC_XML,
	LLSD_ENC_COUNT = LLSD_ENC_LAST - LLSD_ENC_FIRST

} llsd_serializer_t;
#define IS_VALID_SERIALIZER( x ) ((x >= LLSD_ENC_FIRST) && (x < LLSD_ENC_LAST))

typedef enum llsd_bin_enc_s
{
	LLSD_NONE,
	LLSD_BASE16,
	LLSD_BASE64,
	LLSD_BASE85,

	LLSD_BIN_ENC_LAST,
	LLSD_BIN_ENC_FIRST = LLSD_NONE,
	LLSD_BIN_ENC_COUNT = LLSD_BIN_ENC_LAST - LLSD_BIN_ENC_FIRST,

	LLSD_RAW	/* used for some special cases */

} llsd_bin_enc_t;
#define IS_VALID_BINARY_ENCODING( x ) ((x >= LLSD_BIN_ENC_FIRST) && (x < LLSD_BIN_ENC_LAST))

extern int8_t const * const llsd_xml_bin_enc_type_strings[LLSD_BIN_ENC_COUNT];
extern int8_t const * const llsd_notation_bin_enc_type_strings[LLSD_BIN_ENC_COUNT];

/* LLSD magic values */
#define UUID_LEN (16)
#define UUID_STR_LEN (36)
#define DATE_STR_LEN (24)

typedef struct llsd_s llsd_t;

/* new/delete llsd objects */
llsd_t * llsd_new( llsd_type_t type_, ... );
void llsd_delete( void * p );

/* utility macros */
#define llsd_new_undef() llsd_new( LLSD_UNDEF )
#define llsd_new_boolean( val ) llsd_new ( LLSD_BOOLEAN, val )
#define llsd_new_integer( val ) llsd_new ( LLSD_INTEGER, val )
#define llsd_new_real( val ) llsd_new ( LLSD_REAL, val )
#define llsd_new_uuid( bits ) llsd_new ( LLSD_UUID, bits )
#define llsd_new_string( s, o ) llsd_new( LLSD_STRING, s, o )
#define llsd_new_uri( s, o ) llsd_new( LLSD_URI, s, o )
#define llsd_new_binary( p, len, o ) llsd_new( LLSD_BINARY, p, len, o )
#define llsd_new_date( d ) llsd_new( LLSD_DATE, d )
#define llsd_new_array( s ) llsd_new( LLSD_ARRAY, s )
#define llsd_new_map( s ) llsd_new( LLSD_MAP, s )

/* get the type of the particular object */
llsd_type_t llsd_get_type( llsd_t * llsd );
int8_t const * llsd_get_type_string( llsd_type_t type_ );
int8_t const * llsd_get_bin_enc_type_string( llsd_bin_enc_t enc, llsd_serializer_t fmt );
#define llsd_is_array(x) (llsd_get_type(x) == LLSD_ARRAY)
#define llsd_is_map(x) (llsd_get_type(x) == LLSD_MAP)

/* container interface */
typedef struct llsd_itr_s {
	list_itr_t li;
	ht_itr_t hi;
} llsd_itr_t;

#define LLSD_ITR_EQ( i, j ) ((i.li == j.li) && ITR_EQ(i.hi,j.hi))

int llsd_array_append( llsd_t * arr, llsd_t * data );
int llsd_array_unappend( llsd_t * arr );
int llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data );
int llsd_map_remove( llsd_t * map, llsd_t * key );

llsd_itr_t llsd_itr_begin( llsd_t * llsd );
llsd_itr_t llsd_itr_end( llsd_t * llsd );
llsd_itr_t llsd_itr_rbegin( llsd_t * llsd );
#define llsd_itr_rend(x) llsd_itr_end(x)
llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr );
llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr );

int llsd_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key );
llsd_t * llsd_map_find_llsd( llsd_t * map, llsd_t * key );
llsd_t * llsd_map_find( llsd_t * map, uint8_t const * const key );

/* conversion interface */
int llsd_as_boolean( llsd_t * llsd, int * v );
int llsd_as_integer( llsd_t * llsd, int32_t * v );
int llsd_as_double( llsd_t * llsd, double * v );
int llsd_as_uuid( llsd_t * llsd, uint8_t uuid[UUID_LEN] );
int llsd_as_string( llsd_t * llsd, uint8_t ** v );
int llsd_as_binary( llsd_t * llsd, uint8_t ** v, uint32_t * len );

/* compare two llsd items */
int llsd_equal( llsd_t * l, llsd_t * r );

/* get the count of the cotainer types */
int llsd_get_count( llsd_t * llsd );
#define llsd_is_empty(x) (llsd_get_count(x) == 0)

/* callback functions used for parsing/serializing */
typedef struct llsd_ops_s
{
	int (*undef_fn)( void * const user_data );
	int (*boolean_fn)( int const value, void * const user_data );
	int (*integer_fn)( int32_t const value, void * const user_data );
	int (*real_fn)( double const value, void * const user_data );
	int (*uuid_fn)( uint8_t const value[UUID_LEN], void * const user_data );
	int (*string_fn)( uint8_t const * str, int const own_it, void * const user_data );
	int (*date_fn)( double const value, void * const user_data );
	int (*uri_fn)( uint8_t const * uri, int const own_it, void * const user_data );
	int (*binary_fn)( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data );
	int (*array_begin_fn)( uint32_t const size, void * const user_data );
	int (*array_value_begin_fn)(void * const user_data);
	int (*array_value_end_fn)( void * const user_data );
	int (*array_end_fn)( uint32_t const size, void * const user_data );
	int (*map_begin_fn)( uint32_t const size, void * const user_data );
	int (*map_key_begin_fn)( void * const user_data );
	int (*map_key_end_fn)( void * const user_data );
	int (*map_value_begin_fn)( void * const user_data );
	int (*map_value_end_fn)( void * const user_data );
	int (*map_end_fn)( uint32_t const size, void * const user_data );

} llsd_ops_t;

/* states of the parser/serializer */
typedef enum state_e
{
	TOP_LEVEL			= 0x0001,
	ARRAY_BEGIN			= 0x0002,
	ARRAY_VALUE_BEGIN	= 0x0004,
	ARRAY_VALUE			= 0x0008,
	ARRAY_VALUE_END		= 0x0010,
	ARRAY_END			= 0x0020,
	MAP_BEGIN			= 0x0040,
	MAP_KEY_BEGIN		= 0x0080,
	MAP_KEY				= 0x0100,
	MAP_KEY_END			= 0x0200,
	MAP_VALUE_BEGIN		= 0x0400,
	MAP_VALUE			= 0x0800,
	MAP_VALUE_END		= 0x1000,
	MAP_END				= 0x2000
} state_t;

#endif /*LLSD_H*/

