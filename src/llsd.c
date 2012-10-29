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
#include <sys/uio.h>
#include <math.h>
#include <time.h>

#define DEBUG_ON
#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/pair.h>

#include "llsd.h"

/* the llsd types */
typedef int				llsd_bool_t;
typedef int32_t			llsd_int_t;
typedef double			llsd_real_t;
typedef uint8_t			llsd_uuid_t[UUID_LEN];
typedef uint8_t *		llsd_string_t;
typedef double			llsd_date_t;
typedef uint8_t *		llsd_uri_t;
typedef struct iovec	llsd_binary_t;
typedef list_t			llsd_array_t;
typedef ht_t			llsd_map_t;

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
	uint32_t const len = ( p ? strnlen( p, 64 ) : 0 );
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
	llsd_t * lval = pair_first( left );
	llsd_t * rval = pair_first( right );
	return llsd_equal( lval, rval );
}

static void llsd_pair_delete(void * value)
{
	pair_t * p = (pair_t*)value;
	llsd_delete( pair_first( p ) );
	llsd_delete( pair_second( p ) );
	pair_delete( p );
}

static int llsd_initialize( llsd_t * llsd, llsd_type_t type_, ... )
{
	va_list args;
	uint8_t * p;
	uint32_t len;
	int own_it;

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
			p = va_arg( args, uint8_t* );
			own_it = va_arg( args, int );
			va_end( args );

			if ( !own_it && p )
			{
				llsd->string_ = strdup( p );
				CHECK_PTR_RET( llsd->string_, FALSE );
			}
			else
				llsd->string_ = p;

			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->date_ = va_arg( args, double );
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			own_it = va_arg( args, int );
			va_end( args );

			if ( !own_it && p )
			{
				llsd->uri_ = strdup( p );
				CHECK_PTR_RET( llsd->uri_, FALSE );
			}
			else
				llsd->string_ = p;
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			p = va_arg( args, uint8_t* );
			llsd->binary_.iov_len = va_arg( args, uint32_t );
			own_it = va_arg( args, int );
			va_end( args );

			if ( !own_it && p )
			{
				llsd->binary_.iov_base = CALLOC( llsd->binary_.iov_len, sizeof(uint8_t) );
				CHECK_PTR_RET( llsd->binary_.iov_base, FALSE );
				MEMCPY( llsd->binary_.iov_base, p, llsd->binary_.iov_len );
			}
			else
				llsd->binary_.iov_base = p;
			break;

		case LLSD_ARRAY:
			va_start( args, type_ );
			len = va_arg( args, uint32_t );
			va_end( args );
			CHECK_RET( list_initialize( &(llsd->array_), len, &llsd_delete ), FALSE );
			break;

		case LLSD_MAP:
			va_start( args, type_ );
			len = va_arg( args, uint32_t );
			va_end( args );
			CHECK_RET( ht_initialize( &(llsd->map_), len, &llsd_pair_hash, &llsd_pair_eq, &llsd_pair_delete ), FALSE );
			break;
	}
	return TRUE;
}

llsd_t * llsd_new( llsd_type_t type_, ... )
{
	va_list args;
	llsd_t * llsd;
	int a1;
	uint8_t * a2;
	double a3;
	uint32_t a4;
	
	/* allocate the llsd object */
	llsd = (llsd_t*)CALLOC(1, sizeof(llsd_t));
	CHECK_PTR_RET_MSG( llsd, NULL, "failed to heap allocate llsd object\n" );

	switch( type_ )
	{
		case LLSD_UNDEF:
			CHECK_GOTO( llsd_initialize( llsd, type_ ), fail_llsd_new );
			break;

		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a1 ), fail_llsd_new );
			break;

		case LLSD_REAL:
		case LLSD_DATE:
			va_start( args, type_ );
			a3 = va_arg( args, double );
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a3 ), fail_llsd_new );
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a2 ), fail_llsd_new );
			break;

		case LLSD_STRING:
		case LLSD_URI:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			a1 = va_arg( args, int );
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a2, a1 ), fail_llsd_new );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );	/* pointer to buffer */
			a4 = va_arg( args, uint32_t );	/* size */
			a1 = va_arg( args, int );
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a2, a4, a1 ), fail_llsd_new );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			va_start( args, type_ );
			a4 = va_arg( args, uint32_t ); /* initial capacity */
			va_end( args );
			CHECK_GOTO( llsd_initialize( llsd, type_, a4 ), fail_llsd_new );
			break;
	}

	return llsd;

