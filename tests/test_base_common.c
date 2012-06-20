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

/* NOTE: this file is included inline inside of the test_base16.c,
 * test_base64.c, and test_base85.c files.
 */

static uint8_t * data = "Man is distinguished, not only by his reason, but by this "
						"singular passion from other animals, which is a lust of the "
						"mind, that by a perseverance of delight in the continued and "
						"indefatigable generation of knowledge, exceeds the short "
						"vehemence of any carnal pleasure.";

static void test_encoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = get_encoded_len( strlen(data) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*encode_fn)( data, strlen(data), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( enc, out, outlen ), 0 );

	FREE( out );
}

static void test_decoding( void )
{
	uint32_t outlen;
	uint8_t * out;
	outlen = (*decoded_len_fn)( enc, strlen(enc) );
	out = CALLOC( outlen, sizeof(uint8_t) );

	CU_ASSERT_PTR_NOT_NULL_FATAL( out );

	/* encode the data */
	CU_ASSERT_EQUAL( (*decode_fn)( enc, strlen(enc), out, &outlen ), TRUE );

	/* make sure the encoded output matches expected output */
	CU_ASSERT_EQUAL( MEMCMP( data, out, outlen ), 0 );

	FREE( out );
}

static void test_encdec_random( void )
{
}

static void test_decenc_random( void )
{
}

static CU_pSuite add_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "encoding test data", test_encoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding test data", test_decoding), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "encoding/decoding random data", test_encdec_random), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "decoding/encoding random data", test_decenc_random), NULL );
	return pSuite;
}

