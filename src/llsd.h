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
#define XML_SIG_LEN (38)

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

extern int8_t const * const llsd_type_strings[LLSD_TYPE_COUNT];

#define TYPE_TO_STRING( t ) (((t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST)) ? \
							 llsd_type_strings[t] : T("INVALID") )

typedef enum llsd_serializer_s
{
	LLSD_ENC_XML,
	LLSD_ENC_BINARY,
	LLSD_ENC_NOTATION,

	LLSD_ENC_LAST,
	LLSD_ENC_FIRST = LLSD_ENC_XML,
	LLSD_ENC_COUNT = LLSD_ENC_LAST - LLSD_ENC_FIRST

} llsd_serializer_t;

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
#define llsd_new_empty_array() llsd_new( LLSD_ARRAY )
#define llsd_new_empty_map() llsd_new( LLSD_MAP )
#define llsd_new_boolean( val ) llsd_new ( LLSD_BOOLEAN, val )
#define llsd_new_integer( val ) llsd_new ( LLSD_INTEGER, val )
#define llsd_new_real( val ) llsd_new ( LLSD_REAL, val )
#define llsd_new_uuid( bits ) llsd_new ( LLSD_UUID, bits )
#define llsd_new_string( s ) llsd_new( LLSD_STRING, s )
#define llsd_new_uri( s ) llsd_new( LLSD_URI, s )
#define llsd_new_binary( p, len ) llsd_new( LLSD_BINARY, p, len )
#define llsd_new_date( d ) llsd_new( LLSD_DATE, d )

/* get the type of the particular object */
llsd_type_t llsd_get_type( llsd_t * llsd );
int8_t const * llsd_get_type_string( llsd_type_t type_ );
int8_t const * llsd_get_bin_enc_type_string( llsd_bin_enc_t enc, llsd_serializer_t fmt );
#define llsd_is_array(x) (llsd_get_type(x) == LLSD_ARRAY)
#define llsd_is_map(x) (llsd_get_type(x) == LLSD_MAP)

/* compare two llsd items */
int llsd_equal( llsd_t * l, llsd_t * r );

#endif /*LLSD_H*/

