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
#include "llsd_xml_serializer.h"
#include "base64.h"

typedef enum xs_step_e
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
} xs_step_t;

#define VALUE_STATES (TOP_LEVEL | ARRAY_START | ARRAY_VALUE_END | MAP_KEY_END )
#define STRING_STATES ( VALUE_STATES | MAP_START | MAP_VALUE_END )

#define PUSH(x) (list_push_head( state->step_stack, (void*)x ))
#define TOP		((uint32_t)list_get_head( state->step_stack ))
#define POP		(list_pop_head( state->step_stack))

#define PUSHML(x) (list_push_head( state->multiline_stack, (void*)x ))
#define TOPML	  ((int)list_get_head( state->multiline_stack ))
#define POPML	  (list_pop_head( state->multiline_stack ))


typedef struct xs_state_s
{
	int pretty;
	int indent;
	FILE * fout;
	list_t * step_stack;
	list_t * multiline_stack;
} xs_state_t;

#define WRITE_STR(x,y) (fwrite( x, sizeof(uint8_t), y, state->fout ))
#define COMMA WRITE_STR(",",1)
#define COLON WRITE_STR(":",1)
#define NL { if(state->pretty && TOPML) WRITE_STR("\n",1); }

#define INDENT_SPACES (4)
#define INDENT { if(state->pretty && state->indent && TOPML) fprintf( state->fout, "%*s", state->indent * INDENT_SPACES, " " ); }
#define INC_INDENT { if(state->pretty) state->indent++; }
#define DEC_INDENT { if(state->pretty) state->indent--; }

#define UNDEF		WRITE_STR("<undef />",9)
#define BOOLEAN_S	WRITE_STR("<boolean>",9)
#define BOOLEAN_E	WRITE_STR("</boolean>",10)
#define INTEGER_S	WRITE_STR("<integer>",9)
#define INTEGER_E	WRITE_STR("</integer>",10)
#define INTEGER_N	WRITE_STR("<integer />",11)
#define REAL_S		WRITE_STR("<real>",6)
#define REAL_E		WRITE_STR("</real>",7)
#define REAL_N		WRITE_STR("<real />",8)
#define UUID_S		WRITE_STR("<uuid>",6)
#define UUID_E		WRITE_STR("</uuid>",7)
#define UUID_N		WRITE_STR("<uuid />",8)
#define STRING_S	WRITE_STR("<string>",8)
#define STRING_E	WRITE_STR("</string>",9)
#define STRING_N	WRITE_STR("<string />",10)
#define DATE_S		WRITE_STR("<date>",6)
#define DATE_E		WRITE_STR("</date>",7)
#define DATE_N		WRITE_STR("<date />",8)
#define URI_S		WRITE_STR("<uri>",5)
#define URI_E		WRITE_STR("</uri>",6)
#define URI_N		WRITE_STR("<uri />",7)
#define BINARY_S	WRITE_STR("<binary encoding=\"",18)
#define BINARY_E	WRITE_STR("</binary>",9)
#define BINARY_N	WRITE_STR("<binary />",10)
#define ARRAY_S		WRITE_STR("<array>",7)
#define ARRAY_E		WRITE_STR("</array>",8)
#define ARRAY_N		WRITE_STR("<array />",9)
#define MAP_S		WRITE_STR("<map>",5)
#define MAP_E		WRITE_STR("</map>",6)
#define MAP_N		WRITE_STR("<map />",7)
#define KEY_S		WRITE_STR("<key>",5)
#define KEY_E		WRITE_STR("</key>",6)
#define LLSD_S		WRITE_STR("<llsd>",6)
#define LLSD_E		WRITE_STR("</llsd>",7)

