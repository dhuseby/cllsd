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

#ifndef LLSD_XML_H
#define LLSD_XML_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>

#include "llsd.h"

llsd_t * llsd_parse_xml( FILE * fin );
size_t llsd_format_xml( llsd_t * llsd, FILE * fout );
size_t llsd_get_xml_zero_copy_size( llsd_t * llsd, int pretty );
size_t llsd_format_xml_zero_copy( llsd_t * llsd, struct iovec * v, int pretty );

#endif /* LLSD_XML_H */
