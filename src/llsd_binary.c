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
#include <endian.h>

#include "debug.h"
#include "macros.h"
#include "llsd.h"
#include "llsd_util.h"
#include "llsd_const.h"
#include "llsd_binary.h"
#include "array.h"
#include "hashtable.h"

static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	llsd->value.binary_.data_size = size;
	llsd->value.binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_string( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_STRING;
	llsd->value.string_.str_len = size;
	llsd->value.string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_URI;
	llsd->value.uri_.uri_len = size;
	llsd->value.uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_date( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_DATE;
	llsd->value.date_.use_dval = TRUE;
}

static llsd_t * llsd_reserve_array( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_ARRAY;
	array_initialize( &(llsd->value.array_.array), size, &llsd_delete );
	return llsd;
}

static llsd_t * llsd_reserve_map( uint32_t size )
{
	llsd_t * llsd = (llsd_t *)CALLOC( 1, sizeof(llsd_t) );
	llsd->type_ = LLSD_MAP;
	ht_initialize( &(llsd->value.map_.ht), 
				   size, 
				   &fnv_key_hash, 
				   &llsd_delete, 
				   &key_eq, 
				   &llsd_delete );
	return llsd;
}

#define INDENT(f, x, y) (( x ) ? fprintf( f, "%*s", y, " " ) : 0)

llsd_t * llsd_parse_binary( FILE * fin )
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
				llsd = llsd_reserve_string( t1 ); /* allocates t1 bytes */
				fread( llsd->value.string_.str, sizeof(uint8_t), t1, fin );
				return llsd;

			case 'l':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_uri( t1 ); /* allocates t1 bytes */
				fread( llsd->value.uri_.uri, sizeof(uint8_t), t1, fin );
				return llsd;

			case 'd':
				fread( &t2, sizeof(double), 1, fin );
				t2.ull = be64toh( t2.ull );
				return llsd_new_date( t2.d, NULL, 0 );

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

static int indent = 0;

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
			WARN( "%*sUNDEF %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
			p = ( llsd_as_bool( llsd ) == TRUE ) ? '1' : '0';
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			WARN( "%*sBOOLEAN %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_INTEGER:
			p = 'i';
			t1 = htonl( llsd_as_int( llsd ) );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			WARN( "%*sINTEGER %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_REAL:
			p = 'r';
			t2.d = llsd_as_real( llsd );
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			WARN( "%*sREAL %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_UUID:
			p = 'u';
			/* de-stringify the uuid if needed */
			llsd_destringify_uuid( llsd );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += fwrite( &(llsd->value.uuid_.bits[0]), sizeof(uint8_t), UUID_LEN, fout );
			WARN( "%*sUUID %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_STRING:
			p = 's';
			/* unescape the string if needed */
			llsd_unescape_string( llsd );
			s = llsd->value.string_.str_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd_as_string( llsd ).str, sizeof(uint8_t), s, fout );
			WARN( "%*sSTRING %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_DATE:
			p = 'd';
			/* de-stringify date if needed */
			if ( !llsd->value.date_.use_dval )
			{
				WARN("found date with FALSE use_dval\n");
			}
			llsd_destringify_date( llsd );
			if ( !llsd->value.date_.use_dval )
			{
				WARN("found date with FALSE use_dval\n");
			}
			t2.d = llsd->value.date_.dval;
			t2.ull = htobe64( t2.ull );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t2, sizeof(uint64_t), 1, fout ) * sizeof(uint64_t) );
			WARN( "%*sDATE %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_URI:
			p = 'l';
			/* unescape uri if needed */
			llsd_unescape_uri( llsd );
			s = llsd->value.uri_.uri_len;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.uri_.uri, sizeof(uint8_t), s, fout );
			WARN( "%*sURI %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_BINARY:
			p = 'b';
			/* decode binary if needed */
			llsd_decode_binary( llsd );
			s = llsd->value.binary_.data_size;
			t1 = htonl( s );
			num += fwrite( &p, sizeof(uint8_t), 1, fout );
			num += ( fwrite( &t1, sizeof(uint32_t), 1, fout ) * sizeof(uint32_t) );
			num += fwrite( llsd->value.binary_.data, sizeof(uint8_t), s, fout );
			WARN( "%*sBINARY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_ARRAY:
			p = '[';
			s = array_size( &(llsd->value.array_.array) );
			WARN( "%*s[[ (%d)\n", indent, " ", s );
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
			WARN( "%*s]] ARRAY %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
		case LLSD_MAP:
			p = '{';
			s = ht_size( &(llsd->value.map_.ht) );
			WARN( "%*s{{ (%d)\n", indent, " ", s );
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
			WARN( "%*s}} MAP %lu - %lu\n", indent, " ", start, ftell( fout ) - 1 );
			break;
	}
	return num;
}

