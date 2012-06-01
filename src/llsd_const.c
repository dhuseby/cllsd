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

#include <cutil/macros.h>

#include "llsd.h"
#include "llsd_const.h"

/* constants */
llsd_t const undefined =
{
	.type_ = LLSD_UNDEF
};

llsd_int_t const zero_int =
{
	.v = 0,
	.be = 0
};

llsd_int_t const one_int =
{
	.v = 1,
	.be = 0
};

llsd_real_t const zero_real =
{
	.v = 0.0,
	.be = 0
};

llsd_real_t const one_real =
{
	.v = 1.0,
	.be = 0
};

static uint8_t bits[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
llsd_uuid_t const zero_uuid = 
{ 
	.dyn_bits = FALSE,
	.dyn_str = FALSE,
	.bits = bits,
	.str = NULL
};

llsd_string_t const false_string = 
{
	.dyn_str = FALSE,
	.dyn_esc = FALSE,
	.key_esc = FALSE,
	.str_len = 5,
	.str = "false",
	.esc_len = 0,
	.esc = NULL
};
llsd_string_t const true_string = 
{
	.dyn_str = FALSE,
	.dyn_esc = FALSE,
	.key_esc = FALSE,
	.str_len = 4,
	.str = "true",
	.esc_len = 0,
	.esc = NULL
};
llsd_string_t const empty_string = 
{
	.dyn_str = FALSE,
	.dyn_esc = FALSE,
	.key_esc = FALSE,
	.str_len = 0,
	.str = NULL,
	.esc_len = 0,
	.esc = NULL
};

static uint8_t zero_data [] = { '0' };
llsd_binary_t const false_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 1,
	.data = zero_data,
	.enc_size = 0,
	.enc = NULL
};
static uint8_t one_data[] = { '1' };
llsd_binary_t const true_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 1,
	.data = one_data,
	.enc_size = 0,
	.enc = NULL
};
llsd_binary_t const empty_binary =
{
	.dyn_data = FALSE,
	.dyn_enc = FALSE,
	.encoding = LLSD_NONE,
	.data_size = 0,
	.data = NULL,
	.enc_size = 0,
	.enc = NULL
};
llsd_uri_t const empty_uri = 
{
	.dyn_uri = FALSE,
	.dyn_esc = FALSE,
	.uri_len = 0,
	.uri = "",
	.esc_len = 0,
	.esc = NULL
};
llsd_date_t const empty_date =
{
	.use_dval = TRUE,
	.dyn_str = FALSE,
	.len = 0,
	.dval = 0.0,
	.str = NULL
};
llsd_array_t const empty_array =
{
	.array = {
		.pfn = NULL,
		.num_nodes = 0,
		.buffer_size = 0,
		.data_head = -1,
		.free_head = -1,
		.node_buffer = NULL
	}
};
llsd_map_t const empty_map =
{
	.ht = {
		.khfn = NULL,
		.kefn = NULL,
		.kdfn = NULL,
		.vdfn = NULL,
		.prime_index = 0,
		.num_tuples = 0,
		.initial_capacity = 0,
		.load_factor = 0.0f,
		.tuples = NULL
	}
};

