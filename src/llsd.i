/* swig 2.0 interface file for llsd */
%module llsd
%{
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cllsd/llsd.h>
#include <cllsd/llsd_serializer.h>
#include <cllsd/llsd_parser.h>
%}

%include <cllsd/llsd.h>
%include <cllsd/llsd_serializer.h>
%include <cllsd/llsd_parser.h>

#if defined(SWIGPYTHON)
    %typemap(in) int {
        $1 = PyInt_AsLong( $input );
    }
    %typemap(in) FILE* {
        if ( PyFile_Check( $input ) )
        {
            $1 = PyFile_AsFile( $input );
        }
        else
        {
            PyErr_SetSTring( PyExc_TypeError, "$1_name must be a file type." );
            return NULL;
        }
    }
#else
    #warning no "in" typemap defined
#endif

