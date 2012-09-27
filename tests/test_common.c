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

static int indent = 0;

static llsd_type_t get_random_llsd_type( void )
{
	return (llsd_type_t)(rand() % LLSD_TYPE_COUNT);
}

static llsd_t* get_random_str( int zero )
{
	static uint8_t str[1024];
	int i;
	int len = (rand() % 128);
	if ( !zero && !len )
		len++;

	for ( i = 0; i < len; i++ )
	{
		/* get a random, printable ascii character */
		str[i] = (32 + (rand() % 94));
	}
	str[len] = '\0';
	DEBUG( "%*sSTRING %s\n", indent, " ", str );
	/* tell it to copy the string into the LLSD */
	return llsd_new_string( str, FALSE );
}

static llsd_t* get_random_uri( void )
{
	static uint8_t uri[1024];
	int i;
	int len = (rand() % 128);

	for ( i = 0; i < len; i++ )
	{
		/* get a random, printable ascii character */
		uri[i] = (32 + (rand() % 94));
		/*uri[i] = (rand() % 26) + 'a';*/
	}
	uri[len] = '\0';
	DEBUG( "%*sURI %s\n", indent, " ", uri );
	/* tell it to copy the uri into the LLSD */
	return llsd_new_uri( uri, FALSE );
}

static llsd_t* get_random_bin( void )
{
	static uint8_t bin[1024];
	int i;
	int len = (rand() % 1024);

	if ( len == 0 )
	{
		return llsd_new_binary( NULL, 0, FALSE );	
	}

	for ( i = 0; i < len; i++ )
	{
		/* get a random byte*/
		bin[i] = (uint8_t)(rand() % 256);
	}
	DEBUG( "%*sBINARY %d\n", indent, " ", len );
	/* tell it to copy the binary into the LLSD */
	return llsd_new_binary( bin, len, FALSE );
}

static llsd_t* get_random_uuid( void )
{
	static uint8_t bits[UUID_LEN];
	int i;

	for ( i = 0; i < UUID_LEN; i++ )
	{
		/* get a random byte*/
		bits[i] = (uint8_t)(rand() % 256);
	}
	DEBUG( "%*sUUID %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", indent, " ", bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7], bits[8], bits[9], bits[10], bits[11], bits[12], bits[13], bits[14], bits[15] );
	return llsd_new_uuid( bits );
}

static llsd_t * get_random_boolean( void )
{
	int b = rand() % 2;
	DEBUG( "%*sBOOLEAN %s\n", indent, " ", ( b ? "true" : "false" ) );
	return llsd_new_boolean( b );
}

static llsd_t * get_random_integer( void )
{
	int i = rand();
	DEBUG( "%*sINTEGER %d\n", indent, " ", i );
	return llsd_new_integer( i );
}

static llsd_t * get_random_real( void )
{
	double d = 1.0 * rand();
	DEBUG( "%*sREAL %lf\n", indent, " ", d );
	return llsd_new_real( d );
}

static llsd_t* get_random_date( void )
{
	double d = 1.0 * rand();
	double int_time = floor( d );
	time_t seconds = (time_t)int_time;
	int32_t useconds = (int32_t)( ( d - int_time) * 1000000.0 );
	struct tm parts = *gmtime(&seconds);
	DEBUG( "%*sDATE %04d-%02d-%02dT%02d:%02d:%02d.%03dZ\n", indent, " ", parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec, ((useconds != 0) ? (int32_t)(useconds / 1000.f + 0.5f) : 0) );
	return llsd_new_date( d );
}

/* forward declaration */
static llsd_t * get_random_map( uint32_t size );

