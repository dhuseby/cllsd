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

llsd_t * llsd_parse_binary( FILE * fin )
{
}

llsd_t * llsd_parse_notation( FILE * fin )
{
}

llsd_t * llsd_parse_xml( FILE * fin )
{
}

#define SIG_LEN (18)
llsd_t * llsd_parse( FILE *fin )
{
	uint8_t sig[18];

	CHECK_RET_MSG( fin, NULL, "invalid file pointer\n" );

	fread( sig, sizeof(uint8_t), SIG_LEN, fin );
	if ( strncmp( sig, "<? LLSD/Binary ?>\n", SIG_LEN ) == 0 )
	{
		rewind( fin );
		return llsd_parse_binary( fin );
	}
	else if ( strncmp( sig, "<?llsd/notation?>\n", SIG_LEN ) == 0 )
	{
		rewind( fin );
		return llsd_parse_notation( fin );
	}
	else
	{
		rewind( fin );
		return llsd_parse_xml( fin );
	}
}

void llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout )
{
	CHECK_PTR( llsd );
	CHECK_PTR( fout );

	switch ( fmt )
	{
		case LLSD_XML:
			return llsd_format_xml( llsd, fout );
		case LLSD_NOTATION:
			return llsd_format_notation( llsd, fout );
		case LLSD_BINARY:
			return llsd_format_binary( llsd, fout );
	}
}

