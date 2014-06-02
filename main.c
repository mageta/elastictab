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
#include "stdlib.h"
#include "errno.h"
#include "assert.h"
#include "string.h"

#include "elastictab.h"

#define __test_exec_and_expr(rc, command, test_expr, fail_label)               \
	{                                                                      \
		rc = command;                                                  \
		if (!(test_expr)) {                                            \
			fprintf(stderr,                                        \
				"test_exec @ <%s:%s:%d> failed with [%d]\n",   \
				__FILE__, __func__, __LINE__, rc);             \
			goto fail_label;                                       \
		}                                                              \
	}
#define __test_exec_and_rc0(rc, command, fail_label)                           \
	{                                                                      \
		__test_exec_and_expr(rc, command, (rc) == 0, fail_label);      \
	}

int test_basic()
{
	int rc = 0;
#define __test_basic_buffer_length	(1 << 10)
	char test_buffer[__test_basic_buffer_length];
	char test_buffer_check[__test_basic_buffer_length] =
	    "aaaaaaaaa  aaa       aaaaaaaaa  \n"
	    "bbbb       bbbbbbbbb bbb        \n"
	    "cccccccccc cc        cccccccccc cc\n"
	    "                     ccccccc    \n"
	    "abc        abc       abc\n"
	    "                     abcabca    abcabc\tabcabc\n\0";
	size_t test_buffer_check_strlen = strlen(test_buffer_check);

	struct elastic_print ep;

	__test_exec_and_rc0(rc, elastic_print_create(&ep, 3, 8), err);

	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"aaaaaaaaa\taaa\taaaaaaaaa"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"bbbb\tbbbbbbbbb\tbbb"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"cccccccccc\tcc\tcccccccccc\tcc"),
			    err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"\t\tccccccc"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"abc\tabc\tabc\n\t\tabcabca\tabcabc\tabcabc"), 
			    err_destroy_ep);

	fprintf(stdout, "%zd, %zd, [%zd, %zd, %zd], %zd\n", ep.columns,
		ep.lines_count, ep.column_widths[0], ep.column_widths[1],
		ep.column_widths[2], ep.column_widths_min);

	__test_exec_and_expr(
	    rc,
	    elastic_print_snput(&ep, test_buffer, __test_basic_buffer_length),
	    rc == (int)test_buffer_check_strlen, err_destroy_ep);

	__test_exec_and_rc0(rc, strcmp(test_buffer, test_buffer_check),
			    err_destroy_ep);

	elastic_print_destory(&ep);

	fputs(test_buffer, stdout);

/* out: */
	assert(rc == 0);
	return rc;
err_destroy_ep:
	elastic_print_destory(&ep);
err:
	assert(rc != 0);
	return rc;
}

int test_zero_columns()
{
	int rc = 0;
#define __test_basic_buffer_length	(1 << 10)
	char test_buffer[__test_basic_buffer_length];
	char test_buffer_check[__test_basic_buffer_length] =
	    "aaaaaaaaa\taaa\taaaaaaaaa\n"
	    "bbbb\tbbbbbbbbb\tbbb\n"
	    "cccccccccc\tcc\tcccccccccc\tcc\n"
	    "\t\tccccccc\n"
	    "abc\tabc\tabc\n\t\tabcabca\n\0";
	size_t test_buffer_check_strlen = strlen(test_buffer_check);

	struct elastic_print ep;

	__test_exec_and_rc0(rc, elastic_print_create(&ep, 0, 8), err);

	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"aaaaaaaaa\taaa\taaaaaaaaa"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"bbbb\tbbbbbbbbb\tbbb"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"cccccccccc\tcc\tcccccccccc\tcc"),
			    err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"\t\tccccccc"), err_destroy_ep);
	__test_exec_and_rc0(rc, elastic_print_add_printf(&ep,
				"abc\tabc\tabc\n\t\tabcabca"), err_destroy_ep);

	fprintf(stdout, "%zd, %zd, %zd\n", ep.columns, ep.lines_count,
		ep.column_widths_min);

	__test_exec_and_expr(
	    rc,
	    elastic_print_snput(&ep, test_buffer, __test_basic_buffer_length),
	    rc == (int)test_buffer_check_strlen, err_destroy_ep);

	__test_exec_and_rc0(rc, strcmp(test_buffer, test_buffer_check),
			    err_destroy_ep);

	elastic_print_destory(&ep);

	fputs(test_buffer, stdout);

/* out: */
	assert(rc == 0);
	return rc;
err_destroy_ep:
	elastic_print_destory(&ep);
err:
	assert(rc != 0);
	return rc;
}

int
main()
{
	int rc = 0;

	fprintf(stdout, "running test 'test_basic()' .. \n");
	__test_exec_and_rc0(rc, test_basic(), err);

	fputs("\n", stdout);

	fprintf(stdout, "running test 'test_zero_columns()' .. \n");
	__test_exec_and_rc0(rc, test_zero_columns(), err);

	return EXIT_SUCCESS;
err:
	return EXIT_FAILURE;
}