static llsd_t * get_random_array( uint32_t size )
{
	uint32_t total = 0;
	uint32_t s = 0;
	llsd_type_t type_;

	/* create the array */
	llsd_t * arr = llsd_new_array();
	
	DEBUG( "%*s[[\n", indent, " " );
	indent += 4;

	/* now populate it with random data */
	while( total < size )
	{
		type_ = get_random_llsd_type();

		switch( type_ )
		{
			case LLSD_UNDEF:
				llsd_array_append( arr, llsd_new_undef() );
				total++;
				break;
			case LLSD_BOOLEAN:
				llsd_array_append( arr, get_random_boolean() );
				total++;
				break;
			case LLSD_INTEGER:
				llsd_array_append( arr, get_random_integer() );
				total++;
				break;
			case LLSD_REAL:
				llsd_array_append( arr, get_random_real() );
				total++;
				break;
			case LLSD_UUID:
				llsd_array_append( arr, get_random_uuid() );
				total++;
				break;
			case LLSD_STRING:
				llsd_array_append( arr, get_random_str( TRUE ) );
				total++;
				break;
			case LLSD_DATE:
				llsd_array_append( arr, get_random_date() );
				total++;
				break;
			case LLSD_URI:
				llsd_array_append( arr, get_random_uri() );
				total++;
				break;
			case LLSD_BINARY:
				llsd_array_append( arr, get_random_bin() );
				total++;
				break;
			case LLSD_ARRAY:	
				s = (rand() % (size - total));
				llsd_array_append( arr, get_random_array( s ) );
				if ( s == 0 )
				{
					total++;
				}
				else
				{
					total += s;
				}
				break;
			case LLSD_MAP:
				s = (rand() % (size - total));
				llsd_array_append( arr, get_random_map( s ) );
				if ( s == 0 )
				{
					total++;
				}
				else
				{
					total += s;
				}
			break;
		}
	}
	
	indent -= 4;
	DEBUG( "%*s]]\n", indent, " " );

	return arr;
}

static llsd_t * get_random_map( uint32_t size )
{
	uint32_t total = 0;
	uint32_t s = 0;
	llsd_type_t type_;
	llsd_t * map;
	llsd_t * key;

	/* create the map */
	map = llsd_new_map();

	DEBUG( "%*s{{\n", indent, " " );
	indent += 4;

	/* now populate it with random data */
	while( total < size )
	{
		/* get a random type */
		type_ = get_random_llsd_type();

		/* get a random key */
		key = get_random_str( FALSE );

		switch( type_ )
		{
			case LLSD_UNDEF:
				llsd_map_insert( map, key, llsd_new_undef() );
				total++;
				break;
			case LLSD_BOOLEAN:
				llsd_map_insert( map, key, get_random_boolean() );
				total++;
				break;
			case LLSD_INTEGER:
				llsd_map_insert( map, key, get_random_integer() );
				total++;
				break;
			case LLSD_REAL:
				llsd_map_insert( map, key, get_random_real() );
				total++;
				break;
			case LLSD_UUID:
				llsd_map_insert( map, key, get_random_uuid() );
				total++;
				break;
			case LLSD_STRING:
				llsd_map_insert( map, key, get_random_str( TRUE ) );
				total++;
				break;
			case LLSD_DATE:
				llsd_map_insert( map, key, get_random_date() );
				total++;
				break;
			case LLSD_URI:
				llsd_map_insert( map, key, get_random_uri() );
				total++;
				break;
			case LLSD_BINARY:
				llsd_map_insert( map, key, get_random_bin() );
				total++;
				break;
			case LLSD_ARRAY:	
				s = (rand() % (size - total));
				llsd_map_insert( map, key, get_random_array( s ) );
				if ( s == 0 )
				{
					total++;
				}
				else
				{
					total += s;
				}
				break;
			case LLSD_MAP:
				s = (rand() % (size - total));
				llsd_map_insert( map, key, get_random_map( s ) );
				if ( s == 0 )
				{
					total++;
				}
				else
				{
					total += s;
				}
				break;
		}
		DEBUG( "%*s----------\n", indent, " " );
	}
	indent -= 4;
	DEBUG( "%*s}}\n", indent, " " );

	return map;
}

static llsd_t * get_random_llsd( uint32_t size, uint32_t seed )
{
	llsd_t * llsd;
	indent = 0;

	/* set the seed */
	srand( seed );

	DEBUG( "\n" );
	if ( rand() % 2 )
	{
		return get_random_map( size );
	}
	else
	{
		return get_random_array( size );
	}
}



