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

#include <time.h>

#include "llsd.h"
#include "llsd_json_parser.h"

int llsd_json_check_sig_file( FILE * fin )
{
	CHECK_PTR_RET( fin, FALSE );
	/* there is no header in JSON files */
	return TRUE;
}

typedef struct js_state_s
{
	llsd_ops_t * ops;
	void * user_data;
	list_t * count_stack;
	list_t * state_stack;
} js_state_t;

#define PUSH(x)		(list_push_head( parser_state->state_stack, (void*)x ))
#define TOP			((uint_t)list_get_head( parser_state->state_stack ))
#define POP			(list_pop_head( parser_state->state_stack ))

#define PUSHC(x)	(list_push_head( parser_state->count_stack, (void*)x ))
#define TOPC		((int_t)list_get_head( parser_state->count_stack ))
#define POPC		(list_pop_head( parser_state->count_stack ))

#define BEGIN_VALUE_STATES ( TOP_LEVEL | ARRAY_BEGIN | ARRAY_VALUE_END | MAP_KEY_END )
#define BEGIN_STRING_STATES ( BEGIN_VALUE_STATES | MAP_VALUE_END | MAP_BEGIN )
static int begin_value( uint_t valid_states, llsd_type_t type_, js_state_t * parser_state )
{
	state_t state = TOP_LEVEL;

	/* make sure we have a valid LLSD type */
	CHECK_RET( IS_VALID_LLSD_TYPE( type_ ), FALSE );
	
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( parser_state, FALSE );
	CHECK_PTR_RET( parser_state->state_stack, FALSE );

	/* make sure we're in a valid state */
	state = TOP;
	CHECK_RET( (state & valid_states), FALSE );

	/* transition toe the next state based on type_ and current state */
	switch ( type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			switch( state )
			{
				case ARRAY_BEGIN:
				case ARRAY_VALUE_END:
					CHECK_RET( (*(parser_state->ops->array_value_begin_fn))( parser_state->user_data ), FALSE );
					POP;
					PUSH( ARRAY_VALUE_BEGIN );
					break;
				case MAP_KEY_END:
					CHECK_RET( (*(parser_state->ops->map_value_begin_fn))( parser_state->user_data ), FALSE );
					POP;
					PUSH( MAP_VALUE_BEGIN );
					break;
				case TOP_LEVEL:
					break;
			}
		break;
		
		case LLSD_STRING:
			switch( state )
			{
				case ARRAY_BEGIN:
				case ARRAY_VALUE_END:
					CHECK_RET( (*(parser_state->ops->array_value_begin_fn))( parser_state->user_data ), FALSE );
					POP;
					PUSH( ARRAY_VALUE_BEGIN );
					break;
				case MAP_BEGIN:
				case MAP_VALUE_END:
					CHECK_RET( (*(parser_state->ops->map_key_begin_fn))( parser_state->user_data ), FALSE );
					POP;
					PUSH( MAP_KEY_BEGIN );
					break;
				case MAP_KEY_END:
					CHECK_RET( (*(parser_state->ops->map_value_begin_fn))( parser_state->user_data ), FALSE );
					POP;
					PUSH( MAP_VALUE_BEGIN );
					break;
				case TOP_LEVEL:
					break;
			}
		break;
	}

	return TRUE;
}

static int incc( js_state_t * parser_state )
{
	int_t c = 0;
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( parser_state, FALSE );
	CHECK_PTR_RET( parser_state->count_stack, FALSE );
	c = TOPC;
	POPC;
	PUSHC( ++c );
	return TRUE;
}

