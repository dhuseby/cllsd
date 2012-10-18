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

#include <math.h>
#include <time.h>

#include "llsd_serializer.h"
#include "llsd_notation_serializer.h"
#include "base64.h"

typedef struct ns_state_s
{
	int pretty;
	int indent;
	FILE * fout;
	list_t * count_stack;
	list_t * multiline_stack;
} ns_state_t;

#define PUSHML(x) (list_push_head( state->multiline_stack, (void*)x ))
#define TOPML	  ((int)list_get_head( state->multiline_stack ))
#define POPML	  (list_pop_head( state->multiline_stack ))

#define PUSHC(x)  (list_push_head( state->count_stack, (void*)x ))
#define TOPC	  ((int)list_get_head( state->count_stack ))
#define POPC	  (list_pop_head( state->count_stack ))

#define WRITE_CHAR(x) (fwrite( x, sizeof(uint8_t), 1, state->fout ))
#define COMMA { if(TOPC) WRITE_CHAR(","); }
#define COLON WRITE_CHAR(":")
#define NL { if(state->pretty && TOPML) WRITE_CHAR("\n"); }

#define INDENT_SPACES (4)
#define INDENT { if(state->pretty && state->indent && TOPML) fprintf( state->fout, "%*s", state->indent * INDENT_SPACES, " " ); }
#define INC_INDENT { if(state->pretty) state->indent++; }
#define DEC_INDENT { if(state->pretty) state->indent--; }

static int llsd_notation_undef( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( fwrite( "!", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_notation_boolean( int const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* NOTE: we use 1 and 0 because the parser is a tiny bit faster */
	CHECK_RET( fwrite( (value ? "1" : "0"), sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	return TRUE;
}

static int llsd_notation_integer( int32_t const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( fprintf( state->fout, "i%d", value ) >= 2, FALSE );
	return TRUE;
}

static int llsd_notation_real( double const value, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( fprintf( state->fout, "r%F", value ) > 2, FALSE );
	return TRUE;
}

static int llsd_notation_uuid( uint8_t const value[UUID_LEN], void * const user_data )
{
	int ret = 0;
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	ret = fprintf( state->fout, 
		"u%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
		value[0], value[1], value[2], value[3], 
		value[4], value[5], 
		value[6], value[7], 
		value[8], value[9], 
		value[10], value[11], value[12], value[13], value[14], value[15] );
	CHECK_RET( ret == 37, FALSE );
	return TRUE;
}

static int llsd_notation_string( uint8_t const * str, int const own_it, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* use raw string format because the parser is a little faster */
	fprintf( state->fout, "s(%d)\"%s\"", strlen(str), str );
	return TRUE;
}

static int llsd_notation_date( double const value, void * const user_data )
{
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	int_time = floor( value );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( value - int_time) * 1000000.0 );
	parts = *gmtime(&seconds);
	fprintf( state->fout,
		"d\"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ\"",
		parts.tm_year + 1900,
		parts.tm_mon + 1,
		parts.tm_mday,
		parts.tm_hour,
		parts.tm_min,
		parts.tm_sec,
		((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );

	return TRUE;
}

static int llsd_notation_uri( uint8_t const * uri, int const own_it, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( uri, FALSE );
	fprintf( state->fout, "l\"%s\"", uri );
	return TRUE;
}

static int llsd_notation_binary( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data )
{
	uint8_t * buf;
	uint32_t outlen = 0;
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	if ( len > 0 )
	{
		CHECK_PTR_RET( data, FALSE );
		outlen = BASE64_LENGTH( len );
		buf = CALLOC( outlen, sizeof(uint8_t) );
		CHECK_PTR_RET( buf, FALSE );
		CHECK_RET( base64_encode( data, len, buf, &outlen ), FALSE );
		CHECK_RET( fwrite( "b64\"", sizeof(uint8_t), 4, state->fout ) == 4, FALSE );
		CHECK_RET( fwrite( buf, sizeof(uint8_t), outlen, state->fout ) == outlen, FALSE );
		CHECK_RET( fwrite( "\"", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
		FREE( buf );
	}
	else
	{
		CHECK_RET( fwrite( "b(0)\"\"", sizeof(uint8_t), 6, state->fout ) == 6, FALSE );
	}

	return TRUE;
}

static int llsd_notation_array_begin( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	/* push a new item count */
	PUSHC( 0 );

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	CHECK_RET( fwrite( "[", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_notation_array_value_begin( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	COMMA;
	NL;
	INDENT;
	return TRUE;
}

static int llsd_notation_array_value_end( void * const user_data )
{
	int c = 0;
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* increment the item count */
	c = TOPC;
	POPC;
	PUSHC( ++c );
	return TRUE;
}


static int llsd_notation_array_end( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	DEC_INDENT;
	INDENT;
	CHECK_RET( fwrite( "]", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	POPML;
	POPC;
	return TRUE;
}

static int llsd_notation_map_begin( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	/* push a new item counter */
	PUSHC( 0 );

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	CHECK_RET( fwrite( "{", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_notation_map_key_begin( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	COMMA;
	NL;
	INDENT;
	return TRUE;
}

static int llsd_notation_map_key_end( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* output the colon to mark the end of the key */
	COLON;
	return TRUE;
}

static int llsd_notation_map_value_begin( void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

static int llsd_notation_map_value_end( void * const user_data )
{
	int c = 0;
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	c = TOPC;
	POPC;
	PUSHC( ++c );
	return TRUE;
}

static int llsd_notation_map_end( uint32_t const size, void * const user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	DEC_INDENT;
	INDENT;
	CHECK_RET( fwrite( "}", sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	POPML;
	POPC;
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
		&llsd_notation_array_value_begin,
		&llsd_notation_array_value_end,
		&llsd_notation_array_end,
		&llsd_notation_map_begin,
		&llsd_notation_map_key_begin,
		&llsd_notation_map_key_end,
		&llsd_notation_map_value_begin,
		&llsd_notation_map_value_end,
		&llsd_notation_map_end 
	};

	/* write out the notation signature */
	CHECK_RET( fwrite( notation_header, sizeof(uint8_t), NOTATION_SIG_LEN, fout ) == NOTATION_SIG_LEN, FALSE );

	/* allocate the serializer state and store the file pointer */
	state = CALLOC( 1, sizeof(ns_state_t) );
	CHECK_PTR_RET( state, FALSE );

	state->pretty = pretty;
	state->indent = 0;
	state->fout = fout;

	state->count_stack = list_new( 0, NULL );
	if ( state->count_stack == NULL )
	{
		FREE(state);
		return FALSE;
	}

	state->multiline_stack = list_new( 1, NULL );
	if ( state->multiline_stack == NULL )
	{
		list_delete( state->count_stack );
		FREE(state);
		return FALSE;
	}
	PUSHML( FALSE );

	/* return the state as the user_date */
	(*user_data) = state;

	return TRUE;
}


int llsd_notation_serializer_deinit( FILE * fout, void * user_data )
{
	ns_state_t * state = (ns_state_t*)user_data;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( state, FALSE );

	list_delete( state->count_stack );

	CHECK_RET( TOPML == FALSE, FALSE );
	list_delete( state->multiline_stack );

	FREE( state );
	return TRUE;
}


