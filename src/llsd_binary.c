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

static int indent = 0;

size_t llsd_parse_binary_in_place( llsd_bin_t * lb, uint8_t * buf, size_t len )
{
	uint32_t *pi;
	uint64_t *ui;
	llsd_bin_t * p = NULL;
	size_t s = 0;
	int i;

	CHECK_PTR_RET( lb, 0 );
	CHECK_PTR_RET( buf, 0 );

	switch ( buf[0] )
	{
		default:
			WARN( "invalid type character %c\n", buf[0] );
			break;
		case '!':
		case '1':
		case '0':
			WARN( "%*s%s\n", indent, " ", TYPE_TO_STRING( BYTE_TO_TYPE( buf[0] ) ) );
			(*lb) = 
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf, 
					.tlen = sizeof(uint8_t) 
				},
				.i = 
				(llsd_bin_int_t) {
					.v = NULL,
					.vlen = 0
				}
			};
			return 1;
		case 'i':
			WARN( "%*sINTEGER\n", indent, " " );
			pi = (uint32_t*)&(buf[1]);
			(*pi) = ntohl( (*pi) );
			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.i =
				(llsd_bin_int_t) {
					.v = pi,
					.vlen = sizeof(uint32_t)
				}
			};
			return 5;
		case 'r':
		case 'd':
			WARN( "%*s%s\n", indent, " ", TYPE_TO_STRING( BYTE_TO_TYPE( buf[0] ) ) );
			ui = (uint64_t*)&(buf[1]);
			(*ui) = be64toh( (*ui) );
			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.d =
				(llsd_bin_double_t) {
					.v = (double*)ui,
					.vlen = sizeof(double)
				}
			};
			return 9;
		case 'u':
			WARN( "%*sUUID\n", indent, " " );
			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.b =
				(llsd_bin_bytes_t) {
					.v = &(buf[1]),
					.vlen = sizeof(uint32_t)
				}
			};
			return 17;
		case 'b':
		case 's':
		case 'l':
			WARN( "%*s%s\n", indent, " ", TYPE_TO_STRING( BYTE_TO_TYPE( buf[0] ) ) );
			pi = (uint32_t*)&(buf[1]);
			(*pi) = ntohl( (*pi) );
			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.b =
				(llsd_bin_bytes_t) {
					.v = &(buf[5]),
					.vlen = (*pi)
				}
			};
			return (5 + (*pi));
		case '[':
			pi = (uint32_t*)&(buf[1]);
			(*pi) = ntohl( (*pi) );
			WARN( "%*s%s (%d)\n", indent, " ", TYPE_TO_STRING( BYTE_TO_TYPE( buf[0] ) ), (*pi) );
	
			/* allocate a buffer for the llsd_bin_t structs for the array children */
			p = (llsd_bin_t*)CALLOC( (*pi), sizeof(llsd_bin_t) );
			CHECK_PTR_RET_MSG( p, 0, "failed to allocate buffer for array children\n" );
			
			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.a =
				(llsd_bin_array_t) {
					.v = NULL,
					.vlen = ((*pi) * sizeof(llsd_bin_t))
				}
			};

			s = 5;
			for ( i = 0; i < (*pi); i++ )
			{
				/* parse each child */
				s += llsd_parse_binary_in_place( &(p[i]), &(buf[s]), (len - s) );
			}

			/* TODO: initialize array so that it takes ownership of the llsd_bin_t
			 * buffer pointed at by p */

			CHECK_RET_MSG( buf[s] == ']', 0, "missing trailing ']' for array\n" );

			return s + 1; /* account for the closing ']' character */
		case '{':
			pi = (uint32_t*)&(buf[1]);
			(*pi) = ntohl( (*pi) );
			WARN( "%*s%s (%d)\n", indent, " ", TYPE_TO_STRING( BYTE_TO_TYPE( buf[0] ) ), (*pi) );
	
			/* allocate a buffer for the llsd_bin_t structs for the array children */
			p = (llsd_bin_t*)CALLOC( ((*pi) * 2), sizeof(llsd_bin_t) );
			CHECK_PTR_RET_MSG( p, 0, "failed to allocate buffer for map children\n" );

			(*lb) =
			(llsd_bin_t) {
				.t = 
				(llsd_bin_type_t) { 
					.t = buf,
					.tlen = sizeof(uint8_t)
				},
				.m =
				(llsd_bin_map_t) {
					.v = NULL,
					.vlen = (((*pi) * 2) * sizeof(llsd_bin_t))
				}
			};

			s = 5;
			for ( i = 0; i < ((*pi) * 2); i++ )
			{
				/* parse each child */
				s += llsd_parse_binary_in_place( &(p[i]), &(buf[s]), (len - s) );
			}

			/* TODO: initialize map so that it takes ownership of the llsd_bin_t
			 * buffer pointed at by p */

			CHECK_RET_MSG( buf[s] == '}', 0, "missing trailing '}' for array\n" );

			return s + 1; /* account for the closing '}' character */
	}
}


llsd_bin_t * llsd_parse_binary( uint8_t * buf, size_t len )
{
	llsd_bin_t * root = NULL;
	size_t s = 0;

	CHECK_PTR_RET( buf, NULL );
	CHECK_RET( len > 0, NULL );

	/* allocate room for one llsd_bin_t */
	root = (llsd_bin_t*)CALLOC( 1, sizeof(llsd_bin_t) );

	s = llsd_parse_binary_in_place( root, buf, len );

	if ( s != len )
	{
		WARN("failed to parse the full buffer\n");
	}

	return root;
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


