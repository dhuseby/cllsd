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
#include <llsd_binary.h>

/* offset of first by after header */
static size_t const data_offset = BINARY_SIG_LEN;

/* expected values */
static size_t expected_sizes[ LLSD_TYPE_COUNT ] = 
{
	1,		/* LLSD_UNDEF */
	1,		/* LLSD_BOOLEAN */
	5,		/* LLSD_INTEGER */
	9,		/* LLSD_REAL */
	17,		/* LLSD_UUID */
	17,		/* LLSD_STRING 's' + 32-bit size + 'Hello World!' */
	9,		/* LLSD_DATE */
	27,		/* LLSD_URI 'l' + 32-bit size + 'http://ixquick.com' */
	21,		/* LLSD_BINARY 'b' + 32-bit size + '01234567890123456' */
	6,		/* LLSD_ARRAY '[' + 32-bit size + ']' */
	6		/* LLSD_MAP '{' + 32-bit size + '}' */
};

static uint8_t const undef_[] = { '!' };
static uint8_t const boolean_[]= { '1' };
static uint8_t const integer_[] = { 'i', 0x00, 0x00, 0x00, 0x01 };
static uint8_t const real_[] = { 'r', 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t const uuid_[] = { 'u', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
static uint8_t const string_[] = { 's', 0x00, 0x00, 0x00, 0x0c, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
static uint8_t const date_[] = { 'd', 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t const uri_[] = { 'l', 0x00, 0x00, 0x00, 0x16, 'h', 't', 't', 'p', ':', '/', '/', 'w', 'w', 'w', '.', 'i', 'x', 'q', 'u', 'i', 'c', 'k', '.', 'c', 'o', 'm' };
static uint8_t const binary_[] = { 'b', 0x00, 0x00, 0x00, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
static uint8_t const array_[] = { '[', 0x00, 0x00, 0x00, 0x00, ']' };
static uint8_t const map_[] = { '{', 0x00, 0x00, 0x00, 0x00, '}' };

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

static int init_binary_suite( void )
{
	format = LLSD_ENC_BINARY;
	return 0;
}

static int deinit_binary_suite( void )
{
	return 0;
}

static CU_pSuite add_binary_tests( CU_pSuite pSuite )
{
	return pSuite;
}


/* include the test functions common to all serialization formats */
#include "test_common.c"

CU_pSuite add_binary_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Binary LLSD Tests", init_binary_suite, deinit_binary_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in binary serialization specific tests */
	CHECK_PTR_RET( add_binary_tests( pSuite ), NULL );

	/* add tests common to both serialization formats */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