static uint8_t const testbits[UUID_LEN] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
static int8_t const * const teststr = T("Hello World!");
static int8_t const * const testurl = T("http://www.ixquick.com");

static llsd_t * get_llsd( llsd_type_t type_ )
{
	llsd_t * llsd = NULL;

	/* construct the llsd */
	switch( type_ )
	{
		case LLSD_UNDEF:
			llsd = llsd_new( type_ );
			break;

		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
			llsd = llsd_new( type_, 1 );
			break;

		case LLSD_REAL:
		case LLSD_DATE:
			llsd = llsd_new( type_, 1.0 );
			break;

		case LLSD_UUID:
			llsd = llsd_new( type_, testbits );
			break;

		case LLSD_STRING:
			llsd = llsd_new_string( teststr, FALSE );
			break;

		case LLSD_URI:
			llsd = llsd_new_uri( testurl, FALSE );
			break;

		case LLSD_BINARY:
			llsd = llsd_new_binary( testbits, UUID_LEN, FALSE );
			break;

		case LLSD_ARRAY:
			llsd = llsd_new_array();
			break;

		case LLSD_MAP:
			llsd = llsd_new_map();
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
		/* construct the llsd */
		llsd = get_llsd( type_ );
		CU_ASSERT_PTR_NOT_NULL( llsd );

		/* check the type */
		CU_ASSERT_EQUAL( type_, llsd_get_type( llsd ) );

		/* delete the llsd */
		llsd_delete( llsd );
		llsd = NULL;
	}
}

#define BUF_SIZE (4096)
static void test_serialization( void )
{
	llsd_t* llsd;
	llsd_type_t type_;
	long floc = 0;
	long nmemb = 0;
	uint8_t * buf = NULL;

	for ( type_ = LLSD_TYPE_FIRST; type_ < LLSD_TYPE_LAST; type_++ )
	{
		tmpf = tmpfile();
		CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

		/* construct the llsd */
		llsd = get_llsd( type_ );

		/* check the type */
		CU_ASSERT( type_ == llsd_get_type( llsd ) );

		/* serialize it to the file */
		CU_ASSERT_TRUE( llsd_serialize_to_file( llsd, tmpf, format, FALSE ) );

		/* get the location */
		floc = ftell( tmpf );

		/* reset the read location */
		fseek( tmpf, 0, SEEK_SET );

		/* how many bytes were written? */
		nmemb = floc - ftell(tmpf);

		/* allocate a buffer for the contents of the file */
		buf = UT(CALLOC( nmemb, sizeof(uint8_t) ));

		/* read the file into the buffer */
		CU_ASSERT( nmemb == fread( buf, sizeof(uint8_t), nmemb, tmpf ) );

		/* delete the llsd */
		llsd_delete( llsd );
		llsd = NULL;

		/* reset the read location again */
		fseek( tmpf, 0, SEEK_SET );

		/* check that the correct number of bytes were written */
		if ( expected_sizes[type_] != (nmemb - data_offset) )
		{
#if defined(PORTABLE_64_BIT)
			WARN("type: %s, expected: %ld, actual: %ld\n", llsd_get_type_string( type_ ), expected_sizes[type_], (nmemb - data_offset) ); 
#else
			WARN("type: %s, expected: %d, actual: %lu\n", llsd_get_type_string( type_ ), expected_sizes[type_], (nmemb - data_offset) ); 
#endif
			CU_FAIL();
		}

		/* check that the expected data was written */
		if ( 0 != memcmp( &(buf[data_offset]), expected_data[ type_ ], (nmemb - data_offset) ) )
		{
			WARN("type: %s failed memcmp\n", llsd_get_type_string( type_ ) );
			CU_FAIL("memcmp");
		}

		/* try to deserialize the llsd */
		llsd = (llsd_t*)llsd_parse_from_file( tmpf );
		CU_ASSERT_PTR_NOT_NULL_FATAL( llsd );

		/* check the type */
		CU_ASSERT( type_ == llsd_get_type( llsd ) );

		/* clean up */
		fclose( tmpf );
		tmpf = NULL;
		llsd_delete( llsd );
		llsd = NULL;
		FREE( buf );
		buf = NULL;
	}

	FREE(buf);
}

