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

#ifndef __TEST_ALLOCATOR_H__
#define __TEST_ALLOCATOR_H__

void init_alloc( size_t pool_size );
void deinit_alloc( void );
size_t get_heap_size( void );
void dump_heap_blocks( void );

void * calloc_( size_t nmemb, size_t size );
void * malloc_( size_t size );
void free_( void * ptr );
void * realloc_( void * ptr, size_t size );
uint8_t * strdup_( uint8_t const * str );

#endif/*__TEST_ALLOCATOR_H__*/

