//! @file a68g-diagnostics.c
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2024 J. Marcel van der Veer [algol68g@xs4all.nl].

//! @section License
//!
//! This program is free software; you can redistribute it and/or modify it 
//! under the terms of the GNU General Public License as published by the 
//! Free Software Foundation; either version 3 of the License, or 
//! (at your option) any later version.
//!
//! This program is distributed in the hope that it will be useful, but 
//! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//! or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
//! more details. You should have received a copy of the GNU General Public 
//! License along with this program. If not, see [http://www.gnu.org/licenses/].

//! @section Synopsis
//!
//! Error and warning routines.

#include "a68g.h"
#include "a68g-prelude.h"
#include "a68g-transput.h"
#include "a68g-parser.h"

// Error handling routines.

#define TABULATE(n) (8 * (n / 8 + 1) - n)

//! @brief Return error test from errno.

char *error_specification (void)
{
  static BUFFER txt;
  if (errno == 0) {
    ASSERT (a68_bufprt (txt, SNPRINTF_SIZE, "no information") >= 0);
  } else {
    ASSERT (a68_bufprt (txt, SNPRINTF_SIZE, "%s", strerror (errno)) >= 0);
  }
  if (strlen (txt) > 0) {
    txt[0] = TO_LOWER (txt[0]);
  }
  return txt;
}

//! @brief Whether unprintable control character.

BOOL_T unprintable (char ch)
{
  return (BOOL_T) (!IS_PRINT (ch) && ch != TAB_CHAR);
}

//! @brief Format for printing control character.

char *ctrl_char (int ch)
{
  static char txt[SMALL_BUFFER_SIZE];
  ch = TO_UCHAR (ch);
  if (IS_CNTRL (ch) && IS_LOWER (ch + 96)) {
    ASSERT (a68_bufprt (txt, (size_t) SMALL_BUFFER_SIZE, "\\^%c", ch + 96) >= 0);
  } else {
    ASSERT (a68_bufprt (txt, (size_t) SMALL_BUFFER_SIZE, "\\%02x", (unt) ch) >= 0);
  }
  return txt;
}

//! @brief Widen single char to string.

char *char_to_str (char ch)
{
  static char txt[2];
  txt[0] = ch;
  txt[1] = NULL_CHAR;
  return txt;
}

//! @brief Pretty-print diagnostic.

void pretty_diag (FILE_T f, char *p)
{
  int line_width = (f == A68_STDOUT ? A68 (term_width) : MAX_TERM_WIDTH);
  int pos = 1;
  while (p[0] != NULL_CHAR) {
    char *q;
    int k;
// Count the number of characters in token to print.
    if (IS_GRAPH (p[0])) {
      for (k = 0, q = p; q[0] != BLANK_CHAR && q[0] != NULL_CHAR && k <= line_width; q++, k++) {
        ;
      }
    } else {
      k = 1;
    }
// Now see if there is space for the token.
    if (k > line_width) {
      k = 1;
    }
    if ((pos + k) >= line_width) {
      WRITE (f, NEWLINE_STRING);
      pos = 1;
    }
    for (; k > 0; k--, p++, pos++) {
      WRITE (f, char_to_str (p[0]));
    }
  }
  for (; p[0] == BLANK_CHAR; p++, pos++) {
    WRITE (f, char_to_str (p[0]));
  }
}

//! @brief Abnormal end.

void abend (char *reason, char *info, char *file, int line)
{
  ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "%s: exiting: %s: %d: %s", A68 (a68_cmd_name), file, line, reason) >= 0);
  if (info != NO_TEXT) {
    a68_bufcat (A68 (output_line), ", ", BUFFER_SIZE);
    a68_bufcat (A68 (output_line), info, BUFFER_SIZE);
  }
  if (errno != 0) {
    a68_bufcat (A68 (output_line), " (", BUFFER_SIZE);
    a68_bufcat (A68 (output_line), error_specification (), BUFFER_SIZE);
    a68_bufcat (A68 (output_line), ")", BUFFER_SIZE);
  }
  a68_bufcat (A68 (output_line), "\n", BUFFER_SIZE);
  io_close_tty_line ();
  pretty_diag (A68_STDOUT, A68 (output_line));
  a68_exit (EXIT_FAILURE);
}

