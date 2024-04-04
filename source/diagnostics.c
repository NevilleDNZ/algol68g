/*!
\file diagnostics.c
\brief error handling routines
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2006 J. Marcel van der Veer <algol68g@xs4all.nl>.

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

#ifdef HAVE_CURSES
#include <curses.h>
#endif

#define SMALL_BUFFER_SIZE 32

#define TABULATE(n) (8 * (n / 8 + 1) - n)

int error_count, warning_count, run_time_error_count;

/*!
\brief whether unprintable control character
**/

BOOL_T unprintable (char ch)
{
  return ((ch < 32 || ch >= 127) && ch != '\t');
}

/*!
\brief format for printing control character
\param 
**/

char *ctrl_char (int ch)
{
  static char str[SMALL_BUFFER_SIZE];
  ch = TO_UCHAR (ch);
  if (ch > 0 && ch < 27) {
    sprintf (str, "\\^%c", ch + 96);
  } else {
    sprintf (str, "\\%02x", ch);
  }
  return (str);
}

/*!
\brief widen single char to string
\param 
\param
**/

static char *char_to_str (char ch)
{
  static char str[SMALL_BUFFER_SIZE];
  str[0] = ch;
  str[1] = '\0';
  return (str);
}

/*!
\brief pretty-print diagnostic 
\param 
\param
**/

static void pretty_diag (FILE_T f, char *p)
{
  int pos, line_width = (f == STDOUT_FILENO ? term_width : MAX_LINE_WIDTH);
  if (a68c_diags) {
    io_write_string (f, "++++ ");
    pos = 5;
  } else {
    pos = 1;
  }
  while (p[0] != '\0') {
    char *q;
    int k;
    if (IS_GRAPH (p[0])) {
      for (k = 0, q = p; q[0] != ' ' && q[0] != '\0' && k <= line_width - 5; q++, k++) {
	;
      }
    } else {
      k = 1;
    }
    if (k > line_width - 5) {
      k = 1;
    }
    if ((pos + k) >= line_width) {
      if (a68c_diags) {
	io_write_string (f, "\n     ");
	pos = 5;
      } else {
	io_write_string (f, "\n");
	pos = 1;
      }
    }
    for (; k > 0; k--, p++, pos++) {
      io_write_string (f, char_to_str (p[0]));
    }
  }
  for (; p[0] == ' '; p++, pos++) {
    io_write_string (f, char_to_str (p[0]));
  }
}

/*!
\brief abnormal end
\param 
\param
\param
\param
**/

void abend (char *reason, char *info, char *file, int line)
{
  if (a68c_diags) {
    sprintf (output_line, "Abnormal end in line %d of file \"%s\": %s.", line, file, reason);
  } else {
    sprintf (output_line, "abnormal end: %s: %d: %s", file, line, reason);
  }
  if (info != NULL) {
    strcat (output_line, ", ");
    strcat (output_line, info);
  }
  if (errno != 0) {
    strcat (output_line, " (");
    strcat (output_line, ERROR_SPECIFICATION);
    strcat (output_line, ")");
  }
  io_close_tty_line ();
  pretty_diag (STDOUT_FILENO, output_line);
  a68g_exit (EXIT_FAILURE);
}

/*!
\brief position in line where diagnostic points at
\param 
\param
**/

static char *diag_pos (SOURCE_LINE_T * p, DIAGNOSTIC_T * d)
{
  char *pos;
  if (d->where != NULL && p == LINE (d->where)) {
    pos = d->where->info->char_in_line;
  } else {
    pos = p->string;
  }
  if (pos == NULL) {
    pos = p->string;
  }
  for (; IS_SPACE (pos[0]) && pos[0] != '\0'; pos++) {
    ;
  }
  if (pos[0] == '\0') {
    pos = p->string;
  }
  return (pos);
}

/*!
\brief write source line to file with diagnostics
\param f
\param p
\param what
**/

