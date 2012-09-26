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

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>

#include <llsd.h>

#include <base64.h>

static int (*encode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static int (*decode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static uint32_t (*decoded_len_fn)( uint8_t const * in, uint32_t inlen );
static llsd_bin_enc_t encoding;
static uint8_t * enc = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWF"
					   "zb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdG"
					   "hlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsI"
					   "HRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUg"
					   "Y29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Y"
					   "ga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2"
					   "YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

static uint32_t get_encoded_len( uint32_t inlen )
{
	return BASE64_LENGTH( inlen );
}

static int init_base64_suite( void )
{
	encode_fn = &base64_encode;
	decode_fn = &base64_decode;
	decoded_len_fn = &base64_decoded_len;
	encoding = LLSD_BASE64;
	return 0;
}

static int deinit_base64_suite( void )
{
	return 0;
}

static CU_pSuite add_base64_tests( CU_pSuite pSuite )
{
	return pSuite;
}

/* include the test functions common to all serialization formats */
#include "test_base_common.c"

CU_pSuite add_base64_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Base64 Tests", init_base64_suite, deinit_base64_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in base64 specific tests */
	CHECK_PTR_RET( add_base64_tests( pSuite ), NULL );

	/* add in the common tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

