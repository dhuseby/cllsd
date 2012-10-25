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
#include <expat.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/list.h>
#include <cutil/buffer.h>

#include "llsd.h"
#include "llsd_xml_parser.h"

#define XML_SIG_LEN (38)
static uint8_t const * const xml_header = "<?xml version\"1.0\" encoding=\"UTF-8\"?>\n";

int llsd_xml_check_sig_file( FILE * fin )
{
	size_t ret;
	uint8_t sig[XML_SIG_LEN];

	CHECK_PTR_RET( fin, FALSE );

	/* read the signature */
	ret = fread( sig, sizeof(uint8_t), XML_SIG_LEN, fin );

	/* rewind the file */
	rewind( fin );

	/* if it matches the signature, return TRUE, otherwise FALSE */
	return ( memcmp( sig, xml_header, XML_SIG_LEN ) == 0 );
}

typedef struct xp_state_s
{
	llsd_bin_enc_t enc;
	list_t * state_stack;
	buffer_t * buf;
	llsd_ops_t * ops;
	void * user_data;
} xp_state_t;

#define PUSH(x) (list_push_head( parser_state->state_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( parser_state->state_stack ))
#define POP		(list_pop_head( parser_state->state_stack))

#define BEGIN_VALUE_STATES ( TOP_LEVEL | ARRAY_BEGIN | ARRAY_VALUE_END | MAP_KEY_END )
#define BEGIN_STRING_STATES ( BEGIN_VALUE_STATES | MAP_VALUE_END | MAP_BEGIN )
static int begin_value( uint32_t valid_states, llsd_type_t type_, xp_state_t * parser_state )
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

#define VALUE_STATES ( TOP_LEVEL | ARRAY_VALUE_BEGIN | MAP_VALUE_BEGIN )
#define STRING_STATES ( VALUE_STATES | MAP_KEY_BEGIN )
static int value( uint32_t valid_states, llsd_type_t type_, xp_state_t * parser_state )
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
					break;
				case MAP_VALUE_BEGIN:
					POP;
					PUSH( MAP_VALUE );
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
					break;
				case MAP_VALUE_BEGIN:
					POP;
					PUSH( MAP_VALUE );
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
static int end_value( uint32_t valid_states, llsd_type_t type_, xp_state_t * parser_state )
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
				case TOP_LEVEL:
					break;
			}
		break;
		
		case LLSD_STRING:
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
		break;
	}

	return TRUE;
}

static llsd_bin_enc_t llsd_bin_enc_from_attr( char const * attr )
{
	llsd_bin_enc_t enc = LLSD_BASE64;
	CHECK_PTR_RET( attr, LLSD_BASE64 );
	CHECK_RET( attr[0] == 'b', LLSD_BASE64 );

	/* base64, base16, base85 */
	switch( attr[4] )
	{
		case '6':
			return LLSD_BASE64;
		case '1':
			return LLSD_BASE16;
		case '8':
			return LLSD_BASE85;
	}
	return LLSD_BASE64;
}

static llsd_type_t llsd_type_from_tag( char const * tag )
{
	CHECK_PTR_RET( tag, LLSD_TYPE_INVALID );
	switch( tag[0] )
	{
		case 'a':
			return LLSD_ARRAY;
		/* boolean, binary */
		case 'b':
			switch ( tag[1] )
			{
				case 'o':
					return LLSD_BOOLEAN;
				case 'i':
					return LLSD_BINARY;
			}
		case 'd':
			return LLSD_DATE;
		case 'i':
			return LLSD_INTEGER;
		case 'k':
			return LLSD_KEY;
		case 'l':
			return LLSD_LLSD;
		case 'm':
			return LLSD_MAP;
		case 'r':
			return LLSD_REAL;
		case 's':
			return LLSD_STRING;
		/* undef, uri, uuid */
		case 'u':
			switch ( tag[1] )
			{
				case 'n':
					return LLSD_UNDEF;
				case 'r':
					return LLSD_URI;
				case 'u':
					return LLSD_UUID;
			}
	}
	return LLSD_TYPE_INVALID;
}

static int boolean_from_buf( buffer_t * const buf )
{
	CHECK_PTR_RET( buf, FALSE );
	CHECK_RET( buf->iov_len > 0, FALSE );
	switch ( ((uint8_t*)buf->iov_base)[0] )
	{
		case '1':
		case 't':
		case 'T':
			return TRUE;
	}
	return FALSE;
}

