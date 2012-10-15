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

#include "llsd.h"
#include "llsd_parser.h"
#include "llsd_binary_parser.h"
#include "llsd_notation_parser.h"

typedef enum step_e
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
} step_t;

#define VALUE_STATES (TOP_LEVEL | ARRAY_START | ARRAY_VALUE_END | MAP_KEY_END )
#define STRING_STATES ( VALUE_STATES | MAP_START | MAP_VALUE_END )

#define PUSH(x) (list_push_head( state->step_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( state->step_stack ))
#define POP		(list_pop_head( state->step_stack ))

#define PUSHC(x) (list_push_head( state->container_stack, (void*)x ))
#define TOPC	((llsd_t*)list_get_head( state->container_stack ))
#define POPC	(list_pop_head( state->container_stack ))

typedef struct parser_state_s
{
	llsd_t * llsd;
	llsd_t * key;
	list_t * container_stack;
	list_t * step_stack;

} parser_state_t;

static int add_to_container( llsd_t * const container, llsd_t * const key, llsd_t * const value )
{
	CHECK_PTR_RET( container, FALSE );
	CHECK_PTR_RET( value, FALSE );

	if ( llsd_get_type( container ) == LLSD_MAP )
	{
		CHECK_PTR_RET( key, FALSE );
		CHECK_RET( llsd_map_insert( container, key, value ), FALSE );
	}
	else
	{
		CHECK_RET( llsd_get_type( container ) == LLSD_ARRAY, FALSE );
		CHECK_RET( llsd_array_append( container, value ), FALSE );
	}
	return TRUE;
}

static int update_state( uint32_t valid_states, void * const user_data, llsd_t * const v )
{
	llsd_t * container = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );

	/* make sure we're in a valid state */
	CHECK_RET( (TOP & valid_states), FALSE );

	switch ( llsd_get_type( v ) )
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
					CHECK_RET( add_to_container( TOPC, NULL, v ), FALSE );
					break;
				case MAP_KEY_END:
					POP;
					PUSH( MAP_VALUE );
					CHECK_RET( add_to_container( TOPC, state->key, v ), FALSE );
					state->key = NULL;
					break;
				case TOP_LEVEL:
					state->llsd = v;
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
					CHECK_RET( add_to_container( TOPC, NULL, v ), FALSE );
					break;
				case MAP_KEY_END:
					POP;
					PUSH( MAP_VALUE );
					CHECK_RET( add_to_container( TOPC, state->key, v ), FALSE );
					state->key = NULL;
					break;
				case MAP_START:
				case MAP_VALUE_END:
					POP;
					PUSH( MAP_KEY );
					state->key = v;
					break;
				case TOP_LEVEL:
					state->llsd = v;
					break;
			}
		break;
	}
	return TRUE;
}