//! @brief Position in line .

char *where_pos (LINE_T * p, NODE_T * q)
{
  char *pos;
  if (q != NO_NODE && p == LINE (INFO (q))) {
    pos = CHAR_IN_LINE (INFO (q));
  } else {
    pos = STRING (p);
  }
  if (pos == NO_TEXT) {
    pos = STRING (p);
  }
  for (; IS_SPACE (pos[0]) && pos[0] != NULL_CHAR; pos++) {
    ;
  }
  if (pos[0] == NULL_CHAR) {
    pos = STRING (p);
  }
  return pos;
}

//! @brief Position in line where diagnostic points at.

char *diag_pos (LINE_T * p, DIAGNOSTIC_T * d)
{
  char *pos;
  if (WHERE (d) != NO_NODE && p == LINE (INFO (WHERE (d)))) {
    pos = CHAR_IN_LINE (INFO (WHERE (d)));
  } else {
    pos = STRING (p);
  }
  if (pos == NO_TEXT) {
    pos = STRING (p);
  }
  for (; IS_SPACE (pos[0]) && pos[0] != NULL_CHAR; pos++) {
    ;
  }
  if (pos[0] == NULL_CHAR) {
    pos = STRING (p);
  }
  return pos;
}

//! @brief Write source line to file with diagnostics.

void write_source_line (FILE_T f, LINE_T * p, NODE_T * nwhere, int mask)
{
  int line_width = (f == A68_STDOUT ? A68 (term_width) : MAX_TERM_WIDTH);
// Terminate properly.
  if ((STRING (p))[strlen (STRING (p)) - 1] == NEWLINE_CHAR) {
    (STRING (p))[strlen (STRING (p)) - 1] = NULL_CHAR;
    if ((STRING (p))[strlen (STRING (p)) - 1] == CR_CHAR) {
      (STRING (p))[strlen (STRING (p)) - 1] = NULL_CHAR;
    }
  }
// Print line number.
  if (f == A68_STDOUT) {
    io_close_tty_line ();
  } else {
    WRITE (f, NEWLINE_STRING);
  }
  if (NUMBER (p) == 0) {
    ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "      ") >= 0);
  } else {
    ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "%-5d ", NUMBER (p) % 100000) >= 0);
  }
  WRITE (f, A68 (output_line));
