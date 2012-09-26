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

#ifndef LLSD_SERIALIZER_H
#define LLSD_SERIALIZER_H

#include <stdint.h>

#include "llsd.h"

typedef int (*serializer_init_fn)( FILE *, llsd_ops_t * const, int const, void ** const );
typedef int (*serializer_deinit_fn)( FILE *, void * );

int llsd_serialize_to_file( llsd_t * const llsd, FILE * fout, llsd_serializer_t const fmt, int const pretty );

#endif/*LLSD_SERIALIZER_H*/

