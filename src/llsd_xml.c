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
#include <stdlib.h>
#include "llsd_const.h"
#include "llsd_xml.h"

int llsd_stringify_uuid( llsd_t * llsd )
{
	uint8_t * p;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_UUID), FALSE );
	CHECK_RET( (llsd->value.uuid_.len == 0), FALSE );
	CHECK_RET( (llsd->value.uuid_.str == NULL), FALSE );
	CHECK_PTR_RET( llsd->value.uuid_.bits, FALSE );

	/* allocate the string */
	llsd->value.uuid_.str = UT(CALLOC( UUID_STR_LEN, sizeof(uint8_t) ));
	llsd->value.uuid_.dyn_str = TRUE;
	llsd->value.uuid_.len = UUID_STR_LEN;

	/* convert to string and cache it */
	p = llsd->value.uuid_.bits;
	snprintf( llsd->value.uuid_.str, UUID_STR_LEN, 
			  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			  p[0], p[1], p[2], p[3], 
			  p[4], p[5], 
			  p[6], p[7], 
			  p[8], p[9], 
			  p[10], p[11], p[12], p[13], p[14], p[15] );

	return TRUE;
}

/* A valid 8-4-4-4-12 string UUID is converted to a binary UUID */
int llsd_destringify_uuid( llsd_t * llsd )
{
	int i;
	uint8_t * pin;
	uint8_t * pout;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_UUID), FALSE );
	CHECK_RET( (llsd->value.uuid_.len == UUID_STR_LEN), FALSE );
	CHECK_PTR_RET( llsd->value.uuid_.str, FALSE );
	CHECK_RET( (llsd->value.uuid_.bits == NULL), FALSE );

	/* check for 8-4-4-4-12 */
	for ( i = 0; i < 36; i++ )
	{
		if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
			 ( (i >= 9) && (i < 13) ) &&    /* 4 */
			 ( (i >= 14) && (i < 18) ) &&   /* 4 */
			 ( (i >= 19) && (i < 23) ) &&	/* 4 */
			 ( (i >= 24) && (i < 36) ) )	/* 12 */
		{
			if ( !isxdigit( llsd->value.uuid_.str[i] ) )
			{
				return FALSE;
			}
		}
		else if ( llsd->value.uuid_.str[i] != '-' )
		{
			return FALSE;
		}
	}

	/* allocate the bits */
	llsd->value.uuid_.bits = UT(CALLOC( UUID_LEN, sizeof(uint8_t) ));
	llsd->value.uuid_.dyn_bits = TRUE;

	/* covert to UUID */
	pin = llsd->value.uuid_.str;
	pout = llsd->value.uuid_.bits;

	/* 8 */
	pout[0] = hex_to_byte( pin[0], pin[1] );
	pout[1] = hex_to_byte( pin[2], pin[3] );
	pout[2] = hex_to_byte( pin[4], pin[5] );
	pout[3] = hex_to_byte( pin[6], pin[7] );

	/* 4 */
	pout[4] = hex_to_byte( pin[9], pin[10] );
	pout[5] = hex_to_byte( pin[11], pin[12] );

	/* 4 */
	pout[6] = hex_to_byte( pin[14], pin[15] );
	pout[7] = hex_to_byte( pin[16], pin[17] );

	/* 4 */
	pout[8] = hex_to_byte( pin[19], pin[20] );
	pout[9] = hex_to_byte( pin[21], pin[22] );

	/* 12 */
	pout[10] = hex_to_byte( pin[24], pin[25] );
	pout[11] = hex_to_byte( pin[26], pin[27] );
	pout[12] = hex_to_byte( pin[28], pin[29] );
	pout[13] = hex_to_byte( pin[30], pin[31] );
	pout[14] = hex_to_byte( pin[32], pin[33] );
	pout[15] = hex_to_byte( pin[34], pin[35] );

	return TRUE;
}

