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

/* constants */
llsd_t const undefined =
{
	.type_ = LLSD_UNDEF,
	.value.bool_ = FALSE
};

llsd_uuid_t const zero_uuid = 
{ 
	.bits = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

llsd_string_t const false_string = 
{
	.dyn = FALSE,
	.escaped = FALSE,
	.str = "false"
};
llsd_string_t const true_string = 
{
	.dyn = FALSE,
	.escaped = FALSE,
	.str = "true"
};

uint8_t zero_data [] = { '0' };
llsd_binary_t const false_binary =
{
	.dyn = FALSE,
	.size = 1,
	.data = zero_data
};
uint8_t one_data[] = { '1' };
llsd_binary_t const true_binary =
{
	.dyn = FALSE,
	.size = 1,
	.data = one_data
};
llsd_binary_t const empty_binary =
{
	.dyn = FALSE,
	.size = 0,
	.data = 0
};
llsd_uri_t const empty_uri = 
{
	.dyn = FALSE,
	.uri = ""
};
llsd_array_t const empty_array =
{
	.array = {
		.pfn = 0,
		.num_nodes = 0,
		.buffer_size = 0,
		.data_head = -1,
		.free_head = -1,
		.node_buffer = 0
	}
};
llsd_map_t const empty_map =
{
	.ht = {
		.khfn = 0,
		.kefn = 0,
		.kdfn = 0,
		.vdfn = 0,
		.prime_index = 0,
		.num_tuples = 0,
		.initial_capacity = 0,
		.load_factor = 0.0f,
		.tuples = 0,
	}
};

#define FNV_PRIME (0x01000193)
static uint32_t fnv_key_hash(void const * const key)
{
	int i;
	llsd_t * llsd = (llsd_t*)key;
    uint32_t hash = 0x811c9dc5;
	uint8_t const * p = (uint8_t const *)llsd->value.string_.str;
	CHECK_RET_MSG( llsd->type_ == LLSD_STRING, 0, "map key hashing function received non-string\n" );
	while ( (*p) != '\0' )
	{
		hash *= FNV_PRIME;
		hash ^= *p++;
	}
	return hash;
}


static int key_eq(void const * const l, void const * const r)
{
	llsd_t * llsd_l = (llsd_t *)l;
	llsd_t * llsd_r = (llsd_t *)r;
	CHECK_RET_MSG( llsd_l->type_ == LLSD_STRING, -1, "map key compare function received non-string as left side\n" );
	CHECK_RET_MSG( llsd_r->type_ == LLSD_STRING, -1, "map key compare function received non-string as right side\n" );
	return (strcmp( llsd_l->value.string_.str, llsd_r->value.string_.str ) == 0);
}


static void llsd_initialize( llsd_t * llsd, llsd_type_t type_, ... )
{
	va_list args;
	uint8_t * p;

	CHECK_PTR( llsd );
	
	llsd->type_ = type_;

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			break;

		case LLSD_BOOLEAN:
			va_start( args, type_ );
			llsd->value.bool_ = va_arg( args, int );
			va_end(args);
			break;

		case LLSD_INTEGER:
			va_start( args, type_ );
			llsd->value.int_ = va_arg(args, int );
			va_end(args);
			break;

		case LLSD_REAL:
			va_start( args, type_ );
			llsd->value.real_ = va_arg(args, double );
			va_end(args);
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			memcpy( llsd->value.uuid_.bits, va_arg( args, uint8_t* ), UUID_LEN );
			va_end( args );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			llsd->value.string_.dyn = TRUE;
			p = va_arg( args, uint8_t* );
			llsd->value.string_.str = STRDUP( p );
			llsd->value.string_.escaped = va_arg( args, int );
			va_end( args );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->value.date_ = va_arg( args, double );
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			llsd->value.uri_.dyn = TRUE;
			p = va_arg( args, uint8_t* );
			llsd->value.uri_.uri = STRDUP( p );
			va_end( args );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			llsd->value.binary_.dyn = TRUE;
			llsd->value.binary_.size = va_arg( args, int );
			llsd->value.binary_.data = CALLOC( llsd->value.binary_.size, sizeof(uint8_t) );
			p = va_arg( args, uint8_t* );
			memcpy( llsd->value.binary_.data, p, llsd->value.binary_.size );
			va_end( args );
			break;
		case LLSD_ARRAY:
			array_initialize( &(llsd->value.array_.array), DEFAULT_ARRAY_CAPACITY, &llsd_delete );
			break;
		case LLSD_MAP:
			ht_initialize( &(llsd->value.map_.ht), DEFAULT_MAP_CAPACITY, &fnv_key_hash, &llsd_delete, &key_eq, &llsd_delete );
			break;
	}
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
		case LLSD_UUID:
		case LLSD_DATE:
			return;
		case LLSD_STRING:
			if ( llsd->value.string_.dyn )
				FREE( llsd->value.string_.str );
			break;
		case LLSD_URI:
			if ( llsd->value.uri_.dyn )
				FREE( llsd->value.uri_.uri );
			break;
		case LLSD_BINARY:
			if ( llsd->value.binary_.dyn )
				FREE( llsd->value.binary_.data );
			break;
		case LLSD_ARRAY:
			array_deinitialize( &llsd->value.array_.array );
			break;
		case LLSD_MAP:
			ht_deinitialize( &llsd->value.map_.ht );
			break;
	}
}