#define VALUE_STATES ( TOP_LEVEL | ARRAY_VALUE_BEGIN | MAP_VALUE_BEGIN )
#define STRING_STATES ( VALUE_STATES | MAP_KEY_BEGIN )
static int value( uint_t valid_states, llsd_type_t type_, js_state_t * parser_state )
{
	state_t state = TOP_LEVEL;

	/* make sure we have a valid LLSD type */
	CHECK_RET( IS_VALID_LLSD_TYPE( type_ ), FALSE );
	
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( parser_state, FALSE );
	CHECK_PTR_RET( parser_state->state_stack, FALSE );

	/* make sure we're in a valid state */
	state = TOP;
	CHECK_RET( (state & valid_states), FALSE );

	/* transition toe the next state based on type_ and current state */
	switch ( type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			switch( state )
			{
				case ARRAY_VALUE_BEGIN:
					POP;
					PUSH( ARRAY_VALUE );
					incc( parser_state );
					break;
				case MAP_VALUE_BEGIN:
					POP;
					PUSH( MAP_VALUE );
					incc( parser_state );
					break;
				case TOP_LEVEL:
					break;
			}
		break;
		
		case LLSD_STRING:
			switch( state )
			{
				case ARRAY_VALUE_BEGIN:
					POP;
					PUSH( ARRAY_VALUE );
					incc( parser_state );
					break;
				case MAP_VALUE_BEGIN:
					POP;
					PUSH( MAP_VALUE );
					incc( parser_state );
					break;
				case MAP_KEY_BEGIN:
					POP;
					PUSH( MAP_KEY );
					break;
				case TOP_LEVEL:
					break;
			}
		break;
	}

	return TRUE;
}

#define END_VALUE_STATES ( TOP_LEVEL | ARRAY_VALUE | MAP_VALUE )
#define END_STRING_STATES ( END_VALUE_STATES | MAP_KEY )
static int end_value( uint_t valid_states, js_state_t * parser_state )
{
	state_t state = TOP_LEVEL;

	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( parser_state, FALSE );
	CHECK_PTR_RET( parser_state->state_stack, FALSE );

	/* make sure we're in a valid state */
	state = TOP;
	CHECK_RET( (state & valid_states), FALSE );

	switch( state )
	{
		case ARRAY_VALUE:
			CHECK_RET( (*(parser_state->ops->array_value_end_fn))( parser_state->user_data ), FALSE );
			POP;
			PUSH( ARRAY_VALUE_END );
			break;
		case MAP_VALUE:
			CHECK_RET( (*(parser_state->ops->map_value_end_fn))( parser_state->user_data ), FALSE );
			POP;
			PUSH( MAP_VALUE_END );
			break;
		case MAP_KEY:
			CHECK_RET( (*(parser_state->ops->map_key_end_fn))( parser_state->user_data ), FALSE );
			POP;
			PUSH( MAP_KEY_END );
			break;
		case TOP_LEVEL:
			break;
	}

	return TRUE;
}

/* parse a JSON number into either an integer or a real */
static int llsd_json_parse_number( FILE * fin, llsd_type_t * const type_, int * const ival, double * dval )
{
	long pos;
	uint32_t whole, frac;
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( type_, FALSE );
	CHECK_PTR_RET( ival, FALSE );
	CHECK_PTR_RET( dval, FALSE );

	/* remember where we are in the stream */
	pos = ftell( fin );

	whole = 0;
	frac = 0;

	/* try to parse a long float from the stream */
	if ( fscanf( fin, "%d.%d", &whole, &frac ) == 2 )
	{
		fseek( fin, pos, SEEK_SET );
		if ( fscanf( fin, "%lf", dval ) == 1 )
		{
			(*type_) = LLSD_REAL;
			return TRUE;
		}
	}
	else 
	{
		fseek( fin, pos, SEEK_SET );
		if ( fscanf( fin, "%d", ival ) == 1 )
		{
			(*type_) = LLSD_INTEGER;
			return TRUE;
		}
	}

	return FALSE;
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


static int llsd_json_parse_quoted( FILE * fin, uint8_t ** buffer, uint32_t * len, uint8_t quote )
{
	int i;
	int done = FALSE;
	int escaped = FALSE;
	static uint8_t buf[1024];
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*buffer) = NULL;
	(*len) = 0;
	MEMSET( buf, 0, 1024 );

	while( !done )
	{
		for ( i = 0; (i < 1024) && (!done); i++ )
		{
			CHECK_RET( fread( &buf[i], sizeof(uint8_t), 1, fin ) == 1, FALSE );

			/* handle escaped quotes */
			if ( !escaped )
			{
				if ( buf[i] == '\\' )
				{
					/* found escaping char */
					escaped = TRUE;
				}
				else if ( buf[i] == '\"' )
				{
					/* found an unescaped double quote, must be done */
					done = TRUE;
				}
			}
			else
			{
				escaped = FALSE;
			}
		}

		if ( done && (((*len) + i) == 0) )
		{
			/* make sure there is at least one byte in the buffer */
			(*buffer) = REALLOC( (*buffer), 1 );
			CHECK_PTR_RET( (*buffer), FALSE );
		}
		else
		{
			/* increase the buffer size */
			(*buffer) = REALLOC( (*buffer), (*len) + i );
			CHECK_PTR_RET( (*buffer), FALSE );

			/* copy the data over */
			MEMCPY( &((*buffer)[(*len)]), buf, i - (done ? 1 : 0) );
			(*len) += (i - (done ? 1 : 0));
		}

		/* null terminate */
		if ( done )
			(*buffer)[(*len)] = '\0';
	}
	
	return TRUE;
}

