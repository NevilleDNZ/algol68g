/*-------1---------2---------3---------4---------5---------6---------7---------+
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2004 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful,but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      */

/*-------1---------2---------3---------4---------5---------6---------7---------+
This is the scanner. The source file is read and stored internally, is
tokenised, and if needed a refinement preprocessor elaborates a stepwise 
refined program. The result is a linear list of tokens that is input for the 
parser, that will transform the linear list into a syntax tree.

Algol68G tokenises all symbols before the parser is invoked. This means that 
scanning does not use information from the parser.

The scanner does of course do some rudimentary parsing. Format texts can have 
enclosed clauses in them, so we record information in the stack as to know 
what is being scanned. Also, the refinement preprocessor implements a 
(trivial) grammar.

The scanner supports two stropping regimes: bold and quote. Examples of both:

       bold stropping: BEGIN INT i = 1, j = 1; print (i + j) END

       quote stropping: 'BEGIN' 'INT' I = 1, J = 1; PRINT (I + J) 'END'

Quote stropping was used frequently in the (excusez-le-mot) punch-card age. 
Hence, bold stropping is the default. There also existed point stropping, but 
that has not been implemented here.                                           */

#include "algol68g.h"
#include "environ.h"
#include <ctype.h>

#define STOP_CHAR 127

#define IN_PRELUDE(p) (LINE(p)->number <= 0)

void file_format_error (MODULE_T *, int);

static char *scan_buf;
static int max_scan_buf_length, source_file_size;
static SOURCE_LINE_T *saved_l;
static char *saved_c;
static BOOL_T stop_scanner = A_FALSE, read_error = A_FALSE;

/*-------1---------2---------3---------4---------5---------6---------7---------+
append_source_line: append a source line to the internal source file.
str: text line to be appended.
ref_l: previous source line.
l_num: previous source line number.                                           */

