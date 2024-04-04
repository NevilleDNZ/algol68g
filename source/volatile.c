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

/*-------1---------2---------3---------4---------5---------6---------7---------+
These are interpreter routines that cannot be optimised since long jumps would 
clobber some variables in them. Declaring them volatile was not a definite 
answer hence it is done like this. Overal interpreter performance is not 
significantly affected.

THIS FILE MUST BE COMPILED WITH ALL OPTIMISATIONS OFF.                        */

#define SERIAL_CLAUSE(p)\
  if (MASK (p) & OPTIMAL_MASK) {\
    EXECUTE_UNIT_TRACE (SEQUENCE (p));\
  } else {\
    jmp_buf exit_buf;\
    if (!setjmp (exit_buf)) {\
      genie_serial_clause (p, &exit_buf);\
    }\
  }

#define ENQUIRY_CLAUSE(p)\
  if (MASK (p) & OPTIMAL_MASK) {\
    EXECUTE_UNIT_TRACE (SEQUENCE (p));\
  } else {\
      genie_enquiry_clause (p);\
  }

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_closed_clause: execute closed clause.                                   */

PROPAGATOR_T
genie_closed (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_closed;
  self.source = p;
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  open_frame (p, IS_NOT_PROCEDURE_PARM, frame_pointer);
	  SERIAL_CLAUSE (p);
	  CLOSE_FRAME;
	}
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_int_case: execute integral-case-clause.                                 */

