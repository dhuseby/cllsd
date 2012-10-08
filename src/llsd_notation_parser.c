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

static int llsd_notation_parse_boolean( FILE * fin, int * bval )
{
	uint8_t p;
	long offset = -1;
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( bval, FALSE );

	CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );
	(*bval) = FALSE;
	if ( (p == 't') || (p == 'T') || (p == '1') )
		(*bval) = TRUE;

	CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );
	if ( isalpha( p ) )
		offset = ((*bval) ? 3 : 4 );

	/* move to where we need to be for the rest of the parse */
	fseek( fin, offset, SEEK_CUR );

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
	for ( i = 0; i < 36; i++ )
	{
		if ( ( (i >= 0) && (i < 8) ) &&		/* 8 */
			 ( (i >= 9) && (i < 13) ) &&	/* 4 */
			 ( (i >= 14) && (i < 18) ) &&	/* 4 */
			 ( (i >= 19) && (i < 23) ) &&	/* 4 */
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
			(*enc) == LLSD_BASE16;
			break;
		case '6':
			(*enc) == LLSD_BASE64;
			break;
		case '8':
			(*enc) == LLSD_BASE85;
			break;
	}

	return TRUE;
}

static int llsd_notation_parse_quoted( FILE * fin, uint8_t ** buffer, uint32_t * len )
{
	uint8_t p;
	int i;
	int done = FALSE;
	static uint8_t buf[1024];
	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( buffer, FALSE );
	CHECK_PTR_RET( len, FALSE );

	(*len) = 0;

	/* read the quote character */
	CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin) == 1, FALSE );

	while( !done )
	{
		for ( i = 0; (i < 1024) && (!done); i++ )
		{
			CHECK_RET( fread( &buf[i], sizeof(uint8_t), 1, fin ) == 1, FALSE );

			/* check for an unescaped matching quote character */
			if ( (buf[i] == p) && (i > 0) && (buf[i-1] != '\\') )
				done = TRUE;
		}

		/* increase the buffer size */
		(*buffer) = REALLOC( (*buffer), (*len) + i );
		CHECK_PTR_RET( (*buffer), FALSE );

		/* copy the data over */
		MEMCPY( &((*buffer)[(*len)]), buf, i + (done ? 1 : 0) );
		(*len) += i;

		/* null terminate */
		if ( done )
			(*buffer)[(*len)] = '\0';
	}
	
	return TRUE;
}

static int llsd_notation_decode_date( uint8_t * data, double * real_val )
{
	int useconds;
	struct tm parts;
	time_t mktimeEpoch;
	double seconds;

	CHECK_PTR_RET( data, FALSE );
	CHECK_PTR_RET( real_val, FALSE );

	MEMSET( &parts, 0, sizeof(struct tm) );
	mktimeEpoch = mktime(&parts);

	CHECK_RET( sscanf( data, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", &parts.tm_year, &parts.tm_mon, &parts.tm_mday, &parts.tm_hour, &parts.tm_min, &parts.tm_sec, &useconds ) == 7, FALSE );

	/* adjust a few things */
	parts.tm_year -= 1900;
	parts.tm_mon -= 1;

	/* convert to seconds */
	(*real_val) = (double)(mktime(&parts) - mktimeEpoch);

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
	uint8_t * buffer;
	uint8_t * encoded;
	uint32_t len;
	uint32_t enc_len;
	llsd_bin_enc_t encoding;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

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
				break;

			case '1':
			case '0':
			case 't':
			case 'f':
			case 'T':
			case 'F':
				CHECK_RET( fseek( fin, -1, SEEK_CUR ) == 0, FALSE );
				CHECK_RET( llsd_notation_parse_boolean( fin, &bool_val ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( bool_val, user_data ), FALSE );
				break;

			case 'i':
				CHECK_RET( llsd_notation_parse_integer( fin, &int_val ), FALSE );
				CHECK_RET( (*(ops->integer_fn))( int_val, user_data ), FALSE );
				break;

			case 'r':
				CHECK_RET( llsd_notation_parse_real( fin, &real_val ), FALSE );
				CHECK_RET( (*(ops->real_fn))( real_val, user_data ), FALSE );
				break;

			case 'u':
				CHECK_RET( llsd_notation_parse_uuid( fin, uuid ), FALSE );
				CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
				break;

			case 'b':
				CHECK_RET( fread( &p, sizeof(uint8_t), 1, fin ) == 1, FALSE );
				CHECK_RET( fseek( fin, -1, SEEK_CUR ) == 0, FALSE );
				if ( p == '(' )
				{
					/* it is a binary size in parenthesis */
					CHECK_RET( llsd_notation_parse_paren_size( fin, &len ), FALSE );

					/* allocate buffer and read the raw data as binary */
					buffer = CALLOC( len, sizeof(uint8_t) );
					CHECK_PTR_RET( buffer, FALSE );
					ret = fread( buffer, sizeof(uint8_t), len, fin );
					if ( ret != len )
					{
						FREE( buffer );
						return FALSE;
					}

					/* read the trailing double quote */
					ret = fread( &p, sizeof(uint8_t), 1, fin );
					if ( (ret != 1) || (p != '\"') )
					{
						FREE( buffer );
						return FALSE;
					}
				}
				else
				{
					/* it is a base encoding number */
					CHECK_RET( llsd_notation_parse_base_number( fin, &encoding ), FALSE );
					CHECK_RET( (encoding >= LLSD_BASE16) && (encoding <= LLSD_BASE85), FALSE );
					CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len ), FALSE );
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
				}
			
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->binary_fn))( buffer, len, TRUE, user_data ), FALSE );
				break;

			case '\'':
			case '\"':
			case 's':
				if ( p == 's' )
				{
					/* it is a string size in parenthesis */
					CHECK_RET( llsd_notation_parse_paren_size( fin, &enc_len ), FALSE );
				}
				else
				{
					/* put the quote mark back into the stream so that it can be parsed
					 * as part of the parse quoted call that follows */
					CHECK_RET( fseek( fin, -1, SEEK_CUR ) == 0, FALSE );
				}

				/* read the quoted string */
				CHECK_RET( llsd_notation_parse_quoted( fin, &buffer, &len ), FALSE );
			
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
				break;

			case 'l':
				CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len ), FALSE );
#if 0
				if ( !llsd_unescape_uri( encoded, enc_len, &buffer, &len ) )
				{
					FREE( encoded );
					return FALSE;
				}
#endif
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->uri_fn))( encoded, TRUE, user_data ), FALSE );
				break;

			case 'd':
				CHECK_RET( llsd_notation_parse_quoted( fin, &encoded, &enc_len ), FALSE );
				if ( !llsd_notation_decode_date( encoded, &real_val ) )
				{
					FREE( encoded );
					return FALSE;
				}
				CHECK_RET( (*(ops->date_fn))( real_val, user_data ), FALSE );
				break;

			case '[':
				CHECK_RET( (*(ops->array_begin_fn))( 0, user_data ), FALSE );
				break;

			case ']':
				CHECK_RET( (*(ops->array_end_fn))( user_data ), FALSE );
				break;
			
			case '{':
				CHECK_RET( (*(ops->map_begin_fn))( 0, user_data ), FALSE );
				break;

			case '}':
				CHECK_RET( (*(ops->map_end_fn))( user_data ), FALSE );
				break;

			/* eat whitespace and commas */
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case ',':
				break;
		}
	}

	return TRUE;
}