// Pretty print line.
  char *c, *c0;
  c = c0 = STRING (p);
  int pos = 5, col = 1;
  int continuations = 0;
  BOOL_T line_ended = A68_FALSE;
  while (!line_ended) {
    int len = 0;
    char *new_pos = NO_TEXT;
    if (c[0] == NULL_CHAR) {
      a68_bufcpy (A68 (output_line), "", BUFFER_SIZE);
      line_ended = A68_TRUE;
    } else {
      if (IS_GRAPH (c[0])) {
        char *c1;
        a68_bufcpy (A68 (output_line), "", BUFFER_SIZE);
        for (c1 = c; IS_GRAPH (c1[0]) && len <= line_width - 5; c1++, len++) {
          a68_bufcat (A68 (output_line), char_to_str (c1[0]), BUFFER_SIZE);
        }
        if (len > line_width - 5) {
          a68_bufcpy (A68 (output_line), char_to_str (c[0]), BUFFER_SIZE);
          len = 1;
        }
        new_pos = &c[len];
        col += len;
      } else if (c[0] == TAB_CHAR) {
        int n = TABULATE (col);
        len = n;
        col += n;
        a68_bufcpy (A68 (output_line), "", BUFFER_SIZE);
        while (n--) {
          a68_bufcat (A68 (output_line), " ", BUFFER_SIZE);
        }
        new_pos = &c[1];
      } else if (unprintable (c[0])) {
        a68_bufcpy (A68 (output_line), ctrl_char ((int) c[0]), BUFFER_SIZE);
        len = (int) strlen (A68 (output_line));
        new_pos = &c[1];
        col++;
      } else {
        a68_bufcpy (A68 (output_line), char_to_str (c[0]), BUFFER_SIZE);
        len = 1;
        new_pos = &c[1];
        col++;
      }
    }
    if (!line_ended && (pos + len) <= line_width) {
// Still room - print a character.
      WRITE (f, A68 (output_line));
      pos += len;
      c = new_pos;
    } else {
// First see if there are diagnostics to be printed.
      BOOL_T y = A68_FALSE, z = A68_FALSE;
      DIAGNOSTIC_T *d = DIAGNOSTICS (p);
      if (d != NO_DIAGNOSTIC || nwhere != NO_NODE) {
        char *c1;
        for (c1 = c0; c1 != c; c1++) {
          y |= (BOOL_T) (nwhere != NO_NODE && p == LINE (INFO (nwhere)) ? c1 == where_pos (p, nwhere) : A68_FALSE);
          if (mask != A68_NO_DIAGNOSTICS) {
            for (d = DIAGNOSTICS (p); d != NO_DIAGNOSTIC; FORWARD (d)) {
              z = (BOOL_T) (z | (c1 == diag_pos (p, d)));
            }
          }
        }
      }
// If diagnostics are to be printed then print marks.
      if (y || z) {
        DIAGNOSTIC_T *d2;
        char *c1;
        int col_2 = 1;
        WRITE (f, "\n      ");
        for (c1 = c0; c1 != c; c1++) {
          int k = 0, diags_at_this_pos = 0;
          for (d2 = DIAGNOSTICS (p); d2 != NO_DIAGNOSTIC; FORWARD (d2)) {
            if (c1 == diag_pos (p, d2)) {
              diags_at_this_pos++;
              k = NUMBER (d2);
            }
          }
          if (y == A68_TRUE && c1 == where_pos (p, nwhere)) {
            a68_bufcpy (A68 (output_line), "-", BUFFER_SIZE);
          } else if (diags_at_this_pos != 0) {
            if (mask == A68_NO_DIAGNOSTICS) {
              a68_bufcpy (A68 (output_line), " ", BUFFER_SIZE);
            } else if (diags_at_this_pos == 1) {
              ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "%c", digchar (k)) >= 0);
            } else {
              a68_bufcpy (A68 (output_line), "*", BUFFER_SIZE);
            }
          } else {
            if (unprintable (c1[0])) {
              int n = (int) strlen (ctrl_char (c1[0]));
              col_2 += 1;
              a68_bufcpy (A68 (output_line), "", BUFFER_SIZE);
              while (n--) {
                a68_bufcat (A68 (output_line), " ", BUFFER_SIZE);
              }
            } else if (c1[0] == TAB_CHAR) {
              int n = TABULATE (col_2);
              col_2 += n;
              a68_bufcpy (A68 (output_line), "", BUFFER_SIZE);
              while (n--) {
                a68_bufcat (A68 (output_line), " ", BUFFER_SIZE);
              }
            } else {
              a68_bufcpy (A68 (output_line), " ", BUFFER_SIZE);
              col_2++;
            }
          }
          WRITE (f, A68 (output_line));
        }
      }
// Resume pretty printing of line.
      if (!line_ended) {
        continuations++;
        ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "\n.%1d   ", continuations) >= 0);
        WRITE (f, A68 (output_line));
        if (continuations >= 9) {
          WRITE (f, "...");
          line_ended = A68_TRUE;
        } else {
          c0 = c;
          pos = 5;
          col = 1;
        }
      }
    }
  }
// Print the diagnostics.
  if (mask) {
    if (DIAGNOSTICS (p) != NO_DIAGNOSTIC) {
      DIAGNOSTIC_T *d;
      for (d = DIAGNOSTICS (p); d != NO_DIAGNOSTIC; FORWARD (d)) {
        if (mask == A68_RUNTIME_ERROR) {
          if (IS (d, A68_RUNTIME_ERROR) || IS (d, A68_MATH_ERROR) || (IS (d, A68_MATH_WARNING))) {
            WRITE (f, NEWLINE_STRING);
            pretty_diag (f, TEXT (d));
          }
        } else {
          WRITE (f, NEWLINE_STRING);
          pretty_diag (f, TEXT (d));
        }
      }
    }
  }
}