fail_llsd_new:
	llsd_delete( llsd );
	return NULL;
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
		case LLSD_UUID:
			return;

		case LLSD_STRING:
			FREE( llsd->string_ );
			break;

		case LLSD_URI:
			FREE( llsd->uri_ );
			break;

		case LLSD_BINARY:
			FREE( llsd->binary_.iov_base );
			break;

		case LLSD_ARRAY:
			list_deinitialize( &llsd->array_ );
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
	list_push_tail( &(arr->array_), (void*)value );
}

int llsd_array_unappend( llsd_t * arr )
{
	CHECK_PTR_RET( arr, FALSE );
	CHECK_RET( llsd_get_type( arr ) == LLSD_ARRAY, FALSE );
	list_pop_tail( &(arr->array_) );
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

int llsd_map_remove( llsd_t * map, llsd_t * key )
{
	int ret = FALSE;
	pair_t * p = NULL;
	ht_itr_t itr;
	CHECK_PTR_RET( map, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_RET( llsd_get_type(map) == LLSD_MAP, FALSE );
	CHECK_RET( llsd_get_type(key) == LLSD_STRING, FALSE );

	p = pair_new( key, NULL );
	CHECK_PTR_RET( p, FALSE );
	itr = ht_find( &map->map_, (void*)p );
	pair_delete( p );
	CHECK_RET( !ITR_EQ( itr, ht_itr_end( &map->map_ ) ), FALSE );

	/* get the pair object */
	p = ht_get( &map->map_, itr );
	CHECK_PTR_RET( p, FALSE );

	/* remove the pair from the map */
	ret = ht_remove( &map->map_, itr );

	/* now delete the two parts of the pair and the pair itself */
	llsd_delete( pair_first( p ) );
	llsd_delete( pair_second( p ) );
	pair_delete( p );

	return ret;
}

llsd_itr_t llsd_itr_begin( llsd_t * llsd )
{
	llsd_itr_t itr;
	CHECK_PTR_RET( llsd, itr );
	itr.li = list_itr_end( &llsd->array_ );
	itr.hi = ht_itr_end( &llsd->map_ );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			itr.li = list_itr_begin( &llsd->array_ );
			break;
		case LLSD_MAP:
			itr.hi = ht_itr_begin( &llsd->map_ );
			break;
	}
	return itr;
}

llsd_itr_t llsd_itr_end( llsd_t * llsd )
{
	llsd_itr_t itr = 
		(llsd_itr_t){ 
			.li = -1, 
			.hi = (ht_itr_t){ 
				.idx = -1, 
				.itr = -1 
			}
		};
	return itr;
}

llsd_itr_t llsd_itr_rbegin( llsd_t * llsd )
{
	llsd_itr_t itr;
	CHECK_PTR_RET( llsd, itr );
	itr.li = list_itr_rend( &llsd->array_ );
	itr.hi = ht_itr_rend( &llsd->map_ );
	
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			itr.li = list_itr_rbegin( &llsd->array_ );
			break;
		case LLSD_MAP:
			itr.hi = ht_itr_rbegin( &llsd->map_ );
			break;
	}
	return itr;
}

llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr )
{
	llsd_itr_t ret = itr;
	CHECK_PTR_RET( llsd, ret );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			ret.li = list_itr_next( &llsd->array_, ret.li );
			break;
		case LLSD_MAP:
			ret.hi = ht_itr_next( &llsd->map_, ret.hi );
			break;
	}
	return ret;
}

llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr )
{
	llsd_itr_t ret = itr;
	CHECK_PTR_RET( llsd, ret );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			ret.li = list_itr_rnext( &llsd->array_, ret.li );
			break;
		case LLSD_MAP:
			ret.hi = ht_itr_rnext( &llsd->map_, ret.hi );
			break;
	}
	return ret;
}

