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

#ifdef PRE_MACOS_X
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#ifndef WIN32_VERSION
#include <sys/resource.h>
#endif
#endif

void genie_preprocess (NODE_T *, int *);
void get_global_level (NODE_T *);

int frame_stack_size, expr_stack_size, heap_size, handle_pool_size;
jmp_buf genie_exit_label;
int global_level = 0, ret_code, ret_line_number, ret_char_number;
A68_HANDLE nil_handle = { INITIALISED_MASK, 0, 0, 0, NULL, NULL, NULL };
A68_REF nil_ref = { INITIALISED_MASK, NULL, 0, NULL };
A68_FORMAT nil_format = { INITIALISED_MASK, NULL, {INITIALISED_MASK, NULL, 0, NULL} };
A68_POINTER nil_pointer = { INITIALISED_MASK, NULL };
BYTE_T *frame_segment = NULL, *stack_segment = NULL, *heap_segment = NULL, *handle_segment = NULL;
ADDR_T frame_pointer = 0, stack_pointer = 0, heap_pointer = 0, handle_pointer = 0, global_pointer = 0;
int max_lex_lvl = 0;

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_idle: nop for the genie, for instance '+' for INT or REAL.              */

void
genie_idle (NODE_T * p)
{
  (void) p;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_system: pass string on the stack to OS for execution.                   */

void
genie_system (NODE_T * p)
{
  int ret_code, size;
  A68_REF cmd;
  A68_REF ref_z;
  POP (p, &cmd, SIZE_OF (A68_REF));
  TEST_INIT (p, cmd, MODE (STRING));
  size = 1 + a68_string_size (p, cmd);
  ref_z = heap_generator (p, MODE (C_STRING), 1 + size);
  ret_code = system (a_to_c_string (p, (char *) ADDRESS (&ref_z), cmd));
  PUSH_INT (p, ret_code);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_break: bring interpreter to monitor mode, same as raise (SIGINT).       */

void
genie_break (NODE_T * p)
{
  (void) p;
  sys_request_flag = A_TRUE;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
exit_genie: leave interpretation.                                             */

void
exit_genie (NODE_T * p, int ret)
{
  ret_line_number = LINE (p)->number;
  ret_code = ret;
  longjmp (genie_exit_label, 1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_init_rng.                                                               */

void
genie_init_rng (void)
{
  time_t t;
  if (time (&t) != -1)
    {
      struct tm *u = localtime (&t);
      int seed = u->tm_sec + 60 * (u->tm_min + 60 * u->tm_hour);
      init_rng (seed);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tie_label_to_serial: tie label to the clause it is defined in.                */

void
tie_label_to_serial (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  BOOL_T valid_follow;
	  if (NEXT (p) == NULL)
	    {
	      valid_follow = A_TRUE;
	    }
	  else if (WHETHER (NEXT (p), CLOSE_SYMBOL))
	    {
	      valid_follow = A_TRUE;
	    }
	  else if (WHETHER (NEXT (p), END_SYMBOL))
	    {
	      valid_follow = A_TRUE;
	    }
	  else if (WHETHER (NEXT (p), EDOC_SYMBOL))
	    {
	      valid_follow = A_TRUE;
	    }
	  else if (WHETHER (NEXT (p), OD_SYMBOL))
	    {
	      valid_follow = A_TRUE;
	    }
	  else
	    {
	      valid_follow = A_FALSE;
	    }
	  if (valid_follow)
	    {
	      SYMBOL_TABLE (SUB (p))->jump_to = NULL;
	    }
	}
      tie_label_to_serial (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tie_label: tie label to the clause it is defined in.                          */

static void
tie_label (NODE_T * p, NODE_T * unit)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAX (p)->unit = unit;
	}
      tie_label (SUB (p), unit);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tie_label_unit: tie label to the clause it is defined in.                     */

void
tie_label_to_unit (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, LABELED_UNIT))
	{
	  tie_label (SUB (SUB (p)), NEXT (SUB (p)));
	}
      tie_label_to_unit (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
protect_from_sweep: protect constructs from premature sweeping.

Insert annotations in the tree that prevent premature sweeping of temporary 
names and rows. For instance, let x, y be PROC STRING, then x + y can crash by 
the heap sweeper. Annotations are local hence when the block is exited they 
become prone to the heap sweeper.                                             */

void
protect_from_sweep (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      protect_from_sweep (SUB (p));
      p->protect_sweep = NULL;
/* Catch all constructs that give vulnarable intermediate results on the stack.
   Units do not apply, casts work through their enclosed-clauses, denoters are
   protected and identifiers protect themselves.                             */
      switch (ATTRIBUTE (p))
	{
	case FORMULA:
	case MONADIC_FORMULA:
	case GENERATOR:
	case ENCLOSED_CLAUSE:
	case CALL:
	case SLICE:
	case SELECTION:
	case DEPROCEDURING:
	case ROWING:
	  {
	    MOID_T *m = MOID (p);
	    if (m != NULL && (WHETHER (m, REF_SYMBOL) || WHETHER (DEFLEX (m), ROW_SYMBOL)))
	      {
		TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, m, PROTECT_FROM_SWEEP);
		p->protect_sweep = z;
		HEAP (z) = HEAP_SYMBOL;
		z->use = A_TRUE;
	      }
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_attribute: fast way to indicate a mode.                                  */

static int
mode_attribute (MOID_T * p)
{
  if (WHETHER (p, REF_SYMBOL))
    {
      return (REF_SYMBOL);
    }
  else if (WHETHER (p, PROC_SYMBOL))
    {
      return (PROC_SYMBOL);
    }
  else if (WHETHER (p, UNION_SYMBOL))
    {
      return (UNION_SYMBOL);
    }
  else if (p == MODE (INT))
    {
      return (MODE_INT);
    }
  else if (p == MODE (LONG_INT))
    {
      return (MODE_LONG_INT);
    }
  else if (p == MODE (LONGLONG_INT))
    {
      return (MODE_LONGLONG_INT);
    }
  else if (p == MODE (REAL))
    {
      return (MODE_REAL);
    }
  else if (p == MODE (LONG_REAL))
    {
      return (MODE_LONG_REAL);
    }
  else if (p == MODE (LONGLONG_REAL))
    {
      return (MODE_LONGLONG_REAL);
    }
  else if (p == MODE (COMPLEX))
    {
      return (MODE_COMPLEX);
    }
  else if (p == MODE (LONG_COMPLEX))
    {
      return (MODE_LONG_COMPLEX);
    }
  else if (p == MODE (LONGLONG_COMPLEX))
    {
      return (MODE_LONGLONG_COMPLEX);
    }
  else if (p == MODE (BOOL))
    {
      return (MODE_BOOL);
    }
  else if (p == MODE (CHAR))
    {
      return (MODE_CHAR);
    }
  else if (p == MODE (BITS))
    {
      return (MODE_BITS);
    }
  else if (p == MODE (LONG_BITS))
    {
      return (MODE_LONG_BITS);
    }
  else if (p == MODE (LONGLONG_BITS))
    {
      return (MODE_LONGLONG_BITS);
    }
  else if (p == MODE (BYTES))
    {
      return (MODE_BYTES);
    }
  else if (p == MODE (LONG_BYTES))
    {
      return (MODE_LONG_BYTES);
    }
  else if (p == MODE (FILE))
    {
      return (MODE_FILE);
    }
  else if (p == MODE (FORMAT))
    {
      return (MODE_FORMAT);
    }
  else if (p == MODE (PIPE))
    {
      return (MODE_PIPE);
    }
  else
    {
      return (MODE_NO_CHECK);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_empty_table: whether a symbol table contains no definition.             */

static BOOL_T
genie_empty_table (SYMBOL_TABLE_T * t)
{
  return (t->identifiers == NULL && t->operators == NULL && PRIO (t) == NULL && t->indicants == NULL && t->labels == NULL && t->local_identifiers == NULL && t->local_operators == NULL && t->anonymous == NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_preprocess: perform tasks before interpretation.                        */

void
genie_preprocess (NODE_T * p, int *max_lev)
{
  for (; p != NULL; p = NEXT (p))
    {
      p->genie.whether_coercion = whether_coercion (p);
      p->genie.whether_new_lexical_level = whether_new_lexical_level (p);
      p->genie.propagator.unit = genie_unit;
      p->genie.propagator.source = p;
      if (MOID (p) != NULL)
	{
	  MOID (p)->size = moid_size (MOID (p));
	  MOID (p)->short_id = mode_attribute (MOID (p));
	}
      if (SYMBOL_TABLE (p) != NULL)
	{
	  SYMBOL_TABLE (p)->empty_table = genie_empty_table (SYMBOL_TABLE (p));
	  if (LEX_LEVEL (p) > *max_lev)
	    {
	      *max_lev = LEX_LEVEL (p);
	    }
	}
      if (WHETHER (p, FORMAT_TEXT))
	{
	  TAG_T *q = TAX (p);
	  if (q != NULL && NODE (q) != NULL)
	    {
	      NODE (q) = p;
	    }
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *q = TAX (p);
	  if (q != NULL && NODE (q) != NULL && SYMBOL_TABLE (NODE (q)) != NULL)
	    {
	      p->genie.level = LEX_LEVEL (NODE (q));
	    }
	}
      else if (WHETHER (p, IDENTIFIER))
	{
	  TAG_T *q = TAX (p);
	  if (q != NULL && NODE (q) != NULL && SYMBOL_TABLE (NODE (q)) != NULL)
	    {
	      p->genie.level = LEX_LEVEL (NODE (q));
	      p->genie.offset = &frame_segment[FRAME_INFO_SIZE + q->offset];
	    }
	}
      else if (WHETHER (p, OPERATOR))
	{
	  TAG_T *q = TAX (p);
	  if (q != NULL && NODE (q) != NULL && SYMBOL_TABLE (NODE (q)) != NULL)
	    {
	      p->genie.level = LEX_LEVEL (NODE (q));
	      p->genie.offset = &frame_segment[FRAME_INFO_SIZE + q->offset];
	    }
	}
      if (SUB (p) != NULL)
	{
	  PARENT (SUB (p)) = p;
	  genie_preprocess (SUB (p), max_lev);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_global_level: outermost lexical level in the user program.                */

void
get_global_level (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (LINE (p)->number != 0)
	{
	  if (find_keyword (top_keyword, SYMBOL (p)) == NULL)
	    {
	      if (LEX_LEVEL (p) < global_level || global_level == 0)
		{
		  global_level = LEX_LEVEL (p);
		}
	    }
	}
      get_global_level (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
free_genie_heap: free heap allocated by genie.                                */

static void
free_genie_heap (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      free_genie_heap (SUB (p));
      if (p->genie.constant != NULL)
	{
	  free (p->genie.constant);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie: driver for the interpreter.                                            */

void
genie (MODULE_T * module)
{
  max_lex_lvl = 0;
/*  genie_lex_levels (module->top_node, 1); */
  genie_preprocess (module->top_node, &max_lex_lvl);
  sys_request_flag = A_FALSE;
  genie_init_rng ();
  io_close_tty_line ();
  if (module->options.trace)
    {
      sprintf (output_line, "%sgenie 1.0: frame stack %uk, expression stack %uk, heap %uk, handles %uk\n", BARS, frame_stack_size / 1024, expr_stack_size / 1024, heap_size / 1024, handle_pool_size / 1024);
      io_write_string (STDOUT_FILENO, output_line);
    }
  install_signal_handlers ();
/* Dive into the program */
  if (setjmp (genie_exit_label) == 0)
    {
      NODE_T *p = SUB (module->top_node);
      errno = 0;
      ret_code = 0;
      global_level = 0;
      get_global_level (p);
      global_pointer = 0;
      frame_pointer = 0;
      stack_pointer = 0;
      FRAME_DYNAMIC_LINK (frame_pointer) = 0;
      FRAME_STATIC_LINK (frame_pointer) = 0;
      FRAME_TREE (frame_pointer) = (NODE_T *) p;
      FRAME_LEXICAL_LEVEL (frame_pointer) = LEX_LEVEL (p);
      initialise_frame (p);
      genie_init_heap (p, module);
      genie_init_transput (module->top_node);
      cputime_0 = seconds ();
      (void) genie_enclosed (SUB (module->top_node));
    }
  else
    {
/* Abnormal end of program */
      if (ret_code == A_RUNTIME_ERROR)
	{
	  if (module->files.listing.opened)
	    {
	      sprintf (output_line, "\n%sstack traceback", BARS);
	      io_write_string (module->files.listing.fd, output_line);
	      stack_dump (module->files.listing.fd, frame_pointer, 128);
	    }
	}
    }
/* Free heap allocated by genie. */
  free_genie_heap (module->top_node);
/* Normal end of program */
  diagnostics_to_terminal (module->top_line, A_RUNTIME_ERROR);
  if (module->options.trace)
    {
      sprintf (output_line, "\n%sgenie finishes: %.2f seconds\n", BARS, seconds () - cputime_0);
      io_write_string (STDOUT_FILENO, output_line);
    }
}