static int integer_from_buf( buffer_t * const buf, int * ival )
{
	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( ival, FALSE );
	(*ival) = 0;

	if ( buf->iov_len > 0 )
	{
		if ( ((uint8_t*)buf->iov_base)[buf->iov_len-1] != '\0' )
		{
			/* zero terminate the string */
			buffer_append( buf, "\0", 1 );
		}
		CHECK_RET( sscanf( buf->iov_base, "%d", ival ) == 1, FALSE );
	}
	return TRUE;
}

static int real_from_buf( buffer_t * const buf, double * rval )
{
	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( rval, FALSE );
	(*rval) = 0.0;

	if ( buf->iov_len > 0 )
	{
		if ( ((uint8_t*)buf->iov_base)[buf->iov_len-1] != '\0' )
		{
			/* zero terminate the string */
			buffer_append( buf, "\0", 1 );
		}
		CHECK_RET( sscanf( buf->iov_base, "%lf", rval ) == 1, FALSE );
	}
	return TRUE;
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

static int uuid_from_buf( buffer_t * const buf, uint8_t uuid[UUID_LEN] )
{
	int i;
	uint8_t * p;

	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( uuid, FALSE );

	/* zero out the uuid */
	MEMSET(uuid, 0, UUID_LEN);

	if ( buf->iov_len < UUID_STR_LEN )
		return TRUE;

	p = (uint8_t*)buf->iov_base;

	/* check for 8-4-4-4-12 */
	for ( i = 0; i < UUID_STR_LEN; i++ )
	{
		if ( ( (i >=  0) && (i <  8) ) ||	/* 8 */
			 ( (i >=  9) && (i < 13) ) ||	/* 4 */
			 ( (i >= 14) && (i < 18) ) ||	/* 4 */
			 ( (i >= 19) && (i < 23) ) ||	/* 4 */
			 ( (i >= 24) && (i < 36) ) )	/* 12 */
		{
			if ( !isxdigit( p[i] ) )
			{
				return FALSE;
			}
		}
		else if ( p[i] != '-' )
		{
			return FALSE;
		}
	}

	/* 8 */
	uuid[0] = hex_to_byte( p[0], p[1] );
	uuid[1] = hex_to_byte( p[2], p[3] );
	uuid[2] = hex_to_byte( p[4], p[5] );
	uuid[3] = hex_to_byte( p[6], p[7] );

	/* 4 */
	uuid[4] = hex_to_byte( p[9], p[10] );
	uuid[5] = hex_to_byte( p[11], p[12] );

	/* 4 */
	uuid[6] = hex_to_byte( p[14], p[15] );
	uuid[7] = hex_to_byte( p[16], p[17] );

	/* 4 */
	uuid[8] = hex_to_byte( p[19], p[20] );
	uuid[9] = hex_to_byte( p[21], p[22] );

	/* 12 */
	uuid[10] = hex_to_byte( p[24], p[25] );
	uuid[11] = hex_to_byte( p[26], p[27] );
	uuid[12] = hex_to_byte( p[28], p[29] );
	uuid[13] = hex_to_byte( p[30], p[31] );
	uuid[14] = hex_to_byte( p[32], p[33] );
	uuid[15] = hex_to_byte( p[34], p[35] );

	return TRUE;
}

static int binary_from_buf( buffer_t * const buf, llsd_bin_enc_t enc, uint8_t ** data, uint32_t * len)
{
	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( data, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = 0;
	(*data) = NULL;

	/* empty binary tag */
	if ( buf->iov_len == 0 )
		return TRUE;

	/* decode the binary */
	switch( enc )
	{
		case LLSD_BASE16:
			(*len) = base16_decoded_len( buf->iov_base, buf->iov_len );
			(*data) = CALLOC( (*len), sizeof(uint8_t) );
			CHECK_PTR_RET( (*data), FALSE );
			if ( !base16_decode( buf->iov_base, buf->iov_len, (*data), len ) )
			{
				FREE( (*data) );
				(*data) = NULL;
				return FALSE;
			}
			break;
		case LLSD_BASE64:
			(*len) = base64_decoded_len( buf->iov_base, buf->iov_len );
			(*data) = CALLOC( (*len), sizeof(uint8_t) );
			CHECK_PTR_RET( (*data), FALSE );
			if ( !base64_decode( buf->iov_base, buf->iov_len, (*data), len ) )
			{
				FREE( (*data) );
				(*data) = NULL;
				return FALSE;
			}
			break;
		case LLSD_BASE85:
			(*len) = base85_decoded_len( buf->iov_base, buf->iov_len );
			(*data) = CALLOC( (*len), sizeof(uint8_t) );
			CHECK_PTR_RET( (*data), FALSE );
			if ( !base85_decode( buf->iov_base, buf->iov_len, (*data), len ) )
			{
				FREE( (*data) );
				(*data) = NULL;
				return FALSE;
			}
			break;
	}
	return TRUE;
}

static int date_from_buf( buffer_t * const buf, double * rval )
{
	int useconds;
	struct tm parts;
	double seconds;
	uint32_t tmp = 0;

	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( rval, FALSE );
	
	(*rval) = 0.0;

	/* empty date tag */
	if ( buf->iov_len == 0 )
		return TRUE;

	MEMSET( &parts, 0, sizeof(struct tm) );

	if ( ((uint8_t*)buf->iov_base)[buf->iov_len-1] != '\0' )
	{
		/* zero terminate the string */
		buffer_append( buf, "\0", 1 );
	}
	CHECK_RET( sscanf( buf->iov_base, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", &parts.tm_year, &parts.tm_mon, &parts.tm_mday, &parts.tm_hour, &parts.tm_min, &parts.tm_sec, &useconds ) == 7, FALSE );

	/* adjust a few things */
	parts.tm_year -= 1900;
	parts.tm_mon -= 1;

	/* convert to seconds */
	tmp = mktime(&parts);
	
	/* correct for localtime variation */
	tmp -= timezone;

	/* convert to double */
	(*rval) = (double)tmp;

	/* add in the miliseconds */
	(*rval) += ((double)useconds * 1000.0);

	return TRUE;
}

static int decode_xml_escape( uint8_t * const buf, uint8_t * const chr, uint32_t * const len )
{
	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( chr, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*chr) = '\0';
	(*len) = 0;

	switch( buf[1] )
	{
		case 'l':
			CHECK_RET( strncmp( buf, "&lt;", 4 ) == 0, FALSE );
			(*chr) = '<';
			(*len) = 4;
			return TRUE;
		case 'g':
			CHECK_RET( strncmp( buf, "&gt;", 4 ) == 0, FALSE );
			(*chr) = '>';
			(*len) = 4;
			return TRUE;
		case 'a':
			if ( buf[2] == 'm' )
			{
				CHECK_RET( strncmp( buf, "&amp;", 5 ) == 0, FALSE );
				(*chr) = '&';
				(*len) = 5;
				return TRUE;
			}
			else
			{
				CHECK_RET( strncmp( buf, "&apos;", 6 ) == 0, FALSE );
				(*chr) = '\'';
				(*len) = 6;
				return TRUE;
			}
			return FALSE;
		case 'q':
			CHECK_RET( strncmp( buf, "&quot;", 6 ) == 0, FALSE );
			(*chr) = '\"';
			(*len) = 6;
			return TRUE;
	}
	return FALSE;
}

static int decoded_xml_string_len( uint8_t const * const encoded, uint32_t const enc_len, uint32_t * const len )
{
	int amp = FALSE;
	uint8_t const * p;
	uint32_t l = 0;
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = 0;
	p = encoded;
	while( p < (encoded + enc_len) )
	{
		if ( amp )
		{
			if ( *p == ';' )
			{
				l++;
				amp = FALSE;
			}
		}
		else
		{
			if ( *p == '&' )
			{
				amp = TRUE;
			}
			else
			{
				l++;
			}
		}
		p++;
	}
	(*len) = l;
	return TRUE;
}

static int string_from_buf( buffer_t * const buf, uint8_t ** const str, uint32_t * len )
{
	uint8_t c;
	uint8_t * p;
	uint8_t * q;
	uint32_t l;
	CHECK_PTR_RET( buf, FALSE );
	CHECK_PTR_RET( str, FALSE );
	CHECK_PTR_RET( len, FALSE );

	CHECK_RET( decoded_xml_string_len( buf->iov_base, buf->iov_len, &l ), FALSE );
	(*str) = CALLOC( l + 1, sizeof(uint8_t) );
	CHECK_PTR_RET( (*str), FALSE );
	(*len) = l;

	p = (uint8_t*)buf->iov_base;
	q = (*str);
	while ( p < ((uint8_t*)buf->iov_base + buf->iov_len) )
	{
		l = 1;
		if ( *p == '&' )
		{
			CHECK_RET( decode_xml_escape( p, q, &l ), FALSE );
		}
		else
		{
			*q = *p;
			q++;
		}
		p += l;
	}

	return TRUE;
}

static void XMLCALL llsd_xml_start_tag( void * data, char const * el, char const ** attr )
{
	uint32_t size = 0;
	llsd_type_t t = LLSD_UNDEF;
	xp_state_t * parser_state = (xp_state_t*)data;

	CHECK_PTR( parser_state );

	/* get the type */
	t = llsd_type_from_tag( el );

	switch( t )
	{
		case LLSD_LLSD:
			PUSH( TOP_LEVEL );
			break;
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
		case LLSD_URI:
			CHECK( begin_value( BEGIN_VALUE_STATES, t, parser_state ) );
			break;
		case LLSD_KEY:
		case LLSD_STRING:
			CHECK( begin_value( BEGIN_STRING_STATES, LLSD_STRING, parser_state ) );
			break;
		case LLSD_BINARY:
			/* try to get the encoding attribute if there is one */
			parser_state->enc = LLSD_BASE64;
			if ( (attr[0] != NULL) && (strncmp( attr[0], "encoding", 9 ) == 0) )
			{
				parser_state->enc = llsd_bin_enc_from_attr( attr[1] );
			}
			CHECK( begin_value( BEGIN_VALUE_STATES, LLSD_BINARY, parser_state ) );
			break;
		case LLSD_ARRAY:
			/* try to get the size attribute if there is one */
			if ( (attr[0] != NULL) && (strncmp( attr[0], "size", 5 ) == 0) )
			{
				size = atoi( attr[1] );
			}
			CHECK( begin_value( BEGIN_VALUE_STATES, LLSD_ARRAY, parser_state ) );
			CHECK( (*(parser_state->ops->array_begin_fn))( size, parser_state->user_data ) );
			PUSH( ARRAY_BEGIN );
			break;
		case LLSD_MAP:
			/* try to get the size attribute if there is one */
			if ( (attr[0] != NULL) && (strncmp( attr[0], "size", 5 ) == 0) )
			{
				size = atoi( attr[1] );
			}
			CHECK( begin_value( BEGIN_VALUE_STATES, LLSD_MAP, parser_state ) );
			CHECK( (*(parser_state->ops->map_begin_fn))( size, parser_state->user_data ) );
			PUSH( MAP_BEGIN );
			break;
	}

	/* reset the buffer */
	buffer_deinitialize( parser_state->buf );
}


static void XMLCALL llsd_xml_end_tag( void * data, char const * el )
{
	int bool_val;
	int int_val;
	double real_val;
	uint8_t uuid_val[UUID_LEN];
	uint8_t * encoded = NULL;
	uint32_t enc_len = 0;
	uint8_t * buffer = NULL;
	uint32_t len = 0;
	llsd_type_t t = LLSD_UNDEF;
	xp_state_t * parser_state = (xp_state_t*)data;

	CHECK_PTR( parser_state );

	/* get the type */
	t = llsd_type_from_tag( el );

	switch( t )
	{
		case LLSD_LLSD:
			POP;
			break;
		case LLSD_UNDEF:
			CHECK( (*(parser_state->ops->undef_fn))( parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_UNDEF, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_UNDEF, parser_state ) );
			break;
		case LLSD_BOOLEAN:
			bool_val = boolean_from_buf( parser_state->buf );
			CHECK( (*(parser_state->ops->boolean_fn))( bool_val, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_BOOLEAN, parser_state ) );
			break;
		case LLSD_INTEGER:
			CHECK( integer_from_buf( parser_state->buf, &int_val ) );
			CHECK( (*(parser_state->ops->integer_fn))( int_val, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_INTEGER, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_INTEGER, parser_state ) );
			break;
		case LLSD_REAL:
			CHECK( real_from_buf( parser_state->buf, &real_val ) );
			CHECK( (*(parser_state->ops->real_fn))( real_val, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_REAL, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_REAL, parser_state ) );
			break;
		case LLSD_UUID:
			CHECK( uuid_from_buf( parser_state->buf, uuid_val ) );
			CHECK( (*(parser_state->ops->uuid_fn))( uuid_val, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_UUID, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_UUID, parser_state ) );
			break;
		case LLSD_DATE:
			CHECK( date_from_buf( parser_state->buf, &real_val ) );
			CHECK( (*(parser_state->ops->date_fn))( real_val, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_DATE, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_DATE, parser_state ) );
			break;
		case LLSD_KEY:
		case LLSD_STRING:
			/* zero terminate the string */
			buffer_append( parser_state->buf, "\0", 1 );
			CHECK( string_from_buf( parser_state->buf, &buffer, &len ) );
			CHECK( (*(parser_state->ops->string_fn))( buffer, TRUE, parser_state->user_data ) );
			CHECK( value( STRING_STATES, LLSD_STRING, parser_state ) );
			CHECK( end_value( END_STRING_STATES, LLSD_STRING, parser_state ) );
			buffer = NULL;
			len = 0;
			break;
		case LLSD_URI:
			/* zero terminate the string */
			buffer_append( parser_state->buf, "\0", 1 );
			CHECK( string_from_buf( parser_state->buf, &encoded, &enc_len ) );
			CHECK( llsd_unescape_uri( encoded, enc_len, &buffer, &len ) );
			CHECK( (*(parser_state->ops->uri_fn))( buffer, TRUE, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_URI, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_URI, parser_state ) );
			FREE( encoded );
			buffer = NULL;
			enc_len = 0;
			len = 0;
			break;
		case LLSD_BINARY:
			CHECK( binary_from_buf( parser_state->buf, parser_state->enc, &buffer, &len ) );
			CHECK( (*(parser_state->ops->binary_fn))( buffer, len, TRUE, parser_state->user_data ) );
			CHECK( value( VALUE_STATES, LLSD_BINARY, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_BINARY, parser_state ) );
			buffer = NULL;
			len = 0;
			break;
		case LLSD_ARRAY:
			CHECK( (*(parser_state->ops->array_end_fn))( 0, parser_state->user_data ) );
			POP;
			CHECK( value( VALUE_STATES, LLSD_ARRAY, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_ARRAY, parser_state ) );
			break;
		case LLSD_MAP:
			CHECK( (*(parser_state->ops->map_end_fn))( 0, parser_state->user_data ) );
			POP;
			CHECK( value( VALUE_STATES, LLSD_MAP, parser_state ) );
			CHECK( end_value( END_VALUE_STATES, LLSD_MAP, parser_state ) );
			break;
	}

	/* reset the buffer */
	buffer_deinitialize( parser_state->buf );
}

static void XMLCALL llsd_xml_data_handler( void * data, char const * s, int len )
{
	void * buf;
	xp_state_t *state = (xp_state_t*)data;

	CHECK_PTR( state );

	/* extend the buffer and copy the new data into it */
	buf = buffer_append( state->buf, s, len );
	CHECK_PTR( buf );
}

#define XML_BUF_SIZE (4096)

int llsd_xml_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data )
{
	static uint8_t buf[XML_BUF_SIZE];
	int done;
	size_t len;
	XML_Parser p;
	xp_state_t state;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	MEMSET( &state, 0, sizeof( xp_state_t ) );

	/* create the parser */
	p = XML_ParserCreate( NULL );
	CHECK_PTR_RET( p, FALSE );

	/* set up step stack, used to synthesize array value end, map key end, 
	 * and map value end callbacks */
	state.state_stack = list_new( 1, NULL );
	CHECK_PTR_RET( state.state_stack, FALSE );

	/* create the buffer */
	state.buf = buffer_new( NULL, 0 );
	CHECK_PTR_RET( state.buf, FALSE );

	/* store the ops callback pointers */
	state.ops = ops;

	/* store user data pointer to pass back to callbacks */
	state.user_data = user_data;

	/* set the tag handlers */
	XML_SetElementHandler( p, &llsd_xml_start_tag, &llsd_xml_end_tag );
	XML_SetCharacterDataHandler( p, &llsd_xml_data_handler );
	XML_SetUserData( p, (void*)(&state) );

	/* seek past signature */
	fseek( fin, XML_SIG_LEN, SEEK_SET );

	do
	{
		/* read the type marker */
		len = fread( buf, sizeof(uint8_t), XML_BUF_SIZE, fin );
		done = (len < XML_BUF_SIZE);

		if ( XML_Parse( p, buf, len, done ) == XML_STATUS_ERROR )
		{
			DEBUG( "%s\n", XML_ErrorString(XML_GetErrorCode(p)) );
		}
		
	} while ( !done );

	/* clean up the step stack */
	list_delete( state.state_stack );

	/* clean up the buffer */
	buffer_delete( state.buf );

	/* free the parser */
	XML_ParserFree( p );

	return TRUE;
}

