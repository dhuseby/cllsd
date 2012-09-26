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

#include <stdint.h>
#include <stdarg.h>

#include <cutil/macros.h>
#include <cutil/pair.h>

#include "llsd.h"

struct llsd_binary_s
{
	uint8_t	*			data;
	uint32_t			length;
};

/* the llsd types */
typedef int						llsd_bool_t;
typedef int32_t					llsd_int_t;
typedef double					llsd_real_t;
typedef uint8_t					llsd_uuid_t[UUID_LEN];
typedef uint8_t *				llsd_string_t;
typedef double					llsd_date_t;
typedef uint8_t *				llsd_uri_t;
typedef struct llsd_binary_s	llsd_binary_t;
typedef list_t					llsd_array_t;
typedef ht_t					llsd_map_t;

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

int8_t const * const llsd_xml_bin_enc_type_strings[LLSD_BIN_ENC_COUNT] =
{
	T("NONE"),
	T("base16"),
	T("base64"),
	T("base85")
};

int8_t const * const llsd_notation_bin_enc_type_strings[LLSD_BIN_ENC_COUNT] =
{
	T("NONE"),
	T("b16"),
	T("b64"),
	T("b85")
};

#define FNV_PRIME (0x01000193)
static uint32_t llsd_pair_hash( void const * const data )
{
	int i;
	pair_t * pair = (pair_t*)data;
	llsd_t * key = pair_first( pair );
	uint_t hash = 0x811c9dc5;
	uint8_t const * p = (uint8_t const *)key->string_;
	uint32_t const len = strnlen( p, 64 );
	CHECK_RET( (key->type_ == LLSD_STRING), 0 );
	for( i = 0; i < len; i++ )
	{
		hash *= FNV_PRIME;
		hash ^= p[i];
	}
	return hash;
}

static int llsd_pair_eq(void const * const l, void const * const r)
{
	pair_t * left = (pair_t*)l;
	pair_t * right = (pair_t*)r;
	llsd_t * lval = pair_second( left );
	llsd_t * rval = pair_second( right );
	return llsd_equal( lval, rval );
}

static void llsd_pair_delete(void * value)
{
	pair_t * p = (pair_t*)value;
	llsd_delete( pair_first( p ) );
	llsd_delete( pair_second( p ) );
	pair_delete( p );
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
			llsd->int_ = va_arg(args, int );
			va_end(args);
			break;

		case LLSD_REAL:
			va_start( args, type_ );
			llsd->real_ = va_arg(args, double );
			va_end(args);
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			
			if ( p != NULL )
			{
				MEMCPY( llsd->uuid_, p, UUID_LEN );
			}
			else
			{
				MEMSET( llsd->uuid_, 0, UUID_LEN );
			}
			va_end( args );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			llsd->string_ = va_arg( args, uint8_t* );
			va_end( args );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->date_ = va_arg( args, double );
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			llsd->uri_ = va_arg( args, uint8_t* );
			va_end( args );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			llsd->binary_.data = va_arg( args, uint8_t* );
			llsd->binary_.length = va_arg( args, uint32_t );
			va_end( args );
			break;

		case LLSD_ARRAY:
			list_initialize( &(llsd->array_), 0, &llsd_delete );
			break;

		case LLSD_MAP:
			ht_initialize( &(llsd->map_), 0, &llsd_pair_hash, &llsd_pair_eq, &llsd_pair_delete );
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
			llsd_initialize( llsd, type_, a3 );
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
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );	/* poniter to str */
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );	/* pointer to buffer */
			a1 = va_arg( args, int );		/* size */
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1 );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			llsd_initialize( llsd, type_ );
			break;
	}

	return llsd;
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
		case LLSD_DATE:
			return;

		case LLSD_UUID:
			FREE( llsd->uuid_ );
			break;

		case LLSD_STRING:
			FREE( llsd->string_ );
			break;

		case LLSD_URI:
			FREE( llsd->uri_ );
			break;

		case LLSD_BINARY:
			FREE( llsd->binary_.data );
			break;

		case LLSD_ARRAY:
			array_deinitialize( &llsd->array_ );
			break;

		case LLSD_MAP:
			ht_deinitialize( &llsd->map_ );
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

int8_t const * llsd_get_bin_enc_type_string( llsd_bin_enc_t enc, llsd_serializer_t fmt )
{
	CHECK_RET( ((enc >= LLSD_BIN_ENC_FIRST) && (enc < LLSD_BIN_ENC_LAST)), NULL );
	switch( fmt )
	{
		case LLSD_ENC_XML:
			return llsd_xml_bin_enc_type_strings[ enc ];
		case LLSD_ENC_BINARY:
			return NULL;
		case LLSD_ENC_NOTATION:
			return llsd_notation_bin_enc_type_strings[ enc ];
	}
	return NULL;
}

int llsd_array_append( llsd_t * arr, llsd_t * value )
{
	CHECK_PTR_RET( arr, FALSE );
	CHECK_PTR_RET( value, FALSE );
	CHECK_RET( llsd_get_type( arr ) == LLSD_ARRAY, FALSE );
	array_push_tail( &(arr->array_), (void*)value );
}

int llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * value )
{
	pair_t * p = NULL;
	CHECK_PTR_RET( map, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_PTR_RET( value, FALSE );
	CHECK_RET( llsd_get_type( map ) == LLSD_MAP, FALSE );
	CHECK_RET( llsd_get_type( key ) == LLSD_STRING, FALSE );
	p = pair_new( key, value );
	CHECK_PTR_RET( p, FALSE );
	if ( !ht_insert( &(map->map_), (void*)p ) )
	{
		pair_delete( p );
		return FALSE;
	}
	return TRUE;
}

int llsd_equal( llsd_t * l, llsd_t * r )
{
	return TRUE;
}