void
genie_int_case (NODE_T * p, MOID_T * yield)
{
  int unit_count;
  BOOL_T done;
/* CASE */
  A68_INT k;
  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
  ENQUIRY_CLAUSE (NEXT_SUB (p));
  genie_check_initialisation (p, STACK_OFFSET (-SIZE_OF (A68_INT)), MODE (INT), NULL);
  POP_INT (p, &k);
/* IN */
  p = NEXT (p);
  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
  unit_count = 1;
  done = genie_int_case_unit (NEXT_SUB (p), k.value, &unit_count);
  CLOSE_FRAME;
/* OUT */
  p = NEXT (p);
  if (!done)
    {
      if (WHETHER (p, CHOICE) || WHETHER (p, OUT_PART))
	{
	  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
	  SERIAL_CLAUSE (NEXT_SUB (p));
	  CLOSE_FRAME;
	}
      else if (WHETHER (p, CLOSE_SYMBOL) || WHETHER (p, ESAC_SYMBOL))
	{
	  genie_push_undefined (p, yield);
	}
      else
	{
	  genie_int_case (SUB (p), yield);
	}
    }
/* ESAC */
  CLOSE_FRAME;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_united_case: execute united-case-clause.                                */

void
genie_united_case (NODE_T * p, MOID_T * yield)
{
  BOOL_T done = A_FALSE;
/* CASE */
  MOID_T *um;
  ADDR_T save_sp;
  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
  save_sp = stack_pointer;
  ENQUIRY_CLAUSE (NEXT_SUB (p));
  stack_pointer = save_sp;
  um = ((A68_UNION *) STACK_TOP)->value;
/* IN */
  p = NEXT (p);
  if (um != NULL)
    {
      open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
      done = genie_united_case_unit (NEXT_SUB (p), um);
      CLOSE_FRAME;
    }
  else
    {
      done = A_FALSE;
    }
/* OUT */
  p = NEXT (p);
  if (!done)
    {
      if (WHETHER (p, CHOICE) || WHETHER (p, OUT_PART))
	{
	  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
	  SERIAL_CLAUSE (NEXT_SUB (p));
	  CLOSE_FRAME;
	}
      else if (WHETHER (p, CLOSE_SYMBOL) || WHETHER (p, ESAC_SYMBOL))
	{
	  genie_push_undefined (p, yield);
	}
      else
	{
	  genie_united_case (SUB (p), yield);
	}
    }
/* ESAC */
  CLOSE_FRAME;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_conditional: execute conditional-clause.                                */

void
genie_conditional (NODE_T * p, MOID_T * yield)
{
/* IF */
  A68_BOOL z;
  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
  ENQUIRY_CLAUSE (NEXT_SUB (p));
  genie_check_initialisation (p, STACK_OFFSET (-SIZE_OF (A68_BOOL)), MODE (BOOL), NULL);
  POP_BOOL (p, &z);
  p = NEXT (p);
/* THEN */
  if (z.value == A_TRUE)
    {
      open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
      SERIAL_CLAUSE (NEXT_SUB (p));
      CLOSE_FRAME;
/* ELSE */
    }
  else
    {
      p = NEXT (p);
      if (WHETHER (p, CHOICE) || WHETHER (p, ELSE_PART))
	{
	  open_frame (SUB (p), IS_NOT_PROCEDURE_PARM, frame_pointer);
	  SERIAL_CLAUSE (NEXT_SUB (p));
	  CLOSE_FRAME;
	}
      else if (WHETHER (p, CLOSE_SYMBOL) || WHETHER (p, FI_SYMBOL))
	{
	  genie_push_undefined (p, yield);
	}
      else
	{
	  genie_conditional (SUB (p), yield);
	}
    }
/* FI */
  CLOSE_FRAME;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
test_loop_addition: whether addition does not yield INT overflow.             */

void
test_loop_addition (NODE_T * p, int i, int j)
{
  if ((i >= 0 && j >= 0) || (i < 0 && j < 0))
    {
      int ai = (i >= 0 ? i : -i), aj = (j >= 0 ? j : -j);
      if (ai > INT_MAX - aj)
	{
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_loop: execute loop-clause.                                              */

PROPAGATOR_T
genie_loop (NODE_T * p)
{
  PROPAGATOR_T self;
  ADDR_T save_stack_pointer = stack_pointer;
  int from, by, to;
  BOOL_T go_on;
  NODE_T *for_part, *to_part, *psave, *q;
  A68_INT counter;
  self.unit = genie_loop;
  self.source = p;
/* FOR */
  if (WHETHER (p, FOR_PART))
    {
      for_part = NEXT_SUB (p);
      p = NEXT (p);
    }
  else
    {
      for_part = NULL;
    }
/* FROM */
  if (WHETHER (p, FROM_PART))
    {
      A68_INT k;
      EXECUTE_UNIT (NEXT_SUB (p));
      DECREMENT_STACK_POINTER (p, SIZE_OF (A68_INT));
      k = *(A68_INT *) STACK_TOP;
      from = k.value;
      p = NEXT (p);
    }
  else
    {
      from = 1;
    }
/* BY */
  if (WHETHER (p, BY_PART))
    {
      A68_INT k;
      EXECUTE_UNIT (NEXT_SUB (p));
      DECREMENT_STACK_POINTER (p, SIZE_OF (A68_INT));
      k = *(A68_INT *) STACK_TOP;
      by = k.value;
      p = NEXT (p);
    }
  else
    {
      by = 1;
    }
/* TO */
  if (WHETHER (p, TO_PART))
    {
      A68_INT k;
      EXECUTE_UNIT (NEXT_SUB (p));
      DECREMENT_STACK_POINTER (p, SIZE_OF (A68_INT));
      k = *(A68_INT *) STACK_TOP;
      to = k.value;
      to_part = p;
      p = NEXT (p);
    }
  else
    {
      to = (by >= 0 ? MAX_INT : -MAX_INT);
      to_part = NULL;
    }
  q = NEXT_SUB (p);
/* Here the DO .. OD part starts. */
  open_frame (q, IS_NOT_PROCEDURE_PARM, frame_pointer);
  counter.status = INITIALISED_MASK;
  counter.value = from;
  go_on = A_TRUE;
  psave = p;
  while (go_on)
    {
/* Resetting stack pointer is an extra safety measure. */
      stack_pointer = save_stack_pointer;
      p = psave;
      if ((by > 0 && counter.value <= to) || (by < 0 && counter.value >= to) || by == 0)
	{
	  if (for_part != NULL)
	    {
	      *(A68_INT *) (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (for_part)->offset)) = counter;
	    }
/* WHILE */
	  if (WHETHER (p, WHILE_PART))
	    {
	      A68_BOOL z;
	      ENQUIRY_CLAUSE (NEXT_SUB (p));
	      POP_BOOL (p, &z);
/* DO (after WHILE) */
	      if (z.value == A_TRUE)
		{
		  NODE_T *while_part = NEXT (SUB_NEXT (p));
		  open_frame (while_part, IS_NOT_PROCEDURE_PARM, frame_pointer);
		  SERIAL_CLAUSE (while_part);
		  CLOSE_FRAME;
		  if (!(for_part == NULL && to_part == NULL))
		    {
		      test_loop_addition (p, counter.value, by);
		      counter.value += by;
		    }
		}
	      else
		{
		  go_on = A_FALSE;
		}
/* DO (no WHILE) */
	    }
	  else
	    {
	      SERIAL_CLAUSE (NEXT_SUB (p));
	      if (!(for_part == NULL && to_part == NULL))
		{
		  test_loop_addition (p, counter.value, by);
		  counter.value += by;
		}
	    }
/* The genie cannot take things to next iteration, so re-initialise stack frame. */
	  if (SYMBOL_TABLE (q)->initialise_frame)
	    {
	      initialise_frame (q);
	    }
	}
      else
	{
	  go_on = A_FALSE;
	}
    }
/* OD */
  CLOSE_FRAME;
/* Resetting stack pointer is an extra measure. */
  stack_pointer = save_stack_pointer;
  return (self);
}
