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

#include "algol68g.h"
#include "genie.h"
#include "transput.h"
#include "mp.h"

/*-------1---------2---------3---------4---------5---------6---------7---------+
Transput library - Formatted transput

In Algol68G, a value of mode FORMAT looks like a routine text. The value 
comprises a pointer to its environment in the stack, and a pointer where the 
format text is at in the syntax tree.                                         */

#define INT_DIGITS "0123456789"
#define BITS_DIGITS "0123456789abcdefABCDEF"
#define INT_DIGITS_BLANK " 0123456789"
#define BITS_DIGITS_BLANK " 0123456789abcdefABCDEF"
#define SIGN_DIGITS " +-"

/*-------1---------2---------3---------4---------5---------6---------7---------+
format_error: handle format error event.                                      */

void
format_error (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  A68_BOOL z;
  on_event_handler (p, f->format_error_mended, ref_file);
  POP_BOOL (p, &z);
  if (z.value == A_FALSE)
    {
      diagnostic (A_RUNTIME_ERROR, p, "format error; picture without argument to transput");
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
initialise_collitems: initialise processing of pictures.

Every picture has a counter that says whether it has not been used OR the number
of times it can still be used.                                                */

static void
initialise_collitems (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, PICTURE))
	{
	  A68_COLLITEM *z = (A68_COLLITEM *) FRAME_LOCAL (frame_pointer, TAX (p)->offset);
	  z->status = INITIALISED_MASK;
	  z->count = ITEM_NOT_USED;
	}
/* Don't dive into f, g, n frames and collections. */
      if (!(WHETHER (p, ENCLOSED_CLAUSE) || WHETHER (p, COLLECTION)))
	{
	  initialise_collitems (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
open_format_frame: initialise processing of format text.

Open a new frame for the format text and save for return to embedding one.    */

static void
open_format_frame (A68_FILE * file, A68_FORMAT * fmt, BOOL_T embedded, BOOL_T init)
{
  NODE_T *dollar = SUB (fmt->top);
  A68_FORMAT *save;
  open_frame (dollar, IS_PROCEDURE_PARM, fmt->environ.offset);
/* Save old format */
  save = (A68_FORMAT *) FRAME_LOCAL (frame_pointer, TAX (dollar)->offset);
  *save = (embedded == EMBEDDED_FORMAT ? file->format : nil_format);
  file->format = *fmt;
/* Reset all collitems */
  if (init)
    {
      initialise_collitems (dollar);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
end_of_format: handle end-of-format event.

Format-items return immediately to the embedding format text. The outermost
format text calls "on format end".                                            */

int
end_of_format (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  NODE_T *dollar = SUB (file->format.top);
  A68_FORMAT *save = (A68_FORMAT *) FRAME_LOCAL (frame_pointer, TAX (dollar)->offset);
  if (IS_NIL_FORMAT (save))
    {
/* Not embedded, outermost format: execute event routine. */
      A68_BOOL z;
      on_event_handler (p, (FILE_DEREF (&ref_file))->format_end_mended, ref_file);
      POP_BOOL (p, &z);
      if (z.value == A_FALSE)
	{
/* Restart format. */
	  frame_pointer = file->frame_pointer;
	  stack_pointer = file->stack_pointer;
	  open_format_frame (file, &(file->format), NOT_EMBEDDED_FORMAT, A_TRUE);
	}
      return (NOT_EMBEDDED_FORMAT);
    }
  else
    {
/* Embedded format, return to embedding format, cf. RR. */
      CLOSE_FRAME;
      file->format = *save;
      return (EMBEDDED_FORMAT);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_replicator_value: return integral value of replicator.                    */

int
get_replicator_value (NODE_T * p)
{
  int z = 0;
  if (WHETHER (p, STATIC_REPLICATOR))
    {
      A68_INT u;
      if (genie_string_to_value_internal (p, MODE (INT), SYMBOL (p), (BYTE_T *) & u) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, MODE (INT));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      z = u.value;
    }
  else if (WHETHER (p, DYNAMIC_REPLICATOR))
    {
      A68_INT u;
      EXECUTE_UNIT (NEXT_SUB (p));
      POP_INT (p, &u);
      z = u.value;
    }
  else if (WHETHER (p, REPLICATOR))
    {
      z = get_replicator_value (SUB (p));
    }
  return (z >= 0 ? z : 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_format_pattern: return: first available pattern.                         */

static NODE_T *
scan_format_pattern (NODE_T * p, A68_REF ref_file)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, PICTURE_LIST))
	{
	  NODE_T *prio = scan_format_pattern (SUB (p), ref_file);
	  if (prio != NULL)
	    {
	      return (prio);
	    }
	}
      if (WHETHER (p, PICTURE))
	{
	  NODE_T *picture = SUB (p);
	  A68_COLLITEM *collitem = (A68_COLLITEM *) FRAME_LOCAL (frame_pointer, TAX (p)->offset);
	  if (collitem->count != 0)
	    {
	      if (WHETHER (picture, PATTERN))
		{
		  collitem->count = 0;	/* This pattern is now done. */
		  picture = SUB (picture);
		  if (ATTRIBUTE (picture) != FORMAT_PATTERN)
		    {
		      return (picture);
		    }
		  else
		    {
		      NODE_T *pat;
		      A68_FORMAT z;
		      A68_FILE *file = FILE_DEREF (&ref_file);
		      EXECUTE_UNIT (NEXT_SUB (picture));
		      POP (p, &z, SIZE_OF (A68_FORMAT));
		      open_format_frame (file, &z, EMBEDDED_FORMAT, A_TRUE);
		      pat = scan_format_pattern (SUB (file->format.top), ref_file);
		      if (pat != NULL)
			{
			  return (pat);
			}
		      else
			{
			  (void) end_of_format (p, ref_file);
			}
		    }
		}
	      else if (WHETHER (picture, INSERTION))
		{
		  A68_FILE *file = FILE_DEREF (&ref_file);
		  if (file->read_mood)
		    {
		      read_insertion (picture, ref_file);
		    }
		  else if (file->write_mood)
		    {
		      write_insertion (picture, ref_file, INSERTION_NORMAL);
		    }
		  else
		    {
		      abend (A_TRUE, "undetermined mood for insertion", NULL);
		    }
		  collitem->count = 0;	/* This insertion is now done. */
		}
	      else if (WHETHER (picture, REPLICATOR) || WHETHER (picture, COLLECTION))
		{
		  BOOL_T go_on = A_TRUE;
		  NODE_T *select = NULL;
		  if (collitem->count == ITEM_NOT_USED)
		    {
		      if (WHETHER (picture, REPLICATOR))
			{
			  collitem->count = get_replicator_value (SUB (p));
			  picture = NEXT (picture);
			}
		      else
			{
			  collitem->count = 1;
			}
		      initialise_collitems (NEXT_SUB (picture));
		    }
		  else if (WHETHER (picture, REPLICATOR))
		    {
		      picture = NEXT (picture);
		    }
		  while (go_on)
		    {
/* Get format item from collection. If collection is done, but repitition is not,
   then re-initialise the collection and repeat. */
		      select = scan_format_pattern (NEXT_SUB (picture), ref_file);
		      if (select != NULL)
			{
			  return (select);
			}
		      else
			{
			  collitem->count--;
			  go_on = collitem->count > 0;
			  if (go_on)
			    {
			      initialise_collitems (NEXT_SUB (picture));
			    }
			}
		    }
		}
	    }
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_next_format_pattern: return first available pattern.

"mood" can be WANT_PATTERN: pattern needed by caller, so perform end-of-format 
if needed or SKIP_PATTERN: just emptying current pattern/collection/format.   */

NODE_T *
get_next_format_pattern (NODE_T * p, A68_REF ref_file, BOOL_T mood)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  if (file->format.top == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, "patterns exhausted in format");
      exit_genie (p, A_RUNTIME_ERROR);
      return (NULL);
    }
  else
    {
      NODE_T *pat = scan_format_pattern (SUB (file->format.top), ref_file);
      if (pat == NULL)
	{
	  if (mood == WANT_PATTERN)
	    {
	      int z;
	      do
		{
		  z = end_of_format (p, ref_file);
		  pat = scan_format_pattern (SUB (file->format.top), ref_file);
		}
	      while (z == EMBEDDED_FORMAT && pat == NULL);
	      if (pat == NULL)
		{
		  diagnostic (A_RUNTIME_ERROR, p, "patterns exhausted in format");
		  exit_genie (p, A_RUNTIME_ERROR);
		}
	    }
	}
      return (pat);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pattern_error: diagnostic in case mode does not whether picture.                */

void
pattern_error (NODE_T * p, MOID_T * mode, int att)
{
  diagnostic (A_RUNTIME_ERROR, p, "cannot transput M value with A", mode, att);
  exit_genie (p, A_RUNTIME_ERROR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unite_to_number: unite value at top of stack to NUMBER.                       */

static void
unite_to_number (NODE_T * p, MOID_T * mode, BYTE_T * item)
{
  ADDR_T sp = stack_pointer;
  PUSH_POINTER (p, mode);
  PUSH (p, item, MOID_SIZE (mode));
  stack_pointer = sp + MOID_SIZE (MODE (NUMBER));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_insertion: write a group of insertions.                                 */

void
write_insertion (NODE_T * p, A68_REF ref_file, unsigned mood)
{
  for (; p != NULL; p = NEXT (p))
    {
      write_insertion (SUB (p), ref_file, mood);
      if (WHETHER (p, FORMAT_ITEM_L))
	{
	  A68_FILE *file = FILE_DEREF (&ref_file);
	  add_char_transput_buffer (p, FORMATTED_BUFFER, '\n');
	  if (!(file->fd == STDOUT_FILENO && halt_typing))
	    {
	      io_write_string (file->fd, get_transput_buffer (FORMATTED_BUFFER));
	      reset_transput_buffer (FORMATTED_BUFFER);
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_P))
	{
	  A68_FILE *file = FILE_DEREF (&ref_file);
	  add_char_transput_buffer (p, FORMATTED_BUFFER, '\f');
	  if (!(file->fd == STDOUT_FILENO && halt_typing))
	    {
	      io_write_string (file->fd, get_transput_buffer (FORMATTED_BUFFER));
	      reset_transput_buffer (FORMATTED_BUFFER);
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_X) || WHETHER (p, FORMAT_ITEM_Q))
	{
	  add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
	}
      else if (WHETHER (p, FORMAT_ITEM_Y))
	{
	  /* Not supported, parser has warned you. */ ;
	}
      else if (WHETHER (p, LITERAL))
	{
	  if (mood & INSERTION_NORMAL)
	    {
	      add_string_transput_buffer (p, FORMATTED_BUFFER, SYMBOL (p));
	    }
	  else if (mood & INSERTION_BLANK)
	    {
	      int j, k = strlen (SYMBOL (p));
	      for (j = 1; j <= k; j++)
		{
		  add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
		}
	    }
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  if (ATTRIBUTE (SUB (NEXT (p))) != FORMAT_ITEM_K)
	    {
	      for (j = 1; j <= k; j++)
		{
		  write_insertion (NEXT (p), ref_file, mood);
		}
	    }
	  else
	    {
	      int pos = get_transput_buffer_index (FORMATTED_BUFFER);
	      for (j = 1; j < (k - pos); j++)
		{
		  add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
		}
	    }
	  return;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_string_pattern: write str to ref_file cf. current format.               */

static void
write_string_pattern (NODE_T * p, MOID_T * mode, A68_REF ref_file, char **str)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  write_insertion (SUB (p), ref_file, INSERTION_NORMAL);
	}
      else if (WHETHER (p, FORMAT_ITEM_A))
	{
	  if ((*str)[0] != '\0')
	    {
	      add_char_transput_buffer (p, FORMATTED_BUFFER, (*str)[0]);
	      (*str)++;
	    }
	  else
	    {
	      value_error (p, mode, ref_file);
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_S))
	{
	  if ((*str)[0] != '\0')
	    {
	      (*str)++;
	    }
	  else
	    {
	      value_error (p, mode, ref_file);
	    }
	  return;
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      write_string_pattern (NEXT (p), mode, ref_file, str);
	    }
	  return;
	}
      else
	{
	  write_string_pattern (SUB (p), mode, ref_file, str);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_string_c_style: write str to ref_file using C style format %[-][w]s.    */

static void
write_string_c_style (NODE_T * p, char *str)
{
  int k, sign, width;
  if (WHETHER (p, STRING_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
/* Get sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  sign = ATTRIBUTE (q);
	  q = NEXT (q);
	}
      else
	{
	  sign = 0;
	}
/* Get width. */
      if (WHETHER (q, REPLICATOR))
	{
	  width = get_replicator_value (SUB (q));
	}
      else
	{
	  width = strlen (str);
	}
/* Output string. */
      k = width - strlen (str);
      if (k >= 0)
	{
	  if (sign == FORMAT_ITEM_PLUS || sign == 0)
	    {
	      add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	    }
	  while (k--)
	    {
	      add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
	    }
	  if (sign == FORMAT_ITEM_MINUS)
	    {
	      add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	    }
	}
      else
	{
	  error_chars (get_transput_buffer (FORMATTED_BUFFER), width);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_choice_pattern: write appropriate insertion from a choice pattern.      */

static void
write_choice_pattern (NODE_T * p, A68_REF ref_file, int *count)
{
  for (; p != NULL; p = NEXT (p))
    {
      write_choice_pattern (SUB (p), ref_file, count);
      if (WHETHER (p, PICTURE))
	{
	  (*count)--;
	  if (*count == 0)
	    {
	      write_insertion (SUB (p), ref_file, INSERTION_NORMAL);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_boolean_pattern: write appropriate insertion from a boolean pattern.    */

static void
write_boolean_pattern (NODE_T * p, A68_REF ref_file, BOOL_T z)
{
  int k = (z ? 1 : 2);
  write_choice_pattern (p, ref_file, &k);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_number_generic: write value according to a general pattern.             */

static void
write_number_generic (NODE_T * p, MOID_T * mode, BYTE_T * item)
{
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int size;
/* Push arguments. */
  unite_to_number (p, mode, item);
  EXECUTE_UNIT (NEXT_SUB (p));
  POP_REF (p, &row);
  GET_DESCRIPTOR (arr, tup, &row);
  size = ROW_SIZE (tup);
  if (size > 0)
    {
      int i;
      BYTE_T *base_address = ADDRESS (&arr->array);
      for (i = tup->lower_bound; i <= tup->upper_bound; i++)
	{
	  int addr = INDEX_1_DIM (arr, tup, i);
	  int arg = ((A68_INT *) & (base_address[addr]))->value;
	  PUSH_INT (p, arg);
	}
    }
/* Make a string. */
  switch (size)
    {
    case 1:
      {
	genie_whole (p);
	break;
      }
    case 2:
      {
	genie_fixed (p);
	break;
      }
    case 3:
      {
	genie_float (p);
	break;
      }
    default:
      {
	diagnostic (A_RUNTIME_ERROR, p, "1 .. 3 M arguments required", MODE (INT));
	exit_genie (p, A_RUNTIME_ERROR);
	break;
      }
    }
  add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_number_c_style: handle %[+][-][w]d, %[+][-][w][.][d]f/e formats.        */

static void
write_number_c_style (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  BOOL_T sign = 0;
  int width = 0, after = 0;
  char *str = NULL;
  unite_to_number (p, mode, item);
/* Integral C pattern. */
  if (WHETHER (p, INTEGRAL_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
/* Get sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  sign = ATTRIBUTE (q);
	  q = NEXT (q);
	}
      else
	{
	  sign = 0;
	}
/* Get width. */
      if (WHETHER (q, REPLICATOR))
	{
	  width = get_replicator_value (SUB (q));
	}
      else
	{
	  width = 0;
	}
/* Make string. */
      PUSH_INT (p, sign != 0 ? width : -width);
      str = whole (p);
    }
/* Real C patterns. */
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
/* Get sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  sign = ATTRIBUTE (q);
	  q = NEXT (q);
	}
      else
	{
	  sign = 0;
	}
/* Get width. */
      if (WHETHER (q, REPLICATOR))
	{
	  width = get_replicator_value (SUB (q));
	  q = NEXT (q);
	}
      else
	{
	  width = 0;
	}
/* Skip point. */
      if (WHETHER (q, FORMAT_ITEM_POINT))
	{
	  q = NEXT (q);
	}
/* Get decimals. */
      if (WHETHER (q, REPLICATOR))
	{
	  after = get_replicator_value (SUB (q));
	  q = NEXT (q);
	}
      else
	{
	  after = 0;
	}
/* Make string. */
      if (WHETHER (p, FIXED_C_PATTERN))
	{
	  int num_width = 0, max = 0;
	  if (mode == MODE (REAL) || mode == MODE (INT))
	    {
	      max = REAL_WIDTH - 1;
	    }
	  else if (mode == MODE (LONG_REAL) || mode == MODE (LONG_INT))
	    {
	      max = LONG_REAL_WIDTH - 1;
	    }
	  else if (mode == MODE (LONGLONG_REAL) || mode == MODE (LONGLONG_INT))
	    {
	      max = LONGLONG_REAL_WIDTH - 1;
	    }
	  if (after < 0 || after > max)
	    {
	      after = max;
	    }
	  num_width = width;
	  PUSH_INT (p, sign != 0 ? num_width : -num_width);
	  PUSH_INT (p, after);
	  str = fixed (p);
	}
      else if (WHETHER (p, FLOAT_C_PATTERN))
	{
	  int num_width = 0, max = 0, mex = 0, expo = 0;
	  if (mode == MODE (REAL) || mode == MODE (INT))
	    {
	      max = REAL_WIDTH - 1;
	      mex = EXP_WIDTH + 1;
	    }
	  else if (mode == MODE (LONG_REAL) || mode == MODE (LONG_INT))
	    {
	      max = LONG_REAL_WIDTH - 1;
	      mex = LONG_EXP_WIDTH + 1;
	    }
	  else if (mode == MODE (LONGLONG_REAL) || mode == MODE (LONGLONG_INT))
	    {
	      max = LONGLONG_REAL_WIDTH - 1;
	      mex = LONGLONG_EXP_WIDTH + 1;
	    }
	  expo = mex + 1;
	  if (after <= 0 && width > 0)
	    {
	      after = width - (expo + 4);
	    }
	  if (after <= 0 || after > max)
	    {
	      after = max;
	    }
	  num_width = after + expo + 4;
	  PUSH_INT (p, sign != 0 ? num_width : -num_width);
	  PUSH_INT (p, after);
	  PUSH_INT (p, expo);
	  str = fleet (p);
	}
    }
/* Did the conversion succeed? */
  if (strchr (str, ERROR_CHAR) != NULL)
    {
      value_error (p, mode, ref_file);
      error_chars (get_transput_buffer (FORMATTED_BUFFER), width);
    }
  else
    {
/* Edit and output. */
      if (sign == FORMAT_ITEM_MINUS)
	{
	  char *ch = str;
	  while (ch[0] != '\0' && ch[0] == ' ')
	    {
	      ch++;
	    }
	  if (ch[0] != '\0' && ch[0] == '+')
	    {
	      ch[0] = ' ';
	    }
	}
      if (width == 0)
	{
	  add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	}
      else
	{
	  int blanks = width - strlen (str);
	  if (blanks >= 0)
	    {
	      while (blanks--)
		{
		  add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
		}
	      add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	    }
	  else
	    {
	      value_error (p, mode, ref_file);
	      error_chars (get_transput_buffer (FORMATTED_BUFFER), width);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
INTEGRAL, REAL, COMPLEX and BITS patterns.                                    */

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_zd_frames: count Z and D frames in a mould.                             */

static void
count_zd_frames (NODE_T * p, int *z)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, FORMAT_ITEM_D) || WHETHER (p, FORMAT_ITEM_Z))
	{

	  (*z)++;
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      count_zd_frames (NEXT (p), z);
	    }
	  return;
	}
      else
	{
	  count_zd_frames (SUB (p), z);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_d_frames: count D frames in a mould.                                    */

static void
count_d_frames (NODE_T * p, int *z)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, FORMAT_ITEM_D))
	{

	  (*z)++;
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      count_d_frames (NEXT (p), z);
	    }
	  return;
	}
      else
	{
	  count_d_frames (SUB (p), z);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_sign: get sign from sign mould.                                           */

static NODE_T *
get_sign (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      NODE_T *q = get_sign (SUB (p));
      if (q != NULL)
	{
	  return (q);
	}
      else if (WHETHER (p, FORMAT_ITEM_PLUS) || WHETHER (p, FORMAT_ITEM_MINUS))
	{
	  return (p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
shift_sign: shift sign through Z frames until non-zero digit or D frame.      */

static void
shift_sign (NODE_T * p, char **q)
{
  for (; p != NULL && (*q) != NULL; p = NEXT (p))
    {
      shift_sign (SUB (p), q);
      if (WHETHER (p, FORMAT_ITEM_Z))
	{
	  if (((*q)[0] == '+' || (*q)[0] == '-') && (*q)[1] == '0')
	    {
	      char ch = (*q)[0];
	      (*q)[0] = (*q)[1];
	      (*q)[1] = ch;
	      (*q)++;
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_D))
	{
	  (*q) = NULL;
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      shift_sign (NEXT (p), q);
	    }
	  return;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
put_zeroes_to_integral: put n trailing blanks to integral until desired width.*/

static void
put_zeroes_to_integral (NODE_T * p, int n)
{
  while (n--)
    {
      add_char_transput_buffer (p, EDIT_BUFFER, '0');
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
put_sign_to_integral: pad a sign to integral representation.                  */

static void
put_sign_to_integral (NODE_T * p, int sign)
{
  NODE_T *sign_node = get_sign (SUB (p));
  if (WHETHER (sign_node, FORMAT_ITEM_PLUS))
    {
      add_char_transput_buffer (p, EDIT_BUFFER, (sign >= 0 ? '+' : '-'));
    }
  else
    {
      add_char_transput_buffer (p, EDIT_BUFFER, (sign >= 0 ? ' ' : '-'));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
convert_radix: convert "z" to radix "radix", binary up to hexadecimal.        */

static BOOL_T
convert_radix (NODE_T * p, unsigned z, int radix, int width)
{
  static char *images = "0123456789abcdef";
  if (width > 0 && (radix >= 2 && radix <= 16))
    {
      int digit = z % radix;
      BOOL_T success = convert_radix (p, z / radix, radix, width - 1);
      add_char_transput_buffer (p, EDIT_BUFFER, images[digit]);
      return (success);
    }
  else
    {
      return (z == 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
convert_radix_mp: convert "z" to radix "radix", binary up to hexadecimal.     */

static BOOL_T
convert_radix_mp (NODE_T * p, mp_digit * u, int radix, int width, MOID_T * m, mp_digit * v, mp_digit * w)
{
  static char *images = "0123456789abcdef";
  if (width > 0 && (radix >= 2 && radix <= 16))
    {
      int digit, digits = get_mp_digits (m);
      BOOL_T success;
      MOVE_MP (w, u, digits);
      over_mp_digit (p, u, u, (mp_digit) radix, digits);
      mul_mp_digit (p, v, u, (mp_digit) radix, digits);
      sub_mp (p, v, w, v, digits);
      digit = (int) MP_DIGIT (v, 1);
      success = convert_radix_mp (p, u, radix, width - 1, m, v, w);
      add_char_transput_buffer (p, EDIT_BUFFER, images[digit]);
      return (success);
    }
  else
    {
      return (MP_DIGIT (u, 1) == 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_pie_frame: write point, exponent or plus-i-times symbol.                */

static void
write_pie_frame (NODE_T * p, A68_REF ref_file, int att, int sym)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  write_insertion (p, ref_file, INSERTION_NORMAL);
	}
      else if (WHETHER (p, att))
	{
	  write_pie_frame (SUB (p), ref_file, att, sym);
	  return;
	}
      else if (WHETHER (p, sym))
	{
	  add_string_transput_buffer (p, FORMATTED_BUFFER, SYMBOL (p));
	}
      else if (WHETHER (p, FORMAT_ITEM_S))
	{
	  return;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_mould_put_sign: write sign when appropriate.                            */

static void
write_mould_put_sign (NODE_T * p, char **q)
{
  if ((*q)[0] == '+' || (*q)[0] == '-' || (*q)[0] == ' ')
    {
      add_char_transput_buffer (p, FORMATTED_BUFFER, (*q)[0]);
      (*q)++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_mould: write string "q" according to a mould.                           */

static void
write_mould (NODE_T * p, A68_REF ref_file, int type, char **q, unsigned *mood)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Insertions are inserted straight away. Note that we can suppress them using 
   "mood", which is not standard A68. */
      if (WHETHER (p, INSERTION))
	{
	  write_insertion (SUB (p), ref_file, *mood);
	}
      else
	{
	  write_mould (SUB (p), ref_file, type, q, mood);
/* Z frames print blanks until first non-zero digits comes. */
	  if (WHETHER (p, FORMAT_ITEM_Z))
	    {
	      write_mould_put_sign (p, q);
	      if ((*q)[0] == '0')
		{
		  if (*mood & DIGIT_BLANK)
		    {
		      add_char_transput_buffer (p, FORMATTED_BUFFER, ' ');
		      (*q)++;
		      *mood = (*mood & ~INSERTION_NORMAL) | INSERTION_BLANK;
		    }
		  else if (*mood & DIGIT_NORMAL)
		    {
		      add_char_transput_buffer (p, FORMATTED_BUFFER, '0');
		      (*q)++;
		      *mood = DIGIT_NORMAL | INSERTION_NORMAL;
		    }
		}
	      else
		{
		  add_char_transput_buffer (p, FORMATTED_BUFFER, (*q)[0]);
		  (*q)++;
		  *mood = DIGIT_NORMAL | INSERTION_NORMAL;
		}
	    }
/* D frames print a digit. */
	  else if (WHETHER (p, FORMAT_ITEM_D))
	    {
	      write_mould_put_sign (p, q);
	      add_char_transput_buffer (p, FORMATTED_BUFFER, (*q)[0]);
	      (*q)++;
	      *mood = DIGIT_NORMAL | INSERTION_NORMAL;
	    }
/* Suppressible frames. */
	  else if (WHETHER (p, FORMAT_ITEM_S))
	    {
/* Suppressible frames are ignored in a sign-mould. */
	      if (type == SIGN_MOULD)
		{
		  write_mould (NEXT (p), ref_file, type, q, mood);
		}
	      else if (type == INTEGRAL_MOULD)
		{
		  (*q)++;
		}
	      return;
	    }
/* Replicator. */
	  else if (WHETHER (p, REPLICATOR))
	    {
	      int j, k = get_replicator_value (SUB (p));
	      for (j = 1; j <= k; j++)
		{
		  write_mould (NEXT (p), ref_file, type, q, mood);
		}
	      return;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_integral_pattern: write INT value using int pattern.                    */

static void
write_integral_pattern (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  if (!(mode == MODE (INT) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT)))
    {
      pattern_error (p, mode, ATTRIBUTE (p));
    }
  else
    {
      ADDR_T old_stack_pointer = stack_pointer;
      char *str;
      int width = 0, sign = 0;
      unsigned mood;
/* Dive into the pattern if needed. */
      if (WHETHER (p, INTEGRAL_PATTERN))
	{
	  p = SUB (p);
	}
/* Find width. */
      count_zd_frames (p, &width);
/* Make string. */
      reset_transput_buffer (EDIT_BUFFER);
      if (mode == MODE (INT))
	{
	  A68_INT *z = (A68_INT *) item;
	  sign = SIGN (z->value);
	  str = sub_whole (p, ABS (z->value), width);
	}
      else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
	{
	  mp_digit *z = (mp_digit *) item;
	  sign = SIGN (z[2]);
	  z[2] = ABS (z[2]);
	  str = long_sub_whole (p, z, get_mp_digits (mode), width);
	}
/* Edit string and output. */
      if (strchr (str, ERROR_CHAR) != NULL)
	{
	  value_error (p, mode, ref_file);
	}
      if (WHETHER (p, SIGN_MOULD))
	{
	  put_sign_to_integral (p, sign);
	}
      else if (sign < 0)
	{
	  value_error (p, mode, ref_file);
	}
      put_zeroes_to_integral (p, width - strlen (str));
      add_string_transput_buffer (p, EDIT_BUFFER, str);
      str = get_transput_buffer (EDIT_BUFFER);
      if (WHETHER (p, SIGN_MOULD))
	{
	  if (str[0] == '+' || str[0] == '-')
	    {
	      shift_sign (SUB (p), &str);
	    }
	  str = get_transput_buffer (EDIT_BUFFER);
	  mood = DIGIT_BLANK | INSERTION_NORMAL;
	  write_mould (SUB (p), ref_file, SIGN_MOULD, &str, &mood);
	  p = NEXT (p);
	}
      if (WHETHER (p, INTEGRAL_MOULD))
	{			/* This *should* be the case. */
	  mood = DIGIT_NORMAL | INSERTION_NORMAL;
	  write_mould (SUB (p), ref_file, INTEGRAL_MOULD, &str, &mood);
	}
      stack_pointer = old_stack_pointer;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_real_pattern: write REAL value using real pattern.                      */

static void
write_real_pattern (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  if (!(mode == MODE (REAL) || mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL) || mode == MODE (INT) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT)))
    {
      pattern_error (p, mode, ATTRIBUTE (p));
    }
  else
    {
      ADDR_T old_stack_pointer = stack_pointer;
      int stag_digits = 0, frac_digits = 0, expo_digits = 0;
      int stag_width = 0, frac_width = 0, expo_width = 0;
      int length, sign = 0, d_exp;
      NODE_T *q, *sign_mould = NULL, *stag_mould = NULL, *point_frame = NULL, *frac_mould = NULL, *e_frame = NULL, *expo_mould = NULL;
      char *str, *stag_str, *frac_str;
/* Dive into pattern. */
      q = (WHETHER (p, REAL_PATTERN)) ? SUB (p) : p;
/* Dissect pattern and establish widths. */
      if (q != NULL && WHETHER (q, SIGN_MOULD))
	{
	  sign_mould = q;
	  count_zd_frames (SUB (sign_mould), &stag_width);
	  count_d_frames (SUB (sign_mould), &stag_digits);
	  q = NEXT (q);
	}
      if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
	{
	  stag_mould = q;
	  count_zd_frames (SUB (stag_mould), &stag_width);
	  count_zd_frames (SUB (stag_mould), &stag_digits);
	  q = NEXT (q);
	}
      if (q != NULL && WHETHER (q, FORMAT_POINT_FRAME))
	{
	  point_frame = q;
	  q = NEXT (q);
	}
      if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
	{
	  frac_mould = q;
	  count_zd_frames (SUB (frac_mould), &frac_width);
	  count_zd_frames (SUB (frac_mould), &frac_digits);
	  q = NEXT (q);
	}
      if (q != NULL && WHETHER (q, EXPONENT_FRAME))
	{
	  e_frame = SUB (q);
	  expo_mould = NEXT_SUB (q);
	  q = expo_mould;
	  if (WHETHER (q, SIGN_MOULD))
	    {
	      count_zd_frames (SUB (q), &expo_width);
	      count_d_frames (SUB (q), &expo_digits);
	      q = NEXT (q);
	    }
	  if (WHETHER (q, INTEGRAL_MOULD))
	    {
	      count_zd_frames (SUB (q), &expo_width);
	      count_d_frames (SUB (q), &expo_digits);
	    }
	}
/* Make string representation. */
      reset_transput_buffer (EDIT_BUFFER);
      length = 1 + stag_width + frac_width;
      if (mode == MODE (REAL) || mode == MODE (INT))
	{
	  double x;
	  if (mode == MODE (REAL))
	    {
	      x = ((A68_REAL *) item)->value;
	    }
	  else
	    {
	      x = (double) ((A68_INT *) item)->value;
	    }
#ifdef HAVE_IEEE_754
	  if (isnan (x))
	    {
	      char *s = stack_string (p, 1 + length);
	      if (length >= (int) strlen (NAN_STRING))
		{
		  memset (s, ' ', length);
		  strncpy (s, NAN_STRING, strlen (NAN_STRING));
		}
	      else
		{
		  error_chars (s, length);
		}
	      add_string_transput_buffer (p, FORMATTED_BUFFER, s);
	      stack_pointer = old_stack_pointer;
	      return;
	    }
	  else if (isinf (x))
	    {
	      char *s = stack_string (p, 1 + length);
	      if (length >= (int) strlen (INF_STRING))
		{
		  memset (s, ' ', length);
		  strncpy (s, INF_STRING, strlen (INF_STRING));
		}
	      else
		{
		  error_chars (s, length);
		}
	      add_string_transput_buffer (p, FORMATTED_BUFFER, s);
	      stack_pointer = old_stack_pointer;
	      return;
	    }
#endif
	  d_exp = 0;
	  sign = SIGN (x);
	  if (sign_mould != NULL)
	    {
	      put_sign_to_integral (sign_mould, sign);
	    }
	  x = ABS (x);
	  if (expo_mould != NULL)
	    {
	      standardise (&x, stag_digits, frac_digits, &d_exp);
	    }
	  str = sub_fixed (p, x, length, frac_digits);
	}
      else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
	{
	  ADDR_T old_stack_pointer = stack_pointer;
	  int digits = get_mp_digits (mode);
	  mp_digit *x = stack_mp (p, digits);
	  MOVE_MP (x, (mp_digit *) item, digits);
	  d_exp = 0;
	  sign = SIGN (x[2]);
	  if (sign_mould != NULL)
	    {
	      put_sign_to_integral (sign_mould, sign);
	    }
	  x[2] = ABS (x[2]);
	  if (expo_mould != NULL)
	    {
	      long_standardise (p, x, get_mp_digits (mode), stag_digits, frac_digits, &d_exp);
	    }
	  str = long_sub_fixed (p, x, get_mp_digits (mode), length, frac_digits);
	  stack_pointer = old_stack_pointer;
	}
/* Edit and output the string. */
      if (strchr (str, ERROR_CHAR) != NULL)
	{
	  value_error (p, mode, ref_file);
	}
      put_zeroes_to_integral (p, length - strlen (str));
      add_string_transput_buffer (p, EDIT_BUFFER, str);
      stag_str = get_transput_buffer (EDIT_BUFFER);
      if (strchr (stag_str, ERROR_CHAR) != NULL)
	{
	  value_error (p, mode, ref_file);
	}
      str = strchr (stag_str, '.');
      if (frac_mould != NULL)
	{
	  frac_str = &str[1];
	}
      if (str != NULL)
	{
	  str[0] = '\0';
	}
/* Stagnant sign. */
      if (sign_mould != NULL)
	{
	  int digits = 0;
	  count_zd_frames (SUB (sign_mould), &digits);
	  if (digits > 0)
	    {
	      unsigned mood = DIGIT_BLANK | INSERTION_NORMAL;
	      str = stag_str;
	      if (str[0] == '+' || str[0] == '-')
		{
		  shift_sign (SUB (sign_mould), &str);
		}
	      write_mould (SUB (sign_mould), ref_file, SIGN_MOULD, &stag_str, &mood);
	    }
	  else
	    {
	      write_mould_put_sign (SUB (sign_mould), &stag_str);
	    }
	}
      else if (sign < 0)
	{
	  value_error (p, mode, ref_file);
	}
/* Stagnant part. */
      if (stag_mould != NULL)
	{
	  unsigned mood = DIGIT_NORMAL | INSERTION_NORMAL;
	  write_mould (SUB (stag_mould), ref_file, INTEGRAL_MOULD, &stag_str, &mood);
	}
/* Fraction. */
      if (frac_mould != NULL)
	{
	  unsigned mood = DIGIT_NORMAL | INSERTION_NORMAL;
	  if (point_frame != NULL)
	    {
	      write_pie_frame (point_frame, ref_file, FORMAT_POINT_FRAME, FORMAT_ITEM_POINT);
	    }
	  write_mould (SUB (frac_mould), ref_file, INTEGRAL_MOULD, &frac_str, &mood);
	}
/* Exponent. */
      if (expo_mould != NULL)
	{
	  A68_INT z;
	  if (e_frame != NULL)
	    {
	      write_pie_frame (e_frame, ref_file, FORMAT_E_FRAME, FORMAT_ITEM_E);
	    }
	  z.status = INITIALISED_MASK;
	  z.value = d_exp;
	  write_integral_pattern (expo_mould, MODE (INT), (BYTE_T *) & z, ref_file);
	}
      stack_pointer = old_stack_pointer;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_complex_pattern: write COMPLEX value using complex pattern.             */

static void
write_complex_pattern (NODE_T * p, MOID_T * comp, BYTE_T * re, BYTE_T * im, A68_REF ref_file)
{
  NODE_T *real, *plus_i_times, *imag;
/* Dissect pattern. */
  real = SUB (p);
  plus_i_times = NEXT (real);
  imag = NEXT (plus_i_times);
/* Write pattern. */
  write_real_pattern (real, comp, re, ref_file);
  write_pie_frame (plus_i_times, ref_file, FORMAT_I_FRAME, FORMAT_ITEM_I);
  write_real_pattern (imag, comp, im, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_bits_pattern: write BITS value using bits pattern.                      */

static void
write_bits_pattern (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  if (mode == MODE (BITS))
    {
      int width = 0, radix;
      unsigned mood;
      A68_BITS *z = (A68_BITS *) item;
      char *str;
/* Establish width and radix. */
      count_zd_frames (SUB (p), &width);
      radix = get_replicator_value (SUB (SUB (p)));
      if (radix < 2 || radix > 16)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "invalid radix D", radix);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
/* Generate string of correct width. */
      reset_transput_buffer (EDIT_BUFFER);
      if (!convert_radix (p, z->value, radix, width))
	{
	  errno = EDOM;
	  value_error (p, mode, ref_file);
	}
/* Output the edited string. */
      mood = DIGIT_NORMAL & INSERTION_NORMAL;
      str = get_transput_buffer (EDIT_BUFFER);
      write_mould (NEXT_SUB (p), ref_file, INTEGRAL_MOULD, &str, &mood);
    }
  else if (mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      ADDR_T save_sp = stack_pointer;
      int width = 0, radix, digits = get_mp_digits (mode);
      unsigned mood;
      mp_digit *u = (mp_digit *) item;
      mp_digit *v = stack_mp (p, digits);
      mp_digit *w = stack_mp (p, digits);
      char *str;
/* Establish width and radix. */
      count_zd_frames (SUB (p), &width);
      radix = get_replicator_value (SUB (SUB (p)));
      if (radix < 2 || radix > 16)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "invalid radix D", radix);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
/* Generate string of correct width. */
      reset_transput_buffer (EDIT_BUFFER);
      if (!convert_radix_mp (p, u, radix, width, mode, v, w))
	{
	  errno = EDOM;
	  value_error (p, mode, ref_file);
	}
/* Output the edited string. */
      mood = DIGIT_NORMAL & INSERTION_NORMAL;
      str = get_transput_buffer (EDIT_BUFFER);
      write_mould (NEXT_SUB (p), ref_file, INTEGRAL_MOULD, &str, &mood);
      stack_pointer = save_sp;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_real_format: write value at "item" to "file".                     */

static void
genie_write_real_format (NODE_T * p, BYTE_T * item, A68_REF ref_file)
{
  if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) == NULL)
    {
      genie_value_to_string (p, MODE (REAL), item);
      add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
    }
  else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
    {
      write_number_generic (p, MODE (REAL), item);
    }
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      write_number_c_style (p, MODE (REAL), item, ref_file);
    }
  else if (WHETHER (p, REAL_PATTERN))
    {
      write_real_pattern (p, MODE (REAL), item, ref_file);
    }
  else if (WHETHER (p, COMPLEX_PATTERN))
    {
      A68_REAL im;
      im.status = INITIALISED_MASK;
      im.value = 0.0;
      write_complex_pattern (p, MODE (REAL), (BYTE_T *) item, (BYTE_T *) & im, ref_file);
    }
  else
    {
      pattern_error (p, MODE (REAL), ATTRIBUTE (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_long_real_format: write value at "item" to "file".                */

static void
genie_write_long_real_format (NODE_T * p, BYTE_T * item, A68_REF ref_file)
{
  if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) == NULL)
    {
      genie_value_to_string (p, MODE (LONG_REAL), item);
      add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
    }
  else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
    {
      write_number_generic (p, MODE (LONG_REAL), item);
    }
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      write_number_c_style (p, MODE (LONG_REAL), item, ref_file);
    }
  else if (WHETHER (p, REAL_PATTERN))
    {
      write_real_pattern (p, MODE (LONG_REAL), item, ref_file);
    }
  else if (WHETHER (p, COMPLEX_PATTERN))
    {
      ADDR_T old_stack_pointer = stack_pointer;
      mp_digit *z = stack_mp (p, get_mp_digits (MODE (LONG_REAL)));
      SET_MP_ZERO (z, get_mp_digits (MODE (LONG_REAL)));
      z[0] = INITIALISED_MASK;
      write_complex_pattern (p, MODE (LONG_REAL), item, (BYTE_T *) z, ref_file);
      stack_pointer = old_stack_pointer;
    }
  else
    {
      pattern_error (p, MODE (LONG_REAL), ATTRIBUTE (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_longlong_real_format: write value at "item" to "file"             */

static void
genie_write_longlong_real_format (NODE_T * p, BYTE_T * item, A68_REF ref_file)
{
  if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) == NULL)
    {
      genie_value_to_string (p, MODE (LONGLONG_REAL), item);
      add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
    }
  else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
    {
      write_number_generic (p, MODE (LONGLONG_REAL), item);
    }
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      write_number_c_style (p, MODE (LONGLONG_REAL), item, ref_file);
    }
  else if (WHETHER (p, REAL_PATTERN))
    {
      write_real_pattern (p, MODE (LONGLONG_REAL), item, ref_file);
    }
  else if (WHETHER (p, COMPLEX_PATTERN))
    {
      ADDR_T old_stack_pointer = stack_pointer;
      mp_digit *z = stack_mp (p, get_mp_digits (MODE (LONGLONG_REAL)));
      SET_MP_ZERO (z, get_mp_digits (MODE (LONGLONG_REAL)));
      z[0] = INITIALISED_MASK;
      write_complex_pattern (p, MODE (LONGLONG_REAL), item, (BYTE_T *) z, ref_file);
      stack_pointer = old_stack_pointer;
    }
  else
    {
      pattern_error (p, MODE (LONGLONG_REAL), ATTRIBUTE (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_standard_format: print object with "mode" at "item" to "ref_file".*/

static void
genie_write_standard_format (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  errno = 0;
  if (mode == MODE (INT))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_value_to_string (p, mode, item);
	  add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
	}
      else if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) != NULL)
	{
	  write_number_generic (pat, MODE (INT), item);
	}
      else if (WHETHER (pat, INTEGRAL_C_PATTERN) || WHETHER (pat, FIXED_C_PATTERN) || WHETHER (pat, FLOAT_C_PATTERN))
	{
	  write_number_c_style (pat, MODE (INT), item, ref_file);
	}
      else if (WHETHER (pat, INTEGRAL_PATTERN))
	{
	  write_integral_pattern (pat, MODE (INT), item, ref_file);
	}
      else if (WHETHER (pat, REAL_PATTERN))
	{
	  write_real_pattern (pat, MODE (INT), item, ref_file);
	}
      else if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  A68_REAL re, im;
	  re.status = INITIALISED_MASK;
	  re.value = (double) ((A68_INT *) item)->value, im.status = INITIALISED_MASK;
	  im.value = 0.0;
	  write_complex_pattern (pat, MODE (REAL), (BYTE_T *) & re, (BYTE_T *) & im, ref_file);
	}
      else if (WHETHER (pat, CHOICE_PATTERN))
	{
	  int k = ((A68_INT *) item)->value;
	  write_choice_pattern (NEXT_SUB (pat), ref_file, &k);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (LONG_INT))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_value_to_string (p, mode, item);
	  add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
	}
      else if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) != NULL)
	{
	  write_number_generic (pat, MODE (LONG_INT), item);
	}
      else if (WHETHER (pat, INTEGRAL_C_PATTERN) || WHETHER (pat, FIXED_C_PATTERN) || WHETHER (pat, FLOAT_C_PATTERN))
	{
	  write_number_c_style (pat, MODE (LONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, INTEGRAL_PATTERN))
	{
	  write_integral_pattern (pat, MODE (LONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, REAL_PATTERN))
	{
	  write_real_pattern (pat, MODE (LONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  ADDR_T old_stack_pointer = stack_pointer;
	  mp_digit *z = stack_mp (p, get_mp_digits (mode));
	  SET_MP_ZERO (z, get_mp_digits (mode));
	  z[0] = INITIALISED_MASK;
	  write_complex_pattern (pat, MODE (LONG_REAL), item, (BYTE_T *) z, ref_file);
	  stack_pointer = old_stack_pointer;
	}
      else if (WHETHER (pat, CHOICE_PATTERN))
	{
	  int k = mp_to_int (p, (mp_digit *) item, get_mp_digits (mode));
	  write_choice_pattern (NEXT_SUB (pat), ref_file, &k);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (LONGLONG_INT))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_value_to_string (p, mode, item);
	  add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
	}
      else if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) != NULL)
	{
	  write_number_generic (pat, MODE (LONGLONG_INT), item);
	}
      else if (WHETHER (pat, INTEGRAL_C_PATTERN) || WHETHER (pat, FIXED_C_PATTERN) || WHETHER (pat, FLOAT_C_PATTERN))
	{
	  write_number_c_style (pat, MODE (LONGLONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, INTEGRAL_PATTERN))
	{
	  write_integral_pattern (pat, MODE (LONGLONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, REAL_PATTERN))
	{
	  write_real_pattern (pat, MODE (INT), item, ref_file);
	}
      else if (WHETHER (pat, REAL_PATTERN))
	{
	  write_real_pattern (pat, MODE (LONGLONG_INT), item, ref_file);
	}
      else if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  ADDR_T old_stack_pointer = stack_pointer;
	  mp_digit *z = stack_mp (p, get_mp_digits (MODE (LONGLONG_REAL)));
	  SET_MP_ZERO (z, get_mp_digits (mode));
	  z[0] = INITIALISED_MASK;
	  write_complex_pattern (pat, MODE (LONGLONG_REAL), item, (BYTE_T *) z, ref_file);
	  stack_pointer = old_stack_pointer;
	}
      else if (WHETHER (pat, CHOICE_PATTERN))
	{
	  int k = mp_to_int (p, (mp_digit *) item, get_mp_digits (mode));
	  write_choice_pattern (NEXT_SUB (pat), ref_file, &k);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (REAL))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      genie_write_real_format (pat, item, ref_file);
    }
  else if (mode == MODE (LONG_REAL))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      genie_write_long_real_format (pat, item, ref_file);
    }
  else if (mode == MODE (LONGLONG_REAL))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      genie_write_longlong_real_format (pat, item, ref_file);
    }
  else if (mode == MODE (COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  write_complex_pattern (pat, MODE (REAL), &item[0], &item[MOID_SIZE (MODE (REAL))], ref_file);
	}
      else
	{
/* Try writing as two REAL values. */
	  genie_write_real_format (pat, item, ref_file);
	  genie_write_standard_format (p, MODE (REAL), &item[MOID_SIZE (MODE (REAL))], ref_file);
	}
    }
  else if (mode == MODE (LONG_COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  write_complex_pattern (pat, MODE (LONG_REAL), &item[0], &item[MOID_SIZE (MODE (LONG_REAL))], ref_file);
	}
      else
	{
/* Try writing as two LONG REAL values. */
	  genie_write_long_real_format (pat, item, ref_file);
	  genie_write_standard_format (p, MODE (LONG_REAL), &item[MOID_SIZE (MODE (LONG_REAL))], ref_file);
	}
    }
  else if (mode == MODE (LONGLONG_COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  write_complex_pattern (pat, MODE (LONGLONG_REAL), &item[0], &item[MOID_SIZE (MODE (LONGLONG_REAL))], ref_file);
	}
      else
	{
/* Try writing as two LONG LONG REAL values. */
	  genie_write_longlong_real_format (pat, item, ref_file);
	  genie_write_standard_format (p, MODE (LONGLONG_REAL), &item[MOID_SIZE (MODE (LONGLONG_REAL))], ref_file);
	}
    }
  else if (mode == MODE (BOOL))
    {
      A68_BOOL *z = (A68_BOOL *) item;
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  add_char_transput_buffer (p, FORMATTED_BUFFER, (z->value == A_TRUE ? FLIP_CHAR : FLOP_CHAR));
	}
      else if (WHETHER (pat, BOOLEAN_PATTERN))
	{
	  if (NEXT_SUB (pat) == NULL)
	    {
	      add_char_transput_buffer (p, FORMATTED_BUFFER, (z->value == A_TRUE ? FLIP_CHAR : FLOP_CHAR));
	    }
	  else
	    {
	      write_boolean_pattern (pat, ref_file, z->value == A_TRUE);
	    }
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (BITS))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  char *str = (char *) STACK_TOP;
	  genie_value_to_string (p, mode, item);
	  add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	}
      else if (WHETHER (pat, BITS_PATTERN))
	{
	  write_bits_pattern (pat, MODE (BITS), item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  char *str = (char *) STACK_TOP;
	  genie_value_to_string (p, mode, item);
	  add_string_transput_buffer (p, FORMATTED_BUFFER, str);
	}
      else if (WHETHER (pat, BITS_PATTERN))
	{
	  write_bits_pattern (pat, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (CHAR))
    {
      A68_CHAR *z = (A68_CHAR *) item;
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  add_char_transput_buffer (p, FORMATTED_BUFFER, z->value);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  char *q = get_transput_buffer (EDIT_BUFFER);
	  add_char_transput_buffer (p, EDIT_BUFFER, z->value);
	  write_string_pattern (pat, mode, ref_file, &q);
	  if (q[0] != '\0')
	    {
	      value_error (p, mode, ref_file);
	    }
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  char q[2];
	  q[0] = z->value;
	  q[1] = '\0';
	  write_string_c_style (pat, q);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (BYTES))
    {
      A68_BYTES *z = (A68_BYTES *) item;
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  add_string_transput_buffer (p, FORMATTED_BUFFER, z->value);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  char *q = z->value;
	  write_string_pattern (pat, mode, ref_file, &q);
	  if (q[0] != '\0')
	    {
	      value_error (p, mode, ref_file);
	    }
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  char *q = z->value;
	  write_string_c_style (pat, q);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (LONG_BYTES))
    {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) item;
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  add_string_transput_buffer (p, FORMATTED_BUFFER, z->value);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  char *q = z->value;
	  write_string_pattern (pat, mode, ref_file, &q);
	  if (q[0] != '\0')
	    {
	      value_error (p, mode, ref_file);
	    }
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  char *q = z->value;
	  write_string_c_style (pat, q);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
/* Handle these separately instead of printing [] CHAR. */
      A68_REF row = *(A68_REF *) item;
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  PUSH_REF (p, row);
	  add_string_from_stack_transput_buffer (p, FORMATTED_BUFFER);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  char *q;
	  PUSH_REF (p, row);
	  reset_transput_buffer (EDIT_BUFFER);
	  add_string_from_stack_transput_buffer (p, EDIT_BUFFER);
	  q = get_transput_buffer (EDIT_BUFFER);
	  write_string_pattern (pat, mode, ref_file, &q);
	  if (q[0] != '\0')
	    {
	      value_error (p, mode, ref_file);
	    }
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  char *q;
	  PUSH_REF (p, row);
	  reset_transput_buffer (EDIT_BUFFER);
	  add_string_from_stack_transput_buffer (p, EDIT_BUFFER);
	  q = get_transput_buffer (EDIT_BUFFER);
	  write_string_c_style (pat, q);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      genie_write_standard_format (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  BYTE_T *elem = &item[q->offset];
	  genie_check_initialisation (p, elem, MOID (q), NULL);
	  genie_write_standard_format (p, MOID (q), elem, ref_file);
	}
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      BYTE_T *elem = &base_addr[elem_addr];
	      genie_check_initialisation (p, elem, SUB (deflexed), NULL);
	      genie_write_standard_format (p, SUB (deflexed), elem, ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
purge_format_write: at end of write purge all insertions.

Problem here is shutting down embedded formats.                               */

static void
purge_format_write (NODE_T * p, A68_REF ref_file)
{
  BOOL_T go_on;
  do
    {
      A68_FILE *file;
      NODE_T *dollar, *pat;
      A68_FORMAT *old_fmt;
      while ((pat = get_next_format_pattern (p, ref_file, SKIP_PATTERN)) != NULL)
	{
	  format_error (p, ref_file);
	}
      file = FILE_DEREF (&ref_file);
      dollar = SUB (file->format.top);
      old_fmt = (A68_FORMAT *) FRAME_LOCAL (frame_pointer, TAX (dollar)->offset);
      go_on = !IS_NIL_FORMAT (old_fmt);
      if (go_on)
	{
/* Pop embedded format and proceed. */
	  end_of_format (p, ref_file);
	}
    }
  while (go_on);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC ([] SIMPLOUT) VOID print f, write f                                      */

void
genie_write_format (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  genie_stand_out (p);
  PUSH_REF (p, row);
  genie_write_file_format (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, [] SIMPLOUT) VOID put f                                       */

void
genie_write_file_format (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index, formats;
  ADDR_T save_frame_pointer, save_stack_pointer;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLOUT));
  TEST_NIL (p, row, MODE (ROW_SIMPLOUT));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->read_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "read");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.put)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "putting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_WRITE_ACCESS, A68_PROTECTION)) == -1)
	{
	  open_error (p, ref_file, "putting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_FALSE;
	  file->write_mood = A_TRUE;
	  file->char_mood = A_TRUE;
	}
    }
  if (!file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "binary");
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Save stack state since formats have frames. */
  save_frame_pointer = file->frame_pointer;
  save_stack_pointer = file->stack_pointer;
  file->frame_pointer = frame_pointer;
  file->stack_pointer = stack_pointer;
/* Process [] SIMPLOUT. */
  if (file->format.top != NULL)
    {
      open_format_frame (file, &(file->format), NOT_EMBEDDED_FORMAT, A_FALSE);
    }
  formats = 0;
  base_address = ADDRESS (&(arr->array));
  elem_index = 0;
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & (base_address[elem_index]);
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = &(base_address[elem_index + SIZE_OF (A68_POINTER)]);
      if (mode == MODE (FORMAT))
	{
/* Forget about eventual active formats and set up new one. */
	  if (formats > 0)
	    {
	      purge_format_write (p, ref_file);
	    }
	  formats++;
	  frame_pointer = file->frame_pointer;
	  stack_pointer = file->stack_pointer;
	  open_format_frame (file, (A68_FORMAT *) item, NOT_EMBEDDED_FORMAT, A_TRUE);
	}
      else if (mode == MODE (PROC_REF_FILE_VOID))
	{
/* Ignore
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
*/
	}
      else
	{
	  genie_write_standard_format (p, mode, item, ref_file);
	}
      elem_index += MOID_SIZE (MODE (SIMPLOUT));
    }
/* Empty the format to purge insertions. */
  purge_format_write (p, ref_file);
/* Dump the buffer. */
  if (!(file->fd == STDOUT_FILENO && halt_typing))
    {
      io_write_string (file->fd, get_transput_buffer (FORMATTED_BUFFER));
      reset_transput_buffer (FORMATTED_BUFFER);
    }
/* Forget about active formats. */
  frame_pointer = file->frame_pointer;
  stack_pointer = file->stack_pointer;
  file->frame_pointer = save_frame_pointer;
  file->stack_pointer = save_stack_pointer;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
expect: give a value error in case a character is not among expected ones.    */

static BOOL_T
expect (NODE_T * p, MOID_T * m, A68_REF ref_file, const char *items, char ch)
{
  if (strchr (items, ch) == NULL)
    {
      value_error (p, m, ref_file);
      return (A_FALSE);
    }
  else
    {
      return (A_TRUE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_single_char: read one char from file.                                    */

static char
read_single_char (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  int ch = char_scanner (file);
  if (ch == EOF)
    {
      end_of_file_error (p, ref_file);
    }
  return (ch);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_n_chars: scan n chars from file to input buffer.                         */

static void
scan_n_chars (NODE_T * p, int n, MOID_T * m, A68_REF ref_file)
{
  int k;
  (void) m;
  for (k = 0; k < n; k++)
    {
      int ch = read_single_char (p, ref_file);
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_insertion: read a group of insertions.

Algol68G does not check whether the insertions are textually there. It just 
skips them. This because we blank literals in sign moulds before the sign is 
put, which is non-standard Algol68, but convenient.                           */

void
read_insertion (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  for (; p != NULL; p = NEXT (p))
    {
      read_insertion (SUB (p), ref_file);
      if (WHETHER (p, FORMAT_ITEM_L))
	{
	  BOOL_T go_on = !file->eof;
	  while (go_on)
	    {
	      int ch = read_single_char (p, ref_file);
	      go_on = (ch != '\n') && (ch != EOF) && !file->eof;
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_P))
	{
	  BOOL_T go_on = !file->eof;
	  while (go_on)
	    {
	      int ch = read_single_char (p, ref_file);
	      go_on = (ch != '\f') && (ch != EOF) && !file->eof;
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_X) || WHETHER (p, FORMAT_ITEM_Q))
	{
	  if (!file->eof)
	    {
	      (void) read_single_char (p, ref_file);
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_Y))
	{
	  /* Not implemented - parser has warned you. */ ;
	}
      else if (WHETHER (p, LITERAL))
	{
	  /* Skip characters, don't check whether the literal wordly is there. */
	  int len = strlen (SYMBOL (p));
	  while (len-- && !file->eof)
	    {
	      (void) read_single_char (p, ref_file);
	    }
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  if (ATTRIBUTE (SUB (NEXT (p))) != FORMAT_ITEM_K)
	    {
	      for (j = 1; j <= k; j++)
		{
		  read_insertion (NEXT (p), ref_file);
		}
	    }
	  else
	    {
	      int pos = get_transput_buffer_index (INPUT_BUFFER);
	      for (j = 1; j < (k - pos); j++)
		{
		  if (!file->eof)
		    {
		      (void) read_single_char (p, ref_file);
		    }
		}
	    }
	  return;		/* Don't delete this! */
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_string_pattern: read string from ref_file cf. current format.            */

static void
read_string_pattern (NODE_T * p, MOID_T * m, A68_REF ref_file)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  read_insertion (SUB (p), ref_file);
	}
      else if (WHETHER (p, FORMAT_ITEM_A))
	{
	  scan_n_chars (p, 1, m, ref_file);
	}
      else if (WHETHER (p, FORMAT_ITEM_S))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, BLANK_CHAR);
	  return;
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      read_string_pattern (NEXT (p), m, ref_file);
	    }
	  return;
	}
      else
	{
	  read_string_pattern (SUB (p), m, ref_file);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_string_c_style: read str from ref_file using C style format %[-][w]s.    */

static void
read_string_c_style (NODE_T * p, MOID_T * m, A68_REF ref_file)
{
  if (WHETHER (p, STRING_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
/* Skip sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  q = NEXT (q);
	}
/* If width is specified than get exactly that width. */
      if (WHETHER (q, REPLICATOR))
	{
	  int width = get_replicator_value (SUB (q));
	  scan_n_chars (p, width, m, ref_file);
	}
      else
	{
	  genie_read_standard (p, MODE (ROW_CHAR), (BYTE_T *) get_transput_buffer (INPUT_BUFFER), ref_file);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
traverse_choice_pattern: how many literals whether 'len' chars of 'str'         */

static void
traverse_choice_pattern (NODE_T * p, char *str, int len, int *count, int *matches, int *first_match, BOOL_T * full_match)
{
  for (; p != NULL; p = NEXT (p))
    {
      traverse_choice_pattern (SUB (p), str, len, count, matches, first_match, full_match);
      if (WHETHER (p, LITERAL))
	{
	  (*count)++;
	  if (strncmp (SYMBOL (p), str, len) == 0)
	    {
	      (*matches)++;
	      *full_match |= (strcmp (SYMBOL (p), str) == 0);
	      if (*first_match == 0 && *full_match)
		{
		  *first_match = *count;
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_choice_pattern: read appropriate insertion from a choice pattern.

This implementation does not have the RR peculiarity that longest
matching literal must be first, in case of non-unique first chars.            */

static int
read_choice_pattern (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  BOOL_T cont = A_TRUE;
  int longest_match = 0, longest_match_len = 0;
  while (cont)
    {
      int ch = char_scanner (file);
      if (!file->eof)
	{
	  int len, count = 0, matches = 0, first_match = 0;
	  BOOL_T full_match = A_FALSE;
	  add_char_transput_buffer (p, INPUT_BUFFER, ch);
	  len = get_transput_buffer_index (INPUT_BUFFER);
	  traverse_choice_pattern (p, get_transput_buffer (INPUT_BUFFER), len, &count, &matches, &first_match, &full_match);
	  if (full_match && matches == 1 && first_match > 0)
	    {
	      return (first_match);
	    }
	  else if (full_match && matches > 1 && first_match > 0)
	    {
	      longest_match = first_match;
	      longest_match_len = len;
	    }
	  else if (matches == 0)
	    {
	      cont = A_FALSE;
	    }
	}
      else
	{
	  cont = A_FALSE;
	}
    }
  if (longest_match > 0)
    {
/* Push back look-ahead chars. */
      if (get_transput_buffer_index (INPUT_BUFFER) > 0)
	{
	  char *z = get_transput_buffer (INPUT_BUFFER);
	  file->eof = A_FALSE;
	  add_string_transput_buffer (p, file->transput_buffer, &z[longest_match_len]);
	}
      return (longest_match);
    }
  else
    {
      value_error (p, MODE (INT), ref_file);
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_number_generic: read value according to a general-pattern.               */

static void
read_number_generic (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  A68_REF row;
  EXECUTE_UNIT (NEXT_SUB (p));
/* RR says to ignore parameters just calculated, so we will. */
  POP_REF (p, &row);
  genie_read_standard (p, mode, item, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_number_c_style: handle %[+][-][w]d, %[+][-][w][.][d]f/e formats.         */

static void
read_number_c_style (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  BOOL_T sign;
  int width, after;
/* Integral C pattern. */
  if (WHETHER (p, INTEGRAL_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
      if (mode != MODE (INT) && mode != MODE (LONG_INT) && mode != MODE (LONGLONG_INT))
	{
	  pattern_error (p, mode, ATTRIBUTE (p));
	  return;
	}
/* Get sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  sign = ATTRIBUTE (q);
	  q = NEXT (q);
	}
      else
	{
	  sign = 0;
	}
/* Get width. */
      if (WHETHER (q, REPLICATOR))
	{
	  width = get_replicator_value (SUB (q));
	}
      else
	{
	  width = 0;
	}
/* Read string. */
      if (width == 0)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else
	{
	  scan_n_chars (p, (sign != 0) ? width + 1 : width, mode, ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
    }
/* Real C patterns. */
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      NODE_T *q = NEXT_SUB (p);
      if (mode != MODE (REAL) && mode != MODE (LONG_REAL) && mode != MODE (LONGLONG_REAL))
	{
	  pattern_error (p, mode, ATTRIBUTE (p));
	  return;
	}
/* Get sign. */
      if (WHETHER (q, FORMAT_ITEM_PLUS) || WHETHER (q, FORMAT_ITEM_MINUS))
	{
	  sign = ATTRIBUTE (q);
	  q = NEXT (q);
	}
      else
	{
	  sign = 0;
	}
/* Get width. */
      if (WHETHER (q, REPLICATOR))
	{
	  width = get_replicator_value (SUB (q));
	  q = NEXT (q);
	}
      else
	{
	  width = 0;
	}
/* Skip point. */
      if (WHETHER (q, FORMAT_ITEM_POINT))
	{
	  q = NEXT (q);
	}
/* Get decimals. */
      if (WHETHER (q, REPLICATOR))
	{
	  after = get_replicator_value (SUB (q));
	  q = NEXT (q);
	}
      else
	{
	  after = 0;
	}
/* Read string. */
      if (width == 0)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else
	{
	  scan_n_chars (p, (sign != 0) ? width + 1 : width, mode, ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
INTEGRAL, REAL, COMPLEX and BITS patterns.                                    */

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_sign_mould: read sign-mould cf. current format.                          */

static void
read_sign_mould (NODE_T * p, MOID_T * m, A68_REF ref_file, int *sign)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  read_insertion (SUB (p), ref_file);
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      read_sign_mould (NEXT (p), m, ref_file, sign);
	    }
	  return;		/* Leave this! */
	}
      else
	{
	  switch (ATTRIBUTE (p))
	    {
	    case FORMAT_ITEM_Z:
	    case FORMAT_ITEM_D:
	    case FORMAT_ITEM_S:
	    case FORMAT_ITEM_PLUS:
	    case FORMAT_ITEM_MINUS:
	      {
		int ch = read_single_char (p, ref_file);
/* When a sign has been read, digits are expected. */
		if (*sign != 0)
		  {
		    if (expect (p, m, ref_file, INT_DIGITS, ch))
		      {
			add_char_transput_buffer (p, INPUT_BUFFER, ch);
		      }
		    else
		      {
			add_char_transput_buffer (p, INPUT_BUFFER, '0');
		      }
/* When a sign has not been read, a sign is expected.  If there is a digit 
   in stead of a sign, the digit is accepted and '+' is assumed; RR demands a 
   space to preceed the digit, Algol68G does not. */
		  }
		else
		  {
		    if (strchr (SIGN_DIGITS, ch) != NULL)
		      {
			if (ch == '+')
			  {
			    *sign = 1;
			  }
			else if (ch == '-')
			  {
			    *sign = -1;
			  }
			else if (ch == BLANK_CHAR)
			  {
			    /* skip */ ;
			  }
		      }
		    else if (expect (p, m, ref_file, INT_DIGITS, ch))
		      {
			add_char_transput_buffer (p, INPUT_BUFFER, ch);
			*sign = 1;
		      }
		  }
		break;
	      }
	    default:
	      {
		read_sign_mould (SUB (p), m, ref_file, sign);
		break;
	      }
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_integral_mould: read mould cf. current format.                           */

static void
read_integral_mould (NODE_T * p, MOID_T * m, A68_REF ref_file)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  read_insertion (SUB (p), ref_file);
	}
      else if (WHETHER (p, REPLICATOR))
	{
	  int j, k = get_replicator_value (SUB (p));
	  for (j = 1; j <= k; j++)
	    {
	      read_integral_mould (NEXT (p), m, ref_file);
	    }
	  return;		/* Leave this! */
	}
      else if (WHETHER (p, FORMAT_ITEM_Z))
	{
	  int ch = read_single_char (p, ref_file);
	  const char *digits = (m == MODE (BITS) || m == MODE (LONG_BITS) || m == MODE (LONGLONG_BITS)) ? BITS_DIGITS_BLANK : INT_DIGITS_BLANK;
	  if (expect (p, m, ref_file, digits, ch))
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, (ch == BLANK_CHAR) ? '0' : ch);
	    }
	  else
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, '0');
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_D))
	{
	  int ch = read_single_char (p, ref_file);
	  const char *digits = (m == MODE (BITS) || m == MODE (LONG_BITS) || m == MODE (LONGLONG_BITS)) ? BITS_DIGITS : INT_DIGITS;
	  if (expect (p, m, ref_file, digits, ch))
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, ch);
	    }
	  else
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, '0');
	    }
	}
      else if (WHETHER (p, FORMAT_ITEM_S))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, '0');
	}
      else
	{
	  read_integral_mould (SUB (p), m, ref_file);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_integral_pattern: read pattern cf. current format.                       */

static void
read_integral_pattern (NODE_T * p, MOID_T * m, BYTE_T * item, A68_REF ref_file)
{
  NODE_T *q = SUB (p);
  if (q != NULL && WHETHER (q, SIGN_MOULD))
    {
      int sign = 0;
      char *z;
      add_char_transput_buffer (p, INPUT_BUFFER, BLANK_CHAR);
      read_sign_mould (SUB (q), m, ref_file, &sign);
      z = get_transput_buffer (INPUT_BUFFER);
      z[0] = (sign == -1) ? '-' : '+';
      q = NEXT (q);
    }
  if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
    {
      read_integral_mould (SUB (q), m, ref_file);
    }
  genie_string_to_value (p, m, item, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_pie_frame: read '.', 'E' or 'I'.                                         */

static void
read_pie_frame (NODE_T * p, MOID_T * m, A68_REF ref_file, int att, int item, char ch)
{
/* Widen ch to a stringlet. */
  char sym[3];
  sym[0] = ch;
  sym[1] = TO_LOWER (ch);
  sym[2] = '\0';
/* Now read the frame. */
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, INSERTION))
	{
	  read_insertion (p, ref_file);
	}
      else if (WHETHER (p, att))
	{
	  read_pie_frame (SUB (p), m, ref_file, att, item, ch);
	  return;
	}
      else if (WHETHER (p, FORMAT_ITEM_S))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, sym[0]);
	  return;
	}
      else if (WHETHER (p, item))
	{
	  int ch0 = read_single_char (p, ref_file);
	  if (expect (p, m, ref_file, sym, ch0))
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, sym[0]);
	    }
	  else
	    {
	      add_char_transput_buffer (p, INPUT_BUFFER, sym[0]);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_real_pattern: read REAL value using real pattern.                        */

static void
read_real_pattern (NODE_T * p, MOID_T * m, BYTE_T * item, A68_REF ref_file)
{
/* Dive into pattern. */
  NODE_T *q = (WHETHER (p, REAL_PATTERN)) ? SUB (p) : p;
/* Dissect pattern. */
  if (q != NULL && WHETHER (q, SIGN_MOULD))
    {
      int sign = 0;
      char *z;
      add_char_transput_buffer (p, INPUT_BUFFER, BLANK_CHAR);
      read_sign_mould (SUB (q), m, ref_file, &sign);
      z = get_transput_buffer (INPUT_BUFFER);
      z[0] = (sign == -1) ? '-' : '+';
      q = NEXT (q);
    }
  if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
    {
      read_integral_mould (SUB (q), m, ref_file);
      q = NEXT (q);
    }
  if (q != NULL && WHETHER (q, FORMAT_POINT_FRAME))
    {
      read_pie_frame (SUB (q), m, ref_file, FORMAT_POINT_FRAME, FORMAT_ITEM_POINT, '.');
      q = NEXT (q);
    }
  if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
    {
      read_integral_mould (SUB (q), m, ref_file);
      q = NEXT (q);
    }
  if (q != NULL && WHETHER (q, EXPONENT_FRAME))
    {
      read_pie_frame (SUB (q), m, ref_file, FORMAT_E_FRAME, FORMAT_ITEM_E, EXPONENT_CHAR);
      q = NEXT_SUB (q);
      if (q != NULL && WHETHER (q, SIGN_MOULD))
	{
	  int k, sign = 0;
	  char *z;
	  add_char_transput_buffer (p, INPUT_BUFFER, BLANK_CHAR);
	  k = get_transput_buffer_index (INPUT_BUFFER);
	  read_sign_mould (SUB (q), m, ref_file, &sign);
	  z = get_transput_buffer (INPUT_BUFFER);
	  z[k - 1] = (sign == -1) ? '-' : '+';
	  q = NEXT (q);
	}
      if (q != NULL && WHETHER (q, INTEGRAL_MOULD))
	{
	  read_integral_mould (SUB (q), m, ref_file);
	  q = NEXT (q);
	}
    }
  genie_string_to_value (p, m, item, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_complex_pattern: read COMPLEX value using complex pattern.               */

static void
read_complex_pattern (NODE_T * p, MOID_T * comp, MOID_T * m, BYTE_T * re, BYTE_T * im, A68_REF ref_file)
{
  NODE_T *real, *plus_i_times, *imag;
/* Dissect pattern. */
  real = SUB (p);
  plus_i_times = NEXT (real);
  imag = NEXT (plus_i_times);
/* Read pattern. */
  read_real_pattern (real, m, re, ref_file);
  reset_transput_buffer (INPUT_BUFFER);
  read_pie_frame (plus_i_times, comp, ref_file, FORMAT_I_FRAME, FORMAT_ITEM_I, 'I');
  reset_transput_buffer (INPUT_BUFFER);
  read_real_pattern (imag, m, im, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_bits_pattern: read BITS value cf. pattern.                               */

static void
read_bits_pattern (NODE_T * p, MOID_T * m, BYTE_T * item, A68_REF ref_file)
{
  int radix;
  char *z;
  radix = get_replicator_value (SUB (SUB (p)));
  if (radix < 2 || radix > 16)
    {
      diagnostic (A_RUNTIME_ERROR, p, "invalid radix D", radix);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  z = get_transput_buffer (INPUT_BUFFER);
  sprintf (z, "%dr", radix);
  set_transput_buffer_index (INPUT_BUFFER, strlen (z));
  read_integral_mould (NEXT_SUB (p), m, ref_file);
  genie_string_to_value (p, m, item, ref_file);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_real_format: read object with "mode" from "ref_file" and put at "item".  */

static void
genie_read_real_format (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) == NULL)
    {
      genie_read_standard (p, mode, item, ref_file);
    }
  else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
    {
      read_number_generic (p, mode, item, ref_file);
    }
  else if (WHETHER (p, FIXED_C_PATTERN) || WHETHER (p, FLOAT_C_PATTERN))
    {
      read_number_c_style (p, mode, item, ref_file);
    }
  else if (WHETHER (p, REAL_PATTERN))
    {
      read_real_pattern (p, mode, item, ref_file);
    }
  else
    {
      pattern_error (p, mode, ATTRIBUTE (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_read_standard_format: read "mode" from "ref_file" and put at "item".    */

static void
genie_read_standard_format (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  errno = 0;
  reset_transput_buffer (INPUT_BUFFER);
  if (mode == MODE (INT) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (pat, mode, item, ref_file);
	}
      else if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) != NULL)
	{
	  read_number_generic (pat, mode, item, ref_file);
	}
      else if (WHETHER (pat, INTEGRAL_C_PATTERN))
	{
	  read_number_c_style (pat, mode, item, ref_file);
	}
      else if (WHETHER (pat, INTEGRAL_PATTERN))
	{
	  read_integral_pattern (pat, mode, item, ref_file);
	}
      else if (WHETHER (pat, CHOICE_PATTERN))
	{
	  int k = read_choice_pattern (pat, ref_file);
	  if (mode == MODE (INT))
	    {
	      A68_INT *z = (A68_INT *) item;
	      z->value = k;
	      z->status = (z->value > 0) ? INITIALISED_MASK : NULL_MASK;
	    }
	  else
	    {
	      mp_digit *z = (mp_digit *) item;
	      if (k > 0)
		{
		  int_to_mp (p, z, k, get_mp_digits (mode));
		  z[0] = INITIALISED_MASK;
		}
	      else
		{
		  z[0] = NULL_MASK;
		}
	    }
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (REAL) || mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      genie_read_real_format (pat, mode, item, ref_file);
    }
  else if (mode == MODE (COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  read_complex_pattern (pat, mode, MODE (REAL), item, &item[MOID_SIZE (MODE (REAL))], ref_file);
	}
      else
	{
/* Try reading as two REAL values. */
	  genie_read_real_format (pat, MODE (REAL), item, ref_file);
	  genie_read_standard_format (p, MODE (REAL), &item[MOID_SIZE (MODE (REAL))], ref_file);
	}
    }
  else if (mode == MODE (LONG_COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  read_complex_pattern (pat, mode, MODE (LONG_REAL), item, &item[MOID_SIZE (MODE (LONG_REAL))], ref_file);
	}
      else
	{
/* Try reading as two LONG REAL values. */
	  genie_read_real_format (pat, MODE (LONG_REAL), item, ref_file);
	  genie_read_standard_format (p, MODE (LONG_REAL), &item[MOID_SIZE (MODE (LONG_REAL))], ref_file);
	}
    }
  else if (mode == MODE (LONGLONG_COMPLEX))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, COMPLEX_PATTERN))
	{
	  read_complex_pattern (pat, mode, MODE (LONGLONG_REAL), item, &item[MOID_SIZE (MODE (LONGLONG_REAL))], ref_file);
	}
      else
	{
/* Try reading as two LONG LONG REAL values. */
	  genie_read_real_format (pat, MODE (LONGLONG_REAL), item, ref_file);
	  genie_read_standard_format (p, MODE (LONGLONG_REAL), &item[MOID_SIZE (MODE (LONGLONG_REAL))], ref_file);
	}
    }
  else if (mode == MODE (BOOL))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, BOOLEAN_PATTERN))
	{
	  if (NEXT_SUB (pat) == NULL)
	    {
	      genie_read_standard (p, mode, item, ref_file);
	    }
	  else
	    {
	      A68_BOOL *z = (A68_BOOL *) item;
	      int k = read_choice_pattern (pat, ref_file);
	      if (k == 1 || k == 2)
		{
		  z->value = (k == 1) ? A_TRUE : A_FALSE;
		  z->status = INITIALISED_MASK;
		}
	      else
		{
		  z->status = NULL_MASK;
		}
	    }
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (BITS) || mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, BITS_PATTERN))
	{
	  read_bits_pattern (pat, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (CHAR))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  read_string_pattern (pat, MODE (CHAR), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  read_string_pattern (pat, MODE (CHAR), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (BYTES))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  read_string_pattern (pat, MODE (BYTES), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  read_string_c_style (pat, MODE (BYTES), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (LONG_BYTES))
    {
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  read_string_pattern (pat, MODE (LONG_BYTES), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  read_string_c_style (pat, MODE (LONG_BYTES), ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
/* Handle these separately instead of reading [] CHAR. */
      NODE_T *pat = get_next_format_pattern (p, ref_file, WANT_PATTERN);
      if (WHETHER (pat, GENERAL_PATTERN) && NEXT_SUB (pat) == NULL)
	{
	  genie_read_standard (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_PATTERN))
	{
	  read_string_pattern (pat, mode, ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else if (WHETHER (pat, STRING_C_PATTERN))
	{
	  read_string_c_style (pat, mode, ref_file);
	  genie_string_to_value (p, mode, item, ref_file);
	}
      else
	{
	  pattern_error (p, mode, ATTRIBUTE (pat));
	}
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      genie_read_standard_format (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  BYTE_T *elem = &item[q->offset];
	  genie_read_standard_format (p, MOID (q), elem, ref_file);
	}
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      BYTE_T *elem = &base_addr[elem_addr];
	      genie_read_standard_format (p, SUB (deflexed), elem, ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
purge_format_read: at end of read purge all insertions.                       */

static void
purge_format_read (NODE_T * p, A68_REF ref_file)
{
  BOOL_T go_on;
  do
    {
      A68_FILE *file;
      NODE_T *dollar, *pat;
      A68_FORMAT *old_fmt;
/*
    while (get_next_format_pattern (p, ref_file, SKIP_PATTERN) != NULL) {
	;
    }
*/
      while ((pat = get_next_format_pattern (p, ref_file, SKIP_PATTERN)) != NULL)
	{
	  format_error (p, ref_file);
	}
      file = FILE_DEREF (&ref_file);
      dollar = SUB (file->format.top);
      old_fmt = (A68_FORMAT *) FRAME_LOCAL (frame_pointer, TAX (dollar)->offset);
      go_on = !IS_NIL_FORMAT (old_fmt);
      if (go_on)
	{
/* Pop embedded format and proceed. */
	  end_of_format (p, ref_file);
	}
    }
  while (go_on);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_read_format: PROC ([] SIMPLIN) VOID read f.                             */

void
genie_read_format (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  genie_stand_in (p);
  PUSH_REF (p, row);
  genie_read_file_format (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_read_format: PROC (REF FILE, [] SIMPLIN) VOID get f.                    */

void
genie_read_file_format (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index, formats;
  ADDR_T save_frame_pointer, save_stack_pointer;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLIN));
  TEST_NIL (p, row, MODE (ROW_SIMPLIN));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "write");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.get)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "getting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_READ_ACCESS, 0)) == -1)
	{
	  open_error (p, ref_file, "getting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_TRUE;
	  file->write_mood = A_FALSE;
	  file->char_mood = A_TRUE;
	}
    }
  if (!file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "binary");
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Save stack state since formats have frames. */
  save_frame_pointer = file->frame_pointer;
  save_stack_pointer = file->stack_pointer;
  file->frame_pointer = frame_pointer;
  file->stack_pointer = stack_pointer;
/* Process [] SIMPLIN. */
  if (file->format.top != NULL)
    {
      open_format_frame (file, &(file->format), NOT_EMBEDDED_FORMAT, A_FALSE);
    }
  formats = 0;
  base_address = ADDRESS (&(arr->array));
  elem_index = 0;
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & (base_address[elem_index]);
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = (BYTE_T *) & (base_address[elem_index + SIZE_OF (A68_POINTER)]);
      if (mode == MODE (FORMAT))
	{
/* Forget about eventual active formats and set up new one. */
	  if (formats > 0)
	    {
	      purge_format_read (p, ref_file);
	    }
	  formats++;
	  frame_pointer = file->frame_pointer;
	  stack_pointer = file->stack_pointer;
	  open_format_frame (file, (A68_FORMAT *) item, NOT_EMBEDDED_FORMAT, A_TRUE);
	}
      else if (mode == MODE (PROC_REF_FILE_VOID))
	{
/* Ignore
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
*/
	}
      else
	{
	  TEST_NIL (p, *(A68_REF *) item, ref_file);
	  genie_read_standard_format (p, SUB (mode), ADDRESS ((A68_REF *) item), ref_file);
	}
      elem_index += MOID_SIZE (MODE (SIMPLIN));
    }
/* Empty the format to purge insertions. */
  purge_format_read (p, ref_file);
/* Forget about active formats. */
  frame_pointer = file->frame_pointer;
  stack_pointer = file->stack_pointer;
  file->frame_pointer = save_frame_pointer;
  file->stack_pointer = save_stack_pointer;
}
