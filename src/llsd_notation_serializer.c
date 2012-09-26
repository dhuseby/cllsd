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

#include "llsd_serializer.h"
#include "llsd_notation_serializer.h"

typedef struct ns_state_s
{
	FILE * fout;
} ns_state_t;

int llsd_notation_undef( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_boolean( int const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_integer( int32_t const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_real( double const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_uuid( uint8_t const value[UUID_LEN], void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_string( uint8_t const * str, int const own_it, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_date( double const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_uri( uint8_t const * uri, int const own_it, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_binary( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_array_begin( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_array_end( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_map_begin( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

int llsd_notation_map_end( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

#define NOTATION_SIG_LEN (18)
static uint8_t const * const notation_header = "<?llsd/notation?>\n";

int llsd_notation_serializer_init( FILE * fout, llsd_ops_t * const ops, int const pretty, void ** const user_data )
{
	ns_state_t * state = NULL;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( ops, FALSE );
	CHECK_PTR_RET( user_data, FALSE );

	/* give the serializer the proper callback functions */
	(*ops) = (llsd_ops_t){
		&llsd_notation_undef,
		&llsd_notation_boolean,
		&llsd_notation_integer,
		&llsd_notation_real,
		&llsd_notation_uuid,
		&llsd_notation_string,
		&llsd_notation_date,
		&llsd_notation_uri,
		&llsd_notation_binary,
		&llsd_notation_array_begin,
		&llsd_notation_array_end,
		&llsd_notation_map_begin,
		&llsd_notation_map_end 
	};

	/* write out the notation signature */
	CHECK_RET( fwrite( notation_header, sizeof(uint8_t), NOTATION_SIG_LEN, fout ) == NOTATION_SIG_LEN, FALSE );

	/* allocate the serializer state and store the file pointer */
	state = CALLOC( 1, sizeof(ns_state_t) );
	CHECK_PTR_RET( state, FALSE );
	state->fout = fout;

	/* return the state as the user_date */
	(*user_data) = state;

	return TRUE;
}


int llsd_notation_serializer_deinit( FILE * fout, void * user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( state, FALSE );

	FREE( state );
	return TRUE;
}


