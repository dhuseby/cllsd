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

#include "test_binary.h"
#include "test_xml.h"

/*#define POOL_SIZE ( 62 * 1024 * 1024 )*/

int main()
{
	CU_pSuite binary_test_suite;
	CU_pSuite notation_test_suite;
	CU_pSuite xml_test_suite;

	/* initialize debug allocator */
	/*init_alloc( POOL_SIZE );*/

	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add each suite of tests */
	binary_test_suite = add_binary_test_suite();
	xml_test_suite = add_xml_test_suite();

	/* run all tests using the CUnit Basic interface */
	CU_basic_set_mode( CU_BRM_VERBOSE );
	CU_basic_run_tests();

	/* clean up */
	CU_cleanup_registry();

	/* clean up debug allocator */
	/*deinit_alloc();*/

	return CU_get_error();
}

