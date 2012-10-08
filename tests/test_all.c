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

#include <cllsd/llsd.h>

#include "test_flags.h"
#include "test_macros.h"

SUITE( base16 );
SUITE( base64 );
SUITE( base85 );
SUITE( binary );
SUITE( notation );
#if 0
SUITE( xml );
#endif

int main()
{
	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add each suite of tests */
	ADD_SUITE( base16 );
	ADD_SUITE( base64 );
	ADD_SUITE( base85 );
	ADD_SUITE( binary );
	ADD_SUITE( notation );
#if 0
	ADD_SUITE( xml );
#endif

	/* run all tests using the CUnit Basic interface */
	CU_basic_set_mode( CU_BRM_VERBOSE );
	CU_basic_run_tests();

	/* clean up */
	CU_cleanup_registry();

	return CU_get_error();
}