static void
append_source_line (MODULE_T * module, char *str, SOURCE_LINE_T ** ref_l, int *line_num)
{
  SOURCE_LINE_T *z = new_source_line ();
/* Allow shell command in first line, f.i. "#!/usr/share/bin/a68g".          */
  if (*line_num == 1)
    {
      if (strlen (str) >= 2 && strncmp (str, "#!", 2) == 0)
	{
	  (*line_num)++;
	  return;
	}
    }
/* Link line into the chain. */
  z->string = new_fixed_string (str);
  z->number = (*line_num)++;
  z->print_status = NOT_PRINTED;
  z->list = A_FALSE;
  z->messages = NULL;
  z->top_node = NULL;
  z->min_level = INT_MAX;
  z->max_level = 0;
  z->min_proc_level = INT_MAX;
  z->max_proc_level = 0;
  NEXT (z) = NULL;
  PREVIOUS (z) = *ref_l;
  if (module->top_line == NULL)
    {
      module->top_line = z;
    }
  if (*ref_l != NULL)
    {
      NEXT (*ref_l) = z;
    }
  *ref_l = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_source_size: size of source file.                                         */

static int
get_source_size (MODULE_T * module)
{
  FILE_T f = module->files.source.fd;
  return (lseek (f, 0, SEEK_END));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
append_environ: append environment source lines.
str: line to append.
ref_l: source line after which to append.
line_num: number of source line 'ref_l'.                                         */

static void
append_environ (MODULE_T * module, char *str, SOURCE_LINE_T ** ref_l, int *line_num)
{
  char *text = new_string (str);
  while (text != NULL && text[0] != '\0')
    {
      char *car = text;
      char *cdr = strchr (text, '!');
      int zero_line_num = 0;
      cdr[0] = '\0';
      text = &cdr[1];
      (*line_num)++;
      append_source_line (module, car, ref_l, &zero_line_num);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_source_file: read source file and make internal copy.                    */

static BOOL_T
read_source_file (MODULE_T * module)
{
  SOURCE_LINE_T *ref_l = NULL;
  int line_num = 0, k, bytes_read;
  ssize_t l;
  FILE_T f = module->files.source.fd;
  char *prelude_start, *postlude, *buffer;
/* Prelude. */
  if (module->options.stropping == BOLD_STROPPING)
    {
      prelude_start = bold_prelude_start;
      postlude = bold_postlude;
    }
  else if (module->options.stropping == QUOTE_STROPPING)
    {
      prelude_start = quote_prelude_start;
      postlude = quote_postlude;
    }
  else
    {
      prelude_start = NULL;
      postlude = NULL;
    }
  append_environ (module, prelude_start, &ref_l, &line_num);
/* Read the file into a single buffer, so we save on system calls. */
  line_num = 1;
  buffer = (char *) get_temp_heap_space ((unsigned) (8 + source_file_size));
  errno = 0;
  lseek (f, 0, SEEK_SET);
  abend (errno != 0, "error while reading source file", NULL);
  errno = 0;
  bytes_read = io_read (f, buffer, source_file_size);
  abend (errno != 0 || bytes_read != source_file_size, "error while reading source file", NULL);
#ifdef PRE_MACOS_X
/* On the Mac, newlines are \r in stead of \n. */
  for (k = 0; k < source_file_size; k++)
    {
      if (buffer[k] == '\r')
	{
	  buffer[k] = '\n';
	}
    }
#endif
/* Link all lines into the list. */
  k = 0;
  while (k < source_file_size)
    {
      l = 0;
      scan_buf[0] = '\0';
      while (k < source_file_size && buffer[k] != '\n')
	{
	  abend ((IS_CNTRL (buffer[k]) && !IS_SPACE (buffer[k])) || buffer[k] == STOP_CHAR, "error while reading source file", "check for control characters");
	  scan_buf[l++] = buffer[k++];
	  scan_buf[l] = '\0';
	}
      scan_buf[l++] = '\n';
      scan_buf[l] = '\0';
      if (k < source_file_size)
	{
	  k++;
	}
      append_source_line (module, scan_buf, &ref_l, &line_num);
    }
/* Postlude. */
  append_environ (module, postlude, &ref_l, &line_num);
  return (A_TRUE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
restore_char: resets the scanner to a point already passed.
ref_l: source line to restore.
ref_c: char pointer (in source line) to restore.                              */

static void
restore_char (SOURCE_LINE_T ** ref_l, char **ref_c)
{
  *ref_l = saved_l;
  *ref_c = saved_c;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
next_char: get next character from internal copy of source file.
ref_l: source line we're at.
ref_c: char (in source line) we're at.                                        */

static char
next_char (MODULE_T * module, SOURCE_LINE_T ** ref_l, char **ref_s)
{
  saved_l = *ref_l;
  saved_c = *ref_s;
/* Source empty? */
  if (*ref_l == NULL)
    {
      return (STOP_CHAR);
    }
  else
    {
      (*ref_l)->list = (*ref_l)->list || (module->options.nodemask & SOURCE_MASK);
/* Take new line? */
      if ((*ref_s)[0] == '\0')
	{
	  *ref_l = NEXT (*ref_l);
	  if (*ref_l != NULL)
	    {
	      *ref_s = (*ref_l)->string;
	      return ((*ref_s)[0]);
	    }
	  else
	    {
	      return (STOP_CHAR);
	    }
	}
      else
	{
/* Deliver next char. */
	  (*ref_s)++;
	  return ((*ref_s)[0]);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_good_char: find first character that can start a valid symbol.
ref_l:	source line we're at.
ref_c:	char (in source line) we're at.                                       */

static void
find_good_char (MODULE_T * module, char *ref_c, SOURCE_LINE_T ** ref_l, char **ref_s)
{
  while (*ref_c != STOP_CHAR && (IS_CNTRL (*ref_c) || (*ref_c) == ' '))
    {
      if (*ref_l != NULL)
	{
	  (*ref_l)->list = (*ref_l)->list || (module->options.nodemask & SOURCE_MASK);
	}
      *ref_c = next_char (module, ref_l, ref_s);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pragment: handle a pragment (pragmat or comment).
type: type of pragment (#, CO, COMMENT, PR, PRAGMAT).
ref_l: source line we're at.
ref_c: char (in source line) we're at. */

static BOOL_T
pragment (MODULE_T * module, int type, SOURCE_LINE_T ** ref_l, char **ref_c)
{
#define INIT_BUFFER {chars_in_buf = 0; scan_buf[chars_in_buf] = '\0';}
#define ADD_ONE_CHAR(ch) {scan_buf[chars_in_buf ++] = ch; scan_buf[chars_in_buf] = '\0';}
  char c = **ref_c, *term_s = NULL;
  SOURCE_LINE_T *start_l = *ref_l;
  int term_s_length, chars_in_buf;
  BOOL_T stop;
/* Set terminator */
  if (module->options.stropping == BOLD_STROPPING)
    {
      if (type == STYLE_I_COMMENT_SYMBOL)
	{
	  term_s = "CO";
	}
      else if (type == STYLE_II_COMMENT_SYMBOL)
	{
	  term_s = "#";
	}
      else if (type == BOLD_COMMENT_SYMBOL)
	{
	  term_s = "COMMENT";
	}
      else if (type == STYLE_I_PRAGMAT_SYMBOL)
	{
	  term_s = "PR";
	}
      else if (type == BOLD_PRAGMAT_SYMBOL)
	{
	  term_s = "PRAGMAT";
	}
    }
  else if (module->options.stropping == QUOTE_STROPPING)
    {
      if (type == STYLE_I_COMMENT_SYMBOL)
	{
	  term_s = "'CO'";
	}
      else if (type == STYLE_II_COMMENT_SYMBOL)
	{
	  term_s = "#";
	}
      else if (type == BOLD_COMMENT_SYMBOL)
	{
	  term_s = "'COMMENT'";
	}
      else if (type == STYLE_I_PRAGMAT_SYMBOL)
	{
	  term_s = "'PR'";
	}
      else if (type == BOLD_PRAGMAT_SYMBOL)
	{
	  term_s = "'PRAGMAT'";
	}
    }
  term_s_length = strlen (term_s);
/* Scan for terminator, and process pragmat items. */
  INIT_BUFFER;
  find_good_char (module, &c, ref_l, ref_c);
  stop = A_FALSE;
  while (stop == A_FALSE)
    {
      if (c == STOP_CHAR)
	{
/* Oops! We've hit EOF. */
	  diagnostic (A_SYNTAX_ERROR, NULL, "unterminated pragment L", start_l, module);
	  return (A_FALSE);
	}
      else if ((c == '"' || (c == '\'' && module->options.stropping == BOLD_STROPPING)) && (type == STYLE_I_PRAGMAT_SYMBOL || type == BOLD_PRAGMAT_SYMBOL))
	{
/* A ".." or '..' delimited string in a PRAGMAT */
	  char stop_char = c;
	  ADD_ONE_CHAR (c);
	  do
	    {
	      c = next_char (module, ref_l, ref_c);
	      if (c == '\n')
		{
		  diagnostic (A_SYNTAX_ERROR, NULL, "string exceeds end of line in pragment L", start_l, module);
		  return (A_FALSE);
		}
	      else if (IS_PRINT (c))
		{
		  ADD_ONE_CHAR (c);
		}
	    }
	  while (c != stop_char);
	}
      else if (c == '\n')
	{
/* On newline we empty the buffer and scan options when appropriate. */
	  if (type == STYLE_I_PRAGMAT_SYMBOL || type == BOLD_PRAGMAT_SYMBOL)
	    {
	      isolate_options (module, scan_buf);
	    }
	  INIT_BUFFER;
	}
      else if (IS_PRINT (c))
	{
	  ADD_ONE_CHAR (c);
	}
      if (chars_in_buf >= term_s_length)
	{
/* Check whether we encountered the terminator. */
	  stop = (strcmp (term_s, &(scan_buf[chars_in_buf - term_s_length])) == 0);
	}
      c = next_char (module, ref_l, ref_c);
    }
  scan_buf[chars_in_buf - term_s_length] = '\0';
  return (A_TRUE);
#undef ADD_ONE_CHAR
#undef INIT_BUFFER
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_format_item: attribute for format item.                                   */

static int
get_format_item (char ch)
{
  switch (ch)
    {
    case 'a':
    case 'A':
      return (FORMAT_ITEM_A);
    case 'b':
    case 'B':
      return (FORMAT_ITEM_B);
    case 'c':
    case 'C':
      return (FORMAT_ITEM_C);
    case 'd':
    case 'D':
      return (FORMAT_ITEM_D);
    case 'e':
    case 'E':
      return (FORMAT_ITEM_E);
    case 'f':
    case 'F':
      return (FORMAT_ITEM_F);
    case 'g':
    case 'G':
      return (FORMAT_ITEM_G);
    case 'h':
    case 'H':
      return (FORMAT_ITEM_H);
    case 'i':
    case 'I':
      return (FORMAT_ITEM_I);
    case 'j':
    case 'J':
      return (FORMAT_ITEM_J);
    case 'k':
    case 'K':
      return (FORMAT_ITEM_K);
    case 'l':
    case 'L':
    case '/':
      return (FORMAT_ITEM_L);
    case 'm':
    case 'M':
      return (FORMAT_ITEM_M);
    case 'n':
    case 'N':
      return (FORMAT_ITEM_N);
    case 'o':
    case 'O':
      return (FORMAT_ITEM_O);
    case 'p':
    case 'P':
      return (FORMAT_ITEM_P);
    case 'q':
    case 'Q':
      return (FORMAT_ITEM_Q);
    case 'r':
    case 'R':
      return (FORMAT_ITEM_R);
    case 's':
    case 'S':
      return (FORMAT_ITEM_S);
    case 't':
    case 'T':
      return (FORMAT_ITEM_T);
    case 'u':
    case 'U':
      return (FORMAT_ITEM_U);
    case 'v':
    case 'V':
      return (FORMAT_ITEM_V);
    case 'w':
    case 'W':
      return (FORMAT_ITEM_W);
    case 'x':
    case 'X':
      return (FORMAT_ITEM_X);
    case 'y':
    case 'Y':
      return (FORMAT_ITEM_Y);
    case 'z':
    case 'Z':
      return (FORMAT_ITEM_Z);
    case '+':
      return (FORMAT_ITEM_PLUS);
    case '-':
      return (FORMAT_ITEM_MINUS);
    case '.':
      return (FORMAT_ITEM_POINT);
    case '%':
      return (FORMAT_ITEM_ESCAPE);
    default:
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

#define SKIP_WHITE while (IS_SPACE (c)) {c = next_char (module, ref_l, ref_s);}

#define SCAN_EXPONENT_PART\
  (sym++)[0] = 'E';\
  c = next_char (module, ref_l, ref_s);\
  if (c == '+' || c == '-') {\
    (sym++)[0] = c;\
    c = next_char (module, ref_l, ref_s);\
  }\
  if (!IS_DIGIT (c)) {\
    diagnostic (A_SYNTAX_ERROR, NULL, "digit expected L; '0' assumed", *ref_l, module);\
    (sym++)[0] = '0';\
  }\
  while (IS_DIGIT (c)) {\
    (sym++)[0] = c;\
    c = next_char (module, ref_l, ref_s);\
  }

#define EXPONENT_CHARACTER(c) (TO_UPPER (c) == EXPONENT_CHAR || (module->options.stropping == QUOTE_STROPPING && (c) == '\\'))
#define RADIX_CHARACTER(c) (TO_UPPER (c) == RADIX_CHAR)

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_next_token: get next token from internal copy of source file.
in_format: are we scanning a format text.
ref_l: source line we're at.
ref_c: char (in source line) we're at.
start_l: line where token starts.
start_c: character where token starts.
att: attribute designated to token. */

static void
get_next_token (MODULE_T * module, BOOL_T in_format, SOURCE_LINE_T ** ref_l, char **ref_s, SOURCE_LINE_T ** start_l, char **start_c, int *att)
{
  char c = **ref_s, *sym = scan_buf, *operators;
  if (module->options.stropping == BOLD_STROPPING)
    {
      operators = "!%^&?+-~<>/*";
    }
  else if (module->options.stropping == QUOTE_STROPPING)
    {
      operators = "%^&?+-~<>/*";
    }
  else
    {
      operators = NULL;
    }
  sym[0] = '\0';
  find_good_char (module, &c, ref_l, ref_s);
  *start_l = *ref_l;
  *start_c = *ref_s;
  if (c == STOP_CHAR)
    {
/* We are at EOF. */
      (sym++)[0] = STOP_CHAR;
      sym[0] = '\0';
      return;
    }
/*------------+
| In a format |
+------------*/
  if (in_format)
    {
      char *format_items;
      if (module->options.stropping == BOLD_STROPPING)
	{
	  format_items = "/%\\+-.abcdefghijklmnopqrstuvwxyz";
	}
      else
	{			/* if (module->options.stropping == QUOTE_STROPPING) */
	  format_items = "/%\\+-.ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
      if (strchr (format_items, c) != NULL)
	{
/* General format items. */
	  (sym++)[0] = c;
	  sym[0] = '\0';
	  *att = get_format_item (c);
	  next_char (module, ref_l, ref_s);
	  return;
	}
      if (IS_DIGIT (c))
	{
/* INT denoter for static replicator. */
	  while (IS_DIGIT (c))
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	    }
	  sym[0] = '\0';
	  *att = STATIC_REPLICATOR;
	  return;
	}
    }
/*----------------+
| Not in a format |
+----------------*/
  if (IS_UPPER (c))
    {
      if (module->options.stropping == BOLD_STROPPING)
	{
/* Upper case word - bold tag. */
	  while (IS_UPPER (c) || c == '_')
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	    }
	  sym[0] = '\0';
	  *att = BOLD_TAG;
	}
      else if (module->options.stropping == QUOTE_STROPPING)
	{
	  while (IS_UPPER (c) || IS_DIGIT (c) || c == '_')
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	      SKIP_WHITE;
	    }
	  sym[0] = '\0';
	  *att = IDENTIFIER;
	}
    }
  else if (c == '\'')
    {
/* Quote, uppercase word, quote - bold tag. */
      int k = 0;
      c = next_char (module, ref_l, ref_s);
      while (IS_UPPER (c) || IS_DIGIT (c) || c == '_')
	{
	  (sym++)[0] = c;
	  k++;
	  c = next_char (module, ref_l, ref_s);
	  SKIP_WHITE;
	}
      if (k == 0)
	{
	  diagnostic (A_SYNTAX_ERROR, NULL, "quoted bold tag expected L", *ref_l, module);
	}
      sym[0] = '\0';
      *att = BOLD_TAG;
/* Skip terminating quote, or complain if it is not there. */
      if (c == '\'')
	{
	  c = next_char (module, ref_l, ref_s);
	}
      else
	{
	  diagnostic (A_SYNTAX_ERROR, NULL, "terminating quote expected L", *ref_l, module);
	}
    }
  else if (IS_LOWER (c))
    {
/* Lower case word - identifier. */
      while (IS_LOWER (c) || IS_DIGIT (c) || c == '_')
	{
	  (sym++)[0] = c;
	  c = next_char (module, ref_l, ref_s);
	  SKIP_WHITE;
	}
      sym[0] = '\0';
      *att = IDENTIFIER;
    }
  else if (c == '.')
    {
/* Begins with a point symbol - point, dotdot, L REAL denoter. */
      c = next_char (module, ref_l, ref_s);
      if (c == '.')
	{
	  (sym++)[0] = '.';
	  (sym++)[0] = '.';
	  sym[0] = '\0';
	  *att = DOTDOT_SYMBOL;
	  c = next_char (module, ref_l, ref_s);
	}
      else if (!IS_DIGIT (c))
	{
	  (sym++)[0] = '.';
	  sym[0] = '\0';
	  *att = POINT_SYMBOL;
	}
      else
	{
	  (sym++)[0] = '0';
	  (sym++)[0] = '.';
	  while (IS_DIGIT (c))
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	    }
	  if (EXPONENT_CHARACTER (c))
	    {
	      SCAN_EXPONENT_PART;
	    }
	  sym[0] = '\0';
	  *att = REAL_DENOTER;
	}
    }
  else if (IS_DIGIT (c))
    {
/* Something that begins with a digit - L INT denoter, L REAL denoter. */
      while (IS_DIGIT (c))
	{
	  (sym++)[0] = c;
	  c = next_char (module, ref_l, ref_s);
	}
      if (c == '.')
	{
	  c = next_char (module, ref_l, ref_s);
	  if (c == '.')
	    {
	      restore_char (ref_l, ref_s);
	      sym[0] = '\0';
	      *att = INT_DENOTER;
	    }
	  else if (EXPONENT_CHARACTER (c))
	    {
	      (sym++)[0] = '.';
	      (sym++)[0] = '0';
	      SCAN_EXPONENT_PART;
	      *att = REAL_DENOTER;
	    }
	  else if (!IS_DIGIT (c))
	    {
	      restore_char (ref_l, ref_s);
	      sym[0] = '\0';
	      *att = INT_DENOTER;
	    }
	  else
	    {
	      (sym++)[0] = '.';
	      while (IS_DIGIT (c))
		{
		  (sym++)[0] = c;
		  c = next_char (module, ref_l, ref_s);
		}
	      if (EXPONENT_CHARACTER (c))
		{
		  SCAN_EXPONENT_PART;
		}
	      *att = REAL_DENOTER;
	    }
	}
      else if (EXPONENT_CHARACTER (c))
	{
	  SCAN_EXPONENT_PART;
	  *att = REAL_DENOTER;
	}
      else if (RADIX_CHARACTER (c))
	{
	  (sym++)[0] = c;
	  c = next_char (module, ref_l, ref_s);
	  while (IS_DIGIT (c) || IS_LOWER (c) || IS_UPPER (c))
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	    }
	  *att = BITS_DENOTER;
	}
      else
	{
	  *att = INT_DENOTER;
	}
      sym[0] = '\0';
    }
  else if (c == '"')
    {
/* STRING denoter. */
      BOOL_T stop = A_FALSE;
      while (!stop)
	{
	  if (*ref_l == NULL)
	    {
	      diagnostic (A_SYNTAX_ERROR, NULL, "unterminated string L", *start_l, module);
	    }
	  c = next_char (module, ref_l, ref_s);
	  while (*ref_l != NULL && c != '"' && c != STOP_CHAR)
	    {
	      if (c == '\n')
		{
		  diagnostic (A_SYNTAX_ERROR, NULL, "string L exceeds end of line", *start_l, module);
		  *att = in_format ? LITERAL : ROW_CHAR_DENOTER;
		  *scan_buf = STOP_CHAR;
		  return;
		}
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	      if (*ref_l == NULL)
		{
		  diagnostic (A_SYNTAX_ERROR, NULL, "unterminated string L", *start_l, module);
		  *att = in_format ? LITERAL : ROW_CHAR_DENOTER;
		  *scan_buf = STOP_CHAR;
		  return;
		}
	    }
	  if ((c = next_char (module, ref_l, ref_s)) == '"')
	    {
	      (sym++)[0] = '\"';
	    }
	  else
	    {
	      stop = A_TRUE;
	    }
	}
      sym[0] = '\0';
      *att = in_format ? LITERAL : ROW_CHAR_DENOTER;
    }
  else if (strchr ("#$()[]{},;@", c) != NULL)
    {
/* Single character symbols. */
      (sym++)[0] = c;
      next_char (module, ref_l, ref_s);
      sym[0] = '\0';
      *att = 0;
    }
  else if (c == '|')
    {
/* Bar. */
      (sym++)[0] = c;
      c = next_char (module, ref_l, ref_s);
      if (c == ':')
	{
	  (sym++)[0] = c;
	  next_char (module, ref_l, ref_s);
	}
      sym[0] = '\0';
      *att = 0;
    }
  else if (c == '!' && module->options.stropping == QUOTE_STROPPING)
    {
/* Bar, will be replaced with modern variant. */
      (sym++)[0] = '|';
      c = next_char (module, ref_l, ref_s);
      if (c == ':')
	{
	  (sym++)[0] = c;
	  next_char (module, ref_l, ref_s);
	}
      sym[0] = '\0';
      *att = 0;
    }
  else if (c == ':')
    {
/* Colon, semicolon, IS, ISNT. */
      (sym++)[0] = c;
      c = next_char (module, ref_l, ref_s);
      if (c == '=')
	{
	  (sym++)[0] = c;
	  if ((c = next_char (module, ref_l, ref_s)) == ':')
	    {
	      (sym++)[0] = c;
	      c = next_char (module, ref_l, ref_s);
	    }
	}
      else if (c == '/')
	{
	  (sym++)[0] = c;
	  if ((c = next_char (module, ref_l, ref_s)) == '=')
	    {
	      (sym++)[0] = c;
	      if ((c = next_char (module, ref_l, ref_s)) == ':')
		{
		  (sym++)[0] = c;
		  c = next_char (module, ref_l, ref_s);
		}
	    }
	}
      sym[0] = '\0';
      *att = 0;
    }
  else if (c == '=')
    {
/* Operator starting with "=". */
      char *scanned = sym;
      (sym++)[0] = c;
      c = next_char (module, ref_l, ref_s);
      if (strchr ("<>/*=", c) != NULL)
	{
	  (sym++)[0] = c;
	  c = next_char (module, ref_l, ref_s);
	}
      if (c == '=')
	{
	  (sym++)[0] = c;
	  if (next_char (module, ref_l, ref_s) == ':')
	    {
	      (sym++)[0] = ':';
	      c = next_char (module, ref_l, ref_s);
	      if (strlen (sym) < 4 && c == '=')
		{
		  (sym++)[0] = '=';
		  next_char (module, ref_l, ref_s);
		}
	    }
	}
      else if (c == ':')
	{
	  (sym++)[0] = c;
	  sym[0] = '\0';
	  if (next_char (module, ref_l, ref_s) == '=')
	    {
	      (sym++)[0] = '=';
	      next_char (module, ref_l, ref_s);
	    }
	  else if (!(strcmp (scanned, "=:") == 0 || strcmp (scanned, "==:") == 0))
	    {
	      sym[0] = '\0';
	      diagnostic (A_SYNTAX_ERROR, NULL, "expected operator Z to end in Z L", scanned, ":=", *ref_l, module);
	    }
	}
      sym[0] = '\0';
      if (strcmp (scanned, "=") == 0)
	{
	  *att = EQUALS_SYMBOL;
	}
      else
	{
	  *att = OPERATOR;
	}
    }
  else if (strchr (operators, c) != NULL)
    {
/* Operator. */
      char *scanned = sym;
      (sym++)[0] = c;
      c = next_char (module, ref_l, ref_s);
      if (strchr ("<>/*=", c) != NULL)
	{
	  (sym++)[0] = c;
	  c = next_char (module, ref_l, ref_s);
	}
      if (c == '=')
	{
	  (sym++)[0] = c;
	  if (next_char (module, ref_l, ref_s) == ':')
	    {
	      (sym++)[0] = ':';
	      c = next_char (module, ref_l, ref_s);
	      if (strlen (scanned) < 4 && c == '=')
		{
		  (sym++)[0] = '=';
		  next_char (module, ref_l, ref_s);
		}
	    }
	}
      else if (c == ':')
	{
	  (sym++)[0] = c;
	  sym[0] = '\0';
	  if (next_char (module, ref_l, ref_s) == '=')
	    {
	      (sym++)[0] = '=';
	      sym[0] = '\0';
	      next_char (module, ref_l, ref_s);
	    }
	  else if (strcmp (&(scanned[1]), "=:") != 0)
	    {
	      sym[0] = '\0';
	      diagnostic (A_SYNTAX_ERROR, NULL, "expected operator Z to end in Z L", scanned, ":=", *ref_l, module);
	    }
	}
      sym[0] = '\0';
      *att = OPERATOR;
    }
  else
    {
/* Afuuus ... give a warning. */
      if ((int) c >= 32)
	{
	  char str[16];
	  sprintf (str, "\"%c\"", c);
	  diagnostic (A_WARNING, NULL, "unworthy character Z L", str, *ref_l, module);
	}
      else
	{
	  diagnostic (A_WARNING, NULL, "unworthy character D L", c, *ref_l, module);
	}
      (sym++)[0] = c;
      next_char (module, ref_l, ref_s);
      sym[0] = '\0';
      *att = 0;
    }
#undef SKIP_WHITE
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
open_embedded_clause: whether att opens an embedded clause.                   */

static BOOL_T
open_embedded_clause (int att)
{
  switch (att)
    {
    case OPEN_SYMBOL:
    case BEGIN_SYMBOL:
    case PAR_SYMBOL:
    case IF_SYMBOL:
    case CASE_SYMBOL:
    case FOR_SYMBOL:
    case FROM_SYMBOL:
    case BY_SYMBOL:
    case TO_SYMBOL:
    case WHILE_SYMBOL:
    case DO_SYMBOL:
    case SUB_SYMBOL:
    case ACCO_SYMBOL:
      {
	return (A_TRUE);
      }
    default:
      {
	return (A_FALSE);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
close_embedded_clause: whether att closes an embedded clause.                 */

static BOOL_T
close_embedded_clause (int att)
{
  switch (att)
    {
    case CLOSE_SYMBOL:
    case END_SYMBOL:
    case FI_SYMBOL:
    case ESAC_SYMBOL:
    case OD_SYMBOL:
    case BUS_SYMBOL:
    case OCCA_SYMBOL:
      {
	return (A_TRUE);
      }
    default:
      {
	return (A_FALSE);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_lower_case: cast a string to lower case.                                 */

static void
make_lower_case (char *p)
{
  while (p[0] != '\0')
    {
      p[0] = TO_LOWER (p[0]);
      p++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tokenise_source: construct a linear list of tokens.                           */

static void
tokenise_source (MODULE_T * module, NODE_T ** root, int level, BOOL_T in_format, SOURCE_LINE_T ** l, char **s, SOURCE_LINE_T ** start_l, char **start_c)
{
  while (l != NULL && !stop_scanner)
    {
      int att = 0;
      get_next_token (module, in_format, l, s, start_l, start_c, &att);
      if (scan_buf[0] == STOP_CHAR)
	{
	  stop_scanner = A_TRUE;
	}
      else if (strlen (scan_buf) > 0 || att == ROW_CHAR_DENOTER || att == LITERAL)
	{
	  KEYWORD_T *kw = find_keyword (top_keyword, scan_buf);
	  char *c = NULL;
	  BOOL_T make_node = A_TRUE;
	  if (!(kw != NULL && att != ROW_CHAR_DENOTER))
	    {
	      if (att == IDENTIFIER)
		{
		  make_lower_case (scan_buf);
		}
	      c = add_token (&top_token, scan_buf)->text;
	    }
	  else
	    {
	      if (WHETHER (kw, TO_SYMBOL))
		{
/* Merge GO and TO to GOTO. */
		  if (*root != NULL && WHETHER (*root, GO_SYMBOL))
		    {
		      ATTRIBUTE (*root) = GOTO_SYMBOL;
		      SYMBOL (*root) = find_keyword (top_keyword, "GOTO")->text;
		      make_node = A_FALSE;
		    }
		  else
		    {
		      att = ATTRIBUTE (kw);
		      c = kw->text;
		    }
		}
	      else
		{
		  if (att == 0 || att == BOLD_TAG)
		    {
		      att = ATTRIBUTE (kw);
		    }
		  c = kw->text;
/* Handle pragments. */
		  if (att == STYLE_II_COMMENT_SYMBOL || att == STYLE_I_COMMENT_SYMBOL || att == BOLD_COMMENT_SYMBOL)
		    {
		      stop_scanner = !pragment (module, ATTRIBUTE (kw), l, s);
		      make_node = A_FALSE;
		    }
		  else if (att == STYLE_I_PRAGMAT_SYMBOL || att == BOLD_PRAGMAT_SYMBOL)
		    {
		      stop_scanner = !pragment (module, ATTRIBUTE (kw), l, s);
		      if (!stop_scanner)
			{
			  isolate_options (module, scan_buf);
			  set_options (module, module->options.list, A_FALSE);
			  make_node = A_FALSE;
			}
		    }
		}
	    }
/* Add token to the tree. */
	  if (make_node)
	    {
	      NODE_T *q = new_node ();
	      MASK (q) = module->options.nodemask;
	      LINE (q) = *start_l;
	      if ((*start_l)->top_node == NULL)
		{
		  (*start_l)->top_node = q;
		}
	      q->info->char_in_line = *start_c;
	      PRIO (q->info) = 0;
	      q->info->PROCEDURE_LEVEL = 0;
	      q->info->PROCEDURE_NUMBER = 0;
	      ATTRIBUTE (q) = att;
	      SYMBOL (q) = c;
	      PREVIOUS (q) = *root;
	      SUB (q) = NEXT (q) = NULL;
	      SYMBOL_TABLE (q) = NULL;
	      q->info->module = module;
	      MOID (q) = NULL;
	      TAX (q) = NULL;
	      if (*root != NULL)
		{
		  NEXT (*root) = q;
		}
	      if (module->top_node == NULL)
		{
		  module->top_node = q;
		}
	      *root = q;
	    }
/* Redirection in tokenising formats. The scanner is a recursive-descent type as
   to know when it scans a format text and when not.                          */
	  if (in_format && att == FORMAT_DELIMITER_SYMBOL)
	    {
	      return;
	    }
	  else if (!in_format && att == FORMAT_DELIMITER_SYMBOL)
	    {
	      tokenise_source (module, root, level + 1, A_TRUE, l, s, start_l, start_c);
	    }
	  else if (in_format && open_embedded_clause (att))
	    {
	      NODE_T *z = PREVIOUS (*root);
	      if (z != NULL && (WHETHER (z, FORMAT_ITEM_N) || WHETHER (z, FORMAT_ITEM_G) || WHETHER (z, FORMAT_ITEM_F)))
		{
		  tokenise_source (module, root, level, A_FALSE, l, s, start_l, start_c);
		}
	      else if (att == OPEN_SYMBOL)
		{
		  ATTRIBUTE (*root) = FORMAT_ITEM_OPEN;
		}
	      else if (module->options.brackets && att == SUB_SYMBOL)
		{
		  ATTRIBUTE (*root) = FORMAT_ITEM_OPEN;
		}
	      else if (module->options.brackets && att == ACCO_SYMBOL)
		{
		  ATTRIBUTE (*root) = FORMAT_ITEM_OPEN;
		}
	    }
	  else if (!in_format && level > 0 && open_embedded_clause (att))
	    {
	      tokenise_source (module, root, level + 1, A_FALSE, l, s, start_l, start_c);
	    }
	  else if (!in_format && level > 0 && close_embedded_clause (att))
	    {
	      return;
	    }
	  else if (in_format && att == CLOSE_SYMBOL)
	    {
	      ATTRIBUTE (*root) = FORMAT_ITEM_CLOSE;
	    }
	  else if (module->options.brackets && in_format && att == BUS_SYMBOL)
	    {
	      ATTRIBUTE (*root) = FORMAT_ITEM_CLOSE;
	    }
	  else if (module->options.brackets && in_format && att == OCCA_SYMBOL)
	    {
	      ATTRIBUTE (*root) = FORMAT_ITEM_CLOSE;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
lexical_analyzer: tokenise source file, build initial syntax tree.
returns: whether tokenising ended satisfactorily.                             */

BOOL_T
lexical_analyzer (MODULE_T * module)
{
  SOURCE_LINE_T *l, *start_l = NULL;
  char *s = NULL, *start_c = NULL;
  NODE_T *root = NULL;
  scan_buf = NULL;
  max_scan_buf_length = source_file_size = get_source_size (module);
/* Errors in file? */
  if (max_scan_buf_length == 0)
    {
      return (A_FALSE);
    }
  max_scan_buf_length += strlen (bold_prelude_start) + strlen (bold_postlude);
  max_scan_buf_length += strlen (quote_prelude_start) + strlen (quote_postlude);
/* Allocate a scan buffer with 8 bytes extra space. */
  scan_buf = (char *) get_temp_heap_space ((unsigned) (8 + max_scan_buf_length));
/* Errors in file? */
  if (!read_source_file (module))
    {
      return (A_FALSE);
    }
/* Start tokenising. */
  read_error = A_FALSE;
  stop_scanner = A_FALSE;
  if ((l = module->top_line) != NULL)
    {
      s = l->string;
    }
  tokenise_source (module, &root, 0, A_FALSE, &l, &s, &start_l, &start_c);
  return (A_TRUE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
This is a small refinement preprocessor for Algol68G.                         */

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_refinement_terminator*/

static BOOL_T
whether_refinement_terminator (NODE_T * p)
{
  if (WHETHER (p, POINT_SYMBOL))
    {
      if (IN_PRELUDE (NEXT (p)))
	{
	  return (A_TRUE);
	}
      else
	{
	  return (whether (p, POINT_SYMBOL, IDENTIFIER, COLON_SYMBOL, 0));
	}
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_refinements: get refinement definitions in the internal source.           */

void
get_refinements (MODULE_T * z)
{
  NODE_T *p = z->top_node;
  z->top_refinement = NULL;
/* First look where the prelude ends. */
  while (p != NULL && IN_PRELUDE (p))
    {
      p = NEXT (p);
    }
/* Determine whether the program contains refinements at all. */
  while (p != NULL && !IN_PRELUDE (p) && !whether_refinement_terminator (p))
    {
      p = NEXT (p);
    }
  if (p == NULL || IN_PRELUDE (p))
    {
      return;
    }
/* Apparently this is code with refinements. */
  p = NEXT (p);
  if (p == NULL || IN_PRELUDE (p))
    {
/* Ok, we accept a program with no refinements as well. */
      return;
    }
  while (p != NULL && !IN_PRELUDE (p) && whether (p, IDENTIFIER, COLON_SYMBOL, 0))
    {
      REFINEMENT_T *new_one = (REFINEMENT_T *) get_fixed_heap_space (SIZE_OF (REFINEMENT_T)), *x;
      BOOL_T exists;
      NEXT (new_one) = NULL;
      new_one->name = SYMBOL (p);
      new_one->applications = 0;
      new_one->line_defined = LINE (p);
      new_one->line_applied = NULL;
      new_one->tree = p;
      new_one->begin = NULL;
      new_one->end = NULL;
      p = NEXT (NEXT (p));
      if (p == NULL)
	{
	  diagnostic (A_SYNTAX_ERROR, NULL, "empty refinement at end of program", NULL);
	  return;
	}
      else
	{
	  new_one->begin = p;
	}
      while (p != NULL && ATTRIBUTE (p) != POINT_SYMBOL)
	{
	  new_one->end = p;
	  p = NEXT (p);
	}
      if (p == NULL)
	{
	  diagnostic (A_SYNTAX_ERROR, NULL, "point expected at end of program", NULL);
	  return;
	}
      else
	{
	  p = NEXT (p);
	}
/* Do we already have one by this name. */
      x = z->top_refinement;
      exists = A_FALSE;
      while (x != NULL && !exists)
	{
	  if (x->name == new_one->name)
	    {
	      diagnostic (A_SYNTAX_ERROR, new_one->tree, "refinement already defined", NULL);
	      exists = A_TRUE;
	    }
	  x = NEXT (x);
	}
/* Straight insertion in chain */
      if (!exists)
	{
	  NEXT (new_one) = z->top_refinement;
	  z->top_refinement = new_one;
	}
    }
  if (p != NULL && !IN_PRELUDE (p))
    {
      diagnostic (A_SYNTAX_ERROR, p, "invalid refinement definition", NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
put_refinements: put refinement applications in the internal source.          */

void
put_refinements (MODULE_T * z)
{
  REFINEMENT_T *x;
  NODE_T *p, *point;
/* If there are no refinements, there's little to do. */
  if (z->top_refinement == NULL)
    {
      return;
    }
/* Initialisation. */
  x = z->top_refinement;
  while (x != NULL)
    {
      x->applications = 0;
      x = NEXT (x);
    }
/* Before we introduce infinite loops, find where closing-prelude starts. */
  p = z->top_node;
  while (p != NULL && IN_PRELUDE (p))
    {
      p = NEXT (p);
    }
  while (p != NULL && !IN_PRELUDE (p))
    {
      p = NEXT (p);
    }
  abend (p == NULL, INTERNAL_ERROR, NULL);
  point = p;
/* We need to substitute until the first point. */
  p = z->top_node;
  while (p != NULL && ATTRIBUTE (p) != POINT_SYMBOL)
    {
      if (WHETHER (p, IDENTIFIER))
	{
/* See if we can find its definition. */
	  REFINEMENT_T *y = NULL;
	  x = z->top_refinement;
	  while (x != NULL && y == NULL)
	    {
	      if (x->name == SYMBOL (p))
		{
		  y = x;
		}
	      else
		{
		  x = NEXT (x);
		}
	    }
	  if (y != NULL)
	    {
/* We found its definition. */
	      y->applications++;
	      if (y->applications > 1)
		{
		  diagnostic (A_SYNTAX_ERROR, y->line_defined->top_node, "refinement applied more than once", NULL);
		  p = NEXT (p);
		}
	      else
		{
/* Tie the definition in the tree. */
		  y->line_applied = LINE (p);
		  if (PREVIOUS (p) != NULL)
		    {
		      NEXT (PREVIOUS (p)) = y->begin;
		    }
		  if (y->begin != NULL)
		    {
		      PREVIOUS (y->begin) = PREVIOUS (p);
		    }
		  if (NEXT (p) != NULL)
		    {
		      PREVIOUS (NEXT (p)) = y->end;
		    }
		  if (y->end != NULL)
		    {
		      NEXT (y->end) = NEXT (p);
		    }
		  p = y->begin;	/* So we can substitute the refinements within. */
		}
	    }
	  else
	    {
	      p = NEXT (p);
	    }
	}
      else
	{
	  p = NEXT (p);
	}
    }
/* After the point we ignore it all until the prelude. */
  if (p != NULL && WHETHER (p, POINT_SYMBOL))
    {
      if (PREVIOUS (p) != NULL)
	{
	  NEXT (PREVIOUS (p)) = point;
	}
      if (PREVIOUS (point) != NULL)
	{
	  PREVIOUS (point) = PREVIOUS (p);
	}
    }
  else
    {
      diagnostic (A_SYNTAX_ERROR, p, "point expected", NULL);
    }
/* Has the programmer done it well? */
  if (error_count == 0)
    {
      x = z->top_refinement;
      while (x != NULL)
	{
	  if (x->applications == 0)
	    {
	      diagnostic (A_SYNTAX_ERROR, x->line_defined->top_node, "refinement is not applied", NULL);
	    }
	  x = NEXT (x);
	}
    }
}