int llsd_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key )
{
	pair_t * p;
	CHECK_PTR_RET( value, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( !LLSD_ITR_EQ( itr, llsd_itr_end( llsd ) ), FALSE );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			(*value) = (llsd_t*)list_get( &llsd->array_, itr.li );
			(*key) = NULL;
			return TRUE;
		case LLSD_MAP:
			p = (pair_t*)ht_get( &llsd->map_, itr.hi );
			(*key) = pair_first( p );
			(*value) = pair_second( p );
			return TRUE;
	}

	/* non-container iterator just references the llsd */
	(*value) = llsd;
	(*key) = NULL;
	return TRUE;
}

llsd_t * llsd_map_find_llsd( llsd_t * map, llsd_t * key )
{
	pair_t * p = NULL;
	ht_itr_t itr;
	CHECK_PTR_RET( map, NULL );
	CHECK_PTR_RET( key, NULL );
	CHECK_RET( llsd_get_type(map) == LLSD_MAP, NULL );
	CHECK_RET( llsd_get_type(key) == LLSD_STRING, NULL );

	p = pair_new( key, NULL );
	CHECK_PTR_RET( p, NULL );
	itr = ht_find( &map->map_, (void*)p );
	pair_delete( p );
	CHECK_RET( !ITR_EQ( itr, ht_itr_end( &map->map_ ) ), NULL );

	p = ht_get( &map->map_, itr );
	CHECK_PTR_RET( p, NULL );

	return (llsd_t*)pair_second( p );
}

llsd_t * llsd_map_find( llsd_t * map, uint8_t const * const key )
{
	llsd_t t;
	ht_itr_t itr;
	CHECK_PTR_RET( map, NULL );
	CHECK_PTR_RET( key, NULL );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );

	/* wrap the key in an llsd struct */
	memset( &t, 0, sizeof( llsd_t ) );
	t.type_ = LLSD_STRING;
	t.string_ = (uint8_t*)key;
	return llsd_map_find_llsd( map, &t );
}

int llsd_as_integer( llsd_t * llsd, int32_t * v )
{
	int i;
	uint32_t be = 0;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( v, FALSE );
	(*v) = 0;

	switch( llsd->type_ )
	{
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			DEBUG( "illegal conversion of %s to integer\n", llsd_get_type_string( llsd->type_ ) );
			return FALSE;
		case LLSD_UNDEF:
			break;
		case LLSD_BOOLEAN:
			(*v) = llsd->bool_;
			break;
		case LLSD_INTEGER:
			(*v) = llsd->int_;
			break;
		case LLSD_REAL:
			CHECK_RET_MSG( !isnan( llsd->real_ ), FALSE, "converting NaN to integer\n" );
			CHECK_RET_MSG( !isinf( llsd->real_ ), FALSE, "convering infinite to integer\n" );
			if ( llsd->real_ >= UINT32_MAX )
				DEBUG( "truncating real to integer\n" );
			(*v) = lrint( llsd->real_ );
			break;
		case LLSD_DATE:
			CHECK_RET_MSG( !isnan( llsd->date_ ), FALSE, "converting NaN date to integer\n" );
			CHECK_RET_MSG( !isinf( llsd->date_ ), FALSE, "convering infinite date to integer\n" );
			if ( llsd->date_ >= UINT32_MAX )
				DEBUG( "truncating date to integer\n" );
			(*v) = lrint( llsd->date_ );
			break;
		case LLSD_STRING:
			(*v) = atoi( llsd->string_ );
			break;
		case LLSD_BINARY:
			if ( llsd->binary_.iov_len == 0 )
				return TRUE;
			else if ( llsd->binary_.iov_len < 4 )
			{
				for( i = 0; i < llsd->binary_.iov_len; i++ )
				{
					be |= (((uint8_t*)llsd->binary_.iov_base)[i] << ((3 - i) * 8));
				}
				(*v) = ntohl( be );
			}
			else
			{
				(*v) = ntohl( *((uint32_t*)llsd->binary_.iov_base) );
			}
			break;
	}
	return TRUE;
}