llsd_t * llsd_new( llsd_type_t type_, ... )
{
	va_list args;
	llsd_t * llsd;
	int a1;
	uint8_t * a2;
	double a3;
	
	/* allocate the llsd object */
	llsd = CALLOC(1, sizeof(llsd_t));
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
		case LLSD_DATE:
			va_start( args, type_ );
			a3 = va_arg( args, double );
			va_end( args );
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
			a2 = va_arg( args, uint8_t* );
			a1 = va_arg( args, int );
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1 );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a1, a2 );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			llsd_initialize( llsd, type_ );
			break;
	}

	return llsd;
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
	CHECK_RET( ((type_ >= LLSD_TYPE_FIRST) && (type_ <= LLSD_TYPE_LAST)), NULL );
	return llsd_type_strings[ type_ ];
}

int llsd_get_size( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( ((llsd->type_ == LLSD_ARRAY) || (llsd->type_ == LLSD_MAP)), -1, "getting size of non-container llsd object\n" );

	if ( llsd->type_ == LLSD_ARRAY )
		return array_size( &llsd->value.array_.array );
	
	return ht_size( &llsd->value.map_.ht );
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
			if ( memcmp( llsd->value.uuid_.bits, zero_uuid.bits, UUID_LEN ) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_STRING:
			if ( strlen(llsd->value.string_.str) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_BINARY:
			if ( llsd->value.binary_.size == 0 )
				return FALSE;
			return TRUE;
	}
	return FALSE;
}

llsd_int_t llsd_as_int( llsd_t * llsd )
{
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
			if ( (uint64_t)llsd->value.date_ >= UINT32_MAX )
			{
				DEBUG( "truncating 64-bit date value to 32-bit integer...loss of data!" );
			}
			return (llsd_int_t)llsd->value.date_;
		case LLSD_STRING:
			return (llsd_int_t)atoi( llsd->value.string_.str );
		case LLSD_BINARY:
			if ( llsd->value.binary_.size < sizeof(llsd_int_t) )
			{
				return 0;
			}
			return ntohl( *((uint32_t*)llsd->value.binary_.data) );
	}
	return 0;
}

llsd_real_t llsd_as_real( llsd_t * llsd )
{
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
			return (llsd_real_t)atof( llsd->value.string_.str );
		case LLSD_DATE:
			return (llsd_real_t)llsd->value.date_;
		case LLSD_BINARY:
			if ( llsd->value.binary_.size < sizeof(llsd_real_t) )
			{
				return 0.;
			}
			return (llsd_real_t)be64toh( *((uint64_t*)llsd->value.binary_.data) );
	}
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

