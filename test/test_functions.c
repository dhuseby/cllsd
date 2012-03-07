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

/* NOTE: this file is included inline inside of the test_binary.c,
 * test_notation.c, and test_xml.c files.  they use whatever serializer
 * deserializer functions are specified when the suites are initialized.
 */

extern FILE* tmpf;
extern llsd_serializer_t format;




static llsd_t * get_random_array( uint32_t size )
{
	return NULL;
}

static llsd_t * get_random_map( uint32_t size )
{
	return NULL;
}

static llsd_t * get_random_llsd( uint32_t size )
{
	return NULL;
}

static llsd_t * get_random( uint32_t size, uint32_t seed )
{
	llsd_t * llsd;

	/* set the seed */
	srand( seed );

	

	return NULL;
}



uint8_t const bits[UUID_LEN] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
int8_t const * const str = T("Hello World!");
int8_t const * const url = T("http://www.ixquick.com");

static llsd_t * get_llsd( llsd_type_t type_ )
{
	llsd_t * llsd = NULL;

	/* construct the llsd */
	switch( type_ )
	{
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
			llsd = llsd_new( type_, 1 );
			break;

		case LLSD_REAL:
		case LLSD_DATE:
			llsd = llsd_new( type_, 1.0 );
			break;

		case LLSD_UUID:
			llsd = llsd_new( type_, bits );
			break;

		case LLSD_STRING:
			llsd = llsd_new( type_, str, strlen( str ) );
			break;

		case LLSD_URI:
			llsd = llsd_new( type_, url );
			break;

		case LLSD_BINARY:
			llsd = llsd_new( type_, UUID_LEN, bits );
			break;

		case LLSD_UNDEF:
		case LLSD_ARRAY:
		case LLSD_MAP:
			llsd = llsd_new( type_ );
			break;
	}

	CU_ASSERT_PTR_NOT_NULL( llsd );
	CU_ASSERT_EQUAL( type_, llsd_get_type( llsd ) );

	return llsd;
}

static void test_newdel( void )
{
	int i;
	llsd_t* llsd;
	llsd_type_t type_;

	for ( type_ = LLSD_TYPE_FIRST; type_ < LLSD_TYPE_LAST; type_++ )
	{
		/* take a measure of the heap size */

		/* construct the llsd */
		llsd = get_llsd( type_ );
		CU_ASSERT_PTR_NOT_NULL( llsd );

		/* check the type */
		CU_ASSERT_EQUAL( type_, llsd_get_type( llsd ) );

		/* delete the llsd */
		llsd_delete( llsd );
		llsd = NULL;

		/* check the heap size again */

		/* assert that the heap size after delete is the same as before new */
	}
}

#define BUF_SIZE (4096)
static void test_serialization( void )
{
	llsd_t* llsd;
	llsd_type_t type_;
	size_t s = 0;
	uint8_t * buf = UT(CALLOC( BUF_SIZE, sizeof(uint8_t) ));
	CU_ASSERT_PTR_NOT_NULL_FATAL( buf );

	for ( type_ = LLSD_TYPE_FIRST; type_ < LLSD_TYPE_LAST; type_++ )
	{
		tmpf = fmemopen( buf, BUF_SIZE, "w+b" );
		CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

		/* construct the llsd */
		llsd = get_llsd( type_ );

		/* check the type */
		CU_ASSERT( type_ == llsd_get_type( llsd ) );

		/* serialize it to the file */
		s = llsd_format( llsd, format, tmpf );
		fclose( tmpf );
		tmpf = NULL;

		/* delete the llsd */
		llsd_delete( llsd );
		llsd = NULL;

		/* check that the correct number of bytes were written */
		if ( expected_sizes[type_] != (s - data_offset) )
		{
			WARN("type: %s, expected: %d, actual: %d\n", llsd_get_type_string( type_ ), expected_sizes[type_], (s - data_offset) ); 
			CU_FAIL();
		}

		/* check that the expected data was written */
		CU_ASSERT( 0 == memcmp( &(buf[data_offset]), expected_data[ type_ ], (s - data_offset) ) );

		/* reopen the buffer */
		tmpf = fmemopen( buf, BUF_SIZE, "r+b" );
		CU_ASSERT( NULL != tmpf );

		/* try to deserialize the llsd */
		llsd = llsd_parse( tmpf );
		CU_ASSERT( NULL != llsd );

		/* check the type */
		CU_ASSERT( type_ == llsd_get_type( llsd ) );

		/* clean up */
		fclose( tmpf );
		tmpf = NULL;
		llsd_delete( llsd );
		llsd = NULL;
		memset( buf, 0, s );
		s = 0;
	}
}

static CU_pSuite add_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of all types", test_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "serialization of all types", test_serialization), NULL );
	return pSuite;
}


