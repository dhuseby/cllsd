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

#include "llsd.h"
#include "llsd_serializer.h"
#include "llsd_xml_serializer.h"
#include "base64.h"

#define PUSHML(x) (list_push_head( state->multiline_stack, (void*)x ))
#define TOPML	  ((int)list_get_head( state->multiline_stack ))
#define POPML	  (list_pop_head( state->multiline_stack ))

typedef struct xs_state_s
{
	int pretty;
	int indent;
	int key;
	FILE * fout;
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

#define UNDEF_EMPTY		WRITE_STR("<undef />",9)
#define BOOLEAN_BEGIN	WRITE_STR("<boolean>",9)
#define BOOLEAN_END		WRITE_STR("</boolean>",10)
#define INTEGER_BEGIN	WRITE_STR("<integer>",9)
#define INTEGER_END		WRITE_STR("</integer>",10)
#define INTEGER_EMPTY	WRITE_STR("<integer />",11)
#define REAL_BEGIN		WRITE_STR("<real>",6)
#define REAL_END		WRITE_STR("</real>",7)
#define REAL_EMPTY		WRITE_STR("<real />",8)
#define UUID_BEGIN		WRITE_STR("<uuid>",6)
#define UUID_END		WRITE_STR("</uuid>",7)
#define UUID_EMPTY		WRITE_STR("<uuid />",8)
#define STRING_BEGIN	WRITE_STR("<string>",8)
#define STRING_END		WRITE_STR("</string>",9)
#define STRING_EMPTY	WRITE_STR("<string />",10)
#define DATE_BEGIN		WRITE_STR("<date>",6)
#define DATE_END		WRITE_STR("</date>",7)
#define DATE_EMPTY		WRITE_STR("<date />",8)
#define URI_BEGIN		WRITE_STR("<uri>",5)
#define URI_END			WRITE_STR("</uri>",6)
#define URI_EMPTY		WRITE_STR("<uri />",7)
#define BINARY_BEGIN	WRITE_STR("<binary encoding=\"",18)
#define BINARY_END		WRITE_STR("</binary>",9)
#define BINARY_EMPTY	WRITE_STR("<binary />",10)
#define ARRAY_BEGIN		WRITE_STR("<array>",7)
#define ARRAY_END		WRITE_STR("</array>",8)
#define ARRAY_EMPTY		WRITE_STR("<array />",9)
#define MAP_BEGIN		WRITE_STR("<map>",5)
#define MAP_END			WRITE_STR("</map>",6)
#define MAP_EMPTY		WRITE_STR("<map />",7)
#define KEY_BEGIN		WRITE_STR("<key>",5)
#define KEY_END			WRITE_STR("</key>",6)
#define LLSD_BEGIN		WRITE_STR("<llsd>",6)
#define LLSD_END		WRITE_STR("</llsd>",7)

static int llsd_xml_undef( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	UNDEF_EMPTY;
	return TRUE;
}

static int llsd_xml_boolean( int const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	/* NOTE: we use 1 and 0 because the parser is a tiny bit faster */
	BOOLEAN_BEGIN;
	CHECK_RET( fwrite( (value ? "1" : "0"), sizeof(uint8_t), 1, state->fout ) == 1, FALSE );
	BOOLEAN_END;
	return TRUE;
}

static int llsd_xml_integer( int32_t const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	if ( value == 0 )
	{
		INTEGER_EMPTY;
	}
	else
	{
		INTEGER_BEGIN;
		CHECK_RET( fprintf( state->fout, "%d", value ) >= 1, FALSE );
		INTEGER_END;
	}
	return TRUE;
}

static int llsd_xml_real( double const value, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	if ( value == 0.0 )
	{
		REAL_EMPTY;
	}
	else
	{
		REAL_BEGIN;
		CHECK_RET( fprintf( state->fout, "%F", value ) >= 1, FALSE );
		REAL_END;
	}
	return TRUE;
}


static int llsd_xml_uuid( uint8_t const value[UUID_LEN], void * const user_data )
{
	int ret = 0;
	static const zero_uuid[UUID_LEN] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	if ( memcmp( value, zero_uuid, UUID_LEN ) == 0 )
	{
		UUID_EMPTY;
	}
	else
	{
		UUID_BEGIN;
		ret = fprintf( state->fout, 
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			value[0], value[1], value[2], value[3], 
			value[4], value[5], 
			value[6], value[7], 
			value[8], value[9], 
			value[10], value[11], value[12], value[13], value[14], value[15] );
		CHECK_RET( ret == 36, FALSE );
		UUID_END;
	}

	return TRUE;
}

static void llsd_xml_write_string( uint8_t const * str, uint32_t len, xs_state_t * const state )
{
	uint32_t i;
	CHECK_PTR( str );
	CHECK( len > 0 );

	for ( i = 0; i < len; i++ )
	{
		switch( str[i] )
		{
			case '<':
				WRITE_STR( "&lt;", 4 );
				break;
			case '>':
				WRITE_STR( "&gt;", 4 );
				break;
			case '&':
				WRITE_STR( "&amp;", 5 );
				break;
			case '\'':
				WRITE_STR( "&apos;", 6 );
				break;
			case '\"':
				WRITE_STR( "&quot;", 6 );
				break;
			case '\t':
			case '\n':
			case '\r':
				WRITE_STR( &(str[i]), 1 );
				break;
			default:
				if ( str[i] >= 0x20 )
				{
					WRITE_STR( &(str[i]), 1 );
				}
				break;
		}
	}
}

static int llsd_xml_string( uint8_t const * str, int const own_it, void * const user_data )
{
	uint8_t * xml_encoded = NULL;
	size_t len = 0;
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	len = strlen( str );
	if ( state->key )
	{
		llsd_xml_write_string( str, len, state );
	}
	else
	{
		if ( len == 0 )
		{
			STRING_EMPTY;
		}
		else
		{
			STRING_BEGIN;
			llsd_xml_write_string( str, len, state );
			STRING_END;
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
	int_time = floor( value );
	seconds = (time_t)int_time;
	useconds = (int32_t)( ( value - int_time) * 1000000.0 );
	if ( (seconds == 0) && (useconds == 0) )
	{
		DATE_EMPTY; /* epoch */
	}
	else
	{
		parts = *gmtime(&seconds);

		DATE_BEGIN;
		fprintf( state->fout,
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			parts.tm_year + 1900,
			parts.tm_mon + 1,
			parts.tm_mday,
			parts.tm_hour,
			parts.tm_min,
			parts.tm_sec,
			((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
		DATE_END;
	}

	return TRUE;
}

static int llsd_xml_uri( uint8_t const * uri, int const own_it, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	size_t len;
	uint8_t * escaped = NULL;
	uint32_t esc_len = 0;
	CHECK_PTR_RET( state, FALSE );
	CHECK_PTR_RET( uri, FALSE );
	len = strlen(uri);
	if ( len == 0 )
	{
		URI_EMPTY;
	}
	else
	{
		URI_BEGIN;
		llsd_xml_write_string( uri, len, state );
		FREE( escaped );
		URI_END;
	}

	return TRUE;
}

static int llsd_xml_binary( uint8_t const * data, uint32_t const len, int const own_it, void * const user_data )
{
	uint8_t * buf;
	uint32_t outlen = 0;
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	if ( len == 0 )
	{
		BINARY_EMPTY;
	}
	else
	{
		CHECK_PTR_RET( data, FALSE );
		outlen = BASE64_LENGTH( len );
		buf = CALLOC( outlen, sizeof(uint8_t) );
		CHECK_PTR_RET( buf, FALSE );
		CHECK_RET( base64_encode( data, len, buf, &outlen ), FALSE );
		BINARY_BEGIN;
		WRITE_STR("base64\">",8);
		CHECK_RET( fwrite( buf, sizeof(uint8_t), outlen, state->fout ) == outlen, FALSE );
		BINARY_END;
		FREE( buf );
	}

	return TRUE;
}

static int llsd_xml_array_begin( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	NL;
	INDENT;

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	if ( size == 0 )
	{
		ARRAY_EMPTY;
	}
	else
	{
		ARRAY_BEGIN;
	}

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_xml_array_value_begin( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	INDENT;
	return TRUE;
}

static int llsd_xml_array_value_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

static int llsd_xml_array_end( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	DEC_INDENT;
	INDENT;
	if ( size > 0 )
	{
		ARRAY_END;
	}
	POPML;
	return TRUE;
}

static int llsd_xml_map_begin( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );

	NL;
	INDENT;

	/* if there is > 1 item in this array, we want to output items in multi-line format */
	PUSHML( (size > 1) );

	if ( size == 0 )
	{
		MAP_EMPTY;
	}
	else
	{
		MAP_BEGIN;
	}

	/* increment indent */
	INC_INDENT;

	return TRUE;
}

static int llsd_xml_map_key_begin( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	INDENT;
	KEY_BEGIN;
	state->key = TRUE;
	return TRUE;
}

static int llsd_xml_map_key_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	KEY_END;
	state->key = FALSE;
	return TRUE;
}

static int llsd_xml_map_value_begin( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

static int llsd_xml_map_value_end( void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	return TRUE;
}

static int llsd_xml_map_end( uint32_t const size, void * const user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( state, FALSE );
	NL;
	DEC_INDENT;
	INDENT;
	if ( size > 0 )
	{
		MAP_END;
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
		&llsd_xml_array_value_begin,
		&llsd_xml_array_value_end,
		&llsd_xml_array_end,
		&llsd_xml_map_begin,
		&llsd_xml_map_key_begin,
		&llsd_xml_map_key_end,
		&llsd_xml_map_value_begin,
		&llsd_xml_map_value_end,
		&llsd_xml_map_end 
	};

	/* allocate the serializer state and store the file pointer */
	state = CALLOC( 1, sizeof(xs_state_t) );
	CHECK_PTR_RET( state, FALSE );

	state->pretty = pretty;
	state->indent = 1;
	state->key = FALSE;
	state->fout = fout;

	state->multiline_stack = list_new( 1, NULL );
	if ( state->multiline_stack == NULL )
	{
		FREE(state);
		return FALSE;
	}
	PUSHML( TRUE );

	/* write out the xml signature */
	CHECK_RET( fwrite( xml_header, sizeof(uint8_t), XML_SIG_LEN, fout ) == XML_SIG_LEN, FALSE );
	
	/* write out <llsd> tag */
	LLSD_BEGIN;

	/* return the state as the user_date */
	(*user_data) = state;

	return TRUE;
}


int llsd_xml_serializer_deinit( FILE * fout, void * user_data )
{
	xs_state_t * state = (xs_state_t*)user_data;
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( state, FALSE );

	/* write out </llsd> tag */
	NL;
	LLSD_END;

	CHECK_RET( TOPML == TRUE, FALSE );
	list_delete( state->multiline_stack );

	FREE( state );
	return TRUE;
}


