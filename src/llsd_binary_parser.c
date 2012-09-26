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

#include "llsd_binary_parser.h"

#define BINARY_SIG_LEN (18)
static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";

int llsd_binary_check_sig_file( FILE * fin )
{
	size_t ret;
	uint8_t sig[BINARY_SIG_LEN];

	CHECK_PTR_RET( fin, FALSE );

	/* read the signature */
	ret = fread( sig, sizeof(uint8_t), BINARY_SIG_LEN, fin );

	/* rewind the file */
	rewind( fin );

	/* if it matches the signature, return TRUE, otherwise FALSE */
	return ( memcmp( sig, binary_header, BINARY_SIG_LEN ) == 0 )
}

int llsd_binary_parse_file( FILE * fin, llsd_parser_ops_t * const ops, void * const user_data )
{
	size_t ret;
	uint8_t p;
	uint8_t uuid[UUID_LEN];
	uint8_t * buffer;
	uint32_t be_int;
	uint64_t be_real;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	/* seek past signature */
	fseek( fin, BINARY_SIG_LEN, SEEK_SET );

	while( !feof( fin ) )
	{
		/* read the type marker */
		ret = fread( &p, sizeof(uint8_t), 1, fin );
		CHECK_RET( ret == 1, FALSE );

		switch( p )
		{

			case '!':
				CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
				break;

			case '1':
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				break;

			case '0':
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				break;

			case 'i':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				CHECK_RET( (*(ops->integer_fn))( ntohl( be_int ), user_data ), FALSE );
				break;

			case 'r':
				CHECK_RET( fread( &be_real, sizeof(uint64_t), 1, fin ) == 1, FALSE );
				CHECK_RET( (*(ops->real_fn))( (double)be64toh( be_real ), user_data ), FALSE );
				break;

			case 'u':
				CHECK_RET( fread( uuid, sizeof(uint8_t), UUID_LEN, fin ) == UUID_LEN, FALSE );
				CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
				break;

			case 'b':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				buffer = CALLOC( be_int, sizeof(uint8_t) );
				CHECK_PTR_RET( buffer, FALSE );
				ret = fread( buffer, sizeof(uint8_t), be_int, fin );
				if ( ret != be_int )
				{
					FREE( buffer );
					return FALSE;
				}
				buffer = NULL;
				CHECK_RET( (*(ops->binary_fn))( buffer, be_int, user_data ), FALSE );
				break;

			case 's':
				/* in the binary format, strings are the raw byte values */
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				buffer = CALLOC( be_int + 1, sizeof(uint8_t) ); /* add a null byte at the end */
				CHECK_PTR_RET( buffer, FALSE );
				ret = fread( buffer, sizeof(uint8_t), be_int, fin );
				if ( ret != be_int )
				{
					FREE( buffer );
					return FALSE;
				}
				buffer = NULL;
				CHECK_RET( (*(ops->string_fn))( buffer, user_data ), FALSE );
				break;

			case 'l':
				/* in the binary format, uri's are the raw byte values */
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				buffer = CALLOC( be_int + 1, sizeof(uint8_t) ); /* add a null byte at the end */
				CHECK_PTR_RET( buffer, FALSE );
				ret = fread( buffer, sizeof(uint8_t), be_int, fin );
				if ( ret != be_int )
				{
					FREE( buffer );
					return FALSE;
				}
				buffer = NULL;
				CHECK_RET( (*(ops->uri_fn))( buffer, user_data ), FALSE );
				break;

			case 'd':
				CHECK_RET( fread( &be_real, sizeof(double), 1, fin ) == 1, FALSE );
				CHECK_RET( (*(ops->date_fn))( (double)be64toh( be_real ), user_data ), FALSE );
				break;

			case '[':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				CHECK_RET( (*(ops->array_begin_fn))( be_int, user_data ), FALSE );
				break;

			case ']':
				CHECK_RET( (*(ops->array_end_fn))( user_data ), FALSE );
				break;
			
			case '{':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				CHECK_RET( (*(ops->map_begin_fn))( be_int, user_data ), FALSE );
				break;

			case '}':
				CHECK_RET( (*(ops->map_end_fn))( user_data ), FALSE );
				break;
		}
	}

	return TRUE;
}