/* timezone value gets set to the number of seconds offset from GMT for local time */
extern long timezone;

static int llsd_json_decode_date( uint8_t const * const data, double * real_val )
{
	int useconds;
	struct tm parts;
	double seconds;
	uint32_t tmp = 0;

	CHECK_PTR_RET( data, FALSE );
	CHECK_PTR_RET( real_val, FALSE );

	MEMSET( &parts, 0, sizeof(struct tm) );

	CHECK_RET( sscanf( data, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", &parts.tm_year, &parts.tm_mon, &parts.tm_mday, &parts.tm_hour, &parts.tm_min, &parts.tm_sec, &useconds ) == 7, FALSE );

	/* adjust a few things */
	parts.tm_year -= 1900;
	parts.tm_mon -= 1;

	/* convert to seconds */
	tmp = mktime(&parts);
	
	/* correct for localtime variation */
	tmp -= timezone;

	/* convert to double */
	(*real_val) = (double)tmp;

	/* add in the miliseconds */
	(*real_val) += ((double)useconds * 1000.0);

	return TRUE;
}

static int llsd_json_decode_uuid( uint8_t const * const buf, uint8_t uuid[UUID_LEN] )
{
	int i;

	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( uuid, FALSE );

	/* check for 8-4-4-4-12 */
	for ( i = 0; i < UUID_STR_LEN; i++ )
	{
		if ( ( (i >=  0) && (i <  8) ) ||	/* 8 */
			 ( (i >=  9) && (i < 13) ) ||	/* 4 */
			 ( (i >= 14) && (i < 18) ) ||	/* 4 */
			 ( (i >= 19) && (i < 23) ) ||	/* 4 */
			 ( (i >= 24) && (i < 36) ) )	/* 12 */
		{
			if ( !isxdigit( buf[i] ) )
			{
				return FALSE;
			}
		}
		else if ( buf[i] != '-' )
		{
			return FALSE;
		}
	}

	/* 8 */
	uuid[0] = hex_to_byte( buf[0], buf[1] );
	uuid[1] = hex_to_byte( buf[2], buf[3] );
	uuid[2] = hex_to_byte( buf[4], buf[5] );
	uuid[3] = hex_to_byte( buf[6], buf[7] );

	/* 4 */
	uuid[4] = hex_to_byte( buf[9], buf[10] );
	uuid[5] = hex_to_byte( buf[11], buf[12] );

	/* 4 */
	uuid[6] = hex_to_byte( buf[14], buf[15] );
	uuid[7] = hex_to_byte( buf[16], buf[17] );

	/* 4 */
	uuid[8] = hex_to_byte( buf[19], buf[20] );
	uuid[9] = hex_to_byte( buf[21], buf[22] );

	/* 12 */
	uuid[10] = hex_to_byte( buf[24], buf[25] );
	uuid[11] = hex_to_byte( buf[26], buf[27] );
	uuid[12] = hex_to_byte( buf[28], buf[29] );
	uuid[13] = hex_to_byte( buf[30], buf[31] );
	uuid[14] = hex_to_byte( buf[32], buf[33] );
	uuid[15] = hex_to_byte( buf[34], buf[35] );

	return TRUE;
}

#define JSON_BINARY_TAG_LEN (7)

/* binary data is stored as base64 encoded binary data in a string */
static int llsd_json_decode_binary( uint8_t const * const encoded, uint32_t const enc_len,
									uint8_t ** const buffer, uint32_t * const len )
{
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_RET( (enc_len > 0), FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = base64_decoded_len( &(encoded[JSON_BINARY_TAG_LEN]), 
							   (enc_len - JSON_BINARY_TAG_LEN) );

	/* handle zero length binary */
	if ( (*len) == 0 )
	{
		(*buffer) = NULL;
		return TRUE;
	}

	(*buffer) = CALLOC( (*len), sizeof(uint8_t) );
	CHECK_PTR_RET( (*buffer), FALSE );

	if ( !base64_decode( &(encoded[JSON_BINARY_TAG_LEN]), (enc_len - JSON_BINARY_TAG_LEN), (*buffer), len ) )
	{
		FREE( (*buffer) );
		(*buffer) = NULL;
		(*len) = 0;
		return FALSE;
	}

	return TRUE;
}

#define JSON_URI_TAG_LEN (7)

/* uri's are url encoded strings */
static int llsd_json_decode_uri( uint8_t const * const encoded, uint32_t const enc_len,
								 uint8_t ** const buffer, uint32_t * const len )
{
	uint8_t const * p = NULL;
	uint8_t * q = NULL;
	int escaped = FALSE;
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_RET( (enc_len > 0), FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*buffer) = CALLOC( enc_len, sizeof(uint8_t) );
	CHECK_PTR_RET( (*buffer), FALSE );
	q = (*buffer);
	(*len) = 0;

	/* do % decoding */
	p = &encoded[JSON_URI_TAG_LEN];
	while ( p < (encoded + enc_len) )
	{
		if ( !escaped )
		{
			if ( *p == '%' )
			{
				escaped = TRUE;
				p++;
			}
			else
			{
				*q++ = *p++;
				(*len)++;
			}
		}
		else
		{
			CHECK_RET( (p+1) < (encoded + enc_len), FALSE );
			(*q) = hex_to_byte( p[0], p[1] );
			q++;
			p++;
			(*len)++;
			escaped = FALSE;
		}
	}

	return TRUE;
}

static int llsd_json_surrogate_pair( uint8_t const * const data )
{
	CHECK_PTR_RET( data, FALSE );
	uint8_t b1, b2;
	uint16_t s1;
	b1 = hex_to_byte( data[0], data[1] );
	b2 = hex_to_byte( data[2], data[3] );
	s1 = ((b1 << 8) | b2);

	/* if the first four hex characters decode to a value within the valid
	 * range of the lead surrogate in a surrogate pair, return TRUE. */
	return ((s1 >= 0xD800) && (s1 <= 0xDBFF));
}

static int llsd_json_utf8_len( uint8_t const * const first, uint8_t const * const second )
{
	uint8_t b1, b2;
	uint16_t s1, s2;
	uint32_t d1;
	b1 = hex_to_byte( first[0], first[1] );
	b2 = hex_to_byte( first[2], first[3] );
	s1 = ((b1 << 8) | b2);

	if ( (b1 == 0) && (b2 <= 0x7F) )
	{
		/* this is a non-extended character encoded as \uXXXX */
		return 1;
	}

	if ( (s1 >= 0xD800) && (s1 <= 0xDBFF) )
	{
		/* surrogate pair */
		CHECK_PTR_RET( second, 0 );
		return 4;
	}
	else
	{
		if ( s1 <= 0x7FF )
		{
			/* this results in a 2-byte UTF-8 sequence */
			return 2;
		}

		/* this results in a 3-byte UTF-8 sequence */
		return 3;
	}

	return 0;
}

static int llsd_json_utf8_decode( uint8_t const * const first, uint8_t const * const second, 
								  uint8_t * out )
{
	uint8_t b1, b2, b3, b4;
	uint16_t s1, s2;
	uint32_t d1;
	b1 = hex_to_byte( first[0], first[1] );
	b2 = hex_to_byte( first[2], first[3] );
	s1 = ((b1 << 8) | b2);

	if ( (b1 == 0) && (b2 <= 0x7F) )
	{
		/* this is a non-extended character encoded as \uXXXX */
		out[0] = b2;
		return 1;
	}

	if ( (s1 >= 0xD800) && (s1 <= 0xDBFF) )
	{
		/* surrogate pair converts to 20-bit UTF-32 value in the 
		 * range 0x10000 - 0x10FFFF which encodes to a 4-byte
		 * UTF-8 sequence */
		CHECK_PTR_RET( second, FALSE );

		b3 = hex_to_byte( second[0], second[1] );
		b4 = hex_to_byte( second[2], second[3] );
		s2 = ((b3 << 8) | b4);

		/* subtract the range adjusters */
		s1 -= 0xD800;
		s2 -= 0xDC00;

		/* covert to UTF-32 */
		d1 = (((s1 << 16) | s2) + 0x10000);

		/* now covert to UTF-8 */
		out[0] = (0xF0 | ((uint8_t)(((d1 & 0x001C0000) >> 18) & 0x0000003F)));
		out[1] = (0x80 | ((uint8_t)(((d1 & 0x0003F000) >> 12) & 0x0000003F)));
		out[2] = (0x80 | ((uint8_t)(((d1 & 0x00000FC0) >>  6) & 0x0000003F)));
		out[3] = (0x80 | ((uint8_t)(d1 & 0x0000003F)));
		return 4;
	}
	else
	{
		if ( s1 <= 0x7FF )
		{
			/* this results in a 2-byte UTF-8 sequence */
			out[0] = (0xC0 | ((b2 & 0xC0) >> 6) | ((b1 & 0x03) << 3));
			out[1] = (0x80 | (b2 & 0x3F));
			return 2;
		}

		/* this results in a 3-byte UTF-8 sequence */
		out[0] = (0xE0 | ((b1 & 0xF0) >> 4));
		out[1] = (0x80 | ((b1 & 0x0F) << 2) | ((b2 & 0xC0) >> 6));
		out[2] = (0x80 | (b2 & 0x3F));
		return 3;
	}
	return 0;
}

/* strings are backslash encoded, empty strings are valid */
static int llsd_json_decode_string( uint8_t const * const encoded, uint32_t const enc_len,
									uint8_t ** const buffer, uint32_t * const len )
{
	uint8_t const * p = NULL;
	uint8_t * q = NULL;
	int escaped;
	int has_escaped = FALSE;
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	if ( enc_len == 0 )
	{
		/* allocate a single byte to hold the null termination */
		(*buffer) = CALLOC( 1, sizeof( uint8_t ) );
		CHECK_PTR_RET( (*buffer), FALSE );
		(*len) = 0;
		return TRUE;
	}

	(*len) = 0;

	escaped = FALSE;
	p = encoded;
	while( p < (encoded + enc_len) )
	{
		if ( !escaped )
		{
			if ( *p == '\\' )
			{
				escaped = TRUE;
				has_escaped = TRUE;
			}
			
			(*len)++;
			p++;
		}
		else
		{
			/* process \uXXXX encoded extended chars */
			if ( *p == 'u' )
			{
				CHECK_RET( (p+4) < (encoded + enc_len), FALSE );
				if ( llsd_json_surrogate_pair( p+1 ) )
				{
					/* process an UTF-32 encoded subordinate pair */
					CHECK_RET( (p + 10) < (encoded + enc_len), FALSE );
					CHECK_RET( *(p+5) == '\\', FALSE );
					CHECK_RET( *(p+6) == 'u', FALSE );
					(*len) += llsd_json_utf8_len( p+1, p+5 );
					p += 11;
				}
				else
				{
					/* process an UTF-16 encoded extended char */
					(*len) += llsd_json_utf8_len( p+1, NULL );
					p += 5;
				}
			}
			else
			{
				/* one of the other escaped characters */
				CHECK_RET( (*p == '\"') || (*p == '\\') || (*p == '/') || \
						   (*p == 'b') || (*p == 'f') || (*p == 'n') || \
						   (*p == 'r') || (*p == 't'), FALSE );
				p++;
			}
			escaped = FALSE;
		}
	}

	/* allocate a buffer for the UTF-8 encoded string */
	(*buffer) = CALLOC( (*len) + 1, sizeof(uint8_t) );
	CHECK_PTR_RET( (*buffer), FALSE );

	/* shortcut for strings without escaping */
	if ( !has_escaped )
	{
		MEMCPY( (*buffer), encoded, enc_len );
		return TRUE;
	}

	p = encoded;
	q = (*buffer);
	escaped = FALSE;
	while( p < (encoded + enc_len) )
	{
		if ( !escaped )
		{
			if ( *p == '\\' )
			{
				escaped = TRUE;
				p++;
			}
			else
			{
				/* copy the character over */
				*q++ = *p++;
			}
		}
		else
		{
			/* process \uXXXX encoded extended chars */
			if ( *p == 'u' )
			{
				CHECK_RET( (p+4) < (encoded + enc_len), FALSE );
				if ( llsd_json_surrogate_pair( p+1 ) )
				{
					/* process an UTF-32 encoded subordinate pair */
					CHECK_RET( (p + 10) < (encoded + enc_len), FALSE );
					CHECK_RET( *(p+5) == '\\', FALSE );
					CHECK_RET( *(p+6) == 'u', FALSE );
					q += llsd_json_utf8_decode( p+1, p+5, q );
					p += 11;
				}
				else
				{
					/* process an UTF-16 encoded extended char */
					q += llsd_json_utf8_decode( p+1, NULL, q );
					p += 5;
				}
			}
			else
			{
				switch ( *p++ )
				{
					case '\"':
						*q++ = '\"';
						break;
					case '\\':
						*q++ = '\\';
						break;
					case '/':
						*q++ = '/';
						break;
					case 'b':
						*q++ = '\b';
						break;
					case 'f':
						*q++ = '\f';
						break;
					case 'n':
						*q++ = '\n';
						break;
					case 'r':
						*q++ = '\r';
						break;
					case 't':
						*q++ = '\t';
						break;
				}
			}
			escaped = FALSE;
		}
	}

	return TRUE;
}

/* takes a string and length and tries to decode the string to a date, uuid, binary, or uri.
 * if all of those fail, then it stays as a string.
 * NOTE: empty strings will always be returned as an LLSD_STRING */
static int llsd_json_convert_quoted( uint8_t const * const encoded, uint32_t const enc_len, 
									 llsd_type_t * const type_, double * const dval, 
									 uint8_t uuid[UUID_LEN], uint8_t ** const buffer, 
									 uint32_t * const len )
{
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_PTR_RET( type_, FALSE );
	CHECK_PTR_RET( dval, FALSE );
	CHECK_PTR_RET( uuid, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	/* try to convert it to a date */
	if ( (enc_len == DATE_STR_LEN) && llsd_json_decode_date( encoded, dval ) )
	{
		(*type_) = LLSD_DATE;
		return TRUE;
	}
	else if ( (enc_len == UUID_STR_LEN) && llsd_json_decode_uuid( encoded, uuid ) )
	{
		(*type_) = LLSD_UUID;
		return TRUE;
	}
	else if ( (strncmp( encoded, "||b64||", 7 ) == 0) && 
			  llsd_json_decode_binary( encoded, enc_len, buffer, len ) )
	{
		(*type_) = LLSD_BINARY;
		return TRUE;
	}
	else if ( (strncmp( encoded, "||uri||", 7 ) == 0) && 
			  llsd_json_decode_uri( encoded, enc_len, buffer, len ) )
	{
		(*type_) = LLSD_URI;
		return TRUE;
	}
	else if ( llsd_json_decode_string( encoded, enc_len, buffer, len ) )
	{
		(*type_) = LLSD_STRING;
		return TRUE;
	}

	return FALSE;
}

int llsd_json_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data )
{
	int i;
	uint8_t p;
	size_t ret;
	int bool_val;
	int32_t int_val;
	double real_val;
	uint8_t uuid[UUID_LEN];
	uint8_t * buffer = NULL;
	uint8_t * encoded = NULL;
	uint32_t len;
	uint32_t enc_len;
	int32_t line;
	long line_start;
	long err_column;
	long text_start;
	llsd_type_t type_ = LLSD_NONE;
	js_state_t* parser_state = NULL;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	/* set up step stack, used to synthesize array value end, map key end, 
	 * and map value end callbacks */
	parser_state = CALLOC( 1, sizeof(js_state_t) );
	CHECK_PTR_RET( parser_state, FALSE );

	parser_state->count_stack = list_new( 0, NULL );
	if ( parser_state->count_stack == NULL )
	{
		FREE( parser_state );
		return FALSE;
	}

	parser_state->state_stack = list_new( 1, NULL );
	if ( parser_state->state_stack == NULL )
	{
		list_delete( parser_state->count_stack );
		FREE( parser_state );
		return FALSE;
	}
	parser_state->ops = ops;
	parser_state->user_data = user_data;

	/* start at top level state */
	PUSH( TOP_LEVEL );

	line = 0;
	line_start = ftell( fin );

	while( TRUE )
	{
		/* read the type marker */
		ret = fread( &p, sizeof(uint8_t), 1, fin );
		if ( feof( fin ) )
			break;

		CHECK_RET( ret == 1, FALSE );

		switch( p )
		{

			case 'n': /* null */
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_UNDEF, parser_state ), fail_json_parse );
				CHECK_GOTO( (*(ops->undef_fn))( user_data ), fail_json_parse );
				fseek( fin, 3, SEEK_CUR );
				CHECK_GOTO( value( VALUE_STATES, LLSD_UNDEF, parser_state ), fail_json_parse );
				break;

			case 't': /* true */
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), fail_json_parse );
				CHECK_GOTO( (*(ops->boolean_fn))( TRUE, user_data ), fail_json_parse );
				fseek( fin, 3, SEEK_CUR );
				CHECK_GOTO( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), fail_json_parse );
				break;

			case 'f': /* false */
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), fail_json_parse );
				CHECK_GOTO( (*(ops->boolean_fn))( FALSE, user_data ), fail_json_parse );
				fseek( fin, 4, SEEK_CUR );
				CHECK_GOTO( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), fail_json_parse );
				break;

			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': /* number */
				/* back up one character so that we can parse the number */
				fseek( fin, -1, SEEK_CUR );
				CHECK_GOTO( llsd_json_parse_number( fin, &type_, &int_val, &real_val ), fail_json_parse );
				
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, type_, parser_state ), fail_json_parse );
				switch( type_ )
				{
					case LLSD_INTEGER:
						CHECK_GOTO( (*(ops->integer_fn))( int_val, user_data ), fail_json_parse );
						break;
					case LLSD_REAL:
						CHECK_GOTO( (*(ops->real_fn))( real_val, user_data ), fail_json_parse );
						break;
				}
				CHECK_GOTO( value( VALUE_STATES, type_, parser_state ), fail_json_parse );
				break;

			case '\"':
				/* read the quoted string */
				CHECK_GOTO( llsd_json_parse_quoted( fin, &encoded, &enc_len, p ), fail_json_parse );

				/* try to convert it to date, uuid, uri, binary, or leave it as a string */
				if ( !llsd_json_convert_quoted( encoded, enc_len, &type_, &real_val, uuid, &buffer, &len ) )
				{
					FREE( encoded );
					goto fail_json_parse;
				}
				
				FREE( encoded );
				encoded = NULL;
				
				switch ( type_ )
				{
					case LLSD_DATE:
						CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_DATE, parser_state ), fail_json_parse );
						CHECK_GOTO( (*(ops->date_fn))( real_val, user_data ), fail_json_parse );
						CHECK_GOTO( value( VALUE_STATES, LLSD_DATE, parser_state ), fail_json_parse );
						break;
					case LLSD_UUID:
						CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_UUID, parser_state ), fail_json_parse );
						CHECK_GOTO( (*(ops->uuid_fn))( uuid, user_data ), fail_json_parse );
						CHECK_GOTO( value( VALUE_STATES, LLSD_UUID, parser_state ), fail_json_parse );
						break;
					case LLSD_BINARY:
						CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_BINARY, parser_state ), fail_json_parse );
						/* tell it to take ownership of the memory */
						CHECK_GOTO( (*(ops->binary_fn))( buffer, len, TRUE, user_data ), fail_json_parse );
						CHECK_GOTO( value( VALUE_STATES, LLSD_BINARY, parser_state ), fail_json_parse );
						buffer = NULL;
						break;
					case LLSD_URI:
						CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_URI, parser_state ), fail_json_parse );
						/* tell it to take ownership of the memory */
						CHECK_GOTO( (*(ops->uri_fn))( buffer, TRUE, user_data ), fail_json_parse );
						CHECK_GOTO( value( VALUE_STATES, LLSD_URI, parser_state ), fail_json_parse );
						buffer = NULL;
						break;
					case LLSD_STRING:
						CHECK_GOTO( begin_value( BEGIN_STRING_STATES, LLSD_STRING, parser_state ), fail_json_parse );
						/* tell it to take ownership of the memory */
						CHECK_GOTO( (*(ops->string_fn))( buffer, TRUE, user_data ), fail_json_parse );
						CHECK_GOTO( value( STRING_STATES, LLSD_STRING, parser_state ), fail_json_parse );
						buffer = NULL;
						break;
				}
			
				break;

			case '[':
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_ARRAY, parser_state ), fail_json_parse );
				CHECK_GOTO( (*(ops->array_begin_fn))( 0, user_data ), fail_json_parse );
				PUSH( ARRAY_BEGIN );
				PUSHC( 0 );
				break;

			case ']':
				/* if there were any items in this array, we need to generate the end_value
				 * callbacks here because there isn't a comma to mark the end of the last 
				 * value */
				if ( TOPC )
				{
					CHECK_GOTO( end_value( ARRAY_VALUE, parser_state ), fail_json_parse );
				}

				POP;
				POPC;
				CHECK_GOTO( (*(ops->array_end_fn))( 0, user_data ), fail_json_parse );
				CHECK_GOTO( value( VALUE_STATES, LLSD_ARRAY, parser_state ), fail_json_parse );
				break;
			
			case '{':
				CHECK_GOTO( begin_value( BEGIN_VALUE_STATES, LLSD_MAP, parser_state ), fail_json_parse );
				CHECK_GOTO( (*(ops->map_begin_fn))( 0, user_data ), fail_json_parse );
				PUSH( MAP_BEGIN );
				PUSHC( 0 );
				break;

			case '}':
				/* if there were any items in this array, we need to generate the end_value
				 * callbacks here because there isn't a comma to mark the end of the last 
				 * value */
				if ( TOPC )
				{
					CHECK_GOTO( end_value( MAP_VALUE, parser_state ), fail_json_parse );
				}

				POP;
				POPC;
				CHECK_GOTO( (*(ops->map_end_fn))( 0, user_data ), fail_json_parse );
				CHECK_GOTO( value( VALUE_STATES, LLSD_MAP, parser_state ), fail_json_parse );
				break;

			case ',':
				CHECK_GOTO( end_value( (ARRAY_VALUE | MAP_VALUE), parser_state ), fail_json_parse );
				break;

			case ':':
				CHECK_GOTO( end_value( MAP_KEY, parser_state ), fail_json_parse );
				break;

			/* eat whitespace and commas */
			case '\n':
				line++;
				line_start = ftell( fin );
				break;
			case ' ':
			case '\t':
			case '\r':
				break;
			default:
				WARN( "garbage byte %c at 0x%08x\n", p, (unsigned int)ftell( fin ) - 1 );
				goto fail_json_parse;
		}
	}

	/* clean up the count stack */
	list_delete( parser_state->count_stack );

	/* clean up the step stack */
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( parser_state->state_stack );

	/* free the parser_state */
	FREE( parser_state );

	return TRUE;

