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

#include <stdint.h>

#include <cutil/debug.h>
#include <cutil/macros.h>

#include "llsd.h"
#include "llsd_serializer.h"
#include "llsd_xml_serializer.h"
#include "llsd_binary_serializer.h"
#include "llsd_notation_serializer.h"
#include "llsd_json_serializer.h"

/* forward decl of private serializer driver */
static int llsd_serialize( llsd_t * const llsd, FILE * fout, llsd_ops_t * const ops, void * user_data );

serializer_init_fn const init_fns[LLSD_ENC_COUNT] =
{
	&llsd_xml_serializer_init,
	&llsd_binary_serializer_init,
	&llsd_notation_serializer_init,
	&llsd_json_serializer_init
};

serializer_deinit_fn const deinit_fns[LLSD_ENC_COUNT] =
{
	&llsd_xml_serializer_deinit,
	&llsd_binary_serializer_deinit,
	&llsd_notation_serializer_deinit,
	&llsd_json_serializer_deinit,
};

int llsd_serialize_to_file( llsd_t * const llsd, FILE * fout, llsd_serializer_t const fmt, int const pretty )
{
	void * user_data = NULL;
	llsd_ops_t ops;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( fout, FALSE );
	CHECK_RET( IS_VALID_SERIALIZER( fmt ), FALSE );

	/* initialize the serializer */
	CHECK_PTR_RET( init_fns[fmt], FALSE );
	CHECK_RET( (*(init_fns[fmt]))( fout, &ops, pretty, &user_data ), FALSE );

	/* serialize out the llsd */
	CHECK_RET( llsd_serialize( llsd, fout, &ops, user_data ), FALSE );

	/* deinitializer the serializer */
	CHECK_RET( (*(deinit_fns[fmt]))( fout, user_data ), FALSE );

	return TRUE;
}

