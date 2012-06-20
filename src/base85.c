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

#include <cutil/debug.h>
#include <cutil/macros.h>

#include "base85.h"

static uint32_t eightyfives[5] = { 1, 85, 7225, 614125, 52200625 };

static int in_range( uint8_t ch )
{
	return ((ch >= 0x21) && (ch <= 0x75));
}

static int decode_quintet( uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t padding )
{
	int i;
	uint64_t tmp = 0ULL;
	union {
		uint32_t beval;
		uint8_t cval[4];
	} v;

	CHECK_PTR_RET( in, -1 );
	CHECK_PTR_RET( inlen > 0, -1 );
	CHECK_RET( ((padding >= 0) && (padding < 4)), -1 );

	for ( i = (inlen - 1); i >= 0; i-- )
	{
		CHECK_RET( in_range(in[i]) == TRUE, -1 );
		tmp += (eightyfives[4-i] * (in[i] - 33));
	}

	CHECK_RET( tmp <= 0x00000000FFFFFFFFULL, -1 );

	/* make the 32-bit number big endian */
	v.beval = htonl( (uint32_t)tmp );

	for ( i = 0; i < (4 - padding); i++ )
	{
		out[i] = v.cval[i];
	}

	return (4 - padding);
}

static int encode_quintet( uint8_t const * in, uint32_t inlen, uint8_t * out )
{
	int i;
	uint8_t tmp;
	uint32_t hval;
	union {
		uint32_t beval;
		uint8_t cval[4];
	} v;

	CHECK_PTR_RET( in, -1 );
	CHECK_RET( inlen > 0, -1 );

	v.beval = 0;
	switch ( inlen )
	{
		case 4: v.cval[3] = in[3];
		case 3: v.cval[2] = in[2];
		case 2: v.cval[1] = in[1];
		case 1: v.cval[0] = in[0];
	}

	if ( v.beval == 0x00000000 )
	{
		/* special case for all zeros */
		out[0] = 'z';
		return 1;
	}
	else if (v.beval == 0x20202020 )
	{
		/* special case for four spaces */
		out[0] = 'y';
		return 1;
	}
	else
	{
		/* convert the big endian value to host order for the math */
		hval = ntohl( v.beval );

		for ( i = 4; i >= 0; i-- )
		{
			if ( i < 4 )
				hval /= 85;
			tmp = (hval % 85);
			hval -= tmp;
			if ( out != NULL )
				out[i] = (tmp + 33); /* base is at '!' (ASCII 33) */
		}

		return (1 + inlen);
	}
	return -1;
}

int base85_encode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	int ret;
	int i = 0;
	int o = 0;
	uint32_t ilen;
	uint8_t buf[5];

	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );
	CHECK_PTR_RET( outlen, FALSE );
	CHECK_RET( (*outlen) > 0, FALSE );

	while ( i < inlen )
	{
		if ( o >= (*outlen) )
			break;

		/* figure out how many bytes go into this quintet */
		ilen = ((inlen - i) < 4) ? (inlen - i) : 4;

		MEMSET( buf, 0, (5 * sizeof(uint8_t)) );
		ret = encode_quintet( &(in[i]), ilen, buf );
		CHECK_RET( ret != -1, FALSE );

		/* copy the bytes over */
		MEMCPY( &(out[o]), buf, ret );

		/* move the indexes */
		i += ilen;
		o += ret;
	}

	(*outlen) = o;

	return TRUE;
}

int base85_decode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	int i = 0;;
	int j = 0;
	int o = 0;
	uint8_t buf[5];

	while( i < inlen )
	{
		if ( in[i] == 'z')
		{
			/* 32-bit zero value short-hand */
			CHECK_RET( j == 0, FALSE );
			out[o++] = 0x00;
			out[o++] = 0x00;
			out[o++] = 0x00;
			out[o++] = 0x00;
			i++;
		}
		else if ( in[i] == 'y' )
		{
			/* four space characters */
			CHECK_RET( j == 0, FALSE );
			out[o++] = 0x20;
			out[o++] = 0x20;
			out[o++] = 0x20;
			out[o++] = 0x20;
			i++;
		}
		else
		{
			if ( in_range( in[i] ) )
			{
				buf[j] = in[i];
				j++;
			}
			if ( j == 5 )
			{
				/* decode 5 ascii letters to 4 8-bit bytes */
				o += decode_quintet( buf, 5, &(out[o]), 0 );
				j = 0;
			}
			i++;
		}
	}

	if ( j > 0 )
	{
		switch( j )
		{
		case 1: buf[1] = 'u';
		case 2: buf[2] = 'u';
		case 3: buf[3] = 'u';
		case 4: buf[4] = 'u';
		}
	}

	o += decode_quintet( buf, 5, &(out[o]), (5 - j) );

	if ( outlen != NULL )
		*(outlen) = o;

	return TRUE;
}

uint32_t base85_decoded_len( uint8_t const * in, uint32_t inlen )
{
	int i = 0;
	int j = 0;
	uint32_t outlen = 0;
	uint8_t end[5];
	uint8_t out[4];

	while ( i < inlen )
	{
		switch( in[i] )
		{
			case 'z':
			{
				/* 32-bit zero value short-hand */
				outlen += 4;
				i++;
				if ( j > 0 )
					FAIL("invalid placement of 'z'\n");
				break;
			}
			case 'y':
			{
				/* four space characters */
				outlen += 4;
				i++;
				if ( j > 0 )
					FAIL("invalid placement of 'y'\n");
				break;
			}
			default:
			{
				if ( in_range( in[i] ) )
				{
					end[j] = in[i];
					j++;
				}
				if ( j == 5 )
				{
					outlen += 4;
					j = 0;
				}
				i++;
			}
		}
	}

	if ( j > 0 )
	{
		switch( j )
		{
		case 1: end[1] = 'u';
		case 2: end[2] = 'u';
		case 3: end[3] = 'u';
		case 4: end[4] = 'u';
		}
	}

	outlen += decode_quintet( end, 5, out, (5 - j) );

	return outlen;
}

