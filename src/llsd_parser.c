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

typedef struct parser_state_s
{
	llsd_t * llsd;
	llsd_t * key;
	list_t * container_stack;

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

static int llsd_remove_from_container( llsd_t * container, llsd_t * const key )
{
	CHECK_PTR_RET( container, FALSE );
	if ( llsd_get_type( container ) == LLSD_MAP )
	{
		CHECK_PTR_RET( key, FALSE );
		if ( !llsd_map_remove( container, key ) )
		{
			DEBUG( "failed to clean up bad state!\n" );
			return FALSE;
		}
	}
	else
	{
		if ( !llsd_array_unappend( container ) )
		{
			DEBUG( "failed to clean up bad state!\n" );
			return FALSE;
		}
	}
	return TRUE;
}

static int llsd_update_parser_state( void * const user_data, llsd_t * const v )
{
	llsd_t * container = NULL;
	parser_state_t * state = (parser_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( v, FALSE );

	/* try to get the current container */
	container = list_get_head( state->container_stack );

	switch( llsd_get_type( v ) )
	{
		case LLSD_ARRAY:
		case LLSD_MAP:
			/* for containers */

			/* if there is a container, add this container to it */
			if ( container != NULL )
			{
				/* add the value to the container */
				if ( !llsd_add_to_container( container, state->key, v ) )
				{
					llsd_delete( v );
					return FALSE;
				}
			}

			/* now push this container on the container stack */
			if ( !list_push_head( state->container_stack, v ) )
			{
				CHECK_RET( llsd_remove_from_container( container, state->key ), FALSE );
				llsd_delete( v );	/* delete the value we couldn't store */
				/* if there was a key, delete it */
				if ( state->key != NULL )
				{
					llsd_delete( state->key );
				}
				state->key = NULL;	/* clear out the key pointer */
				return FALSE;
			}
			break;
		
		default:
			/* for non-containers */

			/* if there isn't a container, set the llsd to the literal */
			if ( container == NULL )
			{
				CHECK_RET( state->llsd == NULL, FALSE );
				state->llsd = v;
				return TRUE;
			}
			else if ( (llsd_get_type( container ) == LLSD_MAP) && 
					  (llsd_get_type( v ) == LLSD_STRING) &&
					  (state->key == NULL) )
			{
				/* the current container is a map, the value we're adding is a
				 * string and we don't have a key in the state yet.  this string
				 * must be a key in a key-value pair. */
				state->key = v;
				return TRUE;
			}

			/* add the value to the container */
			if ( !llsd_add_to_container( container, state->key, v ) )
			{
				llsd_delete( v );
				if ( state->key != NULL )
				{
					llsd_delete( state->key );
				}
				state->key = NULL;
				return FALSE;
			}
			state->key = NULL;
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

	CHECK_PTR_RET( container, FALSE );
	CHECK_RET( llsd_get_type( container ) == LLSD_ARRAY, FALSE );

	/* remove the container from the top of the stack */
	if ( !list_pop_head( state->container_stack ) )
	{
		DEBUG( "failed to unwind container stack\n" );
	}

	/* if the container stack is now empty, set the array as the llsd value
	 * and return */
	if ( list_count( state->container_stack ) == 0 )
	{
		state->llsd = container;
	}

	return TRUE;
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

	CHECK_PTR_RET( container, FALSE );
	CHECK_RET( llsd_get_type( container ) == LLSD_MAP, FALSE );

	/* remove the container from the stack */
	if ( !list_pop_head( state->container_stack ) )
	{
		DEBUG( "failed to unwind container stack\n" );
	}

	/* if the container stack is now empty, set the array as the llsd value
	 * and return */
	if ( list_count( state->container_stack ) == 0 )
	{
		state->llsd = container;
	}

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
		&llsd_array_end_fn,
		&llsd_map_begin_fn,
		&llsd_map_end_fn
	};

	CHECK_PTR_RET( fin, NULL );

	MEMSET( &state, 0, sizeof( parser_state_t ) );

	/* create the container stack */
	state.container_stack = list_new( 0, &llsd_delete );
	CHECK_PTR_RET( state.container_stack, NULL );
	
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

	list_delete( state.container_stack );

	if ( !ok )
	{
		llsd_delete( state.llsd );
		return NULL;
	}

	return state.llsd;
}

