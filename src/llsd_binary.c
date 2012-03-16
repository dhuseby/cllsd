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

#include <stdint.h>
#include <assert.h>
#include <endian.h>

#include "debug.h"
#include "macros.h"
#include "llsd_const.h"
#include "llsd_binary.h"
#include "array.h"
#include "hashtable.h"

#define INDENT(f, x, y) (( x ) ? fprintf( f, "%*s", y, " " ) : 0)

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
			llsd_destringify_date( llsd );
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


const llsd_type_t llsd_binary_types[UINT8_MAX + 1] =
{
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_UNDEF,				/* '!' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_BOOLEAN_FALSE,		/* '0' */
	LLSD_BOOLEAN_TRUE,		/* '1' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_ARRAY,				/* '[' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_BINARY,			/* 'b' */
	LLSD_TYPE_INVALID,
	LLSD_DATE,				/* 'd' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_INTEGER,			/* 'i' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_URI,				/* 'l' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_REAL,				/* 'r' */
	LLSD_STRING,			/* 's' */
	LLSD_TYPE_INVALID,
	LLSD_UUID,				/* 'u' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_MAP,				/* '{' */
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
	LLSD_TYPE_INVALID,
};

const uint8_t llsd_binary_bytes[LLSD_TYPE_COUNT] =
{
	'!',	/* LLSD_UNDEF */
	'1',	/* LLSD_BOOLEAN_TRUE */
	'0',	/* LLSD_BOOLEAN_FALSE */
	'i',	/* LLSD_INTEGER */
	'r',	/* LLSD_REAL */
	'u',	/* LLSD_UUID */
	's',	/* LLSD_STRING */
	'd',	/* LLSD_DATE */
	'l',	/* LLSD_URI */
	'b',	/* LLSD_BINARY */
	'[',	/* LLSD_ARRAY */
	'{'		/* LLSD_MAP */
};