int llsd_stringify_date( llsd_t * llsd )
{
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_DATE), FALSE );
	CHECK_RET( (llsd->value.date_.len == 0), FALSE );
	CHECK_RET( (llsd->value.date_.str == NULL), FALSE );

	/* allocate the string */
	llsd->value.date_.str = UT(CALLOC( DATE_STR_LEN, sizeof(uint8_t) ));
	llsd->value.date_.dyn_str = TRUE;
	llsd->value.date_.len = DATE_STR_LEN;

	int_time = floor( llsd->value.date_.dval );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( llsd->value.date_.dval - int_time) * 1000000.0 );
	parts = *gmtime(&seconds);
	snprintf( llsd->value.date_.str, DATE_STR_LEN, 
			  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			  parts.tm_year + 1900,
			  parts.tm_mon + 1,
			  parts.tm_mday,
			  parts.tm_hour,
			  parts.tm_min,
			  parts.tm_sec,
			  ((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
	return TRUE;
}

int llsd_destringify_date( llsd_t * llsd )
{
	int useconds;
	struct tm parts;
	time_t mktimeEpoch = mktime(&parts);
	double seconds;

	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_DATE), FALSE );
	CHECK_RET( (llsd->value.date_.len == DATE_STR_LEN), FALSE );
	CHECK_RET( llsd->value.date_.str, FALSE );

	sscanf( llsd->value.date_.str, 
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			&parts.tm_year,
			&parts.tm_mon,
			&parts.tm_mday,
			&parts.tm_hour,
			&parts.tm_min,
			&parts.tm_sec,
			&useconds );

	/* adjust a few things */
	parts.tm_year -= 1900;
	parts.tm_mon -= 1;

	/* convert to seconds */
	seconds = (double)(mktime(&parts) - mktimeEpoch);

	/* add in the miliseconds */
	seconds += ((double)useconds * 1000.0);

	/* store it in the date */
	llsd->value.date_.dval = seconds;
	llsd->value.date_.use_dval = TRUE;

	return TRUE;
}

static uint32_t llsd_escaped_string_len( llsd_t * llsd )
{
	uint32_t len = 0;
	int i;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_STRING), 0 );
	CHECK_RET( (llsd->value.string_.str_len > 0), 0 );
	CHECK_PTR_RET( llsd->value.string_.str, 0 );
	CHECK_RET( (llsd->value.string_.esc_len == 0), 0 );
	CHECK_RET( (llsd->value.string_.esc == NULL), 0 );

	for( i = 0; i < llsd->value.string_.str_len; i++ )
	{
		len += notation_esc_len[llsd->value.string_.str[i]];
	}
	return len;
}

int llsd_escape_string( llsd_t * llsd )
{
	int i;
	uint8_t c;
	uint8_t * p;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (llsd->value.string_.str_len > 0), FALSE );
	CHECK_PTR_RET( llsd->value.string_.str, FALSE );
	CHECK_RET( (llsd->value.string_.esc_len == 0), FALSE );
	CHECK_RET( (llsd->value.string_.esc == NULL), FALSE );

	size = llsd_escaped_string_len( llsd );
	llsd->value.string_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->value.string_.esc, FALSE );
	llsd->value.string_.dyn_esc = TRUE;
	llsd->value.string_.esc_len = size;

	p = llsd->value.string_.esc;
	for( i = 0; i < llsd->value.string_.str_len; i++)
	{
		c = llsd->value.string_.str[i];
		MEMCPY( p, notation_esc_chars[c], notation_esc_len[c] );
		p += notation_esc_len[c];

		CHECK_RET( (p <= (llsd->value.string_.esc + llsd->value.string_.esc_len)), FALSE );
	}

	return TRUE;
}

static uint32_t llsd_unescaped_string_len( llsd_t * llsd )
{
	uint8_t* p;
	uint32_t len = 0;
	int i;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_STRING), 0 );
	CHECK_RET( (llsd->value.string_.esc_len > 0), 0 );
	CHECK_PTR_RET( llsd->value.string_.esc, 0 );
	CHECK_RET( (llsd->value.string_.str_len == 0), 0 );
	CHECK_RET( (llsd->value.string_.str == NULL), 0 );

	p = llsd->value.string_.esc;
	while( p < (llsd->value.string_.esc + llsd->value.string_.esc_len) )
	{
		/* skip over the logical character */
		if ( p[0]  == '\\' )
		{
			if ( p[1] == 'x' )
			{
				CHECK_RET( ( isxdigit( p[2] ) && isxdigit( p[3] ) ), 0 );
				p += 4;
			}
			else
			{
				p += 2;
			}
		}
		else
		{
			p++;
		}
		/* increment the unescaped length */
		len += 1;
	}
	return len;
}

