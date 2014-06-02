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

#include "elastictab.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"
#include "assert.h"
#include "stdint.h"
#include "ctype.h"
#include "stdarg.h"

int elastic_print_create(struct elastic_print *eprint, size_t columns,
			 size_t column_widths_min)
{
	int rc;
	size_t i;

	if ((eprint == NULL) || (column_widths_min < 1)) {
		rc = EINVAL;
		goto err;
	}

	memset(eprint, 0, sizeof(*eprint));

	eprint->columns = columns;
	eprint->lines_count = 0;
	eprint->column_widths_min = column_widths_min;

	if (eprint->columns > 0) {
		eprint->column_widths =
		    calloc(eprint->columns, sizeof(*eprint->column_widths));
		if (eprint->column_widths == NULL) {
			rc = ENOMEM;
			goto err;
		}
	} else {
		eprint->column_widths = NULL;
	}

	for (i = 0; i < eprint->columns; i++) {
		eprint->column_widths[i] = eprint->column_widths_min;
	}

	rc = 0;
err:
	return rc;
}

void elastic_print_destory(struct elastic_print *eprint)
{
	size_t i;

	if (eprint == NULL) {
		return;
	}

	if (eprint->column_widths != NULL) {
		free(eprint->column_widths);
		eprint->column_widths = NULL;
	}

	if (eprint->lines != NULL) {
		for (i = 0; i < eprint->lines_count; i++) {
			if (eprint->lines[i] != NULL) {
				free(eprint->lines[i]);
				eprint->lines[i] = NULL;
			}
		}

		free(eprint->lines);
		eprint->lines = NULL;
	}

	memset(eprint, 0, sizeof(*eprint));
}

int elastic_print_add_line(struct elastic_print *eprint, char *line,
			   size_t length)
{
	int rc = 0;

	char **last_line, *cur, c;
	size_t column_i, column_len, i;

	if (eprint == NULL) {
		rc = EINVAL;
		goto err;
	}
	if ((line == NULL) || (length == 0) || (line[0] == '\0')) {
		goto out;
	}

	eprint->lines_count += 1;
	eprint->lines = realloc(eprint->lines,
				(eprint->lines_count) * sizeof(*eprint->lines));
	if (eprint->lines == NULL) {
		rc = ENOMEM;
		goto err_reduce_line;
	}

	last_line = &(eprint->lines[eprint->lines_count - 1]);
	*last_line = malloc((length + 3) * sizeof(**last_line));
	if (*last_line == NULL) {
		rc = ENOMEM;
		goto err_reduce_line;
	}
	/* safety in case someone forgets to account for the 0-terminator in
	 * `line` and to add a ending '\t' to the line. */
	(*last_line)[length] = (*last_line)[length + 1] =
	    (*last_line)[length + 2] = '\0';

	strncpy(*last_line, line, length);

	column_i = 0;
	column_len = 0;
	i = 0;

	cur = *last_line;

#define __set_max_cw(i, len)                                                   \
	{                                                                      \
		if ((i) < eprint->columns) {                                   \
			if (eprint->column_widths[(i)] < (len)) {              \
				eprint->column_widths[(i)] = (len);            \
			}                                                      \
		}                                                              \
	}

	while ((i < length) && (*cur != '\0')) {
		switch (*cur) {
		case '\n':
			c = '\r';

			goto newline;
			break;
		case '\r':
			c = '\n';
newline:
			*cur = '\0';

			if ((i + 1) < length) {
				cur++;
				i++;

				if (*cur != c) {
					cur--;
					i--;
				} else {
					*cur = '\0';
				}
			}

			__set_max_cw(column_i, column_len);

			rc = elastic_print_add_line(eprint, cur + 1,
						    length - (i + 1));
			goto err;

			break;
		case '\t':
			__set_max_cw(column_i, column_len + 1);

			column_len = 0;
			column_i += 1;

			break;
		default:
			if (isprint(*cur)) {
				column_len += 1;
			} else if ((isspace(*cur) && (!isblank(*cur))) ||
				   (*cur == 127)) {
				*cur = ' ';
				column_len += 1;
			}
			break;
		}

		i += 1;
		cur++;
	}

	assert(i <= length);

	if (column_i < eprint->columns) {
		/* still in a valid column */
		assert(i < (length + 2));
		*cur = '\t';

		column_len += 1;
		__set_max_cw(column_i, column_len);

		i += 1;
		cur++;
		assert(i < (length + 2));

		*cur = '\0';
	}

out:
	rc = 0;
	goto err;
err_reduce_line:
	eprint->lines_count -= 1;
err:
	return rc;
}

int elastic_print_add_string(struct elastic_print *eprint, char *str)
{
	if (str == NULL) {
		return 0;
	}

	return elastic_print_add_line(eprint, str, strlen(str) + 1);
}

