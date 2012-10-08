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

typedef enum parser_step_e
{
	PARSER_START,
	PARSER_ARRAY_CONTAINER,
	PARSER_MAP_CONTAINER,
	PARSER_MAP_HAVE_KEY,
	PARSER_DONE,
	PARSER_ERROR

} parser_step_t;

typedef struct parser_state_s
{
	llsd_t * llsd;
	llsd_t * key;
	list_t * container_stack;
	parser_step_t step;

} parser_state_t;

static int llsd_add_to_container( llsd_t * const container, llsd_t * const key, llsd_t * const value )
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

static int llsd_update_parser_state( void * const user_data, llsd_t * const v )
{
	llsd_t * container = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( v, FALSE );

	switch ( state->step )
	{
		case PARSER_START:
			/* v is array	=> PARSER_ARRAY_CONTAINER
			 * v is map		=> PARSER_MAP_CONTAINER
			 * v is other	=> PARSER_DONE */

			if ( list_count( state->container_stack ) != 0 )
				goto parser_error;

			/* handle containers */
			if ( (llsd_get_type( v ) == LLSD_ARRAY) || (llsd_get_type( v ) == LLSD_MAP) )
			{
				/* push the container on the container stack */
				if ( !list_push_head( state->container_stack, v ) )
				{
					llsd_delete( v );
					goto parser_error;
				}

				/* we cleanly made the state transition so update the state value */
				state->step = (llsd_get_type( v ) == LLSD_ARRAY) ? 
								PARSER_ARRAY_CONTAINER :
								PARSER_MAP_CONTAINER;
				return TRUE;
			}
			/* handle integral types */
			else
			{
				/* v is a non-container, so set it as the result and move to 
				 * the PARSER_DONE state. */
				state->llsd = v;
				state->step = PARSER_DONE;
				return TRUE;
			}

			break;

		case PARSER_ARRAY_CONTAINER:
			/* v is array	=> PARSER_ARRAY_CONTAINER
			 * v is map		=> PARSER_MAP_CONTAINER
			 * v is other	=> PARSER_ARRAY_CONTAINER */

			if ( list_count( state->container_stack ) == 0 )
				goto parser_error;

			/* get the current container */
			container = list_get_head( state->container_stack );
			if ( container == NULL )
				goto parser_error;
			if ( llsd_get_type( container ) != LLSD_ARRAY )
				goto parser_error;

			/* add v to the array */
			if ( !llsd_add_to_container( container, NULL, v ) )
			{
				llsd_delete( v );
				goto parser_error;
			}

			/* push containers */
			if ( (llsd_get_type( v ) == LLSD_ARRAY) || (llsd_get_type( v ) == LLSD_MAP) )
			{
				/* push the container on the container stack */
				if ( !list_push_head( state->container_stack, v ) )
				{
					llsd_delete( v );
					goto parser_error;
				}
				state->step = (llsd_get_type( v ) == LLSD_ARRAY) ? 
								PARSER_ARRAY_CONTAINER :
								PARSER_MAP_CONTAINER;
			}
			break;

		case PARSER_MAP_CONTAINER:
			/* v is string	=> PARSER_MAP_HAVE_KEY
			 * v is other	=> PARSER_ERROR */

			if ( list_count( state->container_stack ) == 0 )
				goto parser_error;

			/* get the current container */
			container = list_get_head( state->container_stack );
			if ( container == NULL )
				goto parser_error;
			if ( llsd_get_type( container ) != LLSD_MAP )
				goto parser_error;
			if ( llsd_get_type( v ) != LLSD_STRING )
				goto parser_error;

			/* store the key, move to the second part of the map state */
			state->key = v;
			state->step = PARSER_MAP_HAVE_KEY;
			break;

		case PARSER_MAP_HAVE_KEY:
			/* v is array	=> PARSER_ARRAY_CONTAINER
			 * v is map		=> PARSER_MAP_CONTAINER
			 * v is other	=> PARSER_MAP_CONTAINER */

			if ( list_count( state->container_stack ) == 0 )
				goto parser_error;

			/* get the current container */
			container = list_get_head( state->container_stack );
			if ( container == NULL )
				goto parser_error;
			if ( llsd_get_type( container ) != LLSD_MAP )
				goto parser_error;

			/* make sure we have a string key */
			if ( state->key == NULL )
				goto parser_error;
			if ( llsd_get_type( state->key ) != LLSD_STRING )
				goto parser_error;

			/* add v to the map */
			if ( !llsd_add_to_container( container, state->key, v ) )
			{
				llsd_delete( v );
				goto parser_error;
			}

			/* drop our reference to the key */
			state->key = NULL;

			/* push containers */
			if ( (llsd_get_type( v ) == LLSD_ARRAY) || (llsd_get_type( v ) == LLSD_MAP) )
			{
				/* push the container on the container stack */
				if ( !list_push_head( state->container_stack, v ) )
				{
					llsd_delete( v );
					goto parser_error;
				}
				state->step = (llsd_get_type( v ) == LLSD_ARRAY) ? 
								PARSER_ARRAY_CONTAINER :
								PARSER_MAP_CONTAINER;
			}
			else
			{
				/* we're still in the map container state */
				state->step = PARSER_MAP_CONTAINER;
			}
			break;

		/* should never call this function when in DONE/ERROR state */
		case PARSER_DONE:
		case PARSER_ERROR:
			goto parser_error;
	}

	return TRUE;

parser_error:
	state->step = PARSER_ERROR;
	return FALSE;
}