int llsd_as_double( llsd_t * llsd, double * v )
{
	int i;
	uint64_t be;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( v, FALSE );
	(*v) = 0.0;

	switch( llsd->type_ )
	{
		case LLSD_UUID:
		case LLSD_URI:
		case LLSD_ARRAY:
		case LLSD_MAP:
			DEBUG( "illegal conversion of %s to real\n", llsd_get_type_string( llsd->type_ ) );
			return FALSE;
		case LLSD_UNDEF:
			break;
		case LLSD_BOOLEAN:
			(*v) = (llsd->bool_ ? 1.0 : 0.0);
			break;
		case LLSD_INTEGER:
			(*v) = (double)llsd->int_;
			break;
		case LLSD_REAL:
			(*v) = llsd->real_;
			break;
		case LLSD_STRING:
			(*v) = atof( llsd->string_ );
			break;
		case LLSD_DATE:
			(*v) = llsd->date_;
			break;
		case LLSD_BINARY:
			if ( llsd->binary_.iov_len == 0 )
				return TRUE;
			else if ( llsd->binary_.iov_len < 8 )
			{
				for( i = 0; i < llsd->binary_.iov_len; i++ )
				{
					be |= (((uint8_t*)llsd->binary_.iov_base)[i] << ((7 - i) * 8));
				}
				(*v) = be64toh( be );
			}
			else
			{
				(*v) = be64toh( *((uint64_t*)llsd->binary_.iov_base) );
			}
			break;
	}
	return TRUE;
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

int llsd_as_uuid( llsd_t * llsd, uint8_t uuid[UUID_LEN] )
{
	int i;
	uint8_t * p = NULL;
	CHECK_PTR_RET( llsd, FALSE );
	MEMSET( uuid, 0, UUID_LEN );

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
			DEBUG( "illegal conversion of %s to uuid\n", llsd_get_type_string( llsd->type_ ) );
			return FALSE;

		case LLSD_BINARY:
			/* if len < 16, return null uuid */
			if ( llsd->binary_.iov_len < UUID_LEN )
				return TRUE;

			MEMCPY( uuid, llsd->binary_.iov_base, UUID_LEN );
			break;

		case LLSD_STRING:
			/* if len < UUID_STR_LEN, return null uuid */
			if ( strlen( llsd->string_ ) < UUID_STR_LEN )
				return TRUE;

			p = llsd->string_;

			/* check for 8-4-4-4-12 */
			for ( i = 0; i < 36; i++ )
			{
				if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
					 ( (i >= 9) && (i < 13) ) &&	/* 4 */
					 ( (i >= 14) && (i < 18) ) &&	/* 4 */
					 ( (i >= 19) && (i < 23) ) &&	/* 4 */
					 ( (i >= 24) && (i < 36) ) )	/* 12 */
				{
					if ( !isxdigit( p[i] ) )
					{
						return FALSE;
					}
				}
				else if ( p[i] != '-' )
				{
					return FALSE;
				}
			}

			/* 8 */
			uuid[0] = hex_to_byte( p[0], p[1] );
			uuid[1] = hex_to_byte( p[2], p[3] );
			uuid[2] = hex_to_byte( p[4], p[5] );
			uuid[3] = hex_to_byte( p[6], p[7] );

			/* 4 */
			uuid[4] = hex_to_byte( p[9], p[10] );
			uuid[5] = hex_to_byte( p[11], p[12] );

			/* 4 */
			uuid[6] = hex_to_byte( p[14], p[15] );
			uuid[7] = hex_to_byte( p[16], p[17] );

			/* 4 */
			uuid[8] = hex_to_byte( p[19], p[20] );
			uuid[9] = hex_to_byte( p[21], p[22] );

			/* 12 */
			uuid[10] = hex_to_byte( p[24], p[25] );
			uuid[11] = hex_to_byte( p[26], p[27] );
			uuid[12] = hex_to_byte( p[28], p[29] );
			uuid[13] = hex_to_byte( p[30], p[31] );
			uuid[14] = hex_to_byte( p[32], p[33] );
			uuid[15] = hex_to_byte( p[34], p[35] );
			break;

		case LLSD_UUID:
			MEMCPY( uuid, llsd->uuid_, UUID_LEN );
			break;
	}
	return TRUE;
}

