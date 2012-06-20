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

#ifndef __BASE16_H__
#define __BASE16_H__

#include <stdint.h>

#define BASE16_LENGTH(x) (x * 2)

int base16_encode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
int base16_decode (uint8_t const * in, uint32_t inlen, uint8_t * out, uint32_t * outlen);
uint32_t base16_decoded_len( uint8_t const * in, uint32_t inlen );

#endif/*__BASE16_H__*/