//! @brief Write diagnostics to STDOUT.

void diagnostics_to_terminal (LINE_T * p, int sev)
{
  for (; p != NO_LINE; FORWARD (p)) {
    if (DIAGNOSTICS (p) != NO_DIAGNOSTIC) {
      BOOL_T z = A68_FALSE;
      for (DIAGNOSTIC_T *d = DIAGNOSTICS (p); d != NO_DIAGNOSTIC; FORWARD (d)) {
        if (sev == A68_ALL_DIAGNOSTICS) {
          z = (BOOL_T) (z | (IS (d, A68_WARNING) || IS (d, A68_ERROR) || IS (d, A68_SYNTAX_ERROR) || IS (d, A68_MATH_ERROR) || IS (d, A68_RUNTIME_ERROR) || IS (d, A68_SUPPRESS_SEVERITY)));
        } else if (sev == A68_RUNTIME_ERROR) {
          z = (BOOL_T) (z | (IS (d, A68_RUNTIME_ERROR) || (IS (d, A68_MATH_ERROR))));
        }
      }
      if (z) {
        write_source_line (A68_STDOUT, p, NO_NODE, sev);
      }
    }
  }
}

//! @brief Give an intelligible error and exit.

void scan_error (LINE_T * u, char *v, char *txt)
{
  if (errno != 0) {
    diagnostic (A68_SUPPRESS_SEVERITY, NO_NODE, txt, u, v, error_specification ());
  } else {
    diagnostic (A68_SUPPRESS_SEVERITY, NO_NODE, txt, u, v, ERROR_UNSPECIFIED);
  }
  longjmp (RENDEZ_VOUS (&A68_JOB), 1);
}

//! @brief Give an intelligible warning.

void scan_warning (LINE_T * u, char *v, char *txt)
{
  if (errno != 0) {
    diagnostic (A68_SUPPRESS_SEVERITY, NO_NODE, txt, u, v, error_specification ());
  } else {
    diagnostic (A68_SUPPRESS_SEVERITY, NO_NODE, txt, u, v, ERROR_UNSPECIFIED);
  }
}

//! @brief Get severity text.

char *get_severity (int sev)
{
  switch (sev) {
  case A68_ERROR: {
      return "error";
    }
  case A68_SYNTAX_ERROR: {
      return "syntax error";
    }
  case A68_RUNTIME_ERROR: {
      return "runtime error";
    }
  case A68_MATH_ERROR: {
      return "math error";
    }
  case A68_MATH_WARNING: {
      return "math warning";
    }
  case A68_WARNING: {
      return "warning";
    }
  case A68_SUPPRESS_SEVERITY: {
      return NO_TEXT;
    }
  default: {
      return NO_TEXT;
    }
  }
}

//! @brief Print diagnostic.

void write_diagnostic (int sev, char *b)
{
  char txt[SMALL_BUFFER_SIZE];
  char *severity = get_severity (sev);
  if (severity == NO_TEXT) {
    ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "%s: %s.", A68 (a68_cmd_name), b) >= 0);
  } else {
    a68_bufcpy (txt, get_severity (sev), SMALL_BUFFER_SIZE);
    ASSERT (a68_bufprt (A68 (output_line), SNPRINTF_SIZE, "%s: %s: %s.", A68 (a68_cmd_name), txt, b) >= 0);
  }
  io_close_tty_line ();
  pretty_diag (A68_STDOUT, A68 (output_line));
}

//! @brief Add diagnostic to source line.