llsd_uuid_t llsd_as_uuid( llsd_t * llsd )
{
	static llsd_uuid_t tmp_uuid;
	uint8_t * str;
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
			/* if len < 16, return null uuid, otherwise first 16 bytes converted to uuid */
			if ( llsd->value.binary_.size < 16 )
			{
				return zero_uuid;
			}
			MEMCPY( &(tmp_uuid.bits[0]), llsd->value.binary_.data, UUID_LEN );
			return tmp_uuid;

		case LLSD_STRING:
			str = llsd->value.string_.str;

			/* a valid 8-4-4-4-12 is converted to a UUID, all other values return null UUID */
			/* TODO: add string unescaping when checking for value UUID */
			/* with escaping the UUID string could be longer */
			if ( strnlen( str, 36 ) < 36 )
			{
				return zero_uuid;
			}

			/* TODO: add string unescaping when checking for value UUID */
			/* check for 8-4-4-4-12 */
			for ( i = 0; i < 36; i++ )
			{
				if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
					 ( (i >= 9) && (i < 13) ) &&    /* 4 */
					 ( (i >= 14) && (i < 18) ) &&   /* 4 */
					 ( (i >= 19) && (i < 23) ) &&	/* 4 */
					 ( (i >= 24) && (i < 36) ) )	/* 12 */
				{
					if ( !isxdigit( str[i] ) )
					{
						return zero_uuid;
					}
				}
				else if ( str[i] != '-' )
				{
					return zero_uuid;
				}
			}

			/* covert to UUID */

			/* 8 */
			tmp_uuid.bits[0] = hex_to_byte( str[0], str[1] );
			tmp_uuid.bits[1] = hex_to_byte( str[2], str[3] );
			tmp_uuid.bits[2] = hex_to_byte( str[4], str[5] );
			tmp_uuid.bits[3] = hex_to_byte( str[6], str[7] );

			/* 4 */
			tmp_uuid.bits[4] = hex_to_byte( str[9], str[10] );
			tmp_uuid.bits[5] = hex_to_byte( str[11], str[12] );

			/* 4 */
			tmp_uuid.bits[6] = hex_to_byte( str[14], str[15] );
			tmp_uuid.bits[7] = hex_to_byte( str[16], str[17] );

			/* 4 */
			tmp_uuid.bits[8] = hex_to_byte( str[19], str[20] );
			tmp_uuid.bits[9] = hex_to_byte( str[21], str[22] );

			/* 12 */
			tmp_uuid.bits[10] = hex_to_byte( str[24], str[25] );
			tmp_uuid.bits[11] = hex_to_byte( str[26], str[27] );
			tmp_uuid.bits[12] = hex_to_byte( str[28], str[29] );
			tmp_uuid.bits[13] = hex_to_byte( str[30], str[31] );
			tmp_uuid.bits[14] = hex_to_byte( str[32], str[33] );
			tmp_uuid.bits[15] = hex_to_byte( str[34], str[35] );

			return tmp_uuid;

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
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;

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
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = tmp };
		case LLSD_REAL:
			len = snprintf( tmp, 64, "%f", llsd->value.real_ );
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = tmp };
		case LLSD_UUID:
			p = &(llsd->value.uuid_.bits[0]);
			len = snprintf( tmp, 64, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15] );
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = tmp };
		case LLSD_STRING:
			return llsd->value.string_;
		case LLSD_DATE:
			int_time = floor( llsd->value.date_ );
			seconds = (time_t)int_time;
			useconds = (int32_t)( ( llsd->value.date_ - int_time) * 1000000.0 );
			parts = *gmtime(&seconds);
			if ( useconds != 0 )
			{
				len = snprintf( tmp, 64, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
							   parts.tm_year + 1900,
							   parts.tm_mon + 1,
							   parts.tm_mday,
							   parts.tm_hour,
							   parts.tm_min,
							   parts.tm_sec,
							   (int32_t)(useconds / 1000.f + 0.5f) );
			}
			else
			{
				len = snprintf( tmp, 64, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
							   parts.tm_year + 1900,
							   parts.tm_mon + 1,
							   parts.tm_mday,
							   parts.tm_hour,
							   parts.tm_min,
							   parts.tm_sec );
			}
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = tmp };

		case LLSD_URI:
			/* TODO: make sure it is escaped properly */
			return (llsd_string_t){ .dyn = FALSE, .escaped = TRUE, .str = llsd->value.uri_.uri };

		case LLSD_BINARY:
			/* TODO: check for valid UTF-8 */
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = llsd->value.binary_.data };
	}
}

