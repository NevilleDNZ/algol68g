/*!
\file diagnostics.c
\brief error handling routines
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "algol68g.h"
#include "genie.h"

int error_count, warning_count, run_time_error_count;

/*!
\brief give an intelligible error and exit
\param u
\param txt
**/

void scan_error (SOURCE_LINE_T * u, char *txt)
{
  char *fn = (u == NULL ? A68G_NAME : u->filename);
  if (gnu_diags) {
    sprintf (output_line, "%s: error: %s", fn, txt);
  } else if (a68_prog.files.source.name != NULL && strcmp (a68_prog.files.source.name, fn) != 0) {
    sprintf (output_line, "%s: error: %s", fn, txt);
  } else {
    sprintf (output_line, "error: %s", txt);
  }
  io_close_tty_line ();
  io_write_string (STDOUT_FILENO, output_line);
  if (u != NULL && u->number > 0) {
    sprintf (output_line, " in line %d", u->number);
    io_write_string (STDOUT_FILENO, output_line);
  }
  if (errno != 0) {
    sprintf (output_line, " (%s)", strerror (errno));
    io_write_string (STDOUT_FILENO, output_line);
  }
  a68g_exit (EXIT_FAILURE);
}

/*!
\brief first token at valid line
\param p position in syntax tree, should not be NULL
\return first token at valid line
**/

static NODE_T *first_valid_token (NODE_T * p)
{
  for (; p != NULL; FORWARD (p)) {
    if (LINE (p)->number > 0) {
      return (p);
    } else {
      NODE_T *q = first_valid_token (SUB (p));
      if (q != NULL) {
	return (q);
      }
    }
  }
  return (NULL);
}


/*!
\brief give a diagnostic message
\param sev severity
\param p position in syntax tree, should not be NULL
\param str message string
\param ... various arguments needed by special symbols in str
**/