void write_source_line (FILE_T f, SOURCE_LINE_T * p)
{
  char *c, *c0;
  int continuations = 0;
  int pos = 5, col;
  int line_width = (f == STDOUT_FILENO ? term_width : MAX_LINE_WIDTH);
  BOOL_T line_ended;
/* Terminate properly */
  if ((p->string)[strlen (p->string) - 1] == '\n') {
    (p->string)[strlen (p->string) - 1] = '\0';
    if ((p->string)[strlen (p->string) - 1] == '\r') {
      (p->string)[strlen (p->string) - 1] = '\0';
    }
  }
/* Print line number */
  if (f == STDOUT_FILENO) {
    io_close_tty_line ();
    sprintf (output_line, "%04d ", p->number % 10000);
  } else {
    sprintf (output_line, "\n%04d ", p->number % 10000);
  }
  io_write_string (f, output_line);
/* Pretty print line */
  c = c0 = p->string;
  col = 1;
  line_ended = A_FALSE;
  while (!line_ended) {
    int len = 0;
    char *new_pos = NULL;
    if (c[0] == '\0') {
      strcpy (output_line, "");
      line_ended = A_TRUE;
    } else {
      if (IS_GRAPH (c[0])) {
	char *c1;
	strcpy (output_line, "");
	for (c1 = c; IS_GRAPH (c1[0]) && len <= line_width - 5; c1++, len++) {
	  strcat (output_line, char_to_str (c1[0]));
	}
	if (len > line_width - 5) {
	  strcpy (output_line, char_to_str (c[0]));
	  len = 1;
	}
	new_pos = &c[len];
	col += len;
      } else if (c[0] == '\t') {
	int n = TABULATE (col);
	len = n;
	col += n;
	strcpy (output_line, "");
	while (n--) {
	  strcat (output_line, " ");
	}
	new_pos = &c[1];
      } else if (unprintable (c[0])) {
	strcpy (output_line, ctrl_char ((int) c[0]));
	len = strlen (output_line);
	new_pos = &c[1];
	col++;
      } else {
	strcpy (output_line, char_to_str (c[0]));
	len = 1;
	new_pos = &c[1];
	col++;
      }
    }
    if (!line_ended && (pos + len) <= line_width) {
/* Still room - print a character */
      io_write_string (f, output_line);
      pos += len;
      c = new_pos;
    } else {
/* First see if there are diagnostics to be printed */
      BOOL_T z = A_FALSE;
      DIAGNOSTIC_T *d = p->diagnostics;
      if (d != NULL) {
	char *c1;
	for (c1 = c0; c1 != c; c1++) {
	  for (d = p->diagnostics; d != NULL; FORWARD (d)) {
	    z |= (c1 == diag_pos (p, d));
	  }
	}
      }
/* If diagnostics are to be printed then print marks */
      if (z) {
	DIAGNOSTIC_T *d;
	char *c1;
	int col_2 = 1;
	io_write_string (f, "\n     ");
	for (c1 = c0; c1 != c; c1++) {
	  int k = 0, diags_at_this_pos = 0;
	  for (d = p->diagnostics; d != NULL; FORWARD (d)) {
	    if (c1 == diag_pos (p, d)) {
	      diags_at_this_pos++;
	      k = d->number;
	    }
	  }
	  if (diags_at_this_pos != 0) {
	    if (diags_at_this_pos == 1) {
	      sprintf (output_line, "%c", digit_to_char (k));
	    } else {
	      strcpy (output_line, "*");
	    }
	  } else {
	    if (unprintable (c1[0])) {
	      int n = strlen (ctrl_char (c1[0]));
	      col_2 += 1;
	      strcpy (output_line, "");
	      while (n--) {
		strcat (output_line, " ");
	      }
	    } else if (c1[0] == '\t') {
	      int n = TABULATE (col_2);
	      col_2 += n;
	      strcpy (output_line, "");
	      while (n--) {
		strcat (output_line, " ");
	      }
	    } else {
	      strcpy (output_line, " ");
	      col_2++;
	    }
	  }
	  io_write_string (f, output_line);
	}
      }
/* Resume pretty printing of line */
      if (!line_ended) {
	continuations++;
	sprintf (output_line, "\n.%1d   ", continuations);
	io_write_string (f, output_line);
	if (continuations >= 9) {
	  io_write_string (f, "...");
	  line_ended = A_TRUE;
	} else {
	  c0 = c;
	  pos = 5;
	  col = 1;
	}
      }
    }
  }				/* while */
/* Print the diagnostics */
  if (p->diagnostics != NULL) {
    DIAGNOSTIC_T *d;
    for (d = p->diagnostics; d != NULL; FORWARD (d)) {
      io_write_string (f, "\n");
      pretty_diag (f, d->text);
    }
  }
}

