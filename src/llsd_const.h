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

#ifndef LLSD_CONST_H
#define LLSD_CONST_H

#include <stdint.h>
#include "llsd.h"

extern llsd_t const undefined;
extern llsd_int_t const zero_int;
extern llsd_int_t const one_int;
extern llsd_real_t const zero_real;
extern llsd_real_t const one_real;
extern llsd_uuid_t const zero_uuid;
extern llsd_string_t const false_string;
extern llsd_string_t const true_string;
extern llsd_string_t const empty_string;
extern llsd_binary_t const false_binary;
extern llsd_binary_t const true_binary;
extern llsd_binary_t const empty_binary;
extern llsd_uri_t const empty_uri;
extern llsd_date_t const empty_date;
extern llsd_array_t const empty_array;
extern llsd_map_t const empty_map;

#endif /* LLSD_CONST_H */