int llsd_as_string( llsd_t * llsd, uint8_t ** v )
{
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;
	uint8_t * p = NULL;
	static uint8_t buf[UUID_STR_LEN + 1];
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( v, FALSE );
	MEMSET( buf, 0, UUID_STR_LEN );

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_ARRAY:
		case LLSD_MAP:
			(*v) = buf;
			break;
		case LLSD_BOOLEAN:
			strcpy( buf, (llsd->bool_ ? "true" : "false") );
			(*v) = buf;
			break;
		case LLSD_INTEGER:
			snprintf( buf, UUID_STR_LEN, "%d", llsd->int_ );
			(*v) = buf;
			break;
		case LLSD_REAL:
			snprintf( buf, UUID_STR_LEN, "%f", llsd->real_ );
			(*v) = buf;
			break;
		case LLSD_UUID:
			p = llsd->uuid_;
			snprintf( buf, UUID_STR_LEN + 1, 
			  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			  p[0], p[1], p[2], p[3], 
			  p[4], p[5], 
			  p[6], p[7], 
			  p[8], p[9], 
			  p[10], p[11], p[12], p[13], p[14], p[15] );
			(*v) = buf;
			break;
		case LLSD_STRING:
			(*v) = llsd->string_;
			break;
		case LLSD_DATE:
			int_time = floor( llsd->date_ );
			seconds = (time_t)int_time;
			useconds = (int32_t)( ( llsd->date_ - int_time) * 1000000.0 );
			parts = *gmtime(&seconds);
			snprintf( buf, UUID_STR_LEN + 1, 
					  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
					  parts.tm_year + 1900,
					  parts.tm_mon + 1,
					  parts.tm_mday,
					  parts.tm_hour,
					  parts.tm_min,
					  parts.tm_sec,
					  ((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
			(*v) = buf;
			break;
		case LLSD_URI:
			(*v) = llsd->uri_;
			break;
		case LLSD_BINARY:
			DEBUG( "Be careful! Binary to string conversion doesn't guarantee NULL termination\n" );
			(*v) = llsd->binary_.iov_base;
			break;
	}
	return TRUE;
}

int llsd_as_binary( llsd_t * llsd, uint8_t ** v, uint32_t * len )
{
	static uint8_t dummy = 0;
	static uint32_t be32;
	static uint64_t be64;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( v, FALSE );
	CHECK_PTR_RET( len, FALSE );
	(*v) = NULL;
	(*len) = 0;

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_DATE:
		case LLSD_ARRAY:
		case LLSD_MAP:
			DEBUG( "illegal conversion of %s to binary\n", llsd_get_type_string( llsd->type_ ) );
			return FALSE;
		case LLSD_BOOLEAN:
			dummy = (uint8_t)llsd->bool_;
			(*v) = &dummy;
			(*len) = 1;
			break;
		case LLSD_INTEGER:
			be32 = htonl( llsd->int_ );
			(*v) = (uint8_t*)&be32;
			(*len) = sizeof(uint32_t);
			break;
		case LLSD_REAL:
			be64 = htobe64( (uint64_t)llsd->real_ );
			(*v) = (uint8_t*)&be64;
			(*len) = sizeof(uint64_t);
			break;
		case LLSD_UUID:
			(*v) = llsd->uuid_;
			(*len) = UUID_LEN;
		case LLSD_STRING:
			(*v) = llsd->string_;
			(*len) = strlen( llsd->string_ );
			break;
		case LLSD_URI:
			(*v) = llsd->uri_;
			(*len) = strlen( llsd->uri_ );
			break;
		case LLSD_BINARY:
			(*v) = llsd->binary_.iov_base;
			(*len) = llsd->binary_.iov_len;
			break;
	}
	return TRUE;
}

#define URL_ENCODED_CHAR( x ) ( ( x <= 0x1F ) || \
								( x >= 0x7F ) || \
								( x == ' '  ) || \
								( x == '\'' ) || \
								( x == '"'  ) || \
								( x == '<'  ) || \
								( x == '>'  ) || \
								( x == '%'  ) || \
								( x == '{'  ) || \
								( x == '}'  ) || \
								( x == '|'  ) || \
								( x == '\\' ) || \
								( x == '^'  ) || \
								( x == '['  ) || \
								( x == ']'  ) || \
								( x == '`'  ) )