void add_diagnostic (LINE_T * line, char *pos, NODE_T * p, int sev, char *b)
{
// Add diagnostic and choose GNU style or non-GNU style.
  DIAGNOSTIC_T *msg = (DIAGNOSTIC_T *) get_heap_space ((size_t) SIZE_ALIGNED (DIAGNOSTIC_T));
  DIAGNOSTIC_T **ref_msg;
  BUFFER a, nst;
  char st[SMALL_BUFFER_SIZE];
  char *severity = get_severity (sev);
  int k = 1;
  if (line == NO_LINE && p == NO_NODE) {
    return;
  }
  if (A68 (in_monitor)) {
    monitor_error (b, NO_TEXT);
    return;
  }
  nst[0] = NULL_CHAR;
  if (line == NO_LINE && p != NO_NODE) {
    line = LINE (INFO (p));
  }
  while (line != NO_LINE && NUMBER (line) == 0) {
    FORWARD (line);
  }
  if (line == NO_LINE) {
    return;
  }
  ref_msg = &(DIAGNOSTICS (line));
  while (*ref_msg != NO_DIAGNOSTIC) {
    ref_msg = &(NEXT (*ref_msg));
    k++;
  }
  if (p != NO_NODE) {
    NODE_T *n = NEST (p);
    if (n != NO_NODE && NSYMBOL (n) != NO_TEXT) {
      char *nt = non_terminal_string (A68 (edit_line), ATTRIBUTE (n));
      if (nt != NO_TEXT) {
        if (LINE_NUMBER (n) == 0) {
          ASSERT (a68_bufprt (nst, SNPRINTF_SIZE, ", in %s", nt) >= 0);
        } else {
          if (MOID (n) != NO_MOID) {
            if (LINE_NUMBER (n) == NUMBER (line)) {
              ASSERT (a68_bufprt (nst, SNPRINTF_SIZE, ", in %s %s starting at \"%.64s\" in this line", moid_to_string (MOID (n), MOID_ERROR_WIDTH, p), nt, NSYMBOL (n)) >= 0);
            } else {
              ASSERT (a68_bufprt (nst, SNPRINTF_SIZE, ", in %s %s starting at \"%.64s\" in line %d", moid_to_string (MOID (n), MOID_ERROR_WIDTH, p), nt, NSYMBOL (n), LINE_NUMBER (n)) >= 0);
            }
          } else {
            if (LINE_NUMBER (n) == NUMBER (line)) {
              ASSERT (a68_bufprt (nst, SNPRINTF_SIZE, ", in %s starting at \"%.64s\" in this line", nt, NSYMBOL (n)) >= 0);
            } else {
              ASSERT (a68_bufprt (nst, SNPRINTF_SIZE, ", in %s starting at \"%.64s\" in line %d", nt, NSYMBOL (n), LINE_NUMBER (n)) >= 0);
            }
          }
        }
      }
    }
  }
  if (severity == NO_TEXT) {
    if (FILENAME (line) != NO_TEXT && strcmp (FILE_SOURCE_NAME (&A68_JOB), FILENAME (line)) == 0) {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %x: %s", A68 (a68_cmd_name), (unt) k, b) >= 0);
    } else if (FILENAME (line) != NO_TEXT) {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %s: %x: %s", A68 (a68_cmd_name), FILENAME (line), (unt) k, b) >= 0);
    } else {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %x: %s", A68 (a68_cmd_name), (unt) k, b) >= 0);
    }
  } else {
    a68_bufcpy (st, get_severity (sev), SMALL_BUFFER_SIZE);
    if (FILENAME (line) != NO_TEXT && strcmp (FILE_SOURCE_NAME (&A68_JOB), FILENAME (line)) == 0) {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %s: %x: %s", A68 (a68_cmd_name), st, (unt) k, b) >= 0);
    } else if (FILENAME (line) != NO_TEXT) {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %s: %s: %x: %s", A68 (a68_cmd_name), FILENAME (line), st, (unt) k, b) >= 0);
    } else {
      ASSERT (a68_bufprt (a, SNPRINTF_SIZE, "%s: %s: %x: %s", A68 (a68_cmd_name), st, (unt) k, b) >= 0);
    }
  }
// cppcheck might complain here but this memory is not returned, for obvious reasons.
  *ref_msg = msg;
  ATTRIBUTE (msg) = sev;
  if (nst[0] != NULL_CHAR) {
    a68_bufcat (a, nst, BUFFER_SIZE);
  }
  a68_bufcat (a, ".", BUFFER_SIZE);
  TEXT (msg) = new_string (a, NO_TEXT);
  WHERE (msg) = p;
  LINE (msg) = line;
  SYMBOL (msg) = pos;
  NUMBER (msg) = k;
  NEXT (msg) = NO_DIAGNOSTIC;
}

//! @brief Give a diagnostic message.