void diagnostic (int sev, NODE_T * p, char *str, ...)
{
/*
Legend for special symbols:
# skip extra syntactical information
@ node = non terminal
A att = non terminal
B kw = keyword
C context
D argument in decimal
E string literal from errno
H char argument
K int argument as 'k', 'M' or 'G'
L line number
M moid - if error mode return without giving a message
N mode - MODE (NIL)
O moid - operand
S symbol
X expected attribute
Z string literal. 
*/
  va_list args;
  MESSAGE_T *msg = (MESSAGE_T *) get_heap_space (SIZE_OF (MESSAGE_T));
  MOID_T *moid = NULL;
  char *t = str, st[BUFFER_SIZE], a[BUFFER_SIZE], b[BUFFER_SIZE], *q = b;
  BOOL_T force, extra_syntax = A_TRUE, shorted = A_FALSE;
  int k;
  va_start (args, str);
  q[0] = '\0';
  force = (sev & FORCE_DIAGNOSTIC) != 0;
  sev &= ~FORCE_DIAGNOSTIC;
/* No warnings? */
  if (p != NULL && p->info != NULL && p->info->module != NULL) {
    if (!force && sev == A_WARNING && p->info->module->options.no_warnings) {
      return;
    }
  }
/* Suppressed? */
  if (sev == A_ERROR || sev == A_SYNTAX_ERROR) {
    if (error_count == MAX_ERRORS) {
      strcpy (b, "further error diagnostics suppressed");
      sev = A_ERROR;
      shorted = A_TRUE;
      goto construct_diagnostic;
    } else if (error_count > MAX_ERRORS) {
      error_count++;
      return;
    }
  } else if (sev == A_WARNING) {
    if (warning_count == MAX_ERRORS) {
      strcpy (b, "further warning diagnostics suppressed");
      shorted = A_TRUE;
      goto construct_diagnostic;
    } else if (warning_count > MAX_ERRORS) {
      warning_count++;
      return;
    }

  }
/* Synthesize diagnostic message. */
  while (t[0] != '\0') {
    if (t[0] == '#') {
      extra_syntax = A_FALSE;
    } else if (t[0] == '@') {
      char *nt = non_terminal_string (edit_line, ATTRIBUTE (p));
      if (t != NULL) {
	strcat (q, nt);
      } else {
	strcat (q, "construct");
      }
    } else if (t[0] == 'A') {
      int att = va_arg (args, int);
      char *nt = non_terminal_string (edit_line, att);
      if (nt != NULL) {
	strcat (q, nt);
      } else {
	strcat (q, "construct");
      }
    } else if (t[0] == 'B') {
      int att = va_arg (args, int);
      KEYWORD_T *nt = find_keyword_from_attribute (top_keyword, att);
      if (nt != NULL) {
	strcat (q, "`");
	strcat (q, nt->text);
	strcat (q, "'");
      } else {
	strcat (q, "keyword");
      }
    } else if (t[0] == 'C') {
      int att = va_arg (args, int);
      if (att == NO_SORT) {
	strcat (q, "this");
      }
      if (att == SOFT) {
	strcat (q, "a soft");
      } else if (att == WEAK) {
	strcat (q, "a weak");
      } else if (att == MEEK) {
	strcat (q, "a meek");
      } else if (att == FIRM) {
	strcat (q, "a firm");
      } else if (att == STRONG) {
	strcat (q, "a strong");
      }
    } else if (t[0] == 'D') {
      int a = va_arg (args, int);
      char d[BUFFER_SIZE];
      sprintf (d, "%d", a);
      strncat (q, d, BUFFER_SIZE);
    } else if (t[0] == 'E') {
      char *str = va_arg (args, char *);
      char *y = new_string (str);
      strcat (q, "(");
      if (strlen (y) > 0) {
	y[0] = TO_LOWER (y[0]);
      }
      strcat (q, y);
      strcat (q, ")");
    } else if (t[0] == 'H') {
      char *a = va_arg (args, char *);
      char d[4];
      sprintf (d, "'%c'", a[0]);
      strcat (q, d);
    } else if (t[0] == 'L') {
      SOURCE_LINE_T *a = va_arg (args, SOURCE_LINE_T *);
      char d[32];
      ABNORMAL_END (a == NULL, "NULL source line in error", NULL);
      if (a->number > 0) {
	sprintf (d, "(in line %d)", a->number);
	strcat (q, d);
      }
    } else if (t[0] == 'M') {
      moid = va_arg (args, MOID_T *);
      if (moid == MODE (ERROR)) {
	return;
      } else if (moid == NULL) {
	strcat (q, "\"NULL\"");
      } else if (WHETHER (moid, SERIES_MODE)) {
	if (PACK (moid) != NULL && NEXT (PACK (moid)) == NULL) {
	  strcat (q, moid_to_string (MOID (PACK (moid)), 24));
	} else {
	  strcat (q, moid_to_string (moid, 24));
	}
      } else {
	strcat (q, moid_to_string (moid, 24));
      }
    } else if (t[0] == 'N') {
      strcat (q, "NIL value of mode ");
      moid = va_arg (args, MOID_T *);
      if (moid != NULL) {
	strcat (q, moid_to_string (moid, 24));
      }
    } else if (t[0] == 'O') {
      moid = va_arg (args, MOID_T *);
      if (moid == MODE (ERROR)) {
	return;
      } else if (moid == NULL) {
	strcat (q, "\"NULL\"");
      } else if (moid == MODE (VOID)) {
	strcat (q, "UNION (VOID, ..)");
      } else if (WHETHER (moid, SERIES_MODE)) {
	if (PACK (moid) != NULL && NEXT (PACK (moid)) == NULL) {
	  strcat (q, moid_to_string (MOID (PACK (moid)), 24));
	} else {
	  strcat (q, moid_to_string (moid, 24));
	}
      } else {
	strcat (q, moid_to_string (moid, 24));
      }
    } else if (t[0] == 'S') {
      if (p != NULL && SYMBOL (p) != NULL) {
	strcat (q, "`");
	strncat (q, SYMBOL (p), 64);
	strcat (q, "'");
      } else {
	strcat (q, "symbol");
      }
    } else if (t[0] == 'X') {
      int att = va_arg (args, int);
      char z[BUFFER_SIZE];
      sprintf (z, "`%s'", find_keyword_from_attribute (top_keyword, att)->text);
      strcat (q, new_string (z));
    } else if (t[0] == 'Y') {
      char *str = va_arg (args, char *);
      strcat (q, str);
    } else if (t[0] == 'Z') {
      char *str = va_arg (args, char *);
      strcat (q, "`");
      strcat (q, str);
      strcat (q, "'");
    } else {
      *(q++) = *t;
      *q = '\0';
    }
/* Forward pointer to end of string. */
    while (*q != '\0') {
      q++;
    }
/* Forward pointer in message string. */
    t++;
  }
/* Add information from errno, if any. */
  if (errno != 0) {
    char *str = new_string (ERROR_SPECIFICATION);
    if (str != NULL) {
      strcat (q, " (");
      if (strlen (str) > 0) {
	str[0] = TO_LOWER (str[0]);
      }
      strcat (q, str);
      strcat (q, ")");
    }
  }
/* Construct a diagnostic message. */
construct_diagnostic:
  switch (sev) {
  case A_ERROR:
    {
      error_count++;
      strcpy (st, "error");
      break;
    }
  case A_SYNTAX_ERROR:
    {
      error_count++;
      strcpy (st, "syntax error");
      break;
    }
  case A_RUNTIME_ERROR:
    {
      error_count++;
      strcpy (st, "runtime error");
      break;
    }
  case A_WARNING:
    {
      warning_count++;
      strcpy (st, "warning");
      break;
    }
  }
  if (p == NULL) {
/* Print diagnostic and choose GNU style or non-GNU style. */
    MODULE_T *m;
    if (shorted) {
      m = &a68_prog;
    } else {
      m = va_arg (args, MODULE_T *);
    }
    if (gnu_diags) {
      if (m != NULL && m->files.generic_name != NULL) {
	sprintf (output_line, "%s: %s: %s", m->files.generic_name, st, b);
      } else {
	sprintf (output_line, "%s: %s: %s", A68G_NAME, st, b);
      }
    } else {
      sprintf (output_line, "%s: %s", st, b);
    }
    io_close_tty_line ();
    io_write_string (STDOUT_FILENO, output_line);
  } else {
/* Add diagnostic and choose GNU style or non-GNU style. */
    MESSAGE_T **ref_msg;
    NODE_T *v = first_valid_token (p);
    p = (v == NULL) ? p : v;
    ref_msg = &(LINE (p)->messages);
    k = 1;
    while (*ref_msg != NULL) {
      ref_msg = &(NEXT (*ref_msg));
      k++;
    }
    if (gnu_diags) {
      sprintf (a, "%s: %d: %s: %s (%x)", LINE (p)->filename, LINE (p)->number, st, b, k);
    } else {
      if (LINE (p)->filename != NULL && strcmp (a68_prog.files.source.name, LINE (p)->filename) == 0) {
	sprintf (a, "(%x) %s: %s", k, st, b);
      } else if (LINE (p)->filename != NULL) {
	sprintf (a, "(%x) %s: %s: %s", k, LINE (p)->filename, st, b);
      } else {
	sprintf (a, "(%x) %s: %s", k, st, b);
      }
    }
    msg = (MESSAGE_T *) get_heap_space (SIZE_OF (MESSAGE_T));
    *ref_msg = msg;
    ATTRIBUTE (msg) = sev;
    msg->text = new_string (a);
    msg->where = p;
    msg->number = k;
    msg->message_number = 0;
    NEXT (msg) = NULL;
    if (p->msg == NULL) {
      p->msg = msg;
    }
  }
  va_end (args);
}