/*!
\brief write diagnostics to STDOUT
\param p
\param what
**/

void diagnostics_to_terminal (SOURCE_LINE_T * p, int what)
{
  for (; p != NULL; FORWARD (p)) {
    if (p->diagnostics != NULL) {
      BOOL_T z = A_FALSE;
      DIAGNOSTIC_T *d = p->diagnostics;
      for (; d != NULL; FORWARD (d)) {
	if (what == A_ALL_DIAGNOSTICS) {
	  z |= (WHETHER (d, A_WARNING) || WHETHER (d, A_ERROR) || WHETHER (d, A_SYNTAX_ERROR));
	} else if (what == A_RUNTIME_ERROR) {
	  z |= (WHETHER (d, A_RUNTIME_ERROR));
	}
      }
      if (z) {
	write_source_line (STDOUT_FILENO, p);
      }
    }
  }
}

/*!
\brief give an intelligible error and exit
\param u
\param txt
**/

void scan_error (SOURCE_LINE_T * u, char *v, char *txt)
{
  diagnostic_line (A_ERROR, u, v, txt, ERROR_SPECIFICATION, NULL);
  longjmp (exit_compilation, 1);
}

/*
\brief get severity
\param sev
*/

static char *get_severity (int sev)
{
  switch (sev) {
  case A_ERROR:
    {
      error_count++;
      return ("error");
    }
  case A_SYNTAX_ERROR:
    {
      error_count++;
      return ("syntax error");
    }
  case A_RUNTIME_ERROR:
    {
      error_count++;
      return ("runtime error");
    }
  case A_WARNING:
    {
      warning_count++;
      return ("warning");
    }
  default:
    {
      return (NULL);
    }
  }
}

/*!
\brief print diagnostic and choose GNU style or non-GNU style
*/

static void write_diagnostic (int sev, char *b)
{
  char st[SMALL_BUFFER_SIZE];
  strcpy (st, get_severity (sev));
  if (gnu_diags) {
    if (a68_prog.files.generic_name != NULL) {
      sprintf (output_line, "%s: %s: %s", a68_prog.files.generic_name, st, b);
    } else {
      sprintf (output_line, "%s: %s: %s", A68G_NAME, st, b);
    }
  } else {
    st[0] = TO_UPPER (st[0]);
    b[0] = TO_UPPER (b[0]);
    sprintf (output_line, "%s. %s.", st, b);
  }
  io_close_tty_line ();
  pretty_diag (STDOUT_FILENO, output_line);
}

/*!
\brief add diagnostic and choose GNU style or non-GNU style
*/