int llsd_unescape_string( llsd_t * llsd )
{
	int i;
	uint8_t * p;
	uint8_t * e;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_STRING), FALSE );
	CHECK_RET( (llsd->value.string_.esc_len > 0), FALSE );
	CHECK_PTR_RET( llsd->value.string_.esc, FALSE );
	CHECK_RET( (llsd->value.string_.str_len == 0), FALSE );
	CHECK_RET( (llsd->value.string_.str == NULL), FALSE );

	size = llsd_unescaped_string_len( llsd );
	CHECK_RET( (size > 0), FALSE );
	llsd->value.string_.str = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->value.string_.str, FALSE );
	llsd->value.string_.dyn_str = TRUE;
	llsd->value.string_.str_len = size;

	p = llsd->value.string_.str;
	e = llsd->value.string_.esc;
	while( e < (llsd->value.string_.esc + llsd->value.string_.esc_len) )
	{
		if ( e[0] == '\\' )
		{
			switch ( e[1] )
			{
				case 'x':
					if ( isxdigit( e[2] ) && isxdigit( e[3] ) )
					{
						*p = hex_to_byte( e[2], e[3] );
						p++;
					}
					else
					{
						WARN( "found invalid escape sequence in string\n" );
						return FALSE;
					}
					e += 4;
					break;
				case 'a':
					*p = 0x7;
					p++;
					e += 2;
					break;
				case 'b':
					*p = 0x8;
					p++;
					e += 2;
					break;
				case 'f':
					*p = 0xc;
					p++;
					e += 2;
					break;
				case 'n':
					*p = 0xa;
					p++;
					e += 2;
					break;
				case 'r':
					*p = 0xd;
					p++;
					e += 2;
					break;
				case 't':
					*p = 0x9;
					p++;
					e += 2;
					break;
				case 'v':
					*p = 0xb;
					p++;
					e += 2;
					break;
				case '"':
				case '\'':
				case '\\':
					*p = e[1];
					p++;
					e += 2;
					break;
			}
		}
		else
		{
			*p = *e;
			e++;
		}
	}

	return TRUE;
}

#define URL_ENCODED_CHAR( x ) ( ( (*p >= 0x00) && (*p <= 0x1F) ) || \
								( (*p >= 0x7F) && (*p <= 0xFF) ) || \
								( *p == ' ' ) || \
								( *p == '\'' ) || \
								( *p == '"' ) || \
								( *p == '<' ) || \
								( *p == '>' ) || \
								( *p == '%' ) || \
								( *p == '{' ) || \
								( *p == '}' ) || \
								( *p == '|' ) || \
								( *p == '\\' ) || \
								( *p == '^' ) || \
								( *p == '[' ) || \
								( *p == ']' ) || \
								( *p == '`' ) )

static uint32_t llsd_escaped_uri_len( llsd_t * llsd )
{
	uint8_t * p;
	uint32_t len = 0;
	CHECK_PTR_RET( llsd, 0 );
	CHECK_RET( (llsd->type_ == LLSD_URI), 0 );
	CHECK_RET( (llsd->value.uri_.uri_len > 0), 0 );
	CHECK_PTR_RET( llsd->value.uri_.uri, 0 );
	CHECK_RET( (llsd->value.uri_.esc_len == 0), 0 );
	CHECK_RET( (llsd->value.uri_.esc == NULL), 0 );

	p = llsd->value.uri_.uri;
	while( p < (llsd->value.uri_.uri + llsd->value.uri_.uri_len) )
	{
		if ( URL_ENCODED_CHAR( *p ) )
		{
			len += 3;
		}
		else
		{
			len++;
		}
	}
}

