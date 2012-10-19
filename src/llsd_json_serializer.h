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

#ifndef LLSD_JSON_SERIALIZER_H
#define LLSD_JSON_SERIALIZER_H

#include "llsd_serializer.h"

int llsd_json_serializer_init( FILE * fout, llsd_ops_t * const ops, int const pretty, void ** const user_data );
int llsd_json_serializer_deinit( FILE * fout, void * user_data );

#endif/*LLSD_JSON_SERIALIZER_H*/

