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

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <endian.h>
#include <math.h>
#include <time.h>

#include "debug.h"
#include "macros.h"
#include "hashtable.h"
#include "array.h"
#include "llsd_const.h"
#include "llsd_binary.h"
#include "llsd_xml.h"
#include "llsd.h"
#include "base64.h"

static uint8_t const * const binary_header = "<? LLSD/Binary ?>\n";
static uint8_t const * const xml_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<llsd>\n";
static uint8_t const * const xml_footer = "</llsd>\n";

#define SIG_LEN (18)
llsd_t * llsd_parse( uint8_t * p, size_t len )
{
	CHECK_RET_MSG( p, NULL, "invalid buffer pointer\n" );
	CHECK_RET_MSG( len >= SIG_LEN, NULL, "not enough valid LLSD bytes\n" );

	if ( MEMCMP( p, binary_header, SIG_LEN ) == 0 )
	{
		return (llsd_t *)llsd_parse_binary( &(p[SIG_LEN]), (len - SIG_LEN) );
	}
	else
	{
		return (llsd_t *)llsd_parse_xml( p, len );
	}
}


