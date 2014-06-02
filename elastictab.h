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

#ifndef __ELASTICTAB_H
#define __ELASTICTAB_H

#include "stdio.h"
#include "errno.h"
#include "stdarg.h"
#include "stdint.h"

/*
 * Very simple implementation of elastic tabstops in C. More about the theory
 * can be found at [http://nickgravgaard.com/elastictabstops/](nickgravgaard's
 * elastictabstops).
 *
 * This implementation is useful for stuff like printing help-pages or simple
 * tables. Not so much for interactive environments, as it currently is not
 * dynamic. A instance is initializes to consider a certain amount of columns
 * to be elastic and this number can not be changed later. You would have to
 * create multiple instances to change to columns or support more than one
 * column-set (lines with different lengths of the elastic columns).
 *
 * A simple example could look like this:
 *
 * > struct elastic_print ep;
 * >
 * > elastic_print_create(&ep, 3, 8);
 * >
 * > elastic_print_add_printf(&ep, "aaaaaaaaa\taaa\taaaaaaaaa");
 * > elastic_print_add_printf(&ep, "bbbb\tbbbbbbbbb\tbbb");
 * > elastic_print_add_printf(&ep, "cccccccccc\tcc\tcccccccccc\tcc");
 * > elastic_print_add_printf(&ep, "\t\tccccccc");
 * > elastic_print_add_printf(&ep, "abc\tabc\tabc\n\t\tccccccc");
 * >
 * > elastic_print_fput(&ep, stdout);
 * >
 * > elastic_print_destory(&ep);
 *
 * The result would be:
 *
 * > aaaaaaaaa  aaa       aaaaaaaaa
 * > bbbb       bbbbbbbbb bbb
 * > cccccccccc cc        cccccccccc cc
 * >                      ccccccc
 * > abc        abc       abc
 * >                      ccccccc
 */

/** represents one elastictab instance
 *
 * Initialised with `elastic_print_create()` it can be populated with
 * `elastic_print_add_*()` and finally be printed with `elastic_print_*put()`.
 * Every space that is allocated in this process can be freed with a call of
 * `elastic_print_destory()`.
 *
 * The instance MUST not be modified after its creation but by the functions in
 * this header.
 *
 * A "line" is a string finished by a newline (\n). A "column" is a part of a
 * line finished by a tab (\t).
 */
struct elastic_print
{
	/** how many columns should be accounted for in this instance
	 *
	 * only this number of columns/tabs will be elastic in the final
	 * `put()`. The lines can contain more tabs, but those won't be
	 * elastic.
	 *
	 * In case columns is 0, there will be no column-processing done.
	 */
	size_t		columns;

	/** added/processed lines in this instance */
	char **		lines;
	/** count of those `lines` */
	size_t		lines_count;

	/** array of `columns` elements that safe the elastic width of each
	 * column
	 */
	size_t *	column_widths;
	/** minimum width of each column (> 0) */
	size_t		column_widths_min;
};

/** initializes a `struct elastic_print`
 *
 * \para eprint		elastictab instance to be initialised
 * \para columns	number of columns considered in each `add_*()`-call
 * \para column_widths_min
 *			minimum length of each column in this instance (> 0)
 *
 * \returns EINVAL	in case any parameter is wrong
 * \returns ENOMEM	in case a field could not be allocated
 * \returns 0		if everything went OK
 */
int elastic_print_create(struct elastic_print *eprint, size_t columns,
			 size_t column_widths_min);

/** destroys a elastictab-instance and frees all memory
 *
 * \para eprint	instance that shall be destroyed
 */
void elastic_print_destory(struct elastic_print *eprint);

/** adds/processes a line to the given elastictab-instance
 *
 * \para eprint		current elastictab instance
 * \para line		character-string that shall be added to the instance
 * \para length		length in characters in this line
 *
 * \returns EINVAL	in case a parameter is considered wrong
 * \returns ENOMEM	if not enough memory could be allocated to store the new
 *			line
 * \returns 0		in case everything went OK
 *
 * The `line` can be 0-terminated, but doesn't has to be. It can contain
 * multiple by newline separated lines that will be handled as if this call
 * would have been made for each of them individually.
 *
 * During this function-call every found line is processed and searched for
 * tab-characters. Each tab is considered as finish of one column. If the
 * length of this column is longer then the length of the same column in the
 * previous added lines (and the min) it is stored in `column_widths`.
 */
int elastic_print_add_line(struct elastic_print *eprint, char *line,
			   size_t length);

/** same as `elastic_print_add_line()` but with a 0-terminated string
 *
 * \para str	string that shall be added. MUST be 0-terminated.
 *
 * Everything else is the same as with `elastic_print_add_line()`.
 */
int elastic_print_add_string(struct elastic_print *eprint, char *str);

/** adds/processes a printf-like formated line to the given elastictab-instance
 *
 * \para eprint		current elastictab instance
 * \para fmt		the format-string. This is subsequently given to printf
 *			and thus should follow its syntax/semantics (printf(3))
 * \para ...		arguments for the format-string
 *
 * \returns EFAULT	if the subsequent syscalls failed
 * \returns `elastic_print_add_line()`
 *			that can be returned by a call to
 *			`elastic_print_add_line()`
 * \returns 0		in case everything went OK
 *
 * For the syntax/semantics of `fmt` please see the printf-manpage (printf(3))
 */
int elastic_print_add_printf(struct elastic_print *eprint, const char *fmt, ...);

/** prints the given elastictab-instance into the given buffer (0-terminated)
 *
 * \para eprint		current elastictab instance
 * \para buffer		a buffer that can store `buffer_len` characters
 * \para buffer_len	the maximum length of the buffer (including the
 *			0-terminator)
 *
 * \returns -EINVAL	in case a parameter is considered wrong
 * \returns -EFAULT	if the algorithm failed unexpected (data-corruption,
 *			-manipulation, etc.)
 * \returns -ENOMEM	if the buffer is too small to hold everything including
 *			the 0-terminator
 * \returns >= 0	in case everything went OK, it returns ne number of
 *			written characters (excluding the 0-terminator)
 *
 * Prints all added lines into the buffer and expands each considered column to
 * the larges size found while processing all the lines (or the given minimum).
 *
 * The buffer will be 0-terminated even if the call returns EFAULT or ENOMEM,
 * but it will be incomplete.
 */
int elastic_print_snput(struct elastic_print *eprint, char *buffer,
			size_t buffer_len);

/** prints the given elastictab-instance into the given stream
 *
 * \para eprint         current elastictab instance
 * \para steam		the target stream that shall be used
 *
 * \returns -`elastic_print_snput()`
 *			error-values that are reported by `elastic_print_snput()`
 *			are returned as their inverse (-ENOMEM -> ENOMEM)
 * \returns EOF		in case the subsequent `fputs` fails to print everything
 *			into the stream
 * \returns 0		in case everything went OK
 *
 * The lines will be processed in the same way as if you would call
 * `elastic_print_snput()`.
 */
int elastic_print_fput(struct elastic_print *eprint, FILE *stream);

#endif /* __ELASTICTAB_H */