static int llsd_serialize( llsd_t * const llsd, FILE * fout, llsd_ops_t * const ops, void * user_data )
{
	int32_t i;
	double d;
	uint8_t * s;
	uint8_t	uuid[UUID_LEN];
	uint32_t len;
	llsd_t * k, * v;
	llsd_itr_t itr, end;
	CHECK_PTR_RET( llsd, FALSE );
	CHECK_PTR_RET( fout, FALSE );
	CHECK_PTR_RET( ops, FALSE );
	
	switch( llsd_get_type( llsd ) )
	{
		case LLSD_UNDEF:
			CHECK_PTR_RET( ops->undef_fn, FALSE );
			CHECK_RET( (*(ops->undef_fn))( user_data ), FALSE );
			break;

		case LLSD_BOOLEAN:
			CHECK_PTR_RET( ops->boolean_fn, FALSE );
			CHECK_RET( llsd_as_integer( llsd, &i ), FALSE );
			CHECK_RET( (*(ops->boolean_fn))( i, user_data ), FALSE );
			break;

		case LLSD_INTEGER:
			CHECK_PTR_RET( ops->integer_fn, FALSE );
			CHECK_RET( llsd_as_integer( llsd, &i ), FALSE );
			CHECK_RET( (*(ops->integer_fn))( i, user_data ), FALSE );
			break;

		case LLSD_REAL:
			CHECK_PTR_RET( ops->real_fn, FALSE );
			CHECK_RET( llsd_as_double( llsd, &d ), FALSE );
			CHECK_RET( (*(ops->real_fn))( d, user_data ), FALSE );
			break;

		case LLSD_DATE:
			CHECK_PTR_RET( ops->date_fn, FALSE );
			CHECK_RET( llsd_as_double( llsd, &d ), FALSE );
			CHECK_RET( (*(ops->date_fn))( d, user_data ), FALSE );
			break;

		case LLSD_UUID:
			CHECK_PTR_RET( ops->uuid_fn, FALSE );
			CHECK_RET( llsd_as_uuid( llsd, uuid ), FALSE );
			CHECK_RET( (*(ops->uuid_fn))( uuid, user_data ), FALSE );
			break;

		case LLSD_STRING:
			CHECK_PTR_RET( ops->string_fn, FALSE );
			CHECK_RET( llsd_as_string( llsd, &s ), FALSE );
			CHECK_RET( (*(ops->string_fn))( s, FALSE, user_data ), FALSE );
			break;

		case LLSD_URI:
			CHECK_PTR_RET( ops->uri_fn, FALSE );
			CHECK_RET( llsd_as_string( llsd, &s ), FALSE );
			CHECK_RET( (*(ops->uri_fn))( s, FALSE, user_data ), FALSE );
			break;

		case LLSD_BINARY:
			CHECK_PTR_RET( ops->binary_fn, FALSE );
			CHECK_RET( llsd_as_binary( llsd, &s, &len ), FALSE );
			CHECK_RET( (*(ops->binary_fn))( s, len, FALSE, user_data ), FALSE );
			break;

		case LLSD_ARRAY:
			CHECK_PTR_RET( ops->array_begin_fn, FALSE );
			CHECK_PTR_RET( ops->array_value_begin_fn, FALSE );
			CHECK_PTR_RET( ops->array_value_end_fn, FALSE );
			CHECK_PTR_RET( ops->array_end_fn, FALSE );

			/* begin array serialization */
			CHECK_RET( (*(ops->array_begin_fn))( llsd_get_count( llsd ), user_data ), FALSE );

			/* serialize all of the array members */
			len = llsd_get_count( llsd );
			end = llsd_itr_end( llsd );
			for ( itr = llsd_itr_begin( llsd ); !LLSD_ITR_EQ( itr, end ); itr = llsd_itr_next( llsd, itr ) )
			{
				CHECK_RET( llsd_get( llsd, itr, &v, &k ), FALSE );
				CHECK_PTR_RET( v, FALSE );

				/* call array value begin callback */
				CHECK_RET( (*(ops->array_value_begin_fn))( user_data ), FALSE );

				/* recurse for the array value */
				CHECK_RET( llsd_serialize( v, fout, ops, user_data ), FALSE );

				/* call array value end callback */
				CHECK_RET( (*(ops->array_value_end_fn))( user_data ), FALSE );

				len--;
			}

			/* end array serialization */
			CHECK_RET( (*(ops->array_end_fn))( llsd_get_count( llsd ), user_data ), FALSE );
			break;

		case LLSD_MAP:
			CHECK_PTR_RET( ops->map_begin_fn, FALSE );
			CHECK_PTR_RET( ops->map_key_begin_fn, FALSE );
			CHECK_PTR_RET( ops->map_key_end_fn, FALSE );
			CHECK_PTR_RET( ops->map_value_begin_fn, FALSE );
			CHECK_PTR_RET( ops->map_value_end_fn, FALSE );
			CHECK_PTR_RET( ops->map_end_fn, FALSE );

			/* begin map serialization */
			CHECK_RET( (*(ops->map_begin_fn))( llsd_get_count( llsd ), user_data ), FALSE );

			/* serialize all of the map members */
			len = llsd_get_count( llsd );
			end = llsd_itr_end( llsd );
			for( itr = llsd_itr_begin( llsd ); !LLSD_ITR_EQ( itr, end ); itr = llsd_itr_next( llsd, itr ) )
			{
				CHECK_RET( llsd_get( llsd, itr, &v, &k ), FALSE );
				CHECK_PTR_RET( k, FALSE );
				CHECK_PTR_RET( v, FALSE );
				
				/* call key begin callback */
				CHECK_RET( (*(ops->map_key_begin_fn))( user_data ), FALSE );

				/* recurse for the map key */
				CHECK_RET( llsd_serialize( k, fout, ops, user_data ), FALSE );

				/* call key end callback */
				CHECK_RET( (*(ops->map_key_end_fn))( user_data ), FALSE );

				/* call map value begin callback */
				CHECK_RET( (*(ops->map_value_begin_fn))( user_data ), FALSE );

				/* recurse for the map value */
				CHECK_RET( llsd_serialize( v, fout, ops, user_data ), FALSE );

				/* call map value end callback */
				CHECK_RET( (*(ops->map_value_end_fn))( user_data ), FALSE );

				len--;
			}

			/* end map serialization */
			CHECK_RET( (*(ops->map_end_fn))( llsd_get_count( llsd ), user_data ), FALSE );

			break;
	}
}
