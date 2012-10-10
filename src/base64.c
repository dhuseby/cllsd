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

#include "base64.h"


static int encode_quartet( uint8_t const * in, uint32_t inlen, uint8_t * out )
{
	static const char b64str[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	CHECK_PTR_RET( in, FALSE);
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );

	out[0] = b64str[((in[0] & 0xfc) >> 2)];

	switch( inlen )
	{
		case 1:
			out[1] = b64str[((in[0] & 0x03) << 4)];
			out[2] = '=';
			out[3] = '=';
			break;
		case 2:
			out[1] = b64str[(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
			out[2] = b64str[((in[1] & 0x0f) << 2)];
			out[3] = '=';
			break;
		case 3:
			out[1] = b64str[(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
			out[2] = b64str[(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6))];
			out[3] = b64str[(in[2] & 0x3f)];
			break;
	}
	return TRUE;
}

static int in_range( uint8_t ch )
{
	return ( ((ch >= 'A') && (ch <= 'Z')) ||
			 ((ch >= 'a') && (ch <= 'z')) ||
			 ((ch >= '0') && (ch <= '9')) ||
			 (ch == '+') ||
			 (ch == '/') ||
			 (ch == '=') );
}

static uint8_t to_idx( uint8_t ch )
{
	if ( (ch >= 'A') && (ch <= 'Z') )
		return (ch - 'A');
	else if ( (ch >= 'a') && (ch <= 'z') )
		return ( 26 + (ch - 'a') );
	else if ( (ch >= '0') && (ch <= '9') )
		return ( 52 + (ch - '0') );
	else if ( ch == '+' )
		return 62;
	else if ( ch == '/' )
		return 63;
	return 0;
}

static int decode_quartet( uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen )
{
	int i = 0;
	int o = 0;
	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen == 4, FALSE );
	CHECK_PTR_RET( out, FALSE );
	CHECK_RET( in_range( in[i] ), FALSE );
	CHECK_RET( in_range( in[i+1] ), FALSE );
	CHECK_RET( in_range( in[i+2] ), FALSE );
	CHECK_RET( in_range( in[i+3] ), FALSE );

	/* decode four base64 characters into three bytes */
	out[o] = ( (to_idx(in[i]) << 2) | ((to_idx(in[i+1]) & 0x30) >> 4) );
	if ( in[i+2] != '=')
	{
		o++;
		out[o] = ((to_idx(in[i+1]) & 0x0f) << 4);
		out[o] |= ((to_idx(in[i+2]) & 0x3c) >> 2);
	}
	if ( in[i+3] != '=')
	{
		o++;
		out[o] = ((to_idx(in[i+2]) & 0x03) << 6);
		out[o] |= (to_idx(in[i+3]) & 0x3f);
	}

	if ( outlen != NULL )
		(*outlen) = (o + 1);

	return TRUE;
}

int base64_encode( uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	int ret;
	int i = 0;
	int o = 0;
	uint32_t ilen;

	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );
	CHECK_PTR_RET( outlen, FALSE );
	CHECK_RET( (*outlen) > 0, FALSE );

	while( i < inlen )
	{
		if ( (o + 4) > (*outlen) )
			break;

		/* figure out how many bytes go into this quintet */
		ilen = ((inlen - i) < 3) ? (inlen - i) : 3;

		/* encode three bytes as four base64 chars */
		ret = encode_quartet( &(in[i]), ilen, &(out[o]) );

		CHECK_RET( ret == TRUE, FALSE );

		/* move the indexes */
		i += ilen;
		o += 4;
	}

	return TRUE;
}

int base64_decode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen)
{
	int ret;
	int i = 0;
	int o = 0;
	uint32_t olen = 0;

	CHECK_PTR_RET( in, FALSE );
	CHECK_RET( inlen > 0, FALSE );
	CHECK_PTR_RET( out, FALSE );
	CHECK_PTR_RET( outlen, FALSE );
	CHECK_RET( (*outlen) > 0, FALSE );

	while ( i < inlen )
	{
		ret = decode_quartet( &(in[i]), 4, &(out[o]), &olen );
		CHECK_RET( ret != -1, FALSE );

		/* move the indexes */
		i += 4;
		o += olen;
	}

	if ( outlen != NULL )
		(*outlen) = o;

	return TRUE;
}

uint32_t base64_decoded_len( uint8_t const * in, uint32_t inlen )
{
	uint32_t len = 0;
	CHECK_PTR_RET( in, 0 );
	
	len = 3 * (inlen / 4);
	if ( in[ inlen - 1 ] == '=' )
		len--;
	if ( in[ inlen - 2 ] == '=' )
		len--;
	return len;
}


