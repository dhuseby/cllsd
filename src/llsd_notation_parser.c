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
#include "llsd_notation_parser.h"

#define NOTATION_SIG_LEN (18)
static uint8_t const * const notation_header = "<?llsd/notation?>\n";

int llsd_notation_check_sig_file( FILE * fin )
{
	size_t ret;
	uint8_t sig[NOTATION_SIG_LEN];

	CHECK_PTR_RET( fin, FALSE );

	/* read the signature */
	ret = fread( sig, sizeof(uint8_t), NOTATION_SIG_LEN, fin );

	/* rewind the file */
	rewind( fin );

	/* if it matches the signature, return TRUE, otherwise FALSE */
	return ( memcmp( sig, notation_header, NOTATION_SIG_LEN ) == 0 );
}

typedef enum ns_step_e
{
	TOP_LEVEL		= 0x001,
	ARRAY_START		= 0x002,
	ARRAY_VALUE		= 0x004,
	ARRAY_VALUE_END = 0x008,
	ARRAY_END		= 0x010,
	MAP_START		= 0x020,
	MAP_KEY			= 0x040,
	MAP_KEY_END		= 0x080,
	MAP_VALUE		= 0x100,
	MAP_VALUE_END	= 0x200,
	MAP_END			= 0x400
} ns_step_t;

#define VALUE_STATES (TOP_LEVEL | ARRAY_START | ARRAY_VALUE_END | MAP_KEY_END )
#define STRING_STATES ( VALUE_STATES | MAP_START | MAP_VALUE_END )

#define PUSH(x) (list_push_head( step_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( step_stack ))
#define POP		(list_pop_head( step_stack))

static int update_state( uint32_t valid_states, llsd_type_t type_, list_t * step_stack )
{
	/* make sure we have a valid LLSD type */
	CHECK_RET( IS_VALID_LLSD_TYPE( type_ ), FALSE );
	
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( step_stack, FALSE );

	/* make sure we're in a valid state */
	CHECK_RET( (TOP & valid_states), FALSE );

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
			switch( TOP )
			{
				case ARRAY_START:
				case ARRAY_VALUE_END:
					POP;
					PUSH( ARRAY_VALUE );
					break;
				case MAP_KEY_END:
					POP;
					PUSH( MAP_VALUE );
					break;
				case TOP_LEVEL:
					/* no state change */
					break;
			}
		break;
		
		case LLSD_STRING:
			switch( TOP )
			{
				case ARRAY_START:
				case ARRAY_VALUE_END:
					POP;
					PUSH( ARRAY_VALUE );
					break;
				case MAP_KEY_END:
					POP;
					PUSH( MAP_VALUE );
					break;
				case MAP_START:
				case MAP_VALUE_END:
					POP;
					PUSH( MAP_KEY );
					break;
				case TOP_LEVEL:
					/* no state change */
					break;
			}
		break;
	}

	return TRUE;
}
static int llsd_notation_consume_boolean( FILE * fin, int bval )
{
	uint8_t p;
	long offset;
	CHECK_PTR_RET( fin, FALSE );

	if ( (fread( &p, sizeof(uint8_t), 1, fin ) == 1) && isalpha(p) )
	{
		offset = (bval ? 2 : 3 );
		/* move to where we need to be for the rest of the parse */
		fseek( fin, offset, SEEK_CUR );
	}

	return TRUE;
}

static int llsd_notation_parse_integer( FILE * fin, int * ival )
{
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ival, FALSE );
	CHECK_RET( fscanf( fin, "%d", ival ) == 1, FALSE );
	return TRUE;
}

