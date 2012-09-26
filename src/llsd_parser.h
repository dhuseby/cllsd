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

#ifndef LLSD_PARSER_H
#define LLSD_PARSER_H

#include <stdint.h>

#include "llsd.h"

typedef struct llsd_parser_ops_s
{
	int (*undef_fn)( void * const user_data );
	int (*boolean_fn)( int const value, void * const user_data );
	int (*integer_fn)( int32_t const value, void * const user_data );
	int (*real_fn)( double const value, void * const user_data );
	int (*uuid_fn)( uint8_t const value[UUID_LEN], void * const user_data );
	int (*string_fn)( uint8_t const * str, void * const user_data );
	int (*date_fn)( double const value, void * const user_data );
	int (*uri_fn)( uint8_t const * uri, void * const user_data );
	int (*binary_fn)( uint8_t const * data, uint32_t const len, void * const user_data );
	int (*array_begin_fn)( uint32_t const size, void * const user_data );
	int (*array_end_fn)( void * const user_data );
	int (*map_begin_fn)( uint32_t const size, void * const user_data );
	int (*map_end_fn)( void * const user_data );

} llsd_parser_ops_t;

llsd_t * llsd_parse_file( FILE * fin );

#endif/*LLSD_PARSER_H*/