llsd_date_t llsd_as_date( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, 0., "invalid llsd pointer\n" );
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
			return (llsd_date_t)llsd->value.int_;

		case LLSD_REAL:
			return (llsd_date_t)llsd->value.real_;

		case LLSD_STRING:
			/* TODO: parse string format date into a real */
			break;

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
			return (llsd_uri_t){ .dyn = FALSE, .uri = llsd->value.string_.str } ;

		case LLSD_URI:
			return llsd->value.uri_;

		case LLSD_BINARY:
			/* TODO: check for valid UTF-8 */
			return (llsd_uri_t){ .dyn = FALSE, .uri = llsd->value.binary_.data };
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
		case LLSD_URI:
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
			return (llsd_binary_t){ .size = sizeof(llsd_int_t), .data = (uint8_t*)&tmpl };

		case LLSD_REAL:
			tmpll = htobe64( *((uint64_t*)&(llsd->value.real_)) );
			return (llsd_binary_t){ .size = sizeof(llsd_real_t), .data = (uint8_t*)&tmpll };

		case LLSD_UUID:
			return (llsd_binary_t){ .size = UUID_LEN, .data = &(llsd->value.uuid_.bits[0]) };

		case LLSD_STRING:
			return (llsd_binary_t){ .size = strlen( llsd->value.string_.str ), .data = llsd->value.string_.str };

		case LLSD_BINARY:
			return llsd->value.binary_;
	}
}

void llsd_array_append( llsd_t * arr, llsd_t * data )
{
	CHECK_PTR( arr );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(arr) == LLSD_ARRAY, "trying to append data to non array\n" );
	array_push_tail( &(arr->value.array_.array), (void*)data );
}

void llsd_map_insert( llsd_t * map, llsd_t * key, llsd_t * data )
{
	CHECK_PTR( map );
	CHECK_PTR( key );
	CHECK_PTR( data );
	CHECK_MSG( llsd_get_type(map) == LLSD_MAP, "trying to insert k-v-p into non map\n" );
	CHECK_MSG( llsd_get_type(key) == LLSD_STRING, "trying to use non-string as key\n" );
	ht_add( &(map->value.map_.ht), (void*)key, (void*)data );
}

llsd_itr_t llsd_itr_begin( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, (llsd_itr_t)-1 );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_begin( &(llsd->value.array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_begin( &(llsd->value.map_.ht) );
	}
	return (llsd_itr_t)0;
}

llsd_itr_t llsd_itr_end( llsd_t * llsd )
{
	return (llsd_itr_t)-1;
}

llsd_itr_t llsd_itr_rbegin( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, (llsd_itr_t)-1 );
	
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_rbegin( &(llsd->value.array_.array) );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rbegin( &(llsd->value.map_.ht) );
	}
	return (llsd_itr_t)0;
}

