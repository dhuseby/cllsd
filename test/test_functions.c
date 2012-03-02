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

static void test_newdel( void )
{
	uint8_t bits[UUID_LEN] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
	int8_t const * const str = T("Hello World!");
	int8_t const * const url = T("http://www.ixquit.com");
	llsd_t* llsd;
	llsd_type_t type_;

	for ( type_ = LLSD_TYPE_FIRST; type_ < LLSD_TYPE_LAST; type_++ )
	{
		/* take a measure of the heap size */


		/* construct the llsd */
		switch( type_ )
		{
			case LLSD_BOOLEAN:
			case LLSD_INTEGER:
				llsd = llsd_new( type_, 1 );
				break;

			case LLSD_DATE:
				llsd = llsd_new( type_, 1.0 );
				break;

			case LLSD_UUID:
				llsd = llsd_new( type_, bits );
				break;

			case LLSD_STRING:
				llsd = llsd_new( type_, strlen( str ), str );
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

		/* delete the llsd */
		llsd_delete( llsd );

		/* check the heap size again */

		/* asser that the heap size after delete is the same as before new */
	}
}

static CU_pSuite add_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of all types", test_newdel), NULL );
	return pSuite;
}


