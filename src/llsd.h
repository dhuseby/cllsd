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

#include "macros.h"
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
	LLSD_TYPE_COUNT = LLSD_TYPE_LAST - LLSD_TYPE_FIRST,
	LLSD_TYPE_INVALID

} llsd_type_t;

extern int8_t const * const llsd_type_strings[LLSD_TYPE_COUNT];

#define TYPE_TO_STRING( t ) (((t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST)) ? \
							 llsd_type_strings[t] : T("INVALID") )

typedef enum llsd_serializer_s
{
	LLSD_ENC_XML,
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

/* LLSD magic values */
#define UUID_LEN (16)
#define UUID_STR_LEN (36)
#define DATE_STR_LEN (24)
#define DEFAULT_ARRAY_CAPACITY (8)
#define DEFAULT_MAP_CAPACITY (5)

struct llsd_int_s
{
	int32_t				v;
	uint32_t			be;
};

struct llsd_real_s
{
	double				v;
	uint64_t			be;
}

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

/* YYYY-MM-DDTHH:MM:SS.FFFZ */
struct llsd_date_s
{
	int					use_dval: 1;
	int					dyn_str: 1;
	uint32_t			len;
	double				dval;
	uint64_t			be;
	uint8_t*			str;
};

struct llsd_array_s
{
	array_t		array;
	uint32_t	be;
};

struct llsd_map_s
{
	ht_t		ht;
	uint32_t	be;
};

/* the llsd types */
typedef int						llsd_bool_t;
typedef struct llsd_int_s		llsd_int_t;
typedef struct llsd_real_s		llsd_real_t;
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

	};

} llsd_t;

#endif /*LLSD_H*/

