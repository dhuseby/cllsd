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

static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	if ( size > 0 )
	{
		llsd->binary_.data_size = size;
		llsd->binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_string( int escaped, uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_STRING;
	if ( size > 0 )
	{
		if ( escaped )
		{
			llsd->string_.esc_len = size;
			llsd->string_.esc = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
		}
		else
		{
			llsd->string_.str_len = size;
			llsd->string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
		}
	}
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_URI;
	if ( size > 0 )
	{
		llsd->uri_.uri_len = size;
		llsd->uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	}
	return llsd;
}

static llsd_t * llsd_reserve_date( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_DATE;
	llsd->date_.use_dval = TRUE;
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

#define INDENT(f, x, y) (( x ) ? fprintf( f, "%*s", y, " " ) : 0)
static int indent = 0;

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
				ret = fscanf( fin, "%36s", UUID_STR_LEN, llsd->uuid_.str );
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
					DEBUG( "%*sBINARY (%d)\n", indent, " ", llsd->binary_.data_size );
				}
				else if ( ( ret = fscanf( fin, "b%d\"", &i ) ) == 1 )
				{
					/* we have base16 or base64 encoded data */
					llsd = llsd_reserve_binary( 0 );
					llsd_read_until( &(llsd->binary_.enc), &(llsd->binary_.enc_size), '\"', fin );
					DEBUG( "%*sBINARY (%d)\n", indent, " ", llsd->binary_.data_size );
				}
				return llsd;

			case 's':
				t1 = 0;
				ret = fscanf( fin, "(%d)\"", &t1 ) 
				raw = TRUE;
			case '\"':
				p = '\"';
			case '\'':
				llsd = llsd_reserve_string( !raw, t1 ); /* allocates t1 bytes */
				if ( raw )
				{
					llsd_read_until( &(llsd->string_.str), &(llsd->string_.str_len), p, fin );
					llsd->string_.dyn_esc = FALSE;
					llsd->string_.dyn_str = TRUE;
					llsd->string_.esc = NULL;
					llsd->string_.esc_len = 0;
				}
				else
				{
					llsd_read_until( &(llsd->string_.esc), &(llsd->string_.esc_len), p, fin );
					llsd->string_.dyn_esc = TRUE;
					llsd->string_.dyn_str = FALSE;
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
					llsd = llsd_parse_notation( fin );
					if ( llsd != NULL )
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
					key = llsd_parse_binary( fin );
					if ( llsd_get_type( key ) != LLSD_STRING )
					{
						FAIL( "key is not an LLSD_STRING\n" );
					}

					/* parse and append the key/value pair */
					llsd_map_insert( llsd, key, llsd_parse_binary( fin ) );
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


size_t llsd_format_binary( llsd_t * llsd, FILE * fout )
{
	size_t num = 0;
	unsigned long start = ftell( fout );
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
			DEBUG( "%*sUNDEF %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
			p = ( llsd_as_bool( llsd ) == TRUE ) ? '1' : '0';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			DEBUG( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_INTEGER:
			p = 'i';
			llsd->int_.be = htonl( llsd->int_.v );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &(llsd->int_.be), sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			DEBUG( "%*sINTEGER %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_REAL:
			p = 'r';
			t2.d = llsd->real_.v;
			llsd->real_.be = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &(llsd->real_.be), sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			DEBUG( "%*sREAL %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_UUID:
			p = 'u';
			/* de-stringify the uuid if needed */
			llsd_destringify_uuid( llsd );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += fwrite( &(llsd->uuid_.bits[0]), sizeof(uint8_t), UUID_LEN, fout );
			DEBUG( "%*sUUID %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_STRING:
			p = 's';
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			s = llsd->string_.str_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->string_.str, sizeof(uint8_t), s, fout );
			DEBUG( "%*sSTRING %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_DATE:
			p = 'd';
			/* de-stringify date if needed */
			if ( !llsd->date_.use_dval )
			{
				WARN("found date with FALSE use_dval\n");
			}
			llsd_destringify_date( llsd );
			if ( !llsd->date_.use_dval )
			{
				WARN("found date with FALSE use_dval\n");
			}
			t2.d = llsd->date_.dval;
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			DEBUG( "%*sDATE %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_URI:
			p = 'l';
			/* unescape uri if needed */
			llsd_unescape_uri( llsd );
			s = llsd->uri_.uri_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->uri_.uri, sizeof(uint8_t), s, fout );
			DEBUG( "%*sURI %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BINARY:
			p = 'b';
			/* decode binary if needed */
			llsd_decode_binary( llsd );
			s = llsd->binary_.data_size;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->binary_.data, sizeof(uint8_t), s, fout );
			DEBUG( "%*sBINARY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_ARRAY:
			p = '[';
			s = array_size( &(llsd->array_.array) );
			DEBUG( "%*s[[ (%d)\n", indent, " ", s );
			indent += 4;
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
			indent -= 4;
			DEBUG( "%*s]] ARRAY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_MAP:
			p = '{';
			s = ht_size( &(llsd->map_.ht) );
			DEBUG( "%*s{{ (%d)\n", indent, " ", s );
			indent += 4;
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
			indent -= 4;
			DEBUG( "%*s}} MAP %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
	}
	return num;
}

size_t llsd_get_binary_zero_copy_size( llsd_t * llsd )
{
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

size_t llsd_format_binary_zero_copy( llsd_t * llsd, struct iovec * v )
{
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
	return 0;
}