static void add_diagnostic (SOURCE_LINE_T * line, char *pos, NODE_T * p, int sev, char *b)
{
/* Add diagnostic and choose GNU style or non-GNU style. */
  DIAGNOSTIC_T *msg = (DIAGNOSTIC_T *) get_heap_space (SIZE_OF (DIAGNOSTIC_T));
  DIAGNOSTIC_T **ref_msg;
  char a[BUFFER_SIZE], st[SMALL_BUFFER_SIZE], nst[BUFFER_SIZE];
  int k = 1;
  if (line == NULL && p == NULL) {
    return;
  }
  strcpy (st, get_severity (sev));
  nst[0] = '\0';
  if (line == NULL && p != NULL) {
    line = LINE (p);
  }
  while (line != NULL && line->number == 0) {
    line = NEXT (line);
  }
  if (line == NULL) {
    return;
  }
  ref_msg = &(line->diagnostics);
  while (*ref_msg != NULL) {
    ref_msg = &(NEXT (*ref_msg));
    k++;
  }
  if (gnu_diags) {
    sprintf (a, "%s: %d: %s: %s (%x)", line->filename, line->number, st, b, k);
  } else {
    if (p != NULL) {
      NODE_T *n;
      n = NEST (p);
      if (n != NULL && SYMBOL (n) != NULL) {
	char *nt = non_terminal_string (edit_line, ATTRIBUTE (n));
	if (nt != NULL) {
	  if (NUMBER (LINE (n)) == 0) {
	    sprintf (nst, "Detected in %s.", nt);
	  } else {
	    if (MOID (n) != NULL) {
	      if (NUMBER (LINE (n)) == NUMBER (line)) {
		sprintf (nst, "Detected in %s %s starting at \"%.64s\" in this line.", moid_to_string (MOID (n), MOID_ERROR_WIDTH), nt, SYMBOL (n));
	      } else {
		sprintf (nst, "Detected in %s %s starting at \"%.64s\" in line %d.", moid_to_string (MOID (n), MOID_ERROR_WIDTH), nt, SYMBOL (n), NUMBER (LINE (n)));
	      }
	    } else {
	      if (NUMBER (LINE (n)) == NUMBER (line)) {
		sprintf (nst, "Detected in %s starting at \"%.64s\" in this line.", nt, SYMBOL (n));
	      } else {
		sprintf (nst, "Detected in %s starting at \"%.64s\" in line %d.", nt, SYMBOL (n), NUMBER (LINE (n)));
	      }
	    }
	  }
	}
      }
    }
    st[0] = TO_UPPER (st[0]);
    b[0] = TO_UPPER (b[0]);
    if (line->filename != NULL && strcmp (a68_prog.files.source.name, line->filename) == 0) {
      sprintf (a, "(%x) %s. %s.", k, st, b);
    } else if (line->filename != NULL) {
      sprintf (a, "(%x) %s in file \"%s\". %s.", k, st, line->filename, b);
    } else {
      sprintf (a, "(%x) %s. %s.", k, st, b);
    }
  }
  msg = (DIAGNOSTIC_T *) get_heap_space (SIZE_OF (DIAGNOSTIC_T));
  *ref_msg = msg;
  ATTRIBUTE (msg) = sev;
  if (nst[0] != '\0') {
    strcat (a, " ");
    strcat (a, nst);
  }
  msg->text = new_string (a);
  msg->where = p;
  msg->line = line;
  msg->symbol = pos;
  msg->number = k;
  NEXT (msg) = NULL;
}

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

