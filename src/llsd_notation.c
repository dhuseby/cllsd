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
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/array.h>
#include <cutil/hashtable.h>

#include "llsd.h"
#include "llsd_util.h"
#include "llsd_const.h"
#include "llsd_notation.h"

static uint8_t const * const lntabs		= "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

static uint32_t indent = 1;

#define indent_notation(p,f) ( p ? fwrite(lntabs, sizeof(uint8_t), ((indent<=255)?indent:255), f) : 0 );


static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_BINARY;
	if ( size > 0 )
	{
		llsd->binary_.data_size = size;
		llsd->binary_.data = (uint8_t*)CALLOC( size + 1, sizeof(uint8_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_string( int escaped, uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_STRING;
	if ( size > 0 )
	{
		if ( escaped )
		{
			llsd->string_.esc_len = size;
			llsd->string_.esc = (uint8_t*)CALLOC( size + 1, sizeof(uint8_t) );
		}
		else
		{
			llsd->string_.str_len = size;
			llsd->string_.str = (uint8_t*)CALLOC( size + 1, sizeof(uint8_t) );
		}
	}
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_URI;
	if ( size > 0 )
	{
		llsd->uri_.uri_len = size;
		llsd->uri_.uri = (uint8_t*)CALLOC( size + 1, sizeof(uint8_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_date( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_DATE;
	llsd->date_.use_dval = FALSE;
	if ( size > 0 )
	{
		llsd->date_.len = size;
		llsd->date_.str = (uint8_t*)CALLOC( size + 1, sizeof(uint8_t) );
	}
}

static llsd_t * llsd_reserve_array( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_ARRAY;
	array_initialize( &(llsd->array_.array), size, &llsd_delete );
	return llsd;
}

static llsd_t * llsd_reserve_map( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_MAP;
	ht_initialize( &(llsd->map_.ht), 
				   size, 
				   &fnv_key_hash, 
				   &llsd_delete, 
				   &key_eq, 
				   &llsd_delete );
	return llsd;
}


static int llsd_read_until( uint8_t ** out, uint32_t * out_size, uint8_t delim, FILE * fin )
{
	static uint8_t buf[1024];
	int i = 0;
	int total = 0;

	CHECK_PTR_RET( out, 0 );
	CHECK_PTR_RET( out_size, 0 );
	CHECK_PTR_RET( fin, 0 );

	while( !feof( fin ) )
	{
		buf[i] = fgetc( fin );

		if ( ( buf[i] == delim ) || ( i == 1023 ) )
		{
			(*out) = REALLOC( (*out), (total + i + 1) * sizeof(uint8_t) );
			CHECK_PTR_RET_MSG( (*out), 0, "failed to realloc buffer\n" );

			/* copy the data over */
			MEMCPY( &((*out)[total]), buf, i );

			/* ensure that it is NULL terminated */
			(*out)[total + i + 1] = '\0';

			/* update the total */
			total += i;

			if ( buf[i] == delim )
			{
				(*out_size) = total;
				return total;
			}

			/* reset i */
			i = 0;
		}
		else
			i++;
	}

	WARN( "didn't find delimiter before reaching end of file\n" );

	return 0;
}

static size_t llsd_write_notation_start_tag( llsd_t * llsd, FILE * fout, int pretty )
{
	static uint8_t tmp[64];
	int len = 0;
	size_t num = 0;
	int8_t const * str = NULL;
	llsd_type_t t;
	CHECK_PTR_RET( llsd, 0 );
	t = llsd_get_type( llsd );
	CHECK_RET( (t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST), 0 );

	switch ( t )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			return 0;
		case LLSD_INTEGER:
			return fwrite( "i", sizeof(uint8_t), 1, fout );
		case LLSD_REAL:
			return fwrite( "r", sizeof(uint8_t), 1, fout );
		case LLSD_UUID:
			return fwrite( "u", sizeof(uint8_t), 1, fout );
		case LLSD_DATE:
			return fwrite( "d\"", sizeof(uint8_t), 1, fout );
		case LLSD_STRING:
			if ( llsd->string_.raw == TRUE )
			{
				len = snprintf( tmp, 64, "%d", llsd->binary_.data_size );
				num += fwrite( "s(", sizeof(uint8_t), 2, fout );
				num += fwrite( tmp, sizeof(uint8_t), len, fout );
				num += fwrite( ")", sizeof(uint8_t), 1, fout );
			}
			num += fwrite( "\"", sizeof(uint8_t), 1, fout );
			return num;
		case LLSD_URI:
			return fwrite( "l\"", sizeof(uint8_t), 1, fout );
		case LLSD_BINARY:
			if ( llsd->binary_.encoding == LLSD_RAW )
			{
				/* raw binary */
				len = snprintf( tmp, 64, "%d", llsd->binary_.data_size );
				num += fwrite( "b(", sizeof(uint8_t), 2, fout );
				num += fwrite( tmp, sizeof(uint8_t), len, fout );
				num += fwrite( ")", sizeof(uint8_t), 1, fout );
			}
			else
			{
				str = llsd_get_bin_enc_type_string( llsd->binary_.encoding, LLSD_ENC_NOTATION );
				num += fwrite( str, sizeof(uint8_t), strlen(str), fout );
			}
			num += fwrite( "\"", sizeof(uint8_t), 1, fout );
			return num;
		case LLSD_ARRAY:
			return fwrite( "[\n", sizeof(uint8_t), ((pretty && (llsd_get_size(llsd) > 0)) ? 2 : 1), fout );
		case LLSD_MAP:
			return fwrite( "{\n", sizeof(uint8_t), ((pretty && (llsd_get_size(llsd) > 0)) ? 2 : 1), fout );
	}
	return 0;
}

static size_t llsd_write_notation_end_tag( llsd_t * llsd, FILE * fout, int pretty )
{
	size_t num = 0;
	llsd_type_t t;
	CHECK_PTR_RET( llsd, 0 );
	t = llsd_get_type( llsd );
	CHECK_RET( (t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST), 0 );
	switch ( t )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
			return 0;
		case LLSD_DATE:
		case LLSD_BINARY:
		case LLSD_URI:
		case LLSD_STRING:
			return fwrite( "\"", sizeof(uint8_t), 1, fout );
		case LLSD_ARRAY:
			return fwrite( "]", sizeof(uint8_t), 1, fout );
		case LLSD_MAP:
			return fwrite( "}", sizeof(uint8_t), 1, fout );
	}
	return 0;
}


llsd_t * llsd_parse_notation( FILE * fin )
{
	size_t ret;
	int i;
	int raw = FALSE;
	uint8_t p;
	uint32_t t1;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	uint16_t t3[UUID_LEN];
	llsd_t * llsd;
	llsd_t * key;
	llsd_t * childp;
	
	CHECK_PTR_RET( fin, NULL );

	while( !feof( fin ) )
	{
		/* read the type marker */
		ret = fread( &p, sizeof(uint8_t), 1, fin );
		CHECK_RET_MSG( ret == 1, NULL, "failed to read type byte from binary LLSD\n" );

		switch( p )
		{

			case '!':
				DEBUG( "%*sUNDEF\n", indent, " " );
				return llsd_new( LLSD_UNDEF );

			case 't':
			case 'T':
			case '1':
				DEBUG( "%*sBOOLEAN (TRUE)\n", indent, " " );
				return llsd_new( LLSD_BOOLEAN, TRUE );

			case 'f':
			case 'F':
			case '0':
				DEBUG( "%*sBOOLEAN (FALSE)\n", indent, " " );
				return llsd_new( LLSD_BOOLEAN, FALSE );

			case 'i':
				llsd = llsd_new( LLSD_INTEGER, 0 );
				ret = fscanf( fin, "%d", &(llsd->int_.v) );
				DEBUG( "%*sINTEGER (%d)\n", indent, " ", llsd->int_.v );
				return llsd;

			case 'r':
				llsd = llsd_new( LLSD_REAL, 0.0 );
				ret = fscanf( fin, "%lf", &(llsd->real_.v) );
				DEBUG( "%*sREAL (%f)\n", indent, " ", llsd->real_.v );
				return llsd;

			case 'u':
				llsd = llsd_new( LLSD_TYPE_INVALID );
				llsd->type_ = LLSD_UUID;
				llsd->uuid_.str = (uint8_t*)CALLOC( UUID_STR_LEN + 1, sizeof(uint8_t) );
				llsd->uuid_.dyn_str = TRUE;
				ret = fscanf( fin, "%36s", llsd->uuid_.str );
				DEBUG( "%*sUUID (%s)\n", indent, " ", llsd->uuid_.str );
				return llsd;

			case 'b':
				/* try to read in parens and size */
				if ( (ret = fscanf( fin, "(%d)\"", &t1 ) ) == 1)
				{
					/* we have a length and raw binary */
					llsd = llsd_reserve_binary( t1 );
					ret = fread( llsd->binary_.data, sizeof(uint8_t), t1, fin );
					ret = fread( &p, sizeof(uint8_t), 1, fin );
					llsd->binary_.encoding = LLSD_NONE;
					DEBUG( "%*sBINARY (%d)\n", indent, " ", llsd->binary_.data_size );
				}
				else if ( ( ret = fscanf( fin, "b%d\"", &i ) ) == 1 )
				{
					/* we have base16 or base64 encoded data */
					llsd = llsd_reserve_binary( 0 );
					llsd_read_until( &(llsd->binary_.enc), &(llsd->binary_.enc_size), '\"', fin );
					switch( i )
					{
						case 16:
							llsd->binary_.encoding = LLSD_BASE16;
							break;
						case 64:
							llsd->binary_.encoding = LLSD_BASE64;
							break;
						case 85:
							WARN( "illegal binary encoding for notation format\n" );
							llsd->binary_.encoding = LLSD_BASE85;
							break;
					}
					DEBUG( "%*sBINARY (%d)\n", indent, " ", llsd->binary_.data_size );
				}
				return llsd;

			case 's':
				t1 = 0;
				ret = fscanf( fin, "(%d)\"", &t1 );
				raw = TRUE;
				p = '\"'; /* for llsd_read_until */
			case '\"':
			case '\'':
				llsd = llsd_reserve_string( !raw, t1 ); /* allocates t1 bytes */
				if ( raw )
				{
					llsd_read_until( &(llsd->string_.str), &(llsd->string_.str_len), p, fin );
					llsd->string_.dyn_esc = FALSE;
					llsd->string_.dyn_str = TRUE;
					llsd->string_.raw = TRUE;
					llsd->string_.esc = NULL;
					llsd->string_.esc_len = 0;
				}
				else
				{
					llsd_read_until( &(llsd->string_.esc), &(llsd->string_.esc_len), p, fin );
					llsd->string_.dyn_esc = TRUE;
					llsd->string_.dyn_str = FALSE;
					llsd->string_.raw = FALSE;
					llsd->string_.str = NULL;
					llsd->string_.str_len = 0;

				}
				DEBUG( "%*sSTRING (%d)\n", indent, " ", llsd->string_.str_len );
				return llsd;

			case 'l':
				/* read the leading " */
				ret = fread( &p, sizeof(uint8_t), 1, fin );
				llsd = llsd_reserve_uri( 0 ); /* allocates t1 bytes */
				llsd_read_until( &(llsd->uri_.esc), &(llsd->uri_.esc_len), '\"', fin );
				llsd->uri_.dyn_esc = TRUE;
				llsd->uri_.dyn_uri = FALSE;
				llsd->uri_.uri = NULL;
				llsd->uri_.uri_len = 0;
				llsd_unescape_uri( llsd );
				DEBUG( "%*sURI (%d)\n", indent, " ", llsd->uri_.uri_len );
				return llsd;

			case 'd':
				/* read the leading " */
				ret = fread( &p, sizeof(uint8_t), 1, fin );
				llsd = llsd_new( LLSD_TYPE_INVALID );
				llsd->type_ = LLSD_DATE;
				llsd_read_until( &(llsd->date_.str), &(llsd->date_.len), '\"', fin );
				llsd->date_.dyn_str = TRUE;
				llsd->date_.use_dval = FALSE;
				llsd->date_.dval = 0.0;
				DEBUG( "%*sDATE (%s)\n", indent, " ", llsd->date_.str );
				return llsd;

			case '[':
				DEBUG( "%*s[[\n", indent, " " );
				indent += 4;
				llsd = llsd_reserve_array( 0 );
				do
				{
					ret = fread( &p, sizeof(uint8_t), 1, fin );
					childp = llsd_parse_notation( fin );
					if ( childp != NULL )
						llsd_array_append( llsd, childp );
				} while ( childp != NULL );
				indent -= 4;
				DEBUG( "%*s]] ARRAY\n", indent, " " );
				return llsd;

			case '{':
				ret = fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_map( t1 );
				DEBUG( "%*s{{ (%d)\n", indent, " ", t1 );
				indent += 4;
				for ( i = 0; i < t1; ++i )
				{
					/* parse the key */
					key = llsd_parse_notation( fin );
					if ( llsd_get_type( key ) != LLSD_STRING )
					{
						FAIL( "key is not an LLSD_STRING\n" );
					}

					/* parse and append the key/value pair */
					llsd_map_insert( llsd, key, llsd_parse_notation( fin ) );
				}

				ret = fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != '}' )
				{
					FAIL( "map didn't end with '}'\n" );
				}
				indent -= 4;
				DEBUG( "%*s}} MAP\n", indent, " " );

				return llsd;
		}
	}
}


size_t llsd_format_notation( llsd_t * llsd, FILE * fout, int pretty )
{
	static uint8_t tmp[64];
	int len = 0;
	size_t num = 0;
	unsigned long start = ftell( fout );
	uint8_t p;
	uint8_t const * str = NULL;
	uint32_t t1;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	uint16_t t3[UUID_LEN];

	llsd_itr_t itr;
	llsd_string_t s;
	llsd_t *k, *v;
	llsd_type_t t = llsd_get_type( llsd );

	switch ( t )
	{
		case LLSD_UNDEF:
			p = '!';
			num += indent_notation( pretty, fout );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			DEBUG( "%*sUNDEF %lu - %lu\n", indent * 4, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
		case LLSD_STRING:
		case LLSD_URI:
			s = llsd_as_string( llsd );
			num += indent_notation( pretty, fout );
			num += llsd_write_notation_start_tag( llsd, fout, pretty );
			num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
			num += llsd_write_notation_end_tag( llsd, fout, pretty );
			DEBUG( "%*s%s %lu - %lu\n", indent * 4, " ", llsd_get_type_string( t ), start, ftell( fout ) - 1 );
			return num;

		case LLSD_BINARY:
			/* handle the corner case of BASE85 encoding */
			if ( llsd->binary_.encoding == LLSD_BASE85 )
			{
				/* decode the binary */
				llsd_decode_binary( llsd );

				/* now free up the encoded binary data */
				FREE( llsd->binary_.enc );
				llsd->binary_.enc_size = 0;
				llsd->binary_.encoding = LLSD_NONE;
			}
			
			num += indent_notation( pretty, fout );

			num += llsd_write_notation_start_tag( llsd, fout, pretty );
			if ( (llsd->binary_.encoding == LLSD_RAW) || (llsd_get_size( llsd ) == 0) )
			{
				num += fwrite( llsd->binary_.data, sizeof(uint8_t), llsd->binary_.data_size, fout );
			}
			else
			{
				s = llsd_as_string( llsd );
				num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
			}
			num += llsd_write_notation_end_tag( llsd, fout, pretty );
			DEBUG( "%*s%s %lu - %lu\n", indent * 4, " ", llsd_get_type_string( t ), start, ftell( fout ) - 1 );
			return num;


		case LLSD_ARRAY:
		case LLSD_MAP:
			num += indent_notation( pretty, fout );
			num += llsd_write_notation_start_tag( llsd, fout, pretty );
			DEBUG( "%*s%s (%d)\n", indent * 4, " ", llsd_get_type_string( t ), llsd_get_size( llsd ) );
			indent++;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				num += indent_notation( pretty, fout );
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					ASSERT( llsd_get_type( k ) == LLSD_STRING );
					num += llsd_format_notation( v, fout, pretty );
					num += fwrite( ":", sizeof(uint8_t), 1, fout );
				}
				num += llsd_format_notation( v, fout, pretty );
				num += fwrite( ",\n", sizeof(uint8_t), (pretty ? 2 : 1), fout );
			}
			indent--;
			if ( llsd_get_size( llsd ) > 0 )
				num += indent_notation( pretty, fout );
			num += llsd_write_notation_end_tag( llsd, fout, pretty );
			DEBUG( "%*s%s (%d) %lu - %lu\n", indent * 4, " ", llsd_get_type_string( t ), llsd_get_size( llsd ), start, ftell( fout ) - 1 );
			break;
	}
	return num;
}

size_t llsd_get_notation_zero_copy_size( llsd_t * llsd, int pretty )
{
#if 0
	size_t s = 0;
	llsd_itr_t itr;
	llsd_t *k, *v;

	CHECK_PTR_RET( llsd, 0 );

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			return 1;
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
			return 2;
		case LLSD_STRING:
		case LLSD_URI:
		case LLSD_BINARY:
			return 3;
		case LLSD_ARRAY:
			s = 3; /* [, size, ] */
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
		case LLSD_MAP:
			s = 3; /* {, size, } */
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				s += llsd_get_binary_zero_copy_size( k );
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
	}
#endif
	return 0;
}


static uint8_t const * const lbundef	= "!";
static uint8_t const * const lbfalse	= "0";
static uint8_t const * const lbtrue		= "1";
static uint8_t const * const lbinteger	= "i";
static uint8_t const * const lbreal		= "r";
static uint8_t const * const lbuuid		= "u";
static uint8_t const * const lbstring	= "s";
static uint8_t const * const lbdate		= "d";
static uint8_t const * const lburi		= "l";
static uint8_t const * const lbbinary	= "b";
static uint8_t const * const lbarray	= "[";
static uint8_t const * const lbarrayc	= "]";
static uint8_t const * const lbmap		= "{";
static uint8_t const * const lbmapc		= "}";

size_t llsd_format_notation_zero_copy( llsd_t * llsd, struct iovec * v, int pretty )
{
#if 0
	size_t s = 0;
	union {
		uint64_t	ull;
		double		d;
	} t2;
	llsd_itr_t itr;
	llsd_t *key, *val;

	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
			v[0].iov_base = (void*)lbundef;
			v[0].iov_len = sizeof(uint8_t);
			return 1;
		case LLSD_BOOLEAN:
			v[0].iov_base = (void*)( llsd_as_bool( llsd ) ? lbtrue : lbfalse );
			v[0].iov_len = sizeof(uint8_t);
			return 1;
		case LLSD_INTEGER:
			v[0].iov_base = (void*)lbinteger;
			v[0].iov_len = sizeof(uint8_t);
			llsd->int_.be = htonl( llsd->int_.v );
			v[1].iov_base = &(llsd->int_.be);
			v[1].iov_len = sizeof(uint32_t);
			return 2;
		case LLSD_REAL:
			v[0].iov_base = (void*)lbreal;
			v[0].iov_len = sizeof(uint8_t);
			t2.d = llsd->real_.v;
			llsd->real_.be = htobe64( t2.ull );
			v[1].iov_base = &(llsd->real_.be);
			v[1].iov_len = sizeof(uint64_t);
			return 2;
		case LLSD_UUID:
			v[0].iov_base = (void*)lbuuid;
			v[0].iov_len = sizeof(uint8_t);
			llsd_destringify_uuid( llsd );
			v[1].iov_base = llsd->uuid_.bits;
			v[1].iov_len = UUID_LEN;
			return 2;
		case LLSD_STRING:
			v[0].iov_base = (void*)lbstring;
			v[0].iov_len = sizeof(uint8_t);
			llsd_unescape_string( llsd );
			llsd->string_.be = htonl( llsd->string_.str_len );
			v[1].iov_base = &(llsd->string_.be);
			v[1].iov_len = sizeof(uint32_t);
			v[2].iov_base = llsd->string_.str;
			v[2].iov_len = llsd->string_.str_len;
			return 3;
		case LLSD_DATE:
			v[0].iov_base = (void*)lbdate;
			v[0].iov_len = sizeof(uint8_t);
			llsd_destringify_date( llsd );
			llsd->date_.be = htobe64( (uint64_t)llsd->date_.dval );
			v[1].iov_base = &(llsd->date_.dval);
			v[1].iov_len = sizeof(uint64_t);
			return 2;
		case LLSD_URI:
			v[0].iov_base = (void*)lburi;
			v[0].iov_len = sizeof(uint8_t);
			llsd->uri_.be = htonl( llsd->uri_.uri_len );
			v[1].iov_base = &(llsd->uri_.be);
			v[1].iov_len = sizeof(uint32_t);
			v[2].iov_base = llsd->uri_.uri;
			v[2].iov_len = llsd->uri_.uri_len;
			return 3;
		case LLSD_BINARY:
			v[0].iov_base = (void*)lbbinary;
			v[0].iov_len = sizeof(uint8_t);
			llsd_decode_binary( llsd );
			llsd->binary_.be = htonl( llsd->binary_.data_size );
			v[1].iov_base = &(llsd->binary_.be);
			v[1].iov_len = sizeof(uint32_t);
			v[2].iov_base = llsd->binary_.data;
			v[2].iov_len = llsd->binary_.data_size;
			return 3;
		case LLSD_ARRAY:
			v[0].iov_base = (void*)lbarray;
			v[0].iov_len = sizeof(uint8_t);
			llsd->array_.be = htonl( array_size( &(llsd->array_.array) ) );
			v[1].iov_base = &(llsd->array_.be);
			v[1].iov_len = sizeof(uint32_t);
			s = 2;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &val, &key );
				if ( key != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				s += llsd_format_binary_zero_copy( val, &(v[s]) );
			}
			v[s].iov_base = (void*)lbarrayc;
			v[s].iov_len = sizeof(uint8_t);
			s++;
			return s;
		case LLSD_MAP:
			v[0].iov_base = (void*)lbmap;
			v[0].iov_len = sizeof(uint8_t);
			llsd->map_.be = htonl( ht_size( &(llsd->map_.ht) ));
			v[1].iov_base = &(llsd->map_.be);
			v[1].iov_len = sizeof(uint32_t);
			s = 2;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &val, &key );
				s += llsd_format_binary_zero_copy( key, &(v[s]) );
				s += llsd_format_binary_zero_copy( val, &(v[s]) );
			}
			v[s].iov_base = (void*)lbmapc;
			v[s].iov_len = sizeof(uint8_t);
			s++;
			return s;
	}
#endif
	return 0;
}

