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

#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdint.h>

/* This uses that the expression (n+(k-1))/k means the smallest
   integer >= n/k, i.e., the ceiling of n/k.  */
#define BASE64_LENGTH(inlen) ((((inlen) + 2) / 3) * 4)

int base64_encode( uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen );
int base64_decode( uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen );
uint32_t base64_decoded_len( uint8_t const * in, uint32_t inlen );

#endif/* __BASE64_H__ */
