import os
import llsd

l = llsd.llsd_new( llsd.LLSD_UNDEF )
f = open("test.llsd", "w+")
llsd.llsd_serialize_to_file( l, f, llsd.LLSD_ENC_NOTATION, True )
f.close()

