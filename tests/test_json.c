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

#include "test_macros.h"

static size_t const data_offset = 0;

/* expected values */
static size_t expected_sizes[ LLSD_TYPE_COUNT ] = 
{
	4,		/* LLSD_UNDEF	*/
	4,		/* LLSD_BOOLEAN */
	1,		/* LLSD_INTEGER */
	8,		/* LLSD_REAL	*/
	38,		/* LLSD_UUID	*/
	14,		/* LLSD_STRING	*/
	26,		/* LLSD_DATE	*/
	31,		/* LLSD_URI		*/
	33,		/* LLSD_BINARY	*/
	2,		/* LLSD_ARRAY	*/
	2		/* LLSD_MAP		*/
};

static uint8_t const undef_[] = "null";
static uint8_t const boolean_[] = "true";
static uint8_t const integer_[] = "1";
static uint8_t const real_[] = "1.000000";
static uint8_t const uuid_[] = "\"01020304-0506-0708-0900-010203040506\"";
static uint8_t const string_[] = "\"Hello World!\"";
static uint8_t const date_[] = "\"1970-01-01T00:00:01.000Z\"";
static uint8_t const uri_[] = "\"||uri||http://www.ixquick.com\"";
static uint8_t const binary_[] = "\"||b64||AQIDBAUGBwgJAAECAwQFBg==\"";
static uint8_t const array_[] = "[]";
static uint8_t const map_[] = "{}";

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

static int init_json_suite( void )
{
	format = LLSD_ENC_JSON;
	return 0;
}

static int deinit_json_suite( void )
{
	return 0;
}

static CU_pSuite add_json_tests( CU_pSuite pSuite )
{
	return pSuite;
}

/* include the test functions common to all serialization formats */
#include "test_common.c"

CU_pSuite add_json_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("JSON LLSD Tests", init_json_suite, deinit_json_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in json serialization specific tests */
	CHECK_PTR_RET( add_json_tests( pSuite ), NULL );

	/* add in the tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

