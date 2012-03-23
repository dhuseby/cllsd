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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>

#include <llsd.h>
#include <llsd_const.h>
#include <llsd_util.h>
#include <llsd_xml.h>

#include "test_xml.h"

/* offset of first by after header */
static size_t const data_offset = 38;

/* expected values */
static size_t expected_sizes[ LLSD_TYPE_COUNT ] = 
{
	22,		/* LLSD_UNDEF */
	36,		/* LLSD_BOOLEAN */
	33,		/* LLSD_INTEGER */
	34,		/* LLSD_REAL */
	62,		/* LLSD_UUID */
	42,		/* LLSD_STRING '<string>' + 'Hello World!' + '</string>' */
	50,		/* LLSD_DATE */
	46,		/* LLSD_URI '<uri>' + 'http://ixquick.com' + '</uri>' */
	72,		/* LLSD_BINARY '<binary>' + 'AQIDBAUGBwgJAAECAwQFBg==' + '</binary>' */
	37,		/* LLSD_ARRAY '<array size="0">>' + '</array>' */
	33		/* LLSD_MAP '<map size="0"> + '</map>' */
};

static uint8_t const undef_[] = "<llsd><undef /></llsd>";
static uint8_t const boolean_[] = "<llsd><boolean>true</boolean></llsd>";
static uint8_t const integer_[] = "<llsd><integer>1</integer></llsd>";
static uint8_t const real_[] = "<llsd><real>1.000000</real></llsd>";
static uint8_t const uuid_[] = "<llsd><uuid>01020304-0506-0708-0900-010203040506</uuid></llsd>";
static uint8_t const string_[] = "<llsd><string>Hello World!</string></llsd>";
static uint8_t const date_[] = "<llsd><date>1970-01-01T00:00:01.000Z</date></llsd>";
static uint8_t const uri_[] = "<llsd><uri>http://www.ixquick.com</uri></llsd>";
static uint8_t const binary_[] = "<llsd><binary encoding=\"base64\">AQIDBAUGBwgJAAECAwQFBg==</binary></llsd>";
static uint8_t const array_[] = "<llsd><array size=\"0\"></array></llsd>";
static uint8_t const map_[] = "<llsd><map size=\"0\"></map></llsd>";

static uint8_t const * const expected_data[ LLSD_TYPE_COUNT ] =
{
	undef_,
	boolean_,
	integer_,
	real_,
	uuid_,
	string_,
	date_,
	uri_,
	binary_,
	array_,
	map_,
};

static llsd_serializer_t format;
static FILE* tmpf;

static int init_xml_suite( void )
{
	format = LLSD_ENC_XML;
	return 0;
}

static int deinit_xml_suite( void )
{
	return 0;
}

static CU_pSuite add_xml_tests( CU_pSuite pSuite )
{
	return pSuite;
}

/* include the test functions common to all serialization formats */
#include "test_common.c"

CU_pSuite add_xml_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("XML LLSD Tests", init_xml_suite, deinit_xml_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in binary serialization specific tests */
	CHECK_PTR_RET( add_xml_tests( pSuite ), NULL );

	/* add in the tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