static int llsd_escaped_uri_len( uint8_t const * const uri, uint32_t const len, uint32_t * const esc_len )
{
	uint8_t const * p;
	CHECK_PTR_RET( uri, FALSE );
	CHECK_PTR_RET( esc_len, FALSE );
	CHECK_RET( len > 0, FALSE );

	p = uri;
	while( p < (uri + len) )
	{
		if ( URL_ENCODED_CHAR( *p ) )
		{
			(*esc_len) += 3;
		}
		else
		{
			(*esc_len)++;
		}
		p++;
	}
	return TRUE;
}

int llsd_escape_uri( uint8_t const * const uri, uint32_t const len, uint8_t ** const escaped, uint32_t * const esc_len )
{
	int i;
	uint8_t c;
	uint8_t * p;
	uint32_t l = 0;

	CHECK_PTR_RET( uri, FALSE );
	CHECK_RET( len > 0, FALSE );
	CHECK_PTR_RET( escaped, FALSE );
	CHECK_PTR_RET( esc_len, FALSE );

	(*esc_len) = 0;
	(*escaped) = NULL;

	CHECK_RET( llsd_escaped_uri_len( uri, len, &l ), FALSE );
	(*escaped) = CALLOC( l, sizeof(uint8_t) );
	CHECK_PTR_RET( (*escaped), FALSE );
	(*esc_len) = l;

	p = (*escaped);
	for( i = 0; i < len; i++)
	{
		c = uri[i];
		if ( URL_ENCODED_CHAR( c ) )
		{
			sprintf( p, "%%02x", c );
			p += 3;
		}
		else
		{
			*p = c;
			p++;
		}
	}
	return TRUE;
}