llsd_itr_t llsd_itr_next( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_next( &(llsd->value.array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_next( &(llsd->value.map_.ht), (ht_itr_t)itr );
	}
	return (llsd_itr_t)-1;
}

llsd_itr_t llsd_itr_rnext( llsd_t * llsd, llsd_itr_t itr )
{
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			return (llsd_itr_t)array_itr_rnext( &(llsd->value.array_.array), (array_itr_t)itr );
		case LLSD_MAP:
			return (llsd_itr_t)ht_itr_rnext( &(llsd->value.map_.ht), (ht_itr_t)itr );
	}
	return (llsd_itr_t)-1;
}

int llsd_itr_get( llsd_t * llsd, llsd_itr_t itr, llsd_t ** value, llsd_t ** key )
{
	llsd_t * k;
	llsd_t * v;
	CHECK_PTR_RET( value, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( itr != -1, FALSE );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_ARRAY:
			(*value) = (llsd_t*)array_itr_get( &(llsd->value.array_.array), itr );
			(*key) = NULL;
			return TRUE;
		case LLSD_MAP:
			(*value) = (llsd_t*)ht_itr_get( &(llsd->value.map_.ht), itr, (void**)key );

			return TRUE;
	}

	/* non-container iterator just references the llsd */
	(*value) = llsd;
	(*key) = NULL;
	return TRUE;
}


static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	llsd->value.binary_.dyn = FALSE;
	llsd->value.binary_.size = size;
	llsd->value.binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_string( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size + 1 );
	llsd->type_ = LLSD_STRING;
	llsd->value.string_.dyn = FALSE;
	llsd->value.string_.escaped = FALSE;
	llsd->value.string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size + 1 );
	llsd->type_ = LLSD_URI;
	llsd->value.uri_.dyn = FALSE;
	llsd->value.uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_array( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_ARRAY;
	array_initialize( &(llsd->value.array_.array), size, &llsd_delete );
	return llsd;
}

static llsd_t * llsd_reserve_map( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_MAP;
	ht_initialize( &(llsd->value.map_.ht), 
				   size, 
				   &fnv_key_hash, 
				   &llsd_delete, 
				   &key_eq, 
				   &llsd_delete );
	return llsd;
}

