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

#ifndef LLSD_H
#define LLSD_H

#include "array.h"
#include "hashtable.h"

typedef enum llsd_type_e
{
	LLSD_UNDEF,
	LLSD_BOOLEAN,
	LLSD_INTEGER,
	LLSD_REAL,
	LLSD_UUID,
	LLSD_STRING,
	LLSD_DATE,
	LLSD_URI,
	LLSD_BINARY,
	LLSD_ARRAY,
	LLSD_MAP

} llsd_type_t;

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

/* opaque types */
typedef struct llsd_s llsd_t;
typedef struct llsd_array_s llsd_array_t;
typedef struct llsd_map_s llsd_map_t;

/* type for iteration */
typedef struct llsd_itr_s
{
	llsd_t *		obj;
	union
	{
		array_itr_t	aitr;
		ht_itr_t	mitr;
	}				itr;
} llsd_itr_t;

/* new/delete llsd objects */
llsd_t * llsd_new( void );
void llsd_delete( llsd_t * llsd );

/* get the type of the particular object */
llsd_type_t llsd_get_type( llsd_t * llsd );

#if 0
/* iterator interface */
llsd_itr_t llsd_itr_next( llsd_itr_t itr );
llsd_itr_t llsd_itr_prev( llsd_itr_t itr );
llsd_itr_t llsd_itr_begin( llsd_t * llsd );
llsd_itr_t llsd_itr_end( llsd_t * llsd );

/* get the next value in the array/map */
int llsd_itr_get( llsd_itr_t itr, llsd_t * value, llsd_t * string );
#endif

#endif/*LLSD_H*/

