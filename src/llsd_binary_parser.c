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
#include <cutil/list.h>

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

typedef struct bs_state_s
{
	list_t * state_stack;
	llsd_ops_t * ops;
	void * user_data;
} bs_state_t;

#define PUSH(x) (list_push_head( parser_state->state_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( parser_state->state_stack ))
#define POP		(list_pop_head( parser_state->state_stack))

#define BEGIN_VALUE_STATES ( TOP_LEVEL | ARRAY_BEGIN | ARRAY_VALUE_END | MAP_KEY_END )
#define BEGIN_STRING_STATES ( VALUE_STATES | MAP_BEGIN )
static int begin_value( uint32_t valid_states, llsd_type_t type_, bs_state_t * parser_state )
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
static int value( uint32_t valid_states, llsd_type_t type_, bs_state_t * parser_state )
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
#define END_STRING_STATES ( VALUE_STATES | MAP_KEY )
static int end_value( uint32_t valid_states, llsd_type_t type_, bs_state_t * parser_state )
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

int llsd_binary_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data )
{
	size_t ret;
	uint8_t p = '\0';
	uint8_t uuid[UUID_LEN];
	uint8_t * buffer;
	uint32_t be_int;
	uint64_t be_real;
	bs_state_t * parser_state = NULL;

	CHECK_PTR_RET( fin, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	/* set up step stack, used to synthesize array value end, map key end, 
	 * and map value end callbacks */
	parser_state = CALLOC( 1, sizeof(bs_state_t) );
	CHECK_PTR_RET( parser_state, FALSE );
	parser_state->state_stack = list_new( 1, NULL );
	CHECK_PTR_RET( parser_state->state_stack, FALSE );
	parser_state->ops = ops;
	parser_state->user_data = user_data;

	/* start in the top level state */
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
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_UNDEF, parser_state ), FALSE );
				CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_UNDEF, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_UNDEF, parser_state ), FALSE );
				break;

			case '1':
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( TRUE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				break;

			case '0':
				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( (*(ops->boolean_fn))( FALSE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_BOOLEAN, parser_state ), FALSE );
				break;

			case 'i':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_INTEGER, parser_state ), FALSE );
				CHECK_RET( (*(ops->integer_fn))( ntohl( be_int ), user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_INTEGER, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_INTEGER, parser_state ), FALSE );
				break;

			case 'r':
				CHECK_RET( fread( &be_real, sizeof(uint64_t), 1, fin ) == 1, FALSE );
				be_real = be64toh( be_real );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_REAL, parser_state ), FALSE );
				CHECK_RET( (*(ops->real_fn))( *((double*)&be_real), user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_REAL, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_REAL, parser_state ), FALSE );
				break;

			case 'u':
				CHECK_RET( fread( uuid, sizeof(uint8_t), UUID_LEN, fin ) == UUID_LEN, FALSE );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_UUID, parser_state ), FALSE );
				CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_UUID, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_UUID, parser_state ), FALSE );
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

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_BINARY, parser_state ), FALSE );
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->binary_fn))( buffer, be_int, TRUE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_BINARY, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_BINARY, parser_state ), FALSE );

				buffer = NULL;
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

				CHECK_RET( begin_value( BEGIN_STRING_STATES, LLSD_STRING, parser_state ), FALSE );
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->string_fn))( buffer, TRUE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_STRING, parser_state ), FALSE );
				CHECK_RET( end_value( END_STRING_STATES, LLSD_STRING, parser_state ), FALSE );

				buffer = NULL;
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

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_URI, parser_state ), FALSE );
				/* tell it to take ownership of the memory */
				CHECK_RET( (*(ops->uri_fn))( buffer, TRUE, user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_URI, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_URI, parser_state ), FALSE );

				buffer = NULL;
				break;

			case 'd':
				CHECK_RET( fread( &be_real, sizeof(double), 1, fin ) == 1, FALSE );
				be_real = be64toh( be_real );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_DATE, parser_state ), FALSE );
				CHECK_RET( (*(ops->date_fn))( *((double*)&be_real), user_data ), FALSE );
				CHECK_RET( value( VALUE_STATES, LLSD_DATE, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_DATE, parser_state ), FALSE );
				break;

			case '[':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_ARRAY, parser_state ), FALSE );
				CHECK_RET( (*(ops->array_begin_fn))( be_int, user_data ), FALSE );
				PUSH( ARRAY_BEGIN );
				break;

			case ']':
				CHECK_RET( (*(ops->array_end_fn))( 0, user_data ), FALSE );
				POP;
				CHECK_RET( value( VALUE_STATES, LLSD_ARRAY, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_ARRAY, parser_state ), FALSE );
				break;
			
			case '{':
				CHECK_RET( fread( &be_int, sizeof(uint32_t), 1, fin ) == 1, FALSE );
				be_int = ntohl( be_int );

				CHECK_RET( begin_value( BEGIN_VALUE_STATES, LLSD_MAP, parser_state ), FALSE );
				CHECK_RET( (*(ops->map_begin_fn))( be_int, user_data ), FALSE );
				PUSH( MAP_BEGIN );
				break;

			case '}':
				CHECK_RET( (*(ops->map_end_fn))( 0, user_data ), FALSE );
				POP;
				CHECK_RET( value( VALUE_STATES, LLSD_MAP, parser_state ), FALSE );
				CHECK_RET( end_value( END_VALUE_STATES, LLSD_MAP, parser_state ), FALSE );
				break;

			default:
				DEBUG("invalid type byte: %c\n", p );
				return FALSE;
		}
	}

	/* clean up the step stack */
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( parser_state->state_stack );

	/* clean up the parser_state */
	FREE( parser_state );

	return TRUE;
}


