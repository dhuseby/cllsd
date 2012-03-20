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

#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <expat.h>

#include "debug.h"
#include "macros.h"
#include "llsd.h"
#include "llsd_util.h"
#include "llsd_xml.h"

static void XMLCALL llsd_xml_start_tag( void * data, char const * el, char const ** attr )
{
}

static void XMLCALL llsd_xml_end_tag( void * data, char const * el )
{
}

static void XMLCALL llsd_xml_data_handler( void * data, char const * s, int len )
{
}

llsd_t * llsd_parse_xml( FILE * fin )
{
	XML_Parser p;
	CHECK_PTR_RET( fin, NULL );

	/* create the parser */
	p = XML_ParserCreate("UTF-8");
	CHECK_PTR_RET_MSG( p, NULL, "failed to create expat parser\n" );

	/* set the tag start/end handlers */
	XML_SetElementHandler( p, llsd_xml_start_tag, llsd_xml_end_tag );


	/* free the parser */
	XML_ParserFree( p );

	return NULL;
}

#define XML_UNDEF_LEN (9)
#define XML_UNDEFC_LEN (0)
#define XML_BOOLEAN_LEN (9)
#define XML_BOOLEANC_LEN (10)
#define XML_INTEGER_LEN (9)
#define XML_INTEGERC_LEN (10)
#define XML_REAL_LEN (6)
#define XML_REALC_LEN (7)
#define XML_UUID_LEN (6)
#define XML_UUIDC_LEN (7)
#define XML_STRING_LEN (14)
#define XML_STRINGC_LEN (9)
#define XML_DATE_LEN (6)
#define XML_DATEC_LEN (7)
#define XML_URI_LEN (11)
#define XML_URIC_LEN (6)
#define XML_BINARY_LEN (14)
#define XML_BINARYC_LEN (9)
#define XML_ARRAY_LEN (13)
#define XML_ARRAYC_LEN (8)
#define XML_MAP_LEN (11)
#define XML_MAPC_LEN (6)
#define XML_KEY_LEN (5)
#define XML_KEYC_LEN (6)
static uint8_t const * const lxkey		= "<key>";
static uint8_t const * const lxkeyc		= "</key>\n";
static uint8_t const * const lxtabs		= "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

static uint8_t const * const lxstart[LLSD_TYPE_COUNT] =
{
	"<undef />",
	"<boolean>",
	"<integer>",
	"<real>",
	"<uuid>",
	"<string size=\"",
	"<date>",
	"<uri size=\"",
	"<binary size=\"",
	"<array size=\"",
	"<map size=\""
};

#define XML_SIZEC_LEN (2)
static uint8_t const * const lxstartc = "\">\n";

static uint8_t const * const lxend[LLSD_TYPE_COUNT] =
{
	"\n",
	"</boolean>\n",
	"</integer>\n",
	"</real>\n",
	"</uuid>\n",
	"</string>\n",
	"</date>\n",
	"</uri>\n",
	"</binary>\n",
	"</array>\n",
	"</map>\n"
};

static uint32_t lxstartlen[LLSD_TYPE_COUNT] =
{
	XML_UNDEF_LEN,
	XML_BOOLEAN_LEN,
	XML_INTEGER_LEN,
	XML_REAL_LEN,
	XML_UUID_LEN,
	XML_STRING_LEN,
	XML_DATE_LEN,
	XML_URI_LEN,
	XML_BINARY_LEN,
	XML_ARRAY_LEN,
	XML_MAP_LEN
};

static uint32_t lxendlen[LLSD_TYPE_COUNT] =
{
	XML_UNDEFC_LEN,
	XML_BOOLEANC_LEN,
	XML_INTEGERC_LEN,
	XML_REALC_LEN,
	XML_UUIDC_LEN,
	XML_STRINGC_LEN,
	XML_DATEC_LEN,
	XML_URIC_LEN,
	XML_BINARYC_LEN,
	XML_ARRAYC_LEN,
	XML_MAPC_LEN
};

static uint32_t indent = 1;

#define indent_xml(p,f) ( p ? fwrite(lxtabs, sizeof(uint8_t), ((indent<=255)?indent:255), f) : 0 );

