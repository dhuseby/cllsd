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

#ifndef __MACROS_H__
#define __MACROS_H__

/* 
 * local definitions for porting purposes
 */

/* BOOLEANS */
#define FALSE ((int)0)
#define TRUE  ((int)1)

/* used for debugging purposes */
#define ASSERT(x) assert(x)
#define WARN(fmt, ...) { fprintf(stderr, "%20s:%-5d -- " fmt,  __FILE__, __LINE__, ##__VA_ARGS__); fflush(stderr); }
#define LOG(...) { fprintf(stdout, __VA_ARGS__); fflush(stdout); }
#define FAIL(fmt, ...) { fprintf(stderr, "%20s:%-5d -- " fmt,  __FILE__, __LINE__, ##__VA_ARGS__); fflush(stderr); assert(0); }

/* runtime check macros */
#define CHECK(x) { if(!(x)) return; }
#define CHECK_MSG(x, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return; } }
#define CHECK_RET(x, y) { if(!(x)) return (y); }
#define CHECK_RET_MSG(x, y, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } }
#define CHECK_PTR(x) { if(!(x)) return; }
#define CHECK_PTR_MSG(x, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return; } }
#define CHECK_PTR_RET(x, y) { if(!(x)) return (y); }
#define CHECK_PTR_RET_MSG(x, y, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } }

/* abstractions of the memory allocator */
#define MALLOC malloc
#define CALLOC calloc
#define REALLOC realloc
#define FREE free
#define MEMCPY memcpy
#define MEMSET memset
#define STRDUP strdup

/* casting macro for string constants */
#define T(x)    (int8_t*)(x)
#define UT(x)	(uint8_t*)(x)
#define C(x)	(char*)(x)

#endif/*__MACROS_H__*/
 
