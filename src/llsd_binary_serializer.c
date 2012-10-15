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

#include <cutil/list.h>
#include "llsd_serializer.h"
#include "llsd_binary_serializer.h"

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

#define VALUE_STATES (TOP_LEVEL | ARRAY_START | ARRAY_VALUE_END | MAP_KEY_END )
#define STRING_STATES ( VALUE_STATES | MAP_START | MAP_VALUE_END )

#define PUSH(x) (list_push_head( state->step_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( state->step_stack ))
#define POP		(list_pop_head( state->step_stack))

typedef struct bs_state_s
{
	list_t* step_stack;
	FILE * fout;
} bs_state_t;

static int update_state( uint32_t valid_states, llsd_type_t type_, bs_state_t * state )
{
	/* make sure we have a valid LLSD type */
	CHECK_RET( IS_VALID_LLSD_TYPE( type_ ), FALSE );
	
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );

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


static int llsd_binary_undef( void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_UNDEF, state ), FALSE );
	CHECK_RET( fwrite( "!", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_boolean( int const value, void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, state ), FALSE );
	CHECK_RET( fwrite( (value ? "1" : "0"), sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_integer( int32_t const value, void * const user_data )
{
	uint32_t be = htonl( value );
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_INTEGER, state ), FALSE );
	CHECK_RET( fwrite( "i", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_real( double const value, void * const user_data )
{
	uint64_t be = htobe64( *((uint64_t*)&value) );
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_REAL, state ), FALSE );
	CHECK_RET( fwrite( "r", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint64_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_uuid( uint8_t const value[UUID_LEN], void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_UUID, state ), FALSE );
	CHECK_RET( fwrite( "u", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( value, sizeof(uint8_t), UUID_LEN, state->fout ) == UUID_LEN, FALSE );
	return TRUE;
}

static int llsd_binary_string( uint8_t const * str, int const own_it, void * const user_data )
{
	uint32_t be = 0;
	uint32_t len = 0;
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( STRING_STATES, LLSD_STRING, state ), FALSE );
	len = strlen( str );
	be = htonl( len );
	CHECK_RET( fwrite( "s", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( str, sizeof(uint8_t), len, state->fout ) == len, FALSE );
	return TRUE;
}

static int llsd_binary_date( double const value, void * const user_data )
{
	uint64_t be = htobe64( *((uint64_t*)&value) );
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_DATE, state ), FALSE );
	CHECK_RET( fwrite( "d", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint64_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_uri( uint8_t const * uri, int const own_it, void * const user_data )
{
	uint32_t be = 0;
	uint32_t len = 0;
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_URI, state ), FALSE );
	len = strlen( uri );
	be = htonl( len );
	CHECK_RET( fwrite( "l", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( uri, sizeof(uint8_t), len, state->fout ) == len, FALSE );
	return TRUE;
}

static int llsd_binary_binary( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data )
{
	uint32_t be = 0;
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_BINARY, state ), FALSE );
	be = htonl( len );
	CHECK_RET( fwrite( "b", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( data, sizeof(uint8_t), len, state->fout ) == len, FALSE );
	return TRUE;
}

static int llsd_binary_array_begin( uint32_t const size, void * const user_data )
{
	uint32_t be = 0;
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_ARRAY, state ), FALSE );
	PUSH( ARRAY_START );
	be = htonl( size );
	CHECK_RET( fwrite( "[", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_array_value_end( void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after an array value */
	CHECK_RET( (TOP & ARRAY_VALUE), FALSE );
	POP;
	PUSH( ARRAY_VALUE_END );
	return TRUE;
}

static int llsd_binary_array_end( uint32_t const size, void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only happens after array start or array value; never after array value end */
	CHECK_RET( (TOP & (ARRAY_START | ARRAY_VALUE)), FALSE );
	POP;
	CHECK_RET( fwrite( "]", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_map_begin( uint32_t const size, void * const user_data )
{
	uint32_t be = 0;
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_MAP, state ), FALSE );
	PUSH( MAP_START );
	be = htonl( size );
	CHECK_RET( fwrite( "{", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	CHECK_RET( fwrite( &be, sizeof(uint32_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_binary_map_key_end( void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after a map key */
	CHECK_RET( (TOP & MAP_KEY), FALSE );
	POP;
	PUSH( MAP_KEY_END );
	return TRUE;
}

static int llsd_binary_map_value_end( void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after a map value, and never before a map end */
	CHECK_RET( (TOP & MAP_VALUE), FALSE );
	POP;
	PUSH( MAP_VALUE_END );
	return TRUE;
}

static int llsd_binary_map_end( uint32_t const size, void * const user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( (TOP & (MAP_START | MAP_VALUE)), FALSE );
	POP;
	CHECK_RET( fwrite( "}", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

#define BINARY_SIG_LEN (18)
static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";

int llsd_binary_serializer_init( FILE * fout, llsd_ops_t * const ops, int const pretty, void ** const user_data )
{
	bs_state_t * state = NULL;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( ops, FALSE );
	CHECK_PTR_RET( user_data, FALSE );

	/* give the serializer the proper callback functions */
	(*ops) = (llsd_ops_t){
		&llsd_binary_undef,
		&llsd_binary_boolean,
		&llsd_binary_integer,
		&llsd_binary_real,
		&llsd_binary_uuid,
		&llsd_binary_string,
		&llsd_binary_date,
		&llsd_binary_uri,
		&llsd_binary_binary,
		&llsd_binary_array_begin,
		&llsd_binary_array_value_end,
		&llsd_binary_array_end,
		&llsd_binary_map_begin,
		&llsd_binary_map_key_end,
		&llsd_binary_map_value_end,
		&llsd_binary_map_end 
	};

	/* write out the binary signature */
	CHECK_RET( fwrite( binary_header, sizeof(uint8_t), BINARY_SIG_LEN, fout ) == BINARY_SIG_LEN, FALSE );

	/* allocate the serializer state and store the file pointer */
	state = CALLOC( 1, sizeof(bs_state_t) );
	CHECK_PTR_RET( state, FALSE );
	state->fout = fout;
	state->step_stack = list_new( 1, NULL );
	if ( state->step_stack == NULL )
	{
		FREE( state );
		return FALSE;
	}
	PUSH( TOP_LEVEL );

	/* return the state as the user_date */
	(*user_data) = state;

	return TRUE;
}


int llsd_binary_serializer_deinit( FILE * fout, void * user_data )
{
	bs_state_t * state = (bs_state_t*)user_data;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( state, FALSE );
	
	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	list_delete( state->step_stack );

	FREE( state );
	return TRUE;
}


