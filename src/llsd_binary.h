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

#ifndef LLSD_BINARY_TYPES_H
#define LLSD_BINARY_TYPES_H

#include <stdint.h>
#include "llsd.h"

extern const llsd_type_t llsd_binary_types[UINT8_MAX + 1];
extern const uint8_t llsd_binary_bytes[LLSD_TYPE_COUNT];

#define BYTE_TO_TYPE( c ) (llsd_binary_types[c])
#define TYPE_TO_BYTE( t ) (((t >= LLSD_TYPE_FIRST) && (t < LLSD_TYPE_LAST)) ? llsd_binary_bytes[t] : 0 )

#endif//LLSD_BINARY_TYPES_H

