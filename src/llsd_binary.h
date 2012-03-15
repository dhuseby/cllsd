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

#ifndef LLSD_BINARY_H
#define LLSD_BINARY_H

#include <stdint.h>
#include "debug.h"
#include "macros.h"
#include "llsd_const.h"
#include "array.h"
#include "hashtable.h"

typedef struct llsd_bin_type_s
{
	uint8_t *		t;
	size_t			tlen;
} llsd_bin_type_t;

typedef struct llsd_bin_int_s
{
	uint32_t *		v;
	size_t			vlen;
} llsd_bin_int_t;

typedef struct llsd_bin_double_s
{
	double *		v;
	size_t			vlen;
} llsd_bin_double_t;

typedef struct llsd_bin_bytes_s
{
	uint8_t *		v;
	size_t			vlen;
} llsd_bin_bytes_t;

struct llsd_bin_array_s
{
	array_t *		v;
	size_t			vlen;
};

struct llsd_bin_map_s
{
	ht_t *			v;
	size_t			vlen;
};

/* fixed lengths for some types */
#define UUID_LEN (16)

/* binary LLSD types */
typedef llsd_bin_type_t			llsd_bin_undef_t;
typedef llsd_bin_type_t			llsd_bin_bool_t;
typedef llsd_bin_int_t			llsd_bin_integer_t;
typedef llsd_bin_double_t		llsd_bin_real_t;
typedef llsd_bin_bytes_t		llsd_bin_uuid_t;
typedef llsd_bin_bytes_t		llsd_bin_string_t;
typedef llsd_bin_double_t		llsd_bin_date_t;
typedef llsd_bin_bytes_t		llsd_bin_uri_t;
typedef llsd_bin_bytes_t		llsd_bin_binary_t;
typedef struct llsd_bin_array_s llsd_bin_array_t;
typedef struct llsd_bin_map_s	llsd_bin_map_t;

typedef struct llsd_bin_s
{
	llsd_bin_type_t			t;
	union
	{
		llsd_bin_int_t		i;
		llsd_bin_double_t	d;
		llsd_bin_bytes_t	b;
		llsd_bin_array_t	a;
		llsd_bin_map_t		m;
	};
} llsd_bin_t;

extern const llsd_type_t llsd_binary_types[UINT8_MAX + 1];
extern const uint8_t llsd_binary_bytes[LLSD_TYPE_COUNT];

#define BYTE_TO_TYPE( c ) (llsd_binary_types[c])
#define TYPE_TO_BYTE( t ) (((t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST)) ? \
						   llsd_binary_bytes[t] : 0 )

llsd_bin_t * llsd_parse_binary( uint8_t * buf, size_t len );

#endif /* LLSD_BINARY_H */

