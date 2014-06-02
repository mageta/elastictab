/*
 * elastictab: a simple implementation of elastic tabstops in C
 * Copyright (C) 2014  Benjamin Block (bebl@mageta.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "stdio.h"
#include "errno.h"

#include "elastictab.h"

int
main()
{
	struct elastic_print ep;

	elastic_print_create(&ep, 3, 8);

	elastic_print_add_printf(&ep, "aaaaaaaaa\taaa\taaaaaaaaa");
	elastic_print_add_printf(&ep, "bbbb\tbbbbbbbbb\tbbb");
	elastic_print_add_printf(&ep, "cccccccccc\tcc\tcccccccccc\tcc");
	elastic_print_add_printf(&ep, "\t\tccccccc");
	elastic_print_add_printf(&ep, "abc\tabc\tabc\n\t\tccccccc");

	printf("%zd, %zd, [%zd, %zd, %zd], %zd\n", ep.columns, ep.lines_count,
	       ep.column_widths[0], ep.column_widths[1], ep.column_widths[2],
	       ep.column_widths_min);

	elastic_print_fput(&ep, stdout);

	elastic_print_destory(&ep);

	return 0;
}
