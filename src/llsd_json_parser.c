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
#define TOP			((uint32_t)list_get_head( parser_state->state_stack ))
#define POP			(list_pop_head( parser_state->state_stack ))

#define PUSHC(x)	(list_push_head( parser_state->count_stack, (void*)x ))
#define TOPC		((int)list_get_head( parser_state->count_stack ))
#define POPC		(list_pop_head( parser_state->count_stack ))

#define BEGIN_VALUE_STATES ( TOP_LEVEL | ARRAY_BEGIN | ARRAY_VALUE_END | MAP_KEY_END )
#define BEGIN_STRING_STATES ( BEGIN_VALUE_STATES | MAP_VALUE_END | MAP_BEGIN )
static int begin_value( uint32_t valid_states, llsd_type_t type_, js_state_t * parser_state )
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
	int c = 0;
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( parser_state, FALSE );
	CHECK_PTR_RET( parser_state->count_stack, FALSE );
	c = TOPC;
	POPC;
	PUSHC( ++c );
}

#define VALUE_STATES ( TOP_LEVEL | ARRAY_VALUE_BEGIN | MAP_VALUE_BEGIN )
#define STRING_STATES ( VALUE_STATES | MAP_KEY_BEGIN )
static int value( uint32_t valid_states, llsd_type_t type_, js_state_t * parser_state )
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
static int end_value( uint32_t valid_states, js_state_t * parser_state )
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


static int llsd_json_consume_boolean( FILE * fin, int bval )
{
	uint8_t p;
	long offset;
	CHECK_PTR_RET( fin, FALSE );

	if ( (fread( &p, sizeof(uint8_t), 1, fin ) == 1) && isalpha(p) )
	{
		offset = (bval ? 2 : 3 );
		/* move to where we need to be for the rest of the parse */
		fseek( fin, offset, SEEK_CUR );

		return TRUE;
	}

	return FALSE;
}