void diagnostic (STATUS_MASK_T sev, NODE_T * p, char *loc_str, ...)
{
  va_list args;
  MOID_T *moid = NO_MOID;
  char *t = loc_str, b[BUFFER_SIZE];
  BOOL_T force, compose = A68_TRUE, issue = A68_TRUE;
  va_start (args, loc_str);
  b[0] = NULL_CHAR;
  force = (BOOL_T) ((sev & A68_FORCE_DIAGNOSTICS) != 0);
  sev &= ~A68_FORCE_DIAGNOSTICS;
// Node or line?
  LINE_T *line = NO_LINE;
  char *pos = NO_TEXT;
  if (p == NO_NODE) {
    line = va_arg (args, LINE_T *);
    pos = va_arg (args, char *);
  }
// No warnings?
  if (!force && sev == A68_WARNING && OPTION_NO_WARNINGS (&A68_JOB)) {
    va_end (args);
    return;
  }
  if (!force && sev == A68_MATH_WARNING && OPTION_NO_WARNINGS (&A68_JOB)) {
    va_end (args);
    return;
  }
  if (sev == A68_WARNING && OPTION_QUIET (&A68_JOB)) {
    va_end (args);
    return;
  }
  if (sev == A68_MATH_WARNING && OPTION_QUIET (&A68_JOB)) {
    va_end (args);
    return;
  }
// Suppressed?.
  if (sev == A68_ERROR || sev == A68_SYNTAX_ERROR) {
    if (ERROR_COUNT (&A68_JOB) == MAX_ERRORS) {
      a68_bufcpy (b, "further diagnostics suppressed", BUFFER_SIZE);
      compose = A68_FALSE;
      sev = A68_ERROR;
    } else if (ERROR_COUNT (&A68_JOB) > MAX_ERRORS) {
      ERROR_COUNT (&A68_JOB)++;
      compose = issue = A68_FALSE;
    }
  } else if (sev == A68_WARNING || sev == A68_MATH_WARNING) {
    if (WARNING_COUNT (&A68_JOB) == MAX_ERRORS) {
      a68_bufcpy (b, "further diagnostics suppressed", BUFFER_SIZE);
      compose = A68_FALSE;
    } else if (WARNING_COUNT (&A68_JOB) > MAX_ERRORS) {
      WARNING_COUNT (&A68_JOB)++;
      compose = issue = A68_FALSE;
    }
  }
  if (compose) {
// Synthesize diagnostic message.
    if ((sev & A68_NO_SYNTHESIS) != NULL_MASK) {
      sev &= ~A68_NO_SYNTHESIS;
      a68_bufcat (b, t, BUFFER_SIZE);
    } else {
// Legend for special symbols:
// * as first character, copy rest of string literally
// # skip extra syntactical information
// @ non terminal
// A non terminal
// B keyword
// C context
// D argument in decimal
// H char argument
// K 'LONG'
// L line number
// M moid - if error mode return without giving a message
// N mode - M_NIL
// O moid - operand
// S quoted symbol, when possible with typographical display features
// X expected attribute
// Y string literal. 
// Z quoted string literal. 
      if (t[0] == '*') {
        a68_bufcat (b, &t[1], BUFFER_SIZE);
      } else
        while (t[0] != NULL_CHAR) {
          if (t[0] == '#') {
            ;
          } else if (t[0] == '@') {
            if (p == NO_NODE) {
              a68_bufcat (b, "construct", BUFFER_SIZE);
            } else {
              char *nt = non_terminal_string (A68 (edit_line), ATTRIBUTE (p));
              if (t != NO_TEXT) {
                a68_bufcat (b, nt, BUFFER_SIZE);
              } else {
                a68_bufcat (b, "construct", BUFFER_SIZE);
              }
            }
          } else if (t[0] == 'A') {
            int att = va_arg (args, int);
            char *nt = non_terminal_string (A68 (edit_line), att);
            if (nt != NO_TEXT) {
              a68_bufcat (b, nt, BUFFER_SIZE);
            } else {
              a68_bufcat (b, "construct", BUFFER_SIZE);
            }
          } else if (t[0] == 'B') {
            int att = va_arg (args, int);
            KEYWORD_T *nt = find_keyword_from_attribute (A68 (top_keyword), att);
            if (nt != NO_KEYWORD) {
              a68_bufcat (b, "\"", BUFFER_SIZE);
              a68_bufcat (b, TEXT (nt), BUFFER_SIZE);
              a68_bufcat (b, "\"", BUFFER_SIZE);
            } else {
              a68_bufcat (b, "keyword", BUFFER_SIZE);
            }
          } else if (t[0] == 'C') {
            int att = va_arg (args, int);
            if (att == NO_SORT) {
              a68_bufcat (b, "this", BUFFER_SIZE);
            }
            if (att == SOFT) {
              a68_bufcat (b, "a soft", BUFFER_SIZE);
            } else if (att == WEAK) {
              a68_bufcat (b, "a weak", BUFFER_SIZE);
            } else if (att == MEEK) {
              a68_bufcat (b, "a meek", BUFFER_SIZE);
            } else if (att == FIRM) {
              a68_bufcat (b, "a firm", BUFFER_SIZE);
            } else if (att == STRONG) {
              a68_bufcat (b, "a strong", BUFFER_SIZE);
            }
          } else if (t[0] == 'D') {
            int a = va_arg (args, int);
            BUFFER d;
            BUFCLR (d);
            ASSERT (a68_bufprt (d, SNPRINTF_SIZE, "%d", a) >= 0);
            a68_bufcat (b, d, BUFFER_SIZE);
          } else if (t[0] == 'H') {
            char *a = va_arg (args, char *);
            char d[SMALL_BUFFER_SIZE];
            ASSERT (a68_bufprt (d, (size_t) SMALL_BUFFER_SIZE, "\"%c\"", a[0]) >= 0);
            a68_bufcat (b, d, BUFFER_SIZE);
          } else if (t[0] == 'K') {
            a68_bufcat (b, "LONG", BUFFER_SIZE);
          } else if (t[0] == 'L') {
            LINE_T *a = va_arg (args, LINE_T *);
            char d[SMALL_BUFFER_SIZE];
            ABEND (a == NO_LINE, ERROR_INTERNAL_CONSISTENCY, __func__);
            if (NUMBER (a) == 0) {
              a68_bufcat (b, "in standard environment", BUFFER_SIZE);
            } else {
              if (p != NO_NODE && NUMBER (a) == LINE_NUMBER (p)) {
                ASSERT (a68_bufprt (d, (size_t) SMALL_BUFFER_SIZE, "in this line") >= 0);
              } else {
                ASSERT (a68_bufprt (d, (size_t) SMALL_BUFFER_SIZE, "in line %d", NUMBER (a)) >= 0);
              }
              a68_bufcat (b, d, BUFFER_SIZE);
            }
          } else if (t[0] == 'M') {
            moid = va_arg (args, MOID_T *);
            if (moid == NO_MOID || moid == M_ERROR) {
              moid = M_UNDEFINED;
            }
            if (IS (moid, SERIES_MODE)) {
              if (PACK (moid) != NO_PACK && NEXT (PACK (moid)) == NO_PACK) {
                a68_bufcat (b, moid_to_string (MOID (PACK (moid)), MOID_ERROR_WIDTH, p), BUFFER_SIZE);
              } else {
                a68_bufcat (b, moid_to_string (moid, MOID_ERROR_WIDTH, p), BUFFER_SIZE);
              }
            } else {
              a68_bufcat (b, moid_to_string (moid, MOID_ERROR_WIDTH, p), BUFFER_SIZE);
            }
          } else if (t[0] == 'N') {
            a68_bufcat (b, "NIL name of mode ", BUFFER_SIZE);
            moid = va_arg (args, MOID_T *);
            if (moid != NO_MOID) {
              a68_bufcat (b, moid_to_string (moid, MOID_ERROR_WIDTH, p), BUFFER_SIZE);
            }
          } else if (t[0] == 'O') {
            moid = va_arg (args, MOID_T *);
            if (moid == NO_MOID || moid == M_ERROR) {
              moid = M_UNDEFINED;
            }
            if (moid == M_VOID) {
              a68_bufcat (b, "UNION (VOID, ..)", BUFFER_SIZE);
            } else if (IS (moid, SERIES_MODE)) {
              if (PACK (moid) != NO_PACK && NEXT (PACK (moid)) == NO_PACK) {
                a68_bufcat (b, moid_to_string (MOID (PACK (moid)), MOID_ERROR_WIDTH, p), BUFFER_SIZE);
              } else {
                a68_bufcat (b, moid_to_string (moid, MOID_ERROR_WIDTH, p), BUFFER_SIZE);
              }
            } else {
              a68_bufcat (b, moid_to_string (moid, MOID_ERROR_WIDTH, p), BUFFER_SIZE);
            }
          } else if (t[0] == 'S') {
            if (p != NO_NODE && NSYMBOL (p) != NO_TEXT) {
              char *txt = NSYMBOL (p);
              char *sym = NCHAR_IN_LINE (p);
              int n = 0, size = (int) strlen (txt);
              a68_bufcat (b, "\"", BUFFER_SIZE);
              if (txt[0] != sym[0] || (int) strlen (sym) < size) {
                a68_bufcat (b, txt, BUFFER_SIZE);
              } else {
                while (n < size) {
                  if (IS_PRINT (sym[0])) {
                    char str[2];
                    str[0] = sym[0];
                    str[1] = NULL_CHAR;
                    a68_bufcat (b, str, BUFFER_SIZE);
                  }
                  if (TO_LOWER (txt[0]) == TO_LOWER (sym[0])) {
                    txt++;
                    n++;
                  }
                  sym++;
                }
              }
              a68_bufcat (b, "\"", BUFFER_SIZE);
            } else {
              a68_bufcat (b, "symbol", BUFFER_SIZE);
            }
          } else if (t[0] == 'V') {
            a68_bufcat (b, PACKAGE_STRING, BUFFER_SIZE);
          } else if (t[0] == 'X') {
            int att = va_arg (args, int);
            BUFFER z;
            (void) non_terminal_string (z, att);
            a68_bufcat (b, new_string (z, NO_TEXT), BUFFER_SIZE);
          } else if (t[0] == 'Y') {
            char *loc_string = va_arg (args, char *);
            a68_bufcat (b, loc_string, BUFFER_SIZE);
          } else if (t[0] == 'Z') {
            char *loc_string = va_arg (args, char *);
            a68_bufcat (b, "\"", BUFFER_SIZE);
            a68_bufcat (b, loc_string, BUFFER_SIZE);
            a68_bufcat (b, "\"", BUFFER_SIZE);
          } else {
            char q[2];
            q[0] = t[0];
            q[1] = NULL_CHAR;
            a68_bufcat (b, q, BUFFER_SIZE);
          }
          t++;
        }
// Add information from errno, if any.
      if (errno != 0) {
        char *loc_str2 = new_string (error_specification (), NO_TEXT);
        if (loc_str2 != NO_TEXT) {
          char *stu;
          a68_bufcat (b, ", ", BUFFER_SIZE);
          for (stu = loc_str2; stu[0] != NULL_CHAR; stu++) {
            stu[0] = (char) TO_LOWER (stu[0]);
          }
          a68_bufcat (b, loc_str2, BUFFER_SIZE);
        }
      }
    }
  }
// Construct a diagnostic message.
  if (issue) {
    if (sev == A68_WARNING) {
      WARNING_COUNT (&A68_JOB)++;
    } else {
      ERROR_COUNT (&A68_JOB)++;
    }
    if (p == NO_NODE) {
      if (line == NO_LINE) {
        write_diagnostic (sev, b);
      } else {
        add_diagnostic (line, pos, NO_NODE, sev, b);
      }
    } else {
      add_diagnostic (NO_LINE, NO_TEXT, p, sev, b);
      if (sev == A68_MATH_WARNING && p != NO_NODE && LINE (INFO (p)) != NO_LINE) {
        write_source_line (A68_STDOUT, LINE (INFO (p)), p, A68_TRUE);
        WRITE (A68_STDOUT, NEWLINE_STRING);
      }
    }
  }
  va_end (args);
}