#define COMPOSE_DIAGNOSTIC\
  while (t[0] != '\0') {\
    if (t[0] == '#') {\
      extra_syntax = A_FALSE;\
    } else if (t[0] == '@') {\
      char *nt = non_terminal_string (edit_line, ATTRIBUTE (p));\
      if (t != NULL) {\
	strcat (q, nt);\
      } else {\
	strcat (q, "construct");\
      }\
    } else if (t[0] == 'A') {\
      int att = va_arg (args, int);\
      char *nt = non_terminal_string (edit_line, att);\
      if (nt != NULL) {\
	strcat (q, nt);\
      } else {\
	strcat (q, "construct");\
      }\
    } else if (t[0] == 'B') {\
      int att = va_arg (args, int);\
      KEYWORD_T *nt = find_keyword_from_attribute (top_keyword, att);\
      if (nt != NULL) {\
	strcat (q, "\"");\
	strcat (q, nt->text);\
	strcat (q, "\"");\
      } else {\
	strcat (q, "keyword");\
      }\
    } else if (t[0] == 'C') {\
      int att = va_arg (args, int);\
      if (att == NO_SORT) {\
	strcat (q, "this");\
      }\
      if (att == SOFT) {\
	strcat (q, "a soft");\
      } else if (att == WEAK) {\
	strcat (q, "a weak");\
      } else if (att == MEEK) {\
	strcat (q, "a meek");\
      } else if (att == FIRM) {\
	strcat (q, "a firm");\
      } else if (att == STRONG) {\
	strcat (q, "a strong");\
      }\
    } else if (t[0] == 'D') {\
      int a = va_arg (args, int);\
      char d[BUFFER_SIZE];\
      sprintf (d, "%d", a);\
      strncat (q, d, BUFFER_SIZE);\
    } else if (t[0] == 'H') {\
      char *a = va_arg (args, char *);\
      char d[SMALL_BUFFER_SIZE];\
      sprintf (d, "'%c'", a[0]);\
      strcat (q, d);\
    } else if (t[0] == 'L') {\
      SOURCE_LINE_T *a = va_arg (args, SOURCE_LINE_T *);\
      char d[SMALL_BUFFER_SIZE];\
      ABNORMAL_END (a == NULL, "NULL source line in error", NULL);\
      if (a->number > 0) {\
        if (p != NULL && a->number == LINE (p)->number) {\
          sprintf (d, "in this line");\
	} else {\
          sprintf (d, "in line %d", a->number);\
        }\
	strcat (q, d);\
      }\
    } else if (t[0] == 'M') {\
      moid = va_arg (args, MOID_T *);\
      if (moid == MODE (ERROR)) {\
	return;\
      } else if (moid == NULL) {\
	strcat (q, "\"NULL\"");\
      } else if (WHETHER (moid, SERIES_MODE)) {\
	if (PACK (moid) != NULL && NEXT (PACK (moid)) == NULL) {\
	  strcat (q, moid_to_string (MOID (PACK (moid)), MOID_ERROR_WIDTH));\
	} else {\
	  strcat (q, moid_to_string (moid, MOID_ERROR_WIDTH));\
	}\
      } else {\
	strcat (q, moid_to_string (moid, MOID_ERROR_WIDTH));\
      }\
    } else if (t[0] == 'N') {\
      strcat (q, "NIL value of mode ");\
      moid = va_arg (args, MOID_T *);\
      if (moid != NULL) {\
	strcat (q, moid_to_string (moid, MOID_ERROR_WIDTH));\
      }\
    } else if (t[0] == 'O') {\
      moid = va_arg (args, MOID_T *);\
      if (moid == MODE (ERROR)) {\
	return;\
      } else if (moid == NULL) {\
	strcat (q, "\"NULL\"");\
      } else if (moid == MODE (VOID)) {\
	strcat (q, "UNION (VOID, ..)");\
      } else if (WHETHER (moid, SERIES_MODE)) {\
	if (PACK (moid) != NULL && NEXT (PACK (moid)) == NULL) {\
	  strcat (q, moid_to_string (MOID (PACK (moid)), MOID_ERROR_WIDTH));\
	} else {\
	  strcat (q, moid_to_string (moid, MOID_ERROR_WIDTH));\
	}\
      } else {\
	strcat (q, moid_to_string (moid, MOID_ERROR_WIDTH));\
      }\
    } else if (t[0] == 'S') {\
      if (p != NULL && SYMBOL (p) != NULL) {\
	strcat (q, "\"");\
	strncat (q, SYMBOL (p), 64);\
	strcat (q, "\"");\
      } else {\
	strcat (q, "symbol");\
      }\
    } else if (t[0] == 'X') {\
      int att = va_arg (args, int);\
      char z[BUFFER_SIZE];\
      sprintf (z, "\"%s\"", find_keyword_from_attribute (top_keyword, att)->text);\
      strcat (q, new_string (z));\
    } else if (t[0] == 'Y') {\
      char *str = va_arg (args, char *);\
      strcat (q, str);\
    } else if (t[0] == 'Z') {\
      char *str = va_arg (args, char *);\
      strcat (q, "\"");\
      strcat (q, str);\
      strcat (q, "\"");\
    } else {\
      *(q++) = *t;\
      *q = '\0';\
    }\
    while (*q != '\0') {\
      q++;\
    }\
    t++;\
  }

/*!
\brief give a diagnostic message
\param sev severity
\param p position in syntax tree, should not be NULL
\param str message string
\param ... various arguments needed by special symbols in str
**/