static size_t llsd_write_xml_start_tag( llsd_type_t t, FILE * fout, int pretty, int size)
{
	static uint8_t buf[32];
	int sz = 0;
	size_t num = 0;
	CHECK_RET( (t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST), 0 );

	switch ( t )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
			return fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t], fout );
		case LLSD_STRING:
		case LLSD_URI:
		case LLSD_BINARY:
			sz = snprintf(buf, 32, "%d", size);
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t], fout );
			num += fwrite( buf, sizeof(uint8_t), sz, fout );
			num += fwrite( lxstartc, sizeof(uint8_t), XML_SIZEC_LEN, fout );
			return num;
		case LLSD_ARRAY:
		case LLSD_MAP:
			sz = snprintf(buf, 32, "%d", size);
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t], fout );
			num += fwrite( buf, sizeof(uint8_t), sz, fout );
			num += fwrite( lxstartc, sizeof(uint8_t), XML_SIZEC_LEN + (pretty ? 1 : 0), fout );
			return num;
	}
	return num;
}

static size_t llsd_write_xml_end_tag( llsd_type_t t, FILE * fout, int pretty )
{
	CHECK_RET( (t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST), 0 );
	return fwrite( lxend[t], sizeof(uint8_t), lxendlen[t] + (pretty ? 1 : 0), fout );
}


size_t llsd_format_xml( llsd_t * llsd, FILE * fout, int pretty )
{
	size_t num = 0;
	unsigned long start = ftell( fout );

	llsd_itr_t itr;
	llsd_string_t s;
	llsd_t *k, *v;
	llsd_type_t t = llsd_get_type( llsd );

	switch ( t )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_DATE:
			s = llsd_as_string( llsd );
			num += indent_xml( pretty, fout );
			num += llsd_write_xml_start_tag( t, fout, pretty, 0 );
			num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
			num += llsd_write_xml_end_tag( t, fout, pretty );
			DEBUG( "%*s%s %lu - %lu\n", indent * 4, " ", llsd_get_type_string( t ), start, ftell( fout ) - 1 );
			return num;
	
		case LLSD_STRING:
		case LLSD_URI:
		case LLSD_BINARY:
			s = llsd_as_string( llsd );
			num += indent_xml( pretty, fout );
			num += llsd_write_xml_start_tag( t, fout, pretty, s.str_len );
			num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
			num += llsd_write_xml_end_tag( t, fout, pretty );
			DEBUG( "%*s%s %lu - %lu\n", indent * 4, " ", llsd_get_type_string( t ), start, ftell( fout ) - 1 );
			return num;

		case LLSD_ARRAY:
		case LLSD_MAP:
			num += indent_xml( pretty, fout );
			num += llsd_write_xml_start_tag( t, fout, pretty, llsd_get_size( llsd ) );
			DEBUG( "%*s%s (%d)\n", indent * 4, " ", llsd_get_type_string( llsd ), llsd_get_size( llsd ) );
			indent++;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					s = llsd_as_string( k );
					num += indent_xml( pretty, fout );
					num += fwrite( lxkey, sizeof(uint8_t), XML_KEY_LEN, fout );
					num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
					num += fwrite( lxkeyc, sizeof(uint8_t), XML_KEYC_LEN + (pretty ? 1 : 0), fout );
				}
				num += llsd_format_xml( v, fout, pretty );
			}
			indent--;
			num += indent_xml( pretty, fout );
			num += llsd_write_xml_end_tag( t, fout, pretty );
			DEBUG( "%*s%s %lu - %lu\n", indent * 4, " ", llsd_get_type_string( llsd ), start, ftell( fout ) - 1 );
			break;
	}
	return num;

}

size_t llsd_get_xml_zero_copy_size( llsd_t * llsd, int pretty )
{
	FAIL( "XML serialization doesn't support zero copy\n" );
	return 0;
}

size_t llsd_format_xml_zero_copy( llsd_t * llsd, struct iovec * v, int pretty )
{
	FAIL( "XML serialization doesn't support zero copy\n" );
	return 0;
}