static int llsd_undef_fn( void * const user_data )
{
	llsd_t * v = NULL;
	
	/* create the undef */
	v = llsd_new_undef();
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_boolean_fn( int const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the boolean */
	v = llsd_new_boolean( value );
	CHECK_PTR_RET( v, FALSE );
	
	return llsd_update_parser_state( user_data, v );
}

static int llsd_integer_fn( int32_t const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the integer */
	v = llsd_new_integer( value );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_real_fn( double const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the real */
	v = llsd_new_real( value );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_uuid_fn( uint8_t const value[UUID_LEN], void * const user_data )
{
	llsd_t * v = NULL;

	/* create the uuid */
	v = llsd_new_uuid( value );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_string_fn( uint8_t const * str, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the string */
	v = llsd_new_string( str, own_it );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_date_fn( double const value, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the date */
	v = llsd_new_date( value );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_uri_fn( uint8_t const * uri, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the uri */
	v = llsd_new_uri( uri, own_it );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_binary_fn( uint8_t const * data, uint32_t const len, int own_it, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the binary */
	v = llsd_new_binary( data, len, own_it );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_array_begin_fn( uint32_t const size, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the array */
	v = llsd_new_array( size );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_array_end_fn( void * const user_data )
{
	llsd_t * container = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	/* try to get the current container */
	container = list_get_head( state->container_stack );
	if ( container == NULL )
		goto parser_error_array_end;
	if ( llsd_get_type( container ) != LLSD_ARRAY )
		goto parser_error_array_end;

	/* remove the container from the top of the stack */
	if ( !list_pop_head( state->container_stack ) )
		goto parser_error_array_end;

	/* if the container stack is now empty, set the array as the llsd value
	 * and return */
	if ( list_count( state->container_stack ) == 0 )
	{
		state->step = PARSER_DONE;
		state->llsd = container;
	}
	else
	{
		/* try to get the new current container */
		container = list_get_head( state->container_stack );
		if ( container == NULL )
			goto parser_error_array_end;

		/* we cleanly made the state transition so update the state value */
		state->step = (llsd_get_type( container ) == LLSD_ARRAY) ? 
						PARSER_ARRAY_CONTAINER :
						PARSER_MAP_CONTAINER;
	}

	return TRUE;

parser_error_array_end:
	state->step = PARSER_ERROR;
	return FALSE;
}

static int llsd_map_begin_fn( uint32_t const size, void * const user_data )
{
	llsd_t * v = NULL;

	/* create the map */
	v = llsd_new_map( size );
	CHECK_PTR_RET( v, FALSE );

	return llsd_update_parser_state( user_data, v );
}

static int llsd_map_end_fn( void * const user_data )
{
	llsd_t * container = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	/* try to get the current container */
	container = list_get_head( state->container_stack );
	if ( container == NULL )
		goto parser_error_map_end;
	if ( llsd_get_type( container ) != LLSD_MAP )
		goto parser_error_map_end;

	/* remove the container from the stack */
	if ( !list_pop_head( state->container_stack ) )
		goto parser_error_map_end;

	/* if the container stack is now empty, set the array as the llsd value
	 * and return */
	if ( list_count( state->container_stack ) == 0 )
	{
		state->step = PARSER_DONE;
		state->llsd = container;
	}
	else
	{
		/* try to get the new current container */
		container = list_get_head( state->container_stack );
		if ( container == NULL )
			goto parser_error_map_end;

		/* we cleanly made the state transition so update the state value */
		state->step = (llsd_get_type( container ) == LLSD_ARRAY) ? 
						PARSER_ARRAY_CONTAINER :
						PARSER_MAP_CONTAINER;
	}

	return TRUE;

parser_error_map_end:
	state->step = PARSER_ERROR;
	return FALSE;
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
		&llsd_array_end_fn,
		&llsd_map_begin_fn,
		&llsd_map_end_fn
	};

	CHECK_PTR_RET( fin, NULL );

	/* initialize the parser state */
	MEMSET( &state, 0, sizeof( parser_state_t ) );
	state.container_stack = list_new( 0, &llsd_delete );
	CHECK_PTR_RET( state.container_stack, NULL );
	state.step = PARSER_START;
	
	if ( llsd_binary_check_sig_file( fin ) )
	{
		ok = llsd_binary_parse_file( fin, &ops, &state );
	}
#if 0
	else if ( llsd_notation_check_sig_file( fin ) )
	{
		ok = llsd_notation_parse_file( fin, &ops, &state );
	}
	else if ( llsd_xml_check_sig_file( fin ) )
	{
		ok = llsd_xml_parse_file( fin, &ops, &state );
	}
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

	if ( state.step != PARSER_DONE )
	{
		ok = FALSE;
	}

	list_delete( state.container_stack );

	if ( !ok )
	{
		llsd_delete( state.llsd );
		return NULL;
	}

	return state.llsd;
}