int elastic_print_add_printf(struct elastic_print *eprint, const char *fmt, ...)
{
#define __startlen	(1 << 8)
	int rc = 0;
	char * buffer;
	size_t buffer_len = __startlen;
	int written;

	va_list arguments;

	buffer = calloc(buffer_len + 1, sizeof(* buffer));
	if (buffer == NULL) {
		rc = ENOMEM;
		goto err;
	}

	buffer[buffer_len] = '\0';

	va_start(arguments, fmt);

	written = vsnprintf(buffer, buffer_len, fmt, arguments);
	if (written < 0) {
		rc = EFAULT;
		goto err_va_end;
	} else if ((unsigned int) written >= buffer_len) {
		buffer_len = (unsigned int) written + 1;

		buffer = realloc(buffer, (buffer_len + 1) * sizeof(*buffer));
		if (buffer == NULL) {
			rc = ENOMEM;
			goto err_va_end;
		}

		buffer[buffer_len] = '\0';

		written = vsnprintf(buffer, buffer_len, fmt, arguments);
		if ((written < 0) || ((unsigned int) written >= buffer_len)) {
			rc = EFAULT;
			goto err_va_end;
		}
	}

	rc = elastic_print_add_string(eprint, buffer);
	if (rc != 0) {
		goto err_va_end;
	}

/* out: */
	rc = 0;
err_va_end:
	va_end(arguments);
/* err_free_buffer: */
	free(buffer);
err:
	return rc;
}

int elastic_print_snput(struct elastic_print *eprint, char *buffer,
			size_t buffer_len)
{
	int rc;
	size_t line, llength, bleft, written;
	char *cur_buf;

	if ((eprint == NULL) || (buffer == NULL) || (buffer_len < 1)) {
		rc = -EINVAL;
		goto err;
	}

	buffer[0] = '\0';
	buffer[buffer_len - 1] = '\0';

	written = 0;
	cur_buf = &(buffer[0]);
	bleft = buffer_len;

#define __dec_left(b)                                                          \
	{                                                                      \
		if ((b) == 0) {                                                \
			rc = -EFAULT;                                          \
			goto err_terminate;                                    \
		}                                                              \
                                                                               \
		(b) -= 1;                                                      \
		if ((b) <= 1) {                                                \
			rc = -ENOMEM;                                          \
			goto err_terminate;                                    \
		}                                                              \
	}
#define __append_char(buf, c, left, written)                                   \
	{                                                                      \
		if ((left) < 2) {                                              \
			rc = -EFAULT;                                          \
			goto err_terminate;                                    \
		}                                                              \
                                                                               \
		*(buf) = (c);                                                  \
		(buf) += 1;                                                    \
		(written) += 1;                                                \
		*(buf) = '\0';                                                 \
	}

	for (line = 0; line < eprint->lines_count; line += 1) {
		char *cur_line = eprint->lines[line];
		char *cur_c = cur_line;
		size_t column = 0;
		size_t column_s = 0;

		llength = strlen(cur_line);
		if ((llength + 2) > bleft) {
			rc = -ENOMEM;
			goto err_terminate;
		}

		while (*cur_c != '\0') {
			if (bleft <= 1) {
				rc = -ENOMEM;
				goto err_terminate;
			}

			switch (*cur_c) {
			case '\t':
				if (column >= eprint->columns) {
					goto append_normal;
				}

				do {
					__append_char(cur_buf, ' ', bleft,
						      written);
					__dec_left(bleft);

					column_s++;
				} while (column_s <
					 eprint->column_widths[column]);

				column += 1;
				column_s = 0;

				break;
			default:
append_normal:
				__append_char(cur_buf, *cur_c, bleft, written);
				__dec_left(bleft);

				if (isprint(*cur_c) || isblank(*cur_c)) {
					column_s++;
					assert((!(column < eprint->columns)) ||
					       (column_s <
						eprint->column_widths[column]));
				}
				break;
			}

			cur_c++;
		}

		__append_char(cur_buf, '\n', bleft, written);
		__dec_left(bleft);
	}

	/* out: */
	rc = written;
err_terminate:
	buffer[buffer_len - 1] = '\0';
err:
	return rc;
}

int elastic_print_fput(struct elastic_print * eprint, FILE * stream)
{
	const size_t START_LENGTH = ((1 << 8) + 1);
	const float INCREMENT = 1.5;

	int rc;

	char * buffer = NULL;
	size_t buffer_len = START_LENGTH;

	assert((START_LENGTH > 0) && (INCREMENT >= 1));

	if ((eprint == NULL) || (stream == NULL)) {
		rc = EINVAL;
		goto err;
	}

	do {
		buffer = realloc(buffer, (buffer_len + 1) * sizeof(*buffer));
		if (buffer == NULL) {
			rc = ENOMEM;
			goto err;
		}
		buffer[buffer_len] = '\0';

		rc = elastic_print_snput(eprint, buffer, buffer_len);
		if ((rc < 0) && (rc != -ENOMEM)) {
			rc = -rc ;
			goto err_free_buffer;
		} else if (rc == -ENOMEM) {
			buffer_len =
			    (((size_t)(buffer_len * INCREMENT)) > buffer_len ?
				 buffer_len * INCREMENT :
				 buffer_len + 1);
		}
	} while (rc == -ENOMEM);

	/* either nonnegative or EOF on error */
	rc = fputs(buffer, stream);
	if (rc >= 0) {
		/* success */
		rc = 0;
	} else if (rc != EOF) {
		/* invalid according to fputs(3) */
		assert(0);
		rc = EOF;
	}

err_free_buffer:
	free(buffer);
err:
	return rc;
}
