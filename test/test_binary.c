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
#include <llsd_binary.h>
#include <test_allocator.h>

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

#if 0
/* offset of first by after header */
size_t const data_offset = 18;

/* expected values */
size_t expected_sizes[ LLSD_TYPE_COUNT ] = 
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
#endif

static void test_binary_byte_to_type( void )
{
	int c;

	for( c = 0; c <= UINT8_MAX; c++ )
	{
		switch ( c )
		{
			case '!':
				CU_ASSERT_EQUAL( LLSD_UNDEF, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case '0':
				CU_ASSERT_EQUAL( LLSD_BOOLEAN_FALSE, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case '1':
				CU_ASSERT_EQUAL( LLSD_BOOLEAN_TRUE, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'i':
				CU_ASSERT_EQUAL( LLSD_INTEGER, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'r':
				CU_ASSERT_EQUAL( LLSD_REAL, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'u':
				CU_ASSERT_EQUAL( LLSD_UUID, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'b':
				CU_ASSERT_EQUAL( LLSD_BINARY, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 's':
				CU_ASSERT_EQUAL( LLSD_STRING, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'l':
				CU_ASSERT_EQUAL( LLSD_URI, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case 'd':
				CU_ASSERT_EQUAL( LLSD_DATE, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case '[':
				CU_ASSERT_EQUAL( LLSD_ARRAY, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			case '{':
				CU_ASSERT_EQUAL( LLSD_MAP, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
			default:
				CU_ASSERT_EQUAL( LLSD_TYPE_INVALID, BYTE_TO_TYPE( (uint8_t)c ) );
				break;
		}
	}
}

static void test_type_to_binary_byte( void )
{
	llsd_type_t t;

	for( t = LLSD_TYPE_FIRST; t < LLSD_TYPE_LAST; t++ )
	{
		switch ( t )
		{
			case LLSD_UNDEF:
				CU_ASSERT_EQUAL( '!', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_BOOLEAN_FALSE:
				CU_ASSERT_EQUAL( '0', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_BOOLEAN_TRUE:
				CU_ASSERT_EQUAL( '1', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_INTEGER:
				CU_ASSERT_EQUAL( 'i', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_REAL:
				CU_ASSERT_EQUAL( 'r', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_UUID:
				CU_ASSERT_EQUAL( 'u', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_BINARY:
				CU_ASSERT_EQUAL( 'b', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_STRING:
				CU_ASSERT_EQUAL( 's', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_URI:
				CU_ASSERT_EQUAL( 'l', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_DATE:
				CU_ASSERT_EQUAL( 'd', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_ARRAY:
				CU_ASSERT_EQUAL( '[', TYPE_TO_BYTE( t ) );
				break;
			case LLSD_MAP:
				CU_ASSERT_EQUAL( '{', TYPE_TO_BYTE( t ) );
				break;
			default:
				CU_FAIL("invalid LLSD type");
				break;
		}
	}
}

static CU_pSuite add_binary_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "test binary byte to type", test_binary_byte_to_type ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "test type to binary byte", test_type_to_binary_byte ), NULL );
#if 0
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of all types", test_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "serialization of all types", test_serialization), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "serialization of random llsd", test_random_serialize), NULL );
#endif
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

