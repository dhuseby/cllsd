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

#define DEBUG_ON
#include <cutil/debug.h>
#include <cutil/macros.h>

#include "llsd.h"
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
	return ( memcmp( sig, binary_header, BINARY_SIG_LEN ) == 0 );
}

typedef enum bs_step_e
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
} bs_step_t;

#define VALUE_STATES (TOP_LEVEL | ARRAY_START | ARRAY_VALUE | MAP_KEY )
#define STRING_STATES ( VALUE_STATES | MAP_START | MAP_VALUE )

#define PUSH(x) (list_push_head( step_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( step_stack ))
#define POP		(list_pop_head( step_stack))

static int update_state( uint32_t valid_states, llsd_type_t type_, 
						 list_t * step_stack, llsd_ops_t * const ops, 
						 void * const user_data )
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
				case ARRAY_VALUE:
					CHECK_RET( (*(ops->array_value_end_fn))( user_data ), FALSE );
					/* fall through */
				case ARRAY_START:
					POP;
					PUSH( ARRAY_VALUE );
					break;
				case MAP_KEY:
					CHECK_RET( (*(ops->map_key_end_fn))( user_data ), FALSE );
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
				case ARRAY_VALUE:
					CHECK_RET( (*(ops->array_value_end_fn))( user_data ), FALSE );
					/* fall through */
				case ARRAY_START:
					POP;
					PUSH( ARRAY_VALUE );
					break;
				case MAP_KEY:
					CHECK_RET( (*(ops->map_key_end_fn))( user_data ), FALSE );
					POP;
					PUSH( MAP_VALUE );
					break;
				case MAP_VALUE:
					CHECK_RET( (*(ops->map_value_end_fn))( user_data ), FALSE );
					/* fall through */
				case MAP_START:
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



int llsd_binary_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data )
{
	size_t ret;
	uint8_t p = '\0';
	uint8_t uuid[UUID_LEN];
	uint8_t * buffer;
	uint32_t be_int;
	uint64_t be_real;
	list_t * step_stack = NULL;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	/* set up step stack, used to synthesize array value end, map key end, 
	 * and map value end callbacks */
	step_stack = list_new( 1, NULL );
	CHECK_PTR_RET( step_stack, FALSE );
	PUSH( TOP_LEVEL );

	/* seek past signature */
	fseek( fin, BINARY_SIG_LEN, SEEK_SET );

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
				CHECK_RET( update_state( VALUE_STATES, LLSD_UNDEF, step_stack, ops, user_data ), FALSE );
				CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
				break;

			case '1':
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack, ops, user_data ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				break;

			case '0':
				CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, step_stack, ops, user_data ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				break;

			case 'i':
				CHECK_RET( update_state( VALUE_STATES, LLSD_INTEGER, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				CHECK_RET( (*(ops->integer_fn))( ntohl( be_int ), user_data ), FALSE );
				break;

			case 'r':
				CHECK_RET( update_state( VALUE_STATES, LLSD_REAL, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( &be_real, sizeof(uint64_t), 1, fin ) == 1, FALSE );
				be_real = be64toh( be_real );
				CHECK_RET( (*(ops->real_fn))( *((double*)&be_real), user_data ), FALSE );
				break;

			case 'u':
				CHECK_RET( update_state( VALUE_STATES, LLSD_UUID, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( uuid, sizeof(uint8_t), UUID_LEN, fin ) == UUID_LEN, FALSE );
				CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
				break;

			case 'b':
				CHECK_RET( update_state( VALUE_STATES, LLSD_BINARY, step_stack, ops, user_data ), FALSE );
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
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->binary_fn))( buffer, be_int, TRUE, user_data ), FALSE );
				buffer = NULL;
				break;

			case 's':
				CHECK_RET( update_state( STRING_STATES, LLSD_STRING, step_stack, ops, user_data ), FALSE );
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
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
				buffer = NULL;
				break;

			case 'l':
				CHECK_RET( update_state( VALUE_STATES, LLSD_URI, step_stack, ops, user_data ), FALSE );
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
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->uri_fn))( buffer, TRUE, user_data ), FALSE );
				buffer = NULL;
				break;

			case 'd':
				CHECK_RET( update_state( VALUE_STATES, LLSD_DATE, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( &be_real, sizeof(double), 1, fin ) == 1, FALSE );
				be_real = be64toh( be_real );
				CHECK_RET( (*(ops->date_fn))( *((double*)&be_real), user_data ), FALSE );
				break;

			case '[':
				CHECK_RET( update_state( VALUE_STATES, LLSD_ARRAY, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				CHECK_RET( (*(ops->array_begin_fn))( be_int, user_data ), FALSE );
				PUSH( ARRAY_START );
				break;

			case ']':
				CHECK_RET( (*(ops->array_end_fn))( user_data ), FALSE );
				POP;
				break;
			
			case '{':
				CHECK_RET( update_state( VALUE_STATES, LLSD_MAP, step_stack, ops, user_data ), FALSE );
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );
				CHECK_RET( (*(ops->map_begin_fn))( be_int, user_data ), FALSE );
				PUSH( MAP_START );
				break;

			case '}':
				CHECK_RET( (*(ops->map_end_fn))( user_data ), FALSE );
				POP;
				break;

			default:
				DEBUG("invalid type byte: %c\n", p );
				return FALSE;
		}
	}

	/* clean up the step stack */
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( step_stack );

	return TRUE;
}