/* parse a JSON number into either an integer or a real */
static int llsd_json_parse_number( FILE * fin, llsd_type_t * const type_, int * const ival, double * dval )
{
	long pos;
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( type_, FALSE );
	CHECK_PTR_RET( ival, FALSE );
	CHECK_PTR_RET( dval, FALSE );

	/* remember where we are in the stream */
	pos = ftell( fin );

	/* try to parse an integer from the stream */
	if ( fscanf( fin, "%d", ival ) == 1 )
	{
		(*type_) = LLSD_INTEGER;
		return TRUE;
	}

	/* move back to where we started */
	fseek( fin, pos, SEEK_SET );

	/* try to parse a long float from the stream */
	if ( fscanf( fin, "%lf", dval ) == 1 )
	{
		(*type_) = LLSD_REAL;
		return TRUE;
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
	static uint8_t buf[1024];
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = 0;

	while( !done )
	{
		for ( i = 0; (i < 1024) && (!done); i++ )
		{
			CHECK_RET( fread( &buf[i], sizeof(uint8_t), 1, fin ) == 1, FALSE );

			/* check for an unescaped matching quote character */
			if ( buf[i] == quote ) 
			{
				if ( (i == 0) || ((i > 0) && (buf[i-1] != '\\')) )
					done = TRUE;
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

static int llsd_json_parse_uuid( uint8_t const * const buf, uint8_t uuid[UUID_LEN] )
{
	int i;

	CHECK_PTR_RET( data, FALSE );
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

/* binary data is stored as base64 encoded binary data in a string */
static int llsd_json_decode_binary( uint8_t * const encoded, uint32_t const enc_len,
									uint8_t ** const buffer, uint32_t * const len )
{
	uint32_t dlen = 0;
	CHECK_PTR_RET( encoded );
	CHECK_RET( (enc_len > 0), FALSE );
	CHECK_PTR_RET( buffer );
	CHECK_PTR_RET( len );

	dlen = base64_decoded_len( encoded, enc_len );
	(*buffer) = CALLOC( len, sizeof(uint8_t) );
	CHECK_PTR_RET( (*buffer), FALSE );

	if ( !base64_decode( encoded, enc_len, (*buffer), len ) )
	{
		FREE( (*buffer) );
		(*buffer) = NULL;
		(*len) = 0;
		return FALSE;
	}

	return TRUE;
}

#define BUF_SIZE (1024)

/* uri's are url encoded strings */
static int llsd_json_decode_uri( uint8_t * const encoded, uint32_t const enc_len,
								 uint8_t ** const buffer, uint32_t * const len )
{
	static uint8_t buf[BUF_SIZE];
	CHECK_PTR_RET( encoded );
	CHECK_RET( (enc_len > 0), FALSE );
	CHECK_PTR_RET( buffer );
	CHECK_PTR_RET( len );

	return TRUE;
}

/* strings are backslash encoded, empty strings are valid */
static int llsd_json_decode_string( uint8_t * const encoded, uint32_t const enc_len,
									uint8_t ** const buffer, uint32_t * const len )
{
	static uint8_t buf[BUF_SIZE];
	uint8_t * p = NULL;
	int i;
	int escaped;
	CHECK_PTR_RET( encoded );
	CHECK_PTR_RET( buffer );
	CHECK_PTR_RET( len );

	if ( enc_len == 0 )
	{
		/* allocate a single byte to hold the null termination */
		(*buffer) = CALLOC( 1, sizeof( uint8_t ) );
		CHECK_PTR_RET( (*buffer), FALSE );
		(*len) = 0;
		return TRUE;
	}

	(*len) = 0;

	for ( i = 0; i < enc_len; i++ )
	{
		if ( !escaped )
		{
			if ( encoded[i] == '\\' )
			{
				escaped = TRUE;
			}
			
			(*len)++;
		}
		else
		{
			if ( encoded[i] == 'u' )
			{

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
									 llsd_t * const type_, double * const dval, 
									 uint8_t uuid[UUID_LEN], uint8_t ** const buffer, 
									 uint32_t * const len )
{
	CHECK_PTR_RET( encoded, FALSE );
	CHECK_PTR_RET( type_, FALSE );
	CHECK_RET( (enc_len > 0), FALSE );
	CHECK_PTR_RET( dval, FALSE );
	CHECK_PTR_RET( uuid, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len );

	/* try to convert it to a date */
	if ( (enc_len = DATE_STR_LEN) && llsd_json_decode_date( encoded, dval ) )
	{
		(*type_) = LLSD_DATE;
		return TRUE;
	}
	else if ( (enc_len == UUID_STR_LEN) && llsd_json_decode_uuid( encoded, uuid ) )
	{
		(*type_) = LLSD_UUID;
		return TRUE;
	}
	else if ( llsd_json_decode_binary( encoded, enc_len, buffer, len ) )
	{
		(*type_) = LLSD_BINARY;
		return TRUE;
	}
	else if ( llsd_json_decode_uri( encoded, enc_len, buffer, len ) )
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
	llsd_t type_ = LLSD_NONE;
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

	/* seek past signature */
	fseek( fin, NOTATION_SIG_LEN, SEEK_SET );

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
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_UNDEF, parser_state ), FALSE );
				CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_UNDEF, parser_state ), FALSE );
				break;

			case 't': /* true */
				CHECK_RET( llsd_json_consume_boolean( fin, TRUE ), FALSE );
				
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				break;

			case 'f': /* false */
				CHECK_RET( llsd_json_consume_boolean( fin, FALSE ), FALSE );
				
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
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
				CHECK_RET( llsd_json_parse_number( fin, &type_, &int_val, &real_val ), FALSE );
				
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, type_, parser_state ), FALSE );
				switch( type_ )
				{
					case LLSD_INTEGER:
						CHECK_RET( (*(ops->integer_fn))( int_val, user_data ), FALSE );
						break;
					case LLSD_REAL:
						CHECK_RET( (*(ops->real_fn))( real_val, user_data ), FALSE );
						break;
				}
				CHECK_RET( value( VALUE_STATES, type_, parser_state ), FALSE );
				break;

			case '\"':
				/* read the quoted string */
				CHECK_RET( llsd_json_parse_quoted( fin, &encoded, &enc_len, p ), FALSE );

				/* try to convert it to date, uuid, uri, binary, or leave it as a string */
				if ( !llsd_json_convert_quoted( encoded, enc_len, &type_, &real_val, uuid, &buffer, &len ) )
				{
					FREE( encoded );
					return FALSE;
				}
				
				FREE( encoded );
				
				switch ( type_ )
				{
					case LLSD_DATE:
						CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_DATE, parser_state ), FALSE );
						CHECK_RET( (*(ops->date_fn))( real_val, user_data ), FALSE );
						CHECK_RET( value( VALUE_STATES, LLSD_DATE, parser_state ), FALSE );
						break;
					case LLSD_UUID:
						CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_UUID, parser_state ), FALSE );
						CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
						CHECK_RET( value( VALUE_STATES, LLSD_UUID, parser_state ), FALSE );
						break;
					case LLSD_BINARY:
						CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BINARY, parser_state ), FALSE );
						/* tell it to take ownership of the memory */
						CHECK_RET( (*(ops->binary_fn))( buffer, len, TRUE, user_data ), FALSE );
						CHECK_RET( value( VALUE_STATES, LLSD_BINARY, parser_state ), FALSE );
						buffer = NULL;
						break;
					case LLSD_URI:
						CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_URI, parser_state ), FALSE );
						/* tell it to take ownership of the memory */
						CHECK_RET( (*(ops->uri_fn))( buffer, TRUE, user_data ), FALSE );
						CHECK_RET( value( VALUE_STATES, LLSD_URI, parser_state ), FALSE );
						buffer = NULL;
						break;
					case LLSD_STRING:
						CHECK_RET( begin_value( BEGIN_STRING_STATES, LLSD_STRING, parser_state ), FALSE );
						/* tell it to take ownership of the memory */
						CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
						CHECK_RET( value( STRING_STATES, LLSD_STRING, parser_state ), FALSE );
						buffer = NULL;
						break;
				}
			
				break;

			case '[':
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_ARRAY, parser_state ), FALSE );
				CHECK_RET( (*(ops->array_begin_fn))( 0, user_data ), FALSE );
				PUSH( ARRAY_BEGIN );
				PUSHC( 0 );
				break;

			case ']':
				/* if there were any items in this array, we need to generate the end_value
				 * callbacks here because there isn't a comma to mark the end of the last 
				 * value */
				if ( TOPC )
				{
					CHECK_RET( end_value( ARRAY_VALUE, parser_state ), FALSE );
				}

				POP;
				POPC;
				CHECK_RET( (*(ops->array_end_fn))( 0, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_ARRAY, parser_state ), FALSE );
				break;
			
			case '{':
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_MAP, parser_state ), FALSE );
				CHECK_RET( (*(ops->map_begin_fn))( 0, user_data ), FALSE );
				PUSH( MAP_BEGIN );
				PUSHC( 0 );
				break;

			case '}':
				/* if there were any items in this array, we need to generate the end_value
				 * callbacks here because there isn't a comma to mark the end of the last 
				 * value */
				if ( TOPC )
				{
					CHECK_RET( end_value( MAP_VALUE, parser_state ), FALSE );
				}

				POP;
				POPC;
				CHECK_RET( (*(ops->map_end_fn))( 0, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_MAP, parser_state ), FALSE );
				break;

			case ',':
				CHECK_RET( end_value( (ARRAY_VALUE | MAP_VALUE), parser_state ), FALSE );
				break;

			case ':':
				CHECK_RET( end_value( MAP_KEY, parser_state ), FALSE );
				break;

			/* eat whitespace and commas */
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				break;
			default:
				WARN( "garbage byte %c at 0x%08x\n", p, (unsigned int)ftell( fin ) - 1 );
				return FALSE;
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
}


