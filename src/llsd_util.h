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

#include <stdint.h>

#include "array.h"
#include "hashtable.h"

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
	LLSD_TYPE_COUNT = LLSD_TYPE_LAST - LLSD_TYPE_FIRST

} llsd_type_t;

typedef enum llsd_serializer_s
{
	LLSD_ENC_XML,
	LLSD_ENC_NOTATION,
	LLSD_ENC_JSON,
	LLSD_ENC_BINARY,

	LLSD_ENC_LAST,
	LLSD_ENC_FIRST = LLSD_ENC_XML,
	LLSD_ENC_COUNT = LLSD_ENC_LAST - LLSD_ENC_FIRST

} llsd_serializer_t;

typedef enum llsd_bin_enc_s
{
	LLSD_NONE,
	LLSD_BASE16,
	LLSD_BASE64,

	LLSD_BIN_ENC_LAST,
	LLSD_BIN_ENC_FIRST = LLSD_NONE,
	LLSD_BIN_END_COUNT = LLSD_BIN_ENC_LAST - LLSD_BIN_ENC_FIRST

} llsd_bin_enc_t;

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

/* LLSD types */
#define UUID_LEN (16)
#define UUID_STR_LEN (36)
struct llsd_uuid_s
{
	int					dyn_bits: 1;
	int					dyn_str:  1;
	uint32_t			len;
	uint8_t *			str;
	uint8_t *			bits;
};

struct llsd_binary_s
{
	int					dyn_data: 1;
	int					dyn_enc: 1;
	llsd_bin_enc_t		encoding;
	uint32_t			data_size;
	uint8_t	*			data;
	uint32_t			enc_size;
	uint8_t *			enc;
};

/* NOTE: the key_esc flag signifies that the map hashing function
 * should use the escaped version of the string when hashing. this
 * allows us to avoid the copy/unescape process for string keys
 * during loading. */
struct llsd_string_s
{
	int					dyn_str: 1;
	int					dyn_esc: 1;
	int					key_esc: 1;
	uint32_t			str_len;
	uint8_t *			str;
	uint32_t			esc_len;
	uint8_t *			esc;
};

struct llsd_uri_s
{
	int					dyn_uri: 1;
	int					dyn_esc: 1;
	uint32_t			uri_len;
	uint8_t *			uri;
	uint32_t			esc_len;
	uint8_t *			esc;
};

#define DATE_STR_LEN (24)
/* YYYY-MM-DDTHH:MM:SS.FFFZ */
struct llsd_date_s
{
	int					use_dval: 1;
	int					dyn_str: 1;
	uint32_t			len;
	double				dval;
	uint8_t*			str;
};

#define DEFAULT_ARRAY_CAPACITY (8)
struct llsd_array_s
{
	array_t		array;
};

#define DEFAULT_MAP_CAPACITY (5)
struct llsd_map_s
{
	ht_t		ht;
};

/* the llsd types */
typedef int						llsd_bool_t;
typedef int32_t					llsd_int_t;
typedef double					llsd_real_t;
typedef struct llsd_uuid_s		llsd_uuid_t;
typedef struct llsd_string_s	llsd_string_t;
typedef struct llsd_date_s		llsd_date_t;
typedef struct llsd_uri_s		llsd_uri_t;
typedef struct llsd_binary_s	llsd_binary_t;
typedef struct llsd_array_s		llsd_array_t;
typedef struct llsd_map_s		llsd_map_t;


typedef struct llsd_s
{
	llsd_type_t			type_;
	union
	{
		llsd_bool_t		bool_;
		llsd_int_t		int_;
		llsd_real_t		real_;
		llsd_uuid_t		uuid_;
		llsd_string_t	string_;
		llsd_date_t		date_;
		llsd_uri_t		uri_;
		llsd_binary_t	binary_;
		llsd_array_t	array_;
		llsd_map_t		map_;

	}					value;

} llsd_t;

/* iterator type */
typedef int32_t llsd_itr_t;

/* new/delete llsd objects */
llsd_t * llsd_new( llsd_type_t type_, ... );
void llsd_delete( void * p );

/* utility macros */
#define llsd_new_empty_array() llsd_new( LLSD_ARRAY )
#define llsd_new_empty_map() llsd_new( LLSD_MAP )

/* get the type of the particular object */
llsd_type_t llsd_get_type( llsd_t * llsd );
int8_t const * llsd_get_type_string( llsd_type_t type_ );

/* get the size of the cotainer types */
int llsd_get_size( llsd_t * llsd );
#define llsd_is_empty(x) (llsd_get_size(x) == 0)

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

/* string/binary conversions */
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

/* append to containers */
void llsd_array_append( llsd_t * arr, llsd_t * data );
void llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data );
llsd_t * llsd_map_find( llsd_t * map, llsd_t * key );

/* iterator interface */
llsd_itr_t llsd_itr_begin( llsd_t * llsd );
llsd_itr_t llsd_itr_end( llsd_t * llsd );
llsd_itr_t llsd_itr_rbegin( llsd_t * llsd );
#define llsd_itr_rend(x) llsd_itr_end(x)
llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr );
llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr );

/* get the next value in the array/map */
int llsd_itr_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key );

/* serialize/deserialize interface */
llsd_t * llsd_parse( FILE * fin );
size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout, int pretty );

#endif/*LLSD_H*/