static int llsd_undef_fn( void * const user_data )
{
	llsd_t * v = NULL;
	
	/* create the undef */
	v = llsd_new_undef();
	CHECK_PTR_RET( v, FALSE );
	
	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_boolean_fn( int const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the boolean */
	v = llsd_new_boolean( value );
	CHECK_PTR_RET( v, FALSE );
	
	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_integer_fn( int32_t const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the integer */
	v = llsd_new_integer( value );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_real_fn( double const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the real */
	v = llsd_new_real( value );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_uuid_fn( uint8_t const value[UUID_LEN], void * const user_data )
{
	llsd_t * v = NULL;

	/* create the uuid */
	v = llsd_new_uuid( value );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_string_fn( uint8_t const * str, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the string */
	v = llsd_new_string( str, own_it );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( STRING_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_date_fn( double const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the date */
	v = llsd_new_date( value );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_uri_fn( uint8_t const * uri, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the uri */
	v = llsd_new_uri( uri, own_it );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_binary_fn( uint8_t const * data, uint32_t const len, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the binary */
	v = llsd_new_binary( data, len, own_it );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}
	return TRUE;
}

static int llsd_array_begin_fn( uint32_t const size, void * const user_data )
{
	llsd_t * v = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );

	/* create the array */
	v = llsd_new_array( size );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}

	PUSHC( v );
	PUSH( ARRAY_START );
	return TRUE;
}

static int llsd_array_value_end_fn( void * const user_data )
{
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );
	CHECK_RET( (TOP & ARRAY_VALUE), FALSE );

	POP;
	PUSH( ARRAY_VALUE_END );
	return TRUE;
}

static int llsd_array_end_fn( uint32_t const size, void * const user_data )
{
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );
	CHECK_RET( (TOP & (ARRAY_START | ARRAY_VALUE)), FALSE );

	POPC;
	POP;
	return TRUE;
}

static int llsd_map_begin_fn( uint32_t const size, void * const user_data )
{
	llsd_t * v = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );

	/* create the map */
	v = llsd_new_map( size );
	CHECK_PTR_RET( v, FALSE );

	if ( !update_state( VALUE_STATES, user_data, v ) )
	{
		llsd_delete( v );
		return FALSE;
	}

	PUSHC( v );
	PUSH( MAP_START );
	return TRUE;
}

static int llsd_map_key_end_fn( void * const user_data )
{
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );
	CHECK_RET( (TOP & MAP_KEY), FALSE );

	POP;
	PUSH( MAP_KEY_END );
	return TRUE;
}

static int llsd_map_value_end_fn( void * const user_data )
{
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );
	CHECK_RET( (TOP & MAP_VALUE), FALSE );

	POP;
	PUSH( MAP_VALUE_END );
	return TRUE;
}

static int llsd_map_end_fn( uint32_t const size, void * const user_data )
{
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( state->step_stack, FALSE );
	CHECK_RET( (TOP & (MAP_START | MAP_VALUE)), FALSE );

	POPC;
	POP;
	return TRUE;
}

llsd_t * llsd_parse_from_file( FILE * fin )
{
	int ok = FALSE;
	parser_state_t state;
	llsd_ops_t ops = 
	{
		&llsd_undef_fn,
		&llsd_boolean_fn,
		&llsd_integer_fn,
		&llsd_real_fn,
		&llsd_uuid_fn,
		&llsd_string_fn,
		&llsd_date_fn,
		&llsd_uri_fn,
		&llsd_binary_fn,
		&llsd_array_begin_fn,
		&llsd_array_value_end_fn,
		&llsd_array_end_fn,
		&llsd_map_begin_fn,
		&llsd_map_key_end_fn,
		&llsd_map_value_end_fn,
		&llsd_map_end_fn
	};

	CHECK_PTR_RET( fin, NULL );

	/* initialize the parser state */
	MEMSET( &state, 0, sizeof( parser_state_t ) );
	state.container_stack = list_new( 0, &llsd_delete );
	CHECK_PTR_RET( state.container_stack, NULL );
	state.step_stack = list_new( 1, NULL );
	if ( state.step_stack == NULL )
	{
		list_delete( state.container_stack );
		return NULL;
	}
	list_push_head( state.step_stack, (void*)TOP_LEVEL );
	
	if ( llsd_binary_check_sig_file( fin ) )
	{
		ok = llsd_binary_parse_file( fin, &ops, &state );
	}
	else if ( llsd_notation_check_sig_file( fin ) )
	{
		ok = llsd_notation_parse_file( fin, &ops, &state );
	}
	else if ( llsd_xml_check_sig_file( fin ) )
	{
		ok = llsd_xml_parse_file( fin, &ops, &state );
	}
#if 0
	else if ( llsd_json_check_sig_file( fin ) )
	{
		ok = llsd_json_parse_file( fin, &ops, &state );
	}
#endif

	/* make sure we had a complete parse */
	if ( list_count( state.container_stack ) > 0 )
	{
		ok = FALSE;
	}

	if ( (step_t)list_get_head( state.step_stack ) != TOP_LEVEL )
	{
		ok = FALSE;
	}

	list_delete( state.container_stack );
	list_delete( state.step_stack );

	if ( !ok )
	{
		return NULL;
	}

	return state.llsd;
}

