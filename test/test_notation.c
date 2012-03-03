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

#include "test_notation.h"

static llsd_serializer_t format;

static int init_notation_suite( void )
{
	format = LLSD_ENC_NOTATION;
	return 0;
}

static int deinit_notation_suite( void )
{
	return 0;
}

/* include the test functions */
/*#include "test_functions.c"*/

CU_pSuite add_notation_tests()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	/*pSuite = CU_add_suite("Notation LLSD Tests", init_notation_suite, deinit_notation_suite);
	CHECK_PTR_RET( pSuite, NULL );*/

	/* add in the tests */
	/*CHECK_PTR_RET( add_tests( pSuite ), NULL );*/

	return pSuite;
}

