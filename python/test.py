import llsd

l = llsd.llsd_new( llsd.LLSD_UNDEF )

f = open("test.binary.llsd", "w+")
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_BINARY, True )
f.close()

f = open("test.xml.llsd", "w+")
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_XML, True )
f.close()

f = open("test.notation.llsd", "w+")
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_NOTATION, True )
f.close()

f = open("test.json.llsd", "w+")
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_JSON, True )
f.close()