void diagnostic_node (int sev, NODE_T * p, char *str, ...)
{
  va_list args;
  MOID_T *moid = NULL;
  char *t = str, b[BUFFER_SIZE], *q = b;
  BOOL_T force, extra_syntax = A_TRUE, shortcut = A_FALSE;
  int err = errno;
  va_start (args, str);
  q[0] = '\0';
  force = (sev & FORCE_DIAGNOSTIC) != 0;
  sev &= ~FORCE_DIAGNOSTIC;
/* No warnings? */
  if (!force && sev == A_WARNING && no_warnings) {
    return;
  }
/* Suppressed? */
  if (sev == A_ERROR || sev == A_SYNTAX_ERROR) {
    if (error_count == MAX_ERRORS) {
      strcpy (b, "further error diagnostics suppressed");
      sev = A_ERROR;
      shortcut = A_TRUE;
    } else if (error_count > MAX_ERRORS) {
      error_count++;
      return;
    }
  } else if (sev == A_WARNING) {
    if (warning_count == MAX_ERRORS) {
      strcpy (b, "further warning diagnostics suppressed");
      shortcut = A_TRUE;
    } else if (warning_count > MAX_ERRORS) {
      warning_count++;
      return;
    }
  }
  if (!shortcut) {
/* Synthesize diagnostic message. */
    COMPOSE_DIAGNOSTIC;
/* Add information from errno, if any. */
    if (err != 0) {
      char *str = new_string (ERROR_SPECIFICATION);
      if (str != NULL) {
	char *stu;
	strcat (q, " (");
	for (stu = str; stu[0] != '\0'; stu++) {
	  stu[0] = TO_LOWER (stu[0]);
	}
	strncat (q, str, 128);
	strcat (q, ")");
      }
    }
  }
/* Construct a diagnostic message. */
  if (p == NULL) {
    write_diagnostic (sev, b);
  } else {
    add_diagnostic (NULL, NULL, p, sev, b);
  }
  va_end (args);
}

/*!
\brief give a diagnostic message
\param sev severity
\param p position in syntax tree, should not be NULL
\param str message string
\param ... various arguments needed by special symbols in str
**/

void diagnostic_line (int sev, SOURCE_LINE_T * line, char *pos, char *str, ...)
{
  va_list args;
  MOID_T *moid = NULL;
  char *t = str, b[BUFFER_SIZE], *q = b;
  BOOL_T force, extra_syntax = A_TRUE, shortcut = A_FALSE;
  int err = errno;
  NODE_T *p = NULL;
  va_start (args, str);
  q[0] = '\0';
  force = (sev & FORCE_DIAGNOSTIC) != 0;
  sev &= ~FORCE_DIAGNOSTIC;
/* No warnings? */
  if (!force && sev == A_WARNING && no_warnings) {
    return;
  }
/* Suppressed? */
  if (sev == A_ERROR || sev == A_SYNTAX_ERROR) {
    if (error_count == MAX_ERRORS) {
      strcpy (b, "further error diagnostics suppressed");
      sev = A_ERROR;
      shortcut = A_TRUE;
    } else if (error_count > MAX_ERRORS) {
      error_count++;
      return;
    }
  } else if (sev == A_WARNING) {
    if (warning_count == MAX_ERRORS) {
      strcpy (b, "further warning diagnostics suppressed");
      shortcut = A_TRUE;
    } else if (warning_count > MAX_ERRORS) {
      warning_count++;
      return;
    }
  }
  if (!shortcut) {
/* Synthesize diagnostic message. */
    COMPOSE_DIAGNOSTIC;
/* Add information from errno, if any. */
    if (err != 0) {
      char *str = new_string (ERROR_SPECIFICATION);
      if (str != NULL) {
	char *stu;
	strcat (q, " (");
	for (stu = str; stu[0] != '\0'; stu++) {
	  stu[0] = TO_LOWER (stu[0]);
	}
	strncat (q, str, 128);
	strcat (q, ")");
      }
    }
  }
/* Construct a diagnostic message. */
  if (line == NULL) {
    write_diagnostic (sev, b);
  } else {
    add_diagnostic (line, pos, NULL, sev, b);
  }
  va_end (args);
}