static void test_random_serialize( void )
{
	const uint32_t seed = 0xDEADBEEF;
	const uint32_t size = 16384;
	llsd_t * llsd_out = NULL;
	llsd_t * llsd_in = NULL;
	FILE* fin = NULL;
	FILE* fout = NULL;

	/* generate a repeatable, random llsd object */
	llsd_out = get_random_llsd( size, seed );
	CU_ASSERT_PTR_NOT_NULL_FATAL( llsd_out );

	tmpf = fopen( "test.llsd", "w+b" );
	CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

	CU_ASSERT_TRUE( llsd_serialize_to_file( llsd_out, tmpf, format, FALSE ) );
	fclose( tmpf );
	tmpf = NULL;

	tmpf = fopen( "test.llsd", "r+b" );
	CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

	llsd_in = (llsd_t*)llsd_parse_from_file( tmpf );
	CU_ASSERT_PTR_NOT_NULL_FATAL( llsd_in );
	fclose( tmpf );
	tmpf = NULL;

	/* make sure the two llsd structures are equivilent */
	CU_ASSERT_FATAL( llsd_equal( llsd_out, llsd_in ) );

	llsd_delete( llsd_out );
	llsd_out = NULL;
	llsd_delete( llsd_in );
	llsd_in = NULL;
}

#if 0
static void test_random_serialize_zero_copy( void )
{
	ssize_t ret;
	const uint32_t seed = 0xDEADBEEF;
	const uint32_t size = 16384;
	size_t s = 0;
	struct iovec * iov;
	llsd_t * llsd_out = NULL;
	llsd_t * llsd_in = NULL;

	/* get the initial heap size */
	/*size_t heap_size = get_heap_size();*/

	/* generate a repeatable, random llsd object */
	llsd_out = get_random_llsd( size, seed );
	CU_ASSERT_PTR_NOT_NULL_FATAL( llsd_out );

	tmpf = fopen( "testzc.llsd", "w+b" );
	CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

	/* get the zero copy list of iovec structs */
	s = llsd_format_zero_copy( llsd_out, format, &iov, TRUE );

	if ( format == LLSD_ENC_XML )
	{
		CU_ASSERT_EQUAL( s, 3 );
	}
	else
	{
		/* use gather write to write to file */
		ret = writev( fileno( tmpf ), iov, s );
		if ( ret == -1 )
		{
			perror(NULL);
			fclose(tmpf);
			tmpf = NULL;
			llsd_delete( llsd_out );
			llsd_out = NULL;
			return;
		}
		CU_ASSERT_NOT_EQUAL( ret, -1 );
		fclose( tmpf );
		tmpf = NULL;

		tmpf = fopen( "testzc.llsd", "r+b" );
		CU_ASSERT_PTR_NOT_NULL_FATAL( tmpf );

		llsd_in = llsd_parse( tmpf );
		CU_ASSERT_PTR_NOT_NULL_FATAL( llsd_in );

		fclose( tmpf );
		tmpf = NULL;

		/* make sure the two llsd structures are equivilent */
		CU_ASSERT( llsd_equal( llsd_out, llsd_in ) );

		llsd_delete( llsd_in );
		llsd_in = NULL;
	}

	llsd_delete( llsd_out );
	llsd_out = NULL;

	/*CU_ASSERT_EQUAL( heap_size, get_heap_size() );*/
}
#endif

static CU_pSuite add_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of all types", test_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "serialization of all types", test_serialization), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "serialization of random llsd", test_random_serialize), NULL );
#if 0
	CHECK_PTR_RET( CU_add_test( pSuite, "zero copy serialization of random llsd", test_random_serialize_zero_copy), NULL );
	if ( format != LLSD_ENC_XML )
		CHECK_PTR_RET( CU_add_test( pSuite, "serialization of random llsd zero copy", test_random_serialize_zero_copy), NULL );
#endif
	return pSuite;
}