fail_json_parse:
	err_column = ftell( fin );
	fseek( fin, line_start, SEEK_SET );
	buffer = CALLOC( (err_column - line_start) + 1, sizeof(uint8_t) );
	if ( (i = fread( buffer, sizeof(uint8_t), (err_column - line_start), fin )) != (err_column - line_start) )
	{
		fprintf( stderr, "Only read %d bytes from the file when generating error report\n", i );
	}

	text_start = line_start;
	for( i = 0; i < (int)(err_column - line_start); i++ )
	{
		if ( (buffer[i] == ' ') || (buffer[i] == '\t') || (buffer[i] == '\r') )
		{
			text_start++;
		}
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "%s\n", buffer );
	fprintf(stderr, "%*s^", (int)(text_start - line_start), " " );
	fprintf(stderr, "%*s^\n", (int)((err_column - text_start) - 1), " " );
	fprintf(stderr, "%*s%lu\n", (int)(text_start - line_start), " ", text_start );
	fprintf(stderr, "Parse failed on line %d, column %d\n", line, (int)(err_column - line_start) );
	FREE( buffer );

	/* clean up the count stack */
	list_delete( parser_state->count_stack );

	/* clean up the step stack */
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( parser_state->state_stack );

	/* free the parser_state */
	FREE( parser_state );

	return FALSE;
}