static int llsd_notation_parse_real( FILE * fin, double * dval )
{
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( dval, FALSE );
	CHECK_RET( fscanf( fin, "%lf", dval ) == 1, FALSE );
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

static int llsd_notation_parse_uuid( FILE * fin, uint8_t uuid[UUID_LEN] )
{
	int i;
	uint8_t buf[UUID_STR_LEN];

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( uuid, FALSE );

	CHECK_RET( fread( buf, sizeof(uint8_t), UUID_STR_LEN, fin ) == UUID_STR_LEN, FALSE );

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

static int llsd_notation_parse_paren_size( FILE * fin, uint32_t * len )
{
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( len, FALSE );
	CHECK_RET( fscanf( fin, "(%u)", len ) == 1, FALSE );
	return TRUE;
}

static int llsd_notation_parse_base_number( FILE * fin, llsd_bin_enc_t * enc )
{
	uint8_t p[2];
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( enc, FALSE );

	CHECK_RET( fread( p, sizeof(uint8_t), 2, fin ) == 2, FALSE );
	switch ( p[0] )
	{
		case '1':
			(*enc) = LLSD_BASE16;
			break;
		case '6':
			(*enc) = LLSD_BASE64;
			break;
		case '8':
			(*enc) = LLSD_BASE85;
			break;
	}

	return TRUE;
}

static int llsd_notation_parse_raw( FILE * fin, uint8_t ** buffer, uint32_t len, int str )
{
	uint8_t c;
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	(*buffer) = NULL;

	/* read first double quote */
	CHECK_RET( fread( &c, sizeof(uint8_t), 1, fin ) == 1, FALSE );
	CHECK_RET( c == '\"', FALSE );

	/* add 1 for null termination on strings */
	(*buffer) = CALLOC( len + (str ? 1 : 0), sizeof(uint8_t) );
	CHECK_PTR_RET( (*buffer), FALSE );

	/* read the raw data */
	CHECK_RET( fread( (*buffer), sizeof(uint8_t), len, fin ) == len, FALSE );

	/* read second double quote */
	CHECK_RET( fread( &c, sizeof(uint8_t), 1, fin ) == 1, FALSE );
	CHECK_RET( c == '\"', FALSE );

	return TRUE;
}

static int llsd_notation_parse_quoted( FILE * fin, uint8_t ** buffer, uint32_t * len, uint8_t quote )
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

static int llsd_notation_decode_date( uint8_t * data, double * real_val )
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

int llsd_notation_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data )
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
	llsd_bin_enc_t encoding = 0;
	list_t * step_stack = NULL;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	/* set up step stack, used to synthesize array value end, map key end, 
	 * and map value end callbacks */
	step_stack = list_new( 1, NULL );
	CHECK_PTR_RET( step_stack, FALSE );
	PUSH( TOP_LEVEL );

	/* seek past signature */
	fseek( fin, NOTATION_SIG_LEN, SEEK_SET );

	while( TRUE )
	{
		/* read the type marker */
		ret = fread( &p, sizeof(uint8_t), 1, fin );
		if ( feof( fin ) )
			return TRUE;

		CHECK_RET( ret == 1, FALSE );

		switch( p )
		{

			case '!':
				CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_UNDEF, step_stack ), FALSE );
				break;

			case '1':
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack ), FALSE );
				break;

			case '0':
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack ), FALSE );
				break;

			case 't':
			case 'T':
				CHECK_RET( llsd_notation_consume_boolean( fin, TRUE ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack ), FALSE );
				break;

			case 'f':
			case 'F':
				CHECK_RET( llsd_notation_consume_boolean( fin, FALSE ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack ), FALSE );
				break;

			case 'i':
				CHECK_RET( llsd_notation_parse_integer( fin, &int_val ), FALSE );
				CHECK_RET( (*(ops->integer_fn))( int_val, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_INTEGER, step_stack ), FALSE );
				break;

			case 'r':
				CHECK_RET( llsd_notation_parse_real( fin, &real_val ), FALSE );
				CHECK_RET( (*(ops->real_fn))( real_val, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_REAL, step_stack ), FALSE );
				break;

			case 'u':
				CHECK_RET( llsd_notation_parse_uuid( fin, uuid ), FALSE );
				CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_UUID, step_stack ), FALSE );
				break;

			case 'b':
				CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );
				CHECK_RET( fseek( fin, -1, SEEK_CUR ) == 0, FALSE );
				if ( p == '(' )
				{
					/* it is a binary size in parenthesis */
					CHECK_RET( llsd_notation_parse_paren_size( fin, &len ), FALSE );

					/* grab the binary data */
					CHECK_RET( llsd_notation_parse_raw( fin, &buffer, len, FALSE ), FALSE );
				}
				else
				{
					/* it is a base encoding number */
					CHECK_RET( llsd_notation_parse_base_number( fin, &encoding ), FALSE );
					CHECK_RET( (encoding >= LLSD_BASE16) && (encoding <= LLSD_BASE85), FALSE );
				
					/* read the quote character */
					CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );
					
					/* read the quoted string */
					CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len, p ), FALSE );

					/* decode the binary */
					switch( encoding )
					{
						case LLSD_BASE16:
							len = base16_decoded_len( encoded, enc_len );
							buffer = CALLOC( len, sizeof(uint8_t) );
							if ( buffer == NULL )
							{
								FREE( encoded );
								return FALSE;
							}
							if ( !base16_decode( encoded, enc_len, buffer, &len ) )
							{
								FREE( encoded );
								FREE( buffer );
								return FALSE;
							}
							break;
						case LLSD_BASE64:
							len = base64_decoded_len( encoded, enc_len );
							buffer = CALLOC( len, sizeof(uint8_t) );
							if ( buffer == NULL )
							{
								FREE( encoded );
								return FALSE;
							}
							if ( !base64_decode( encoded, enc_len, buffer, &len ) )
							{
								FREE( encoded );
								FREE( buffer );
								return FALSE;
							}
							break;
						case LLSD_BASE85:
							len = base85_decoded_len( encoded, enc_len );
							buffer = CALLOC( len, sizeof(uint8_t) );
							if ( buffer == NULL )
							{
								FREE( encoded );
								return FALSE;
							}
							if ( !base85_decode( encoded, enc_len, buffer, &len ) )
							{
								FREE( encoded );
								FREE( buffer );
								return FALSE;
							}
							break;
					}
					FREE( encoded );
					encoded = NULL;
				}
			
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->binary_fn))( buffer, len, TRUE, user_data ), FALSE );
				buffer = NULL;
				CHECK_RET( update_state( VALUE_STATES, LLSD_BINARY, step_stack ), FALSE );
				break;

			case '\'':
			case '\"':
				/* read the quoted string */
				CHECK_RET( llsd_notation_parse_quoted( fin, &buffer, &len, p ), FALSE );
			
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
				buffer = NULL;
				CHECK_RET( update_state( STRING_STATES, LLSD_STRING, step_stack ), FALSE );
				break;

			case 's':
				/* it is a string size in parenthesis */
				CHECK_RET( llsd_notation_parse_paren_size( fin, &len ), FALSE );

				/* read the raw string, add 1 so that it is null terminated */
				CHECK_RET( llsd_notation_parse_raw( fin, &buffer, len, TRUE ), FALSE );

				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
				buffer = NULL;
				CHECK_RET( update_state( STRING_STATES, LLSD_STRING, step_stack ), FALSE );
				break;

			case 'l':
				/* read the quote character */
				CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );

				/* read the uri */
				CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len, '\"' ), FALSE );