int llsd_escape_uri( llsd_t * llsd )
{
	int i;
	uint8_t c;
	uint8_t * p;
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_URI), FALSE );
	CHECK_RET( (llsd->value.uri_.uri_len > 0), FALSE );
	CHECK_PTR_RET( llsd->value.uri_.uri, FALSE );
	CHECK_RET( (llsd->value.uri_.esc_len == 0), FALSE );
	CHECK_RET( (llsd->value.uri_.esc == NULL), FALSE );

	size = llsd_escaped_uri_len( llsd );
	llsd->value.uri_.esc = UT(CALLOC( size, sizeof(uint8_t) ));
	CHECK_PTR_RET( llsd->value.uri_.esc, FALSE );
	llsd->value.uri_.dyn_esc = TRUE;
	llsd->value.uri_.esc_len = size;

	p = llsd->value.uri_.esc;
	for( i = 0; i < llsd->value.uri_.uri_len; i++)
	{
		c = llsd->value.uri_.uri[i];
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

int llsd_unescape_uri( llsd_t * llsd )
{
	return TRUE;
}

int llsd_encode_binary( llsd_t * llsd, llsd_bin_enc_t encoding )
{
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->value.binary_.data_size != 0), FALSE );
	CHECK_PTR_RET( llsd->value.binary_.data, FALSE );
	CHECK_RET( (llsd->value.binary_.enc_size == 0), FALSE );
	CHECK_RET( (llsd->value.binary_.enc == NULL), FALSE );
	CHECK_RET( ((encoding >= LLSD_BIN_ENC_FIRST) && (encoding < LLSD_BIN_ENC_LAST)), FALSE );

	switch ( encoding )
	{
		case LLSD_BASE16:
			WARN( "base 16 encoding unimplemented\n" );
			return FALSE;
		case LLSD_BASE64:
			/* allocate enc buffer */
			llsd->value.binary_.enc_size = BASE64_LENGTH( llsd->value.binary_.data_size );
			llsd->value.binary_.enc = UT(CALLOC( llsd->value.binary_.enc_size, sizeof(uint8_t) ));
			llsd->value.binary_.dyn_enc = TRUE;
			llsd->value.binary_.encoding = LLSD_BASE64;

			/* encode the data */
			base64_encode( llsd->value.binary_.data,
						   llsd->value.binary_.data_size,
						   llsd->value.binary_.enc,
						   llsd->value.binary_.enc_size );
			break;
	}

	return TRUE;
}

/* Base64: the exact amount is 3 * inlen / 4, minus 1 if the input ends
 with "=" and minus another 1 if the input ends with "==".
 Dividing before multiplying avoids the possibility of overflow.  */
static uint32_t llsd_decoded_binary_len( llsd_t * llsd )
{
	uint32_t len = 0;
	uint32_t size = 0;
	CHECK_PTR_RET( llsd, -1 );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), -1 );
	CHECK_RET( (llsd->value.binary_.enc_size != 0), -1 );
	CHECK_PTR_RET( llsd->value.binary_.enc, -1 );
	CHECK_RET( ((llsd->value.binary_.encoding >= LLSD_BIN_ENC_FIRST) && (llsd->value.binary_.encoding < LLSD_BIN_ENC_LAST)), -1 );

	size = llsd->value.binary_.data_size;

	switch ( llsd->value.binary_.encoding )
	{
		case LLSD_BASE16:
			break;
		case LLSD_BASE64:
			len = 3 * (size / 4) + 2;
			if ( llsd->value.binary_.data[ size - 1 ] == '=' )
				len--;
			if ( llsd->value.binary_.data[ size - 2 ] == '=' )
				len--;
			return len;
	}
	return -1;
}

int llsd_decode_binary( llsd_t * llsd )
{
	uint32_t size;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_RET( (llsd->type_ == LLSD_BINARY), FALSE );
	CHECK_RET( (llsd->value.binary_.data_size == 0), FALSE );
	CHECK_RET( (llsd->value.binary_.data == NULL), FALSE );
	CHECK_RET( (llsd->value.binary_.enc_size != 0), FALSE );
	CHECK_PTR_RET( llsd->value.binary_.enc, FALSE );
	CHECK_RET( ((llsd->value.binary_.encoding >= LLSD_BIN_ENC_FIRST) && (llsd->value.binary_.encoding < LLSD_BIN_ENC_LAST)), FALSE );

	switch ( llsd->value.binary_.encoding )
	{
		case LLSD_BASE16:
			break;
		case LLSD_BASE64:
			/* figure out the size of the needed buffer */
			size = llsd_decoded_binary_len( llsd );
			CHECK_RET( (size > -1), FALSE );

			/* allocate the data buffer */
			llsd->value.binary_.data = UT(CALLOC( size, sizeof(uint8_t) ));
			llsd->value.binary_.dyn_data = TRUE;

			/* decode the base64 data */
			base64_decode( llsd->value.binary_.enc,
						   llsd->value.binary_.enc_size,
						   llsd->value.binary_.data,
						   &(llsd->value.binary_.data_size) );
			CHECK_RET( (llsd->value.binary_.data_size == size), FALSE );
			break;
	}

	return TRUE;
}


size_t llsd_format_xml( llsd_t * llsd, FILE * fout )
{
	return 0;
}