/*!
\brief write diagnostics to STDOUT
\param p
\param what
**/

void diagnostics_to_terminal (SOURCE_LINE_T * p, int what)
{
  for (; p != NULL; FORWARD (p)) {
    int z = 0;
    MESSAGE_T *d = p->messages;
    for (; d != NULL; FORWARD (d)) {
      if (what == A_ALL_DIAGNOSTICS) {
	z |= (WHETHER (d, A_WARNING) || WHETHER (d, A_ERROR) || WHETHER (d, A_SYNTAX_ERROR));
      } else if (what == A_RUNTIME_ERROR) {
	z |= (WHETHER (d, A_RUNTIME_ERROR));
      }
    }
    if (z) {
      MESSAGE_T *d = p->messages;
      char *c;
      if ((p->string)[strlen (p->string) - 1] == '\n') {
	(p->string)[strlen (p->string) - 1] = '\0';
      }
      io_close_tty_line ();
      sprintf (output_line, "%-4d %s\n     ", p->number, p->string);
      io_write_string (STDOUT_FILENO, output_line);
      c = p->string;
      while (*c != '\0') {
	if (*c == ' ' || *c == '\t') {
	  sprintf (output_line, "%c", *c);
	  io_write_string (STDOUT_FILENO, output_line);
	  c++;
	} else {
	  int f = 0, more = 0;
	  for (d = p->messages; d != NULL; FORWARD (d)) {
	    if (d->where->info->char_in_line == c && ++more == 1) {
	      f = d->number;
	    }
	  }
	  c++;
	  if (more != 0) {
	    if (more == 1) {
	      sprintf (output_line, "%c", digit_to_char (f));
	    } else {
	      sprintf (output_line, "*");
	    }
	  } else {
	    sprintf (output_line, " ");
	  }
	  io_write_string (STDOUT_FILENO, output_line);
	}
      }
      for (d = p->messages; d != NULL; FORWARD (d)) {
	if ((what == A_ALL_DIAGNOSTICS) && (WHETHER (d, A_WARNING) || WHETHER (d, A_ERROR) || WHETHER (d, A_SYNTAX_ERROR))) {
	  sprintf (output_line, "%s", d->text);
	  io_close_tty_line ();
	  io_write_string (STDOUT_FILENO, output_line);
	} else if (what == A_RUNTIME_ERROR && WHETHER (d, A_RUNTIME_ERROR)) {
	  sprintf (output_line, "%s", d->text);
	  io_close_tty_line ();
	  io_write_string (STDOUT_FILENO, output_line);
	}
      }
    }
  }
}