#if 0
				if ( !llsd_unescape_uri( encoded, enc_len, &buffer, &len ) )
				{
					FREE( encoded );
					return FALSE;
				}
#endif
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->uri_fn))( encoded, TRUE, user_data ), FALSE );
				encoded = NULL;
				CHECK_RET( update_state( VALUE_STATES, LLSD_URI, step_stack ), FALSE );
				break;

			case 'd':
				/* read the quote character */
				CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );

				/* read in the quoted string */
				CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len, '\"' ), FALSE );

				if ( !llsd_notation_decode_date( encoded, &real_val ) )
				{
					FREE( encoded );
					return FALSE;
				}

				FREE( encoded );
				encoded = NULL;

				CHECK_RET( (*(ops->date_fn))( real_val, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_DATE, step_stack ), FALSE );
				break;

			case '[':
				CHECK_RET( (*(ops->array_begin_fn))( 0, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_ARRAY, step_stack ), FALSE );
				PUSH( ARRAY_START );
				break;

			case ']':
				CHECK_RET( (*(ops->array_end_fn))( 0, user_data ), FALSE );
				POP;
				break;
			
			case '{':
				CHECK_RET( (*(ops->map_begin_fn))( 0, user_data ), FALSE );
				CHECK_RET( update_state( VALUE_STATES, LLSD_MAP, step_stack ), FALSE );
				PUSH( MAP_START );
				break;

			case '}':
				CHECK_RET( (*(ops->map_end_fn))( 0, user_data ), FALSE );
				POP;
				break;

			case ',':
				CHECK_RET( (TOP & (ARRAY_VALUE | MAP_VALUE)), FALSE );
				if ( TOP == ARRAY_VALUE )
				{
					CHECK_RET( (*(ops->array_value_end_fn))( user_data ), FALSE );
					POP;
					PUSH( ARRAY_VALUE_END );
				}
				else
				{
					CHECK_RET( (*(ops->map_value_end_fn))( user_data ), FALSE );
					POP;
					PUSH( MAP_VALUE_END );
				}
				break;

			case ':':
				CHECK_RET( (TOP & MAP_KEY), FALSE );
				CHECK_RET( (*(ops->map_key_end_fn))( user_data ), FALSE );
				POP;
				PUSH( MAP_KEY_END );
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

	/* clean up the step stack */
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( step_stack );

	return TRUE;
}


