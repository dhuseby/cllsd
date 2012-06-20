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
#include <llsd_const.h>
#include <llsd_util.h>

#include <base85.h>

static int (*encode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static int (*decode_fn)(uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
static uint32_t (*decoded_len_fn)( uint8_t const * in, uint32_t inlen );
static llsd_bin_enc_t encoding;
static uint8_t * enc = "9jqo^BlbD-BleB1DJ+*+F(f,q/0JhKF<GL>Cj@.4Gp$d7F!,L7@<6@)"
					   "/0JDEF<G%<+EV:2F!,O<DJ+*.@<*K0@<6L(Df-\\0Ec5e;DffZ(EZee"
					   ".Bl.9pF\"AGXBPCsi+DGm>@3BB/F*&OCAfu2/AKYi(DIb:@FD,*)+C]"
					   "U=@3BN#EcYf8ATD3s@q?d$AftVqCh[NqF<G:8+EV:.+Cf>-FD5W8ARl"
					   "olDIal(DId<j@<?3r@:F%a+D58'ATD4$Bl@l3De:,-DJs`8ARoFb/0J"
					   "MK@qB4^F!,R<AKZ&-DfTqBG%G>uD.RTpAKYo'+CT/5+Cei#DII?(E,9"
					   ")oF*2M7/c";
static uint8_t const z_data[] = { 0x00, 0x00, 0x00, 0x00 };
static uint32_t const z_data_len = 4;
static uint8_t const * z_enc = "z";
static uint8_t const y_data[] = { 0x20, 0x20, 0x20, 0x20 };
static uint32_t const y_data_len = 4;
static uint8_t const * y_enc = "y";

static uint32_t get_encoded_len( uint32_t inlen )
{
	return BASE85_LENGTH( inlen );
}


static void test_z_shortcut_encoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = get_encoded_len( z_data_len );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*encode_fn)( z_data, z_data_len, out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( z_enc, out, outlen ), 0 );

	FREE( out );
}

static void test_z_shortcut_decoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( z_enc, strlen(z_enc) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*decode_fn)( z_enc, strlen(z_enc), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( z_data, out, outlen ), 0 );

	FREE( out );
}

static void test_y_shortcut_encoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = get_encoded_len( y_data_len );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*encode_fn)( y_data, y_data_len, out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( y_enc, out, outlen ), 0 );

	FREE( out );
}

static void test_y_shortcut_decoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( y_enc, strlen(y_enc) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*decode_fn)( y_enc, strlen(y_enc), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( y_data, out, outlen ), 0 );

	FREE( out );
}

static int init_base85_suite( void )
{
	encode_fn = &base85_encode;
	decode_fn = &base85_decode;
	decoded_len_fn = &base85_decoded_len;
	encoding = LLSD_BASE85;
	return 0;
}

static int deinit_base85_suite( void )
{
	return 0;
}

static CU_pSuite add_base85_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "encoding z shortcut test data", test_z_shortcut_encoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding z shortcut test data", test_z_shortcut_decoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "encoding y shortcut test data", test_y_shortcut_encoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding y shortcut test data", test_y_shortcut_decoding), NULL );
	return pSuite;
}

/* include the test functions common to all serialization formats */
#include "test_base_common.c"

CU_pSuite add_base85_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Base85 Tests", init_base85_suite, deinit_base85_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in base85 specific tests */
	CHECK_PTR_RET( add_base85_tests( pSuite ), NULL );

	/* add in the common tests */
	CHECK_PTR_RET( add_tests( pSuite ), NULL );

	return pSuite;
}

