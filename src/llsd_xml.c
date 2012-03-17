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

#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>

#include "llsd.h"
#include "llsd_xml.h"

llsd_t * llsd_parse_xml( FILE * fin )
{
	return NULL;
}

size_t llsd_format_xml( llsd_t * llsd, FILE * fout )
{
	return 0;
}

size_t llsd_get_xml_zero_copy_size( llsd_t * llsd, int pretty )
{
	size_t s = 0;
	CHECK_PTR_RET( llsd, 0 );
	switch ( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_REAL:
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
			/*
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				if ( k != NULL )
				{
					WARN( "received key from array itr_get\n" );
				}
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
			*/
		case LLSD_MAP:
			/*
			itr = llsd_itr_begin( llsd );
			for ( ; itr != llsd_itr_end( llsd ); itr = llsd_itr_next( llsd, itr ) )
			{
				llsd_itr_get( llsd, itr, &v, &k );
				s += llsd_get_binary_zero_copy_size( k );
				s += llsd_get_binary_zero_copy_size( v );
			}
			return s;
			*/
		return 0;
	}
	return 0;
}

size_t llsd_format_xml_zero_copy( llsd_t * llsd, struct iovec * v, int pretty )
{
	return 0;
}