static int llsd_unescaped_uri_len( uint8_t const * const escaped, uint32_t const enc_len, uint32_t * const len )
{
	int esc = FALSE;
	int hex = FALSE;
	uint8_t const * p;

	CHECK_PTR_RET( escaped, FALSE );
	CHECK_RET( enc_len > 0, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = 0;

	p = escaped;
	while ( p < (escaped + enc_len) )
	{
		if ( esc )
		{
			if ( hex )
			{
				CHECK_RET( isxdigit( *p ), FALSE );
				/* found % followed by two hex digits that will decode
				 * to a single char */
				(*len)++;
				esc = FALSE;
				hex = FALSE;
			}
			else
			{
				CHECK_RET( isxdigit( *p ), FALSE );
				hex = TRUE;
			}
		}
		else
		{
			if ( *p == '%' )
			{
				esc = TRUE;
			}
			else
			{
				(*len)++;
			}
		}
		p++;
	}
	return TRUE;
}

int llsd_unescape_uri( uint8_t const * const escaped, uint32_t const esc_len, uint8_t ** const uri, uint32_t * const len )
{
	int esc = FALSE;
	int hex = FALSE;
	uint8_t const * p;
	uint8_t * q;
	uint32_t l;

	CHECK_PTR_RET( escaped, FALSE );
	CHECK_RET( esc_len > 0, FALSE );
	CHECK_PTR_RET( uri, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*uri) = NULL;
	(*len) = 0;

	CHECK_RET( llsd_unescaped_uri_len( escaped, esc_len, &l ), FALSE );
	(*uri) = CALLOC( l, sizeof(uint8_t) );
	CHECK_PTR_RET( (*uri), FALSE );
	(*len) = l;

	p = escaped;
	q = (*uri);
	while ( p < (escaped + esc_len) )
	{
		if ( esc )
		{
			if ( hex )
			{
				CHECK_RET( isxdigit( *p ), FALSE );
				/* found % followed by two hex digits that will decode
				 * to a single char */
				esc = FALSE;
				hex = FALSE;

				/* decode the hex chars to a byte */
				*q = hex_to_byte( *(p-1), *p );
				q++;
			}
			else
			{
				CHECK_RET( isxdigit( *p ), FALSE );
				hex = TRUE;
			}
		}
		else
		{
			if ( *p == '%' )
			{
				esc = TRUE;
			}
			else
			{
				*q = *p;
				q++;
			}
		}
		p++;
	}

	return TRUE;
}

int llsd_equal( llsd_t * l, llsd_t * r )
{
	int ret = TRUE;
	llsd_itr_t litr, lend, ritr, rend;
	llsd_t * lk, * lv, * rk, * rv;
	CHECK_PTR_RET( l, FALSE );
	CHECK_PTR_RET( r, FALSE );
	CHECK_RET( l->type_ == r->type_, FALSE );

	switch( l->type_ )
	{
		case LLSD_UNDEF:
			return TRUE;
		case LLSD_BOOLEAN:
			return (l->bool_ == r->bool_);
		case LLSD_INTEGER:
			return (l->int_ == r->int_);
		case LLSD_REAL:
			return (MEMCMP( &l->real_, &r->real_, sizeof( double ) ) == 0);
		case LLSD_DATE:
			return (MEMCMP( &l->date_, &r->date_, sizeof( double ) ) == 0);
		case LLSD_UUID:
			return (MEMCMP( l->uuid_, r->uuid_, UUID_LEN) == 0);
		case LLSD_STRING:
			return (STRCMP( l->string_, r->string_ ) == 0);
		case LLSD_URI:
			return (STRCMP( l->uri_, r->uri_ ) == 0);
		case LLSD_BINARY:
			CHECK_RET( l->binary_.iov_len == r->binary_.iov_len, FALSE );
			return (MEMCMP( l->binary_.iov_base, r->binary_.iov_base, l->binary_.iov_len ) == 0);
		case LLSD_ARRAY:
			CHECK_RET( list_count( &l->array_ ) == list_count( &r->array_ ), FALSE );
			litr = llsd_itr_begin( l );
			ritr = llsd_itr_begin( r );
			lend = llsd_itr_end( l );
			rend = llsd_itr_end( r );
			for ( ; !LLSD_ITR_EQ( litr, lend ) && !LLSD_ITR_EQ( ritr, rend ); litr = llsd_itr_next( l, litr ), ritr = llsd_itr_next( r, ritr ) )
			{
				CHECK_RET( llsd_get( l, litr, &lv, &lk ), FALSE );
				CHECK_RET( llsd_get( r, ritr, &rv, &rk ), FALSE );
				CHECK_RET( (lk == NULL) && (rk == NULL), FALSE );

				/* recursively check equality */
				if ( !llsd_equal( lv, rv ) )
				{
					WARN( "%s != %s\n", llsd_get_type_string( llsd_get_type( lv ) ), llsd_get_type_string( llsd_get_type( rv ) ) );
					llsd_equal( lv, rv );
					ret = FALSE;
				}
			}
			return ret;
		case LLSD_MAP:
			CHECK_RET( ht_count( &l->map_ ) == ht_count( &r->map_ ), FALSE );
			litr = llsd_itr_begin( l );
			lend = llsd_itr_end( l );
			for ( ; !LLSD_ITR_EQ( litr, lend ); litr = llsd_itr_next( l, litr ) )
			{
				/* get a k-v pair from the left map */
				CHECK_RET( llsd_get( l, litr, &lv, &lk ), FALSE );
				CHECK_PTR_RET( lk, FALSE );

				/* use the left key to look up a value in the right map */
				rv = llsd_map_find_llsd( r, lk );

				/* now recurse */
				if ( !llsd_equal( lv, rv ) )
				{
					WARN( "%s != %s\n", llsd_get_type_string( llsd_get_type( lv ) ), llsd_get_type_string( llsd_get_type( rv ) ) );
					llsd_equal( lv, rv );
					ret = FALSE;
				}
			}
			return ret;
	}
	return FALSE;
}

int llsd_get_count( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, 0 );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			return 0; /* is is so that llsd_is_empty is true for undef */
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_DATE:
		case LLSD_UUID:
			return 1;

		case LLSD_STRING:
			return strlen( llsd->string_ );

		case LLSD_URI:
			return strlen( llsd->uri_ );

		case LLSD_BINARY:
			return llsd->binary_.iov_len;

		case LLSD_ARRAY:
			return list_count( &llsd->array_ );

		case LLSD_MAP:
			return ht_count( &llsd->map_ );
	}
	return 0;
}


