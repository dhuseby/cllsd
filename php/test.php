include("llsd.php");

l = llsd.llsd_new( llsd.LLSD_UNDEF );
f = fopen( "test.binary.llsd", "w+" );
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_BINARY, TRUE );
fclose( f );

