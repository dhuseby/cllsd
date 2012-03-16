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
#include "llsd_const.h"

/* string/binary conversions */
int llsd_stringify_uuid( llsd_t * llsd );
int llsd_destringify_uuid( llsd_t * llsd );
int llsd_stringify_date( llsd_t * llsd );
int llsd_destringify_date( llsd_t * llsd );
int llsd_escape_string( llsd_t * llsd );
int llsd_unescape_string( llsd_t * llsd );
int llsd_esacpe_uri( llsd_t * llsd );
int llsd_unescape_uri( llsd_t * llsd );
int llsd_encode_binary( llsd_t * llsd, llsd_bin_enc_t encoding );
int llsd_decode_binary( llsd_t * llsd );

#endif /* LLSD_XML_H */
