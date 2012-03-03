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

#include <debug.h>
#include <macros.h>
#include <llsd.h>

#include "test_binary.h"

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

/* expected values */
size_t expected_sizes[ LLSD_TYPE_COUNT ] = 
{
	1,		/* LLSD_UNDEF */
	1,		/* LLSD_BOOLEAN */
	5,		/* LLSD_INTEGER */
	9,		/* LLSD_REAL */
	17,		/* LLSD_UUID */
	13,		/* LLSD_STRING 's' + 'Hello World!' */
	9,		/* LLSD_DATE */
	19,		/* LLSD_URI 'l' + 'http://ixquick.com' */
	15,		/* LLSD_BINARY 'b' + 32-bit size + '0123456789' */
	6,		/* LLSD_ARRAY '[' + 32-bit size + ']' */
	6		/* LLSD_MAP '{' + 32-bit size + '}' */
};

uint8_t const undef_[] = { '!' };
uint8_t const boolean_[]= { '1' };
uint8_t const integer_[] = { 'i', 0x00, 0x00, 0x00, 0x01 };
uint8_t const real_[] = { 'r', 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t const uuid_[] = { 'u', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
uint8_t const string_[] = { 's', 0x00, 0x00, 0x00, 0x0c, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
uint8_t const date_[] = { 'd', 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t const uri_[] = { 'l', 0x00, 0x00, 0x00, 0x16, 'h', 't', 't', 'p', ':', '/', '/', 'w', 'w', 'w', '.', 'i', 'x', 'q', 'u', 'i', 'c', 'k', '.', 'c', 'o', 'm' };
uint8_t const binary_[] = { 'b', 0x00, 0x00, 0x00, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
uint8_t const array_[] = { '[', 0x00, 0x00, 0x00, 0x00, ']' };
uint8_t const map_[] = { '{', 0x00, 0x00, 0x00, 0x00, '}' };

uint8_t const * const expected_data[ LLSD_TYPE_COUNT ] =
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

/* include the test functions */
#include "test_functions.c"

CU_pSuite add_binary_tests()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Binary LLSD Tests", init_binary_suite, deinit_binary_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in the tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

