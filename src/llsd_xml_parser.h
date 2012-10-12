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

#ifndef LLSD_XML_PARSER_H
#define LLSD_XML_PARSER_H

#include "llsd.h"
#include "llsd_parser.h"

int llsd_xml_check_sig_file( FILE * fin );
int llsd_xml_parse_file( FILE * fin, llsd_ops_t * const ops, void * const user_data );

#endif/*LLSD_XML_PARSER_H*/