static llsd_t * llsd_parse_binary( FILE * fin )
{
	int i;
	uint8_t p;
	uint32_t t1;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	uint16_t t3[UUID_LEN];
	llsd_t * llsd;
	llsd_t * key;

	CHECK_PTR_RET( fin, NULL );

	while( !feof( fin ) )
	{
		/* read the type marker */
		fread( &p, sizeof(uint8_t), 1, fin );

		switch( p )
		{
			case '!':
				return llsd_new( LLSD_UNDEF );
			case '1':
				return llsd_new( LLSD_BOOLEAN, TRUE );
			case '0':
				return llsd_new( LLSD_BOOLEAN, FALSE );
			case 'i':
				fread( &t1, sizeof(uint32_t), 1, fin );
				return llsd_new( LLSD_INTEGER, ntohl( t1 ) );
			case 'r':
				fread( &t2, sizeof(double), 1, fin );
				t2.ull = be64toh( t2.ull );
				return llsd_new( LLSD_REAL, t2.d );
			case 'u':
				fread( t3, sizeof(uint8_t), UUID_LEN, fin );
				return llsd_new( LLSD_UUID, t3 );
			case 'b':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_binary( t1 );
				fread( llsd->value.binary_.data, sizeof(uint8_t), t1, fin );
				return llsd;
			case 's':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_string( t1 ); /* allocates t1 + 1 bytes */
				fread( llsd->value.string_.str, sizeof(uint8_t), t1 + 1, fin );
				/* TODO: detect if it is escaped and set the escaped flag */
				return llsd;
			case 'l':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_uri( t1 ); /* allocates t1 + 1 bytes */
				fread( llsd->value.uri_.uri, sizeof(uint8_t), t1 + 1, fin );
				return llsd;
			case 'd':
				fread( &t2, sizeof(double), 1, fin );
				t2.ull = be64toh( t2.ull );
				return llsd_new( LLSD_DATE, t2.d );
			case '[':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_array( t1 );
				for ( i = 0; i < t1; ++i )
				{
					/* parse and append the array member */
					llsd_array_append( llsd, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != ']' )
				{
					FAIL( "array didn't end with ']'\n" );
				}
				return llsd;
			case '{':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_map( t1 );
				for ( i = 0; i < t1; ++i )
				{
					/* parse the key */
					key = llsd_parse_binary( fin );
					if ( llsd_get_type( key ) != LLSD_STRING )
					{
						FAIL( "key is not an LLSD_STRING\n" );
					}

					/* parse and append the key/value pair */
					llsd_map_insert( llsd, key, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != '}' )
				{
					FAIL( "map didn't end with '}'\n" );
				}
				return llsd;
		}
	}
}

static llsd_t * llsd_parse_notation( FILE * fin )
{
	return NULL;
}

static llsd_t * llsd_parse_xml( FILE * fin )
{
	return NULL;
}

static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";
static uint8_t const * const notation_header = "<?llsd/notation?>\n";
static uint8_t const * const xml_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<llsd>\n";
static uint8_t const * const xml_footer = "</llsd>\n";

#define SIG_LEN (18)
llsd_t * llsd_parse( FILE *fin )
{
	uint8_t sig[SIG_LEN];

	CHECK_RET_MSG( fin, NULL, "invalid file pointer\n" );

	fread( sig, sizeof(uint8_t), SIG_LEN, fin );
	if ( memcmp( sig, binary_header, SIG_LEN ) == 0 )
	{
		return llsd_parse_binary( fin );
	}
	else if ( memcmp( sig, notation_header, SIG_LEN ) == 0 )
	{
		return llsd_parse_notation( fin );
	}
	else
	{
		rewind( fin );
		return llsd_parse_xml( fin );
	}
}

static size_t llsd_format_xml( llsd_t * llsd, FILE * fout )
{
	return 0;
}

static size_t llsd_format_notation( llsd_t * llsd, FILE * fout )
{
	return 0;
}

static size_t llsd_format_binary( llsd_t * llsd, FILE * fout )
{
	size_t num = 0;
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
			break;
		case LLSD_BOOLEAN:
			p = ( llsd_as_bool( llsd ) == TRUE ) ? '1' : '0';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			break;
		case LLSD_INTEGER:
			p = 'i';
			t1 = htonl( llsd_as_int( llsd ) );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			break;
		case LLSD_REAL:
			p = 'r';
			t2.d = llsd_as_real( llsd );
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			break;
		case LLSD_UUID:
			p = 'u';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += fwrite( &(llsd->value.uuid_.bits[0]), sizeof(uint8_t), UUID_LEN, fout );
			break;
		case LLSD_STRING:
			p = 's';
			s = strlen( llsd->value.string_.str );
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd_as_string( llsd ).str, sizeof(uint8_t), s, fout );
			break;
		case LLSD_DATE:
			p = 'd';
			t2.d = llsd_as_date( llsd );
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			break;
		case LLSD_URI:
			p = 'l';
			s = strlen( llsd->value.uri_.uri );
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.uri_.uri, sizeof(uint8_t), s, fout );
			break;
		case LLSD_BINARY:
			p = 'b';
			s = llsd->value.binary_.size;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.binary_.data, sizeof(uint8_t), s, fout );
			break;
		case LLSD_ARRAY:
			p = '[';
			s = array_size( &(llsd->value.array_.array) );
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
			break;
		case LLSD_MAP:
			p = '{';
			s = ht_size( &(llsd->value.map_.ht) );
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
			break;
	}
	return num;
}

size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout )
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
		case LLSD_ENC_NOTATION:
			s += fwrite( notation_header, sizeof(uint8_t), 18, fout );
			s += llsd_format_notation( llsd, fout );
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

