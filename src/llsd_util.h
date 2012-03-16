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

#ifndef LLSD_H
#define LLSD_H

#include <stdint.h>
#include <stdlib.h>

#include "llsd.h"

/* compare two llsd items */
int llsd_equal( llsd_t * l, llsd_t * r );

/* type conversions */
llsd_bool_t llsd_as_bool( llsd_t * llsd );
llsd_int_t llsd_as_int( llsd_t * llsd );
llsd_real_t llsd_as_real( llsd_t * llsd );
llsd_uuid_t llsd_as_uuid( llsd_t * llsd );
llsd_string_t llsd_as_string( llsd_t * llsd );
llsd_date_t llsd_as_date( llsd_t * llsd );
llsd_uri_t llsd_as_uri( llsd_t * llsd );
llsd_binary_t llsd_as_binary( llsd_t * llsd );
llsd_array_t llsd_as_array( llsd_t * llsd );
llsd_map_t llsd_as_map( llsd_t * llsd );

/* serialize/deserialize interface */
llsd_t * llsd_parse( FILE * fin );
size_t llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout, int pretty );

#endif/*LLSD_H*/

