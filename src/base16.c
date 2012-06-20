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
#include <ctype.h>

#include <cutil/debug.h>
#include <cutil/macros.h>

#include "base16.h"

uint32_t base16_decoded_len( uint8_t const * in, uint32_t inlen )
{
	CHECK_PTR_RET( in, 0 );
	return (inlen / 2);
}

int base16_encode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	static uint8_t const b16str[16] = "0123456789ABCDEF";
	int i;
	int o = 0;

	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );
	CHECK_RET( (*outlen) > 0, FALSE );

	for ( i = 0; i < inlen; i++ )
	{
		if ( o >= ((*outlen) - 1) )
			break;

		out[o++] = b16str[ ((in[i] & 0xF0) >> 4) ];
		out[o++] = b16str[ (in[i] & 0x0F) ];
	}

	/* return how many bytes we created */
	(*outlen) = o;

	return TRUE;
}

static int is_base16( uint8_t c )
{
	return isxdigit((int)c);
}

static uint8_t dhex( uint8_t c )
{
	if ( (c >= 0x30) && (c <= 0x39) )		/* '0' <= c <= '9' */
	{
		return (c - 0x30);
	}
	else if ( (c >= 0x41) && (c <= 0x46) )	/* 'A' <= c <= 'F' */
	{
		return ( (c - 0x41) + 10 );
	}
	else if ( (c >= 0x61) && (c <= 0x66) )	/* 'a' <= c <= 'f' */
	{
		return ( (c - 0x61) + 10 );
	}
	
	FAIL( "invalid base16 character\n" );
}

int base16_decode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	int i = 0;
	int o = 0;

	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );

	while( i < (inlen - 1) )
	{
		if ( !is_base16(in[i]) || !is_base16(in[i+1]) )
		{
			return FALSE;
		}
	
		out[o] = ( (dhex(in[i]) << 4) | (dhex(in[i+1])) );
		
		i += 2;
		o++;
	}

	if ( outlen != NULL )
		*(outlen) = o;

	return TRUE;
}