static int update_state( uint32_t valid_states, llsd_type_t type_, xs_state_t * state )
{
	/* make sure we have a valid LLSD type */
	CHECK_RET( IS_VALID_LLSD_TYPE( type_ ), FALSE );
	
	/* make sure we have a valid state object pointer */
	CHECK_PTR_RET( state, FALSE );

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

static int llsd_xml_undef( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_UNDEF, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	UNDEF;

	return TRUE;
}

static int llsd_xml_boolean( int const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_BOOLEAN, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	/* NOTE: we use 1 and 0 because the parser is a tiny bit faster */
	BOOLEAN_S;
	CHECK_RET( fwrite( (value ? "1" : "0"), sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	BOOLEAN_E;

	return TRUE;
}

static int llsd_xml_integer( int32_t const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_INTEGER, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	if ( value == 0 )
	{
		INTEGER_N;
	}
	else
	{
		INTEGER_S;
		CHECK_RET( fprintf( state->fout, "%d", value ) >= 1, FALSE );
		INTEGER_E;
	}
	
	return TRUE;
}

static int llsd_xml_real( double const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_REAL, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	if ( value == 0.0 )
	{
		REAL_N;
	}
	else
	{
		REAL_S;
		CHECK_RET( fprintf( state->fout, "%F", value ) >= 1, FALSE );
		REAL_E;
	}
	
	return TRUE;
}


static int llsd_xml_uuid( uint8_t const value[UUID_LEN], void * const user_data )
{
	int ret = 0;
	static const zero_uuid[UUID_LEN] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_UUID, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	if ( memcmp( value, zero_uuid, UUID_LEN ) == 0 )
	{
		UUID_N;
	}
	else
	{
		UUID_S;
		ret = fprintf( state->fout, 
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			value[0], value[1], value[2], value[3], 
			value[4], value[5], 
			value[6], value[7], 
			value[8], value[9], 
			value[10], value[11], value[12], value[13], value[14], value[15] );
		CHECK_RET( ret == 36, FALSE );
		UUID_E;
	}

	return TRUE;
}

static int llsd_xml_string( uint8_t const * str, int const own_it, void * const user_data )
{
	uint8_t * xml_encoded = NULL;
	size_t len = 0;
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( STRING_STATES, LLSD_STRING, state ), FALSE );

	if ( TOP & (ARRAY_VALUE | MAP_KEY) )
	{
		NL;
		INDENT;
	}

	len = strlen( str );
	if ( TOP & MAP_KEY )
	{
		KEY_S;
		WRITE_STR(str, len);
		KEY_E;
	}
	else
	{
		if ( len == 0 )
		{
			STRING_N;
		}
		else
		{
			STRING_S;
			WRITE_STR(str, len);
			STRING_E;
		}
	}

	return TRUE;
}

static int llsd_xml_date( double const value, void * const user_data )
{
	double int_time;
	int32_t useconds;
	time_t seconds;
	struct tm parts;
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_DATE, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}

	int_time = floor( value );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( value - int_time) * 1000000.0 );
	if ( (seconds == 0) && (useconds == 0) )
	{
		DATE_N; /* epoch */
	}
	else
	{
		parts = *gmtime(&seconds);

		DATE_S;
		fprintf( state->fout,
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			parts.tm_year + 1900,
			parts.tm_mon + 1,
			parts.tm_mday,
			parts.tm_hour,
			parts.tm_min,
			parts.tm_sec,
			((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
		DATE_E;
	}

	return TRUE;
}

static int llsd_xml_uri( uint8_t const * uri, int const own_it, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	size_t len;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( uri, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_URI, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}

	len = strlen(uri);
	if ( len == 0 )
	{
		URI_N;
	}
	else
	{
		URI_S;
		WRITE_STR(uri, len);
		URI_E;
	}

	return TRUE;
}

static int llsd_xml_binary( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data )
{
	uint8_t * buf;
	uint32_t outlen = 0;
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( update_state( VALUE_STATES, LLSD_BINARY, state ), FALSE );

	if ( TOP == ARRAY_VALUE )
	{
		NL;
		INDENT;
	}
	if ( len == 0 )
	{
		BINARY_N;
	}
	else
	{
		CHECK_PTR_RET( data, FALSE );
		outlen = BASE64_LENGTH( len );
		buf = CALLOC( outlen, sizeof(uint8_t) );
		CHECK_PTR_RET( buf, FALSE );
		CHECK_RET( base64_encode( data, len, buf, &outlen ), FALSE );
		BINARY_S;
		WRITE_STR("base64\">",8);
		CHECK_RET( fwrite( buf, sizeof(uint8_t), outlen, state->fout ) == outlen, FALSE );
		BINARY_E;
		FREE( buf );
	}

	return TRUE;
}

static int llsd_xml_array_begin( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	CHECK_RET( update_state( VALUE_STATES, LLSD_ARRAY, state ), FALSE );

	if ( (TOP == ARRAY_VALUE) || ((TOP == MAP_VALUE) && (size > 1)) )
	{
		NL;
		INDENT;
	}

	PUSH( ARRAY_START );

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	if ( size == 0 )
	{
		ARRAY_N;
	}
	else
	{
		ARRAY_S;
	}

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_xml_array_value_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after an array value */
	CHECK_RET( (TOP & ARRAY_VALUE), FALSE );
	POP;
	PUSH( ARRAY_VALUE_END );
	return TRUE;
}


static int llsd_xml_array_end( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only happens after array start or array value; never after array value end */
	CHECK_RET( (TOP & (ARRAY_START | ARRAY_VALUE)), FALSE );
	POP;
	NL;
	DEC_INDENT;
	INDENT;
	if ( size > 0 )
	{
		ARRAY_E;
	}
	POPML;
	return TRUE;
}

static int llsd_xml_map_begin( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	CHECK_RET( update_state( VALUE_STATES, LLSD_MAP, state ), FALSE );

	if ( (TOP == ARRAY_VALUE) || ((TOP == MAP_VALUE) && (size > 1)) )
	{
		NL;
		INDENT;
	}

	PUSH( MAP_START );

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	if ( size == 0 )
	{
		MAP_N;
	}
	else
	{
		MAP_S;
	}

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_xml_map_key_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after a map key */
	CHECK_RET( (TOP & MAP_KEY), FALSE );
	POP;
	PUSH( MAP_KEY_END );
	return TRUE;
}

static int llsd_xml_map_value_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* this only ever happens after a map value, and never before a map end */
	CHECK_RET( (TOP & MAP_VALUE), FALSE );
	POP;
	PUSH( MAP_VALUE_END );
	return TRUE;
}

static int llsd_xml_map_end( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	CHECK_RET( (TOP & (MAP_START | MAP_VALUE)), FALSE );
	POP;
	NL;
	DEC_INDENT;
	INDENT;
	if ( size > 0 )
	{
		MAP_E;
	}
	POPML;
	return TRUE;
}

#define XML_SIG_LEN (38)
static uint8_t const * const xml_header = "<?xml version\"1.0\" encoding=\"UTF-8\"?>\n";

int llsd_xml_serializer_init( FILE * fout, llsd_ops_t * const ops, int const pretty, void ** const user_data )
{
	xs_state_t * state = NULL;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( ops, FALSE );
	CHECK_PTR_RET( user_data, FALSE );

	/* give the serializer the proper callback functions */
	(*ops) = (llsd_ops_t){
		&llsd_xml_undef,
		&llsd_xml_boolean,
		&llsd_xml_integer,
		&llsd_xml_real,
		&llsd_xml_uuid,
		&llsd_xml_string,
		&llsd_xml_date,
		&llsd_xml_uri,
		&llsd_xml_binary,
		&llsd_xml_array_begin,
		&llsd_xml_array_value_end,
		&llsd_xml_array_end,
		&llsd_xml_map_begin,
		&llsd_xml_map_key_end,
		&llsd_xml_map_value_end,
		&llsd_xml_map_end 
	};

	/* allocate the serializer state and store the file pointer */
	state = CALLOC( 1, sizeof(xs_state_t) );
	CHECK_PTR_RET( state, FALSE );

	state->pretty = pretty;
	state->indent = 1;
	state->fout = fout;

	state->step_stack = list_new( 1, NULL );
	if ( state->step_stack == NULL )
	{
		FREE( state );
		return FALSE;
	}
	PUSH( TOP_LEVEL );

	state->multiline_stack = list_new( 1, NULL );
	if ( state->multiline_stack == NULL )
	{
		list_delete( state->step_stack );
		FREE(state);
		return FALSE;
	}
	PUSHML( FALSE );

	/* write out the xml signature */
	CHECK_RET( fwrite( xml_header, sizeof(uint8_t), XML_SIG_LEN, fout ) == XML_SIG_LEN, FALSE );
	LLSD_S;

	/* return the state as the user_date */
	(*user_data) = state;

	return TRUE;
}


int llsd_xml_serializer_deinit( FILE * fout, void * user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( state, FALSE );

	CHECK_RET( TOP == TOP_LEVEL, FALSE );
	LLSD_E;
	list_delete( state->step_stack );

	CHECK_RET( TOPML == FALSE, FALSE );
	list_delete( state->multiline_stack );

	FREE( state );
	return TRUE;
}


