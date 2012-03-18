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

#include "debug.h"
#include "macros.h"
#include "llsd.h"
#include "llsd_util.h"
#include "llsd_xml.h"

llsd_t * llsd_parse_xml( FILE * fin )
{
	return NULL;
}

#define XML_UNDEF_LEN (9)
#define XML_BOOLEAN_LEN (9)
#define XML_BOOLEANC_LEN (10)
#define XML_INTEGER_LEN (9)
#define XML_INTEGERC_LEN (10)
#define XML_REAL_LEN (6)
#define XML_REALC_LEN (7)
#define XML_UUID_LEN (6)
#define XML_UUIDC_LEN (7)
#define XML_STRING_LEN (8)
#define XML_STRINGC_LEN (9)
#define XML_BINARY_LEN (8)
#define XML_BINARYC_LEN (9)
#define XML_DATE_LEN (6)
#define XML_DATEC_LEN (7)
#define XML_URI_LEN (5)
#define XML_URIC_LEN (6)
#define XML_MAP_LEN (5)
#define XML_MAPC_LEN (6)
#define XML_ARRAY_LEN (7)
#define XML_ARRAYC_LEN (8)
#define XML_KEY_LEN (5)
#define XML_KEYC_LEN (6)
static uint8_t const * const lxkey		= "<key>";
static uint8_t const * const lxkeyc		= "</key>\n";
static uint8_t const * const lxtabs		= "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

static uint8_t const * const lxstart[LLSD_TYPE_COUNT] =
{
	"<undef />\n",
	"<boolean>",
	"<integer>",
	"<real>",
	"<uuid>",
	"<string>",
	"<date>",
	"<uri>",
	"<binary>",
	"<array>\n",
	"<map>\n"
};

static uint8_t const * const lxend[LLSD_TYPE_COUNT] =
{
	"<undef />\n",
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
	XML_BINARY_LEN,
	XML_DATE_LEN,
	XML_URI_LEN,
	XML_MAP_LEN,
	XML_ARRAY_LEN,
};

static uint32_t lxendlen[LLSD_TYPE_COUNT] =
{
	XML_UNDEF_LEN,
	XML_BOOLEANC_LEN,
	XML_INTEGERC_LEN,
	XML_REALC_LEN,
	XML_UUIDC_LEN,
	XML_STRINGC_LEN,
	XML_BINARYC_LEN,
	XML_DATEC_LEN,
	XML_URIC_LEN,
	XML_MAPC_LEN,
	XML_ARRAYC_LEN,
};

static uint32_t indent = 1;

size_t llsd_format_xml( llsd_t * llsd, FILE * fout, int pretty )
{
	size_t num = 0;
	unsigned long start = ftell( fout );
	llsd_string_t s;

	llsd_itr_t itr;
	llsd_t *k, *v;
	llsd_type_t t = llsd_get_type( llsd );

	switch ( t )
	{
		case LLSD_UNDEF:
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t] + (pretty ? 1 : 0), fout );
			DEBUG( "%*sUNDEF %lu - %lu\n", indent, "\t", start, ftell( fout ) - 1 );
			break;
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
			s = llsd_as_string( llsd );
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t], fout );
			num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
			num += fwrite( lxend[t], sizeof(uint8_t), lxendlen[t] + (pretty ? 1 : 0), fout );
			DEBUG( "%*s%s %lu - %lu\n", indent, "\t", llsd_get_type_string( t ), start, ftell( fout ) - 1 );
			break;
		case LLSD_ARRAY:
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t] + (pretty ? 1 : 0), fout );
			DEBUG( "%*s[[ (%d)\n", indent, "\t", llsd_get_size( llsd ) );
			indent++;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				num += llsd_format_xml( v, fout, pretty );
			}
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxend[t], sizeof(uint8_t), lxendlen[t] + (pretty ? 1 : 0), fout );
			indent--;
			DEBUG( "%*s]] ARRAY %lu - %lu\n", indent, "\t", start, ftell( fout ) - 1 );
			break;
		case LLSD_MAP:
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxstart[t], sizeof(uint8_t), lxstartlen[t] + (pretty ? 1 : 0), fout );
			DEBUG( "%*s{{ (%d)\n", indent, "\t", llsd_get_size( llsd ) );
			indent++;
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				s = llsd_as_string( k );
				num += fwrite( lxkey, sizeof(uint8_t), XML_KEY_LEN, fout );
				num += fwrite( s.str, sizeof(uint8_t), s.str_len, fout );
				num += fwrite( lxkeyc, sizeof(uint8_t), XML_KEYC_LEN + (pretty ? 1 : 0), fout );
				num += llsd_format_xml( v, fout, pretty );
			}
			if ( pretty )
				num += fwrite( lxtabs, sizeof(uint8_t), indent, fout );
			num += fwrite( lxend[t], sizeof(uint8_t), lxendlen[t] + (pretty ? 1 : 0), fout );
			indent--;
			DEBUG( "%*s}} MAP %lu - %lu\n", indent, "\t", start, ftell( fout ) - 1 );
			break;
	}
	return num;

}

size_t llsd_get_xml_zero_copy_size( llsd_t * llsd, int pretty )
{
	size_t s = 0;
	CHECK_PTR_RET( llsd, 0 );
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
			/*
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
			*/
		case LLSD_MAP:
			/*
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				s += llsd_get_binary_zero_copy_size( k );
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
			*/
		return 0;
	}
	return 0;
}

size_t llsd_format_xml_zero_copy( llsd_t * llsd, struct iovec * v, int pretty )
{
	return 0;
}

