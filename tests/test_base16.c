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

#include <base16.h>

static int (*encode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static int (*decode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static uint32_t (*decoded_len_fn)( uint8_t const * in, uint32_t inlen );
static llsd_bin_enc_t encoding;
static uint8_t * enc = "4D616E2069732064697374696E677569736865642C206E6F74206F6"
					   "E6C792062792068697320726561736F6E2C20627574206279207468"
					   "69732073696E67756C61722070617373696F6E2066726F6D206F746"
					   "8657220616E696D616C732C2077686963682069732061206C757374"
					   "206F6620746865206D696E642C20746861742062792061207065727"
					   "365766572616E6365206F662064656C6967687420696E2074686520"
					   "636F6E74696E75656420616E6420696E6465666174696761626C652"
					   "067656E65726174696F6E206F66206B6E6F776C656467652C206578"
					   "6365656473207468652073686F727420766568656D656E6365206F6"
					   "620616E79206361726E616C20706C6561737572652E";
static uint8_t * low = "4d616e2069732064697374696e677569736865642c206e6f74206f6"
					   "e6c792062792068697320726561736f6e2c20627574206279207468"
					   "69732073696e67756c61722070617373696f6e2066726f6d206f746"
					   "8657220616e696d616c732c2077686963682069732061206c757374"
					   "206f6620746865206d696e642c20746861742062792061207065727"
					   "365766572616e6365206f662064656c6967687420696e2074686520"
					   "636f6e74696e75656420616e6420696e6465666174696761626c652"
					   "067656e65726174696f6e206f66206b6e6f776c656467652c206578"
					   "6365656473207468652073686f727420766568656d656e6365206f6"
					   "620616e79206361726e616c20706c6561737572652e";
static uint8_t * bad = "jk2e23r3";

/* forward declaration */
static void test_lower_case_decoding( void );
static void test_encode_short_output_buffer( void );
static void test_decode_short_output_buffer( void );
static void test_decoding_bad_data( void );

static uint32_t get_encoded_len( uint32_t inlen )
{
	return BASE16_LENGTH( inlen );
}

static int init_base16_suite( void )
{
	encode_fn = &base16_encode;
	decode_fn = &base16_decode;
	decoded_len_fn = &base16_decoded_len;
	encoding = LLSD_BASE16;
	return 0;
}

static int deinit_base16_suite( void )
{
	return 0;
}

static CU_pSuite add_base16_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding lower case data", test_lower_case_decoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "encoding to short output buffer", test_encode_short_output_buffer), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding to short output buffer", test_decode_short_output_buffer), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding bad data", test_decoding_bad_data), NULL );
	return pSuite;
}

/* include the test functions common to all serialization formats */
#include "test_base_common.c"

static void test_lower_case_decoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( low, strlen(low) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*decode_fn)( low, strlen(low), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( data, out, outlen ), 0 );

	FREE( out );
}

static void test_encode_short_output_buffer( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = get_encoded_len( strlen(data) ) - 10;
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*encode_fn)( data, strlen(data), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( enc, out, outlen ), 0 );

	FREE( out );
}

static void test_decode_short_output_buffer( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( enc, strlen(enc) ) - 10;
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*decode_fn)( enc, strlen(enc), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( data, out, outlen ), 0 );

	FREE( out );
}

static void test_decoding_bad_data( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( bad, strlen(bad) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data, must get a failure here */
	CU_ASSERT_EQUAL( (*decode_fn)( bad, strlen(bad), out, &outlen ), FALSE );

	FREE( out );
}

CU_pSuite add_base16_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Base16 Tests", init_base16_suite, deinit_base16_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in base16 specific tests */
	CHECK_PTR_RET( add_base16_tests( pSuite ), NULL );

	/* add in the common tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

