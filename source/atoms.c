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
#include "mp.h"

/*-------1---------2---------3---------4---------5---------6---------7---------+
This file contains interpreter ("genie") routines related to executing primitive 
A68 actions.

The genie is self-optimising as when it traverses the tree, it stores terminals 
it ends up in at the root where traversing for that terminal started. 
Such piece of information is called a PROPAGATOR.                             */

void genie_unit_trace (NODE_T *);
void genie_serial_units_no_label_linear (NODE_T *, int);
void genie_serial_units_no_label (NODE_T *, int, NODE_T **);
A68_REF genie_allocate_declarer (NODE_T *, ADDR_T *, A68_REF, BOOL_T);

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_check_initialisation: whether item at "w" of mode "q" is initialised.   */

BOOL_T
genie_check_initialisation (NODE_T * p, BYTE_T * w, MOID_T * q, BOOL_T * result)
{
  BOOL_T initialised = A_TRUE, recognised = A_FALSE;
  switch (q->short_id)
    {
    case MODE_NO_CHECK:
    case UNION_SYMBOL:
      {
	initialised = A_TRUE;
	recognised = A_TRUE;
	break;
      }
    case REF_SYMBOL:
      {
	A68_REF *z = (A68_REF *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case PROC_SYMBOL:
      {
	A68_PROCEDURE *z = (A68_PROCEDURE *) w;
	initialised = (z->body.status & INITIALISED_MASK) && (z->environ.status & INITIALISED_MASK);
	recognised = A_TRUE;
	break;
      }
    case MODE_INT:
      {
	A68_INT *z = (A68_INT *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_REAL:
      {
	A68_REAL *z = (A68_REAL *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_COMPLEX:
      {
	A68_REAL *r = (A68_REAL *) w;
	A68_REAL *i = (A68_REAL *) (w + SIZE_OF (A68_REAL));
	initialised = (r->status & INITIALISED_MASK) && (i->status & INITIALISED_MASK);
	recognised = A_TRUE;
	break;
      }
    case MODE_LONG_INT:
    case MODE_LONGLONG_INT:
    case MODE_LONG_REAL:
    case MODE_LONGLONG_REAL:
    case MODE_LONG_BITS:
    case MODE_LONGLONG_BITS:
      {
	mp_digit *z = (mp_digit *) w;
	initialised = (int) z[0] & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_LONG_COMPLEX:
      {
	mp_digit *r = (mp_digit *) w;
	mp_digit *i = (mp_digit *) (w + size_long_mp ());
	initialised = ((int) r[0] & INITIALISED_MASK) && ((int) i[0] & INITIALISED_MASK);
	recognised = A_TRUE;
	break;
      }
    case MODE_LONGLONG_COMPLEX:
      {
	mp_digit *r = (mp_digit *) w;
	mp_digit *i = (mp_digit *) (w + size_longlong_mp ());
	initialised = ((int) r[0] & INITIALISED_MASK) && ((int) i[0] & INITIALISED_MASK);
	recognised = A_TRUE;
	break;
      }
    case MODE_BOOL:
      {
	A68_BOOL *z = (A68_BOOL *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_CHAR:
      {
	A68_CHAR *z = (A68_CHAR *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_BITS:
      {
	A68_BITS *z = (A68_BITS *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_BYTES:
      {
	A68_BYTES *z = (A68_BYTES *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_LONG_BYTES:
      {
	A68_LONG_BYTES *z = (A68_LONG_BYTES *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_FILE:
      {
	A68_FILE *z = (A68_FILE *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_FORMAT:
      {
	A68_FORMAT *z = (A68_FORMAT *) w;
	initialised = z->status & INITIALISED_MASK;
	recognised = A_TRUE;
	break;
      }
    case MODE_PIPE:
      {
	A68_REF *read = (A68_REF *) w;
	A68_REF *write = (A68_REF *) (w + SIZE_OF (A68_REF));
	A68_INT *pid = (A68_INT *) (w + 2 * SIZE_OF (A68_REF));
	initialised = (read->status & INITIALISED_MASK) && (write->status & INITIALISED_MASK) && (pid->status & INITIALISED_MASK);
	recognised = A_TRUE;
	break;
      }
    }
  if (result == NULL)
    {
      if (recognised == A_TRUE && initialised == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, EMPTY_VALUE_ERROR_FROM, q);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
  else
    {
      *result = initialised;
    }
  return (recognised);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_constant: push constant stored in the tree.                             */

PROPAGATOR_T
genie_constant (NODE_T * p)
{
  PUSH (p, p->genie.constant, MOID_SIZE (MOID (p)));
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_uniting: unite value in the stack and push result.                      */

PROPAGATOR_T
genie_uniting (NODE_T * p)
{
  PROPAGATOR_T self;
  ADDR_T sp = stack_pointer;
  MOID_T *u = MOID (p);
  int size = MOID_SIZE (u);
  self.unit = genie_uniting;
  self.source = p;
  if (ATTRIBUTE (MOID (SUB (p))) != UNION_SYMBOL)
    {
      PUSH_POINTER (p, (void *) unites_to (MOID (SUB (p)), u));
      EXECUTE_UNIT (SUB (p));
    }
  else
    {
      A68_UNION *m = (A68_UNION *) STACK_TOP;
      EXECUTE_UNIT (SUB (p));
      m->value = (void *) unites_to ((MOID_T *) m->value, u);
    }
  stack_pointer = sp + size;
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_constant_widening: keep widened constant as a constant.                  */

static void
make_constant_widening (NODE_T * p, MOID_T * m, PROPAGATOR_T * self)
{
  if (SUB (p) != NULL && SUB (p)->genie.constant != NULL)
    {
      int size = MOID_SIZE (m);
      self->unit = genie_constant;
      p->genie.constant = (void *) get_heap_space ((unsigned) size);
      MOVE (p->genie.constant, (void *) (STACK_OFFSET (-size)), (unsigned) size);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_widening_int_to_real: (optimised) push INT widened to REAL.             */

PROPAGATOR_T
genie_widening_int_to_real (NODE_T * p)
{
  A68_INT i;
  EXECUTE_UNIT (SUB (p));
  POP_INT (p, &i);
  PUSH_REAL (p, (double) i.value);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_widening: widen value in the stack.                                     */

PROPAGATOR_T
genie_widening (NODE_T * p)
{
#define COERCE_FROM_TO(p, a, b) (MOID (p) == (b) && MOID (SUB (p)) == (a))
  PROPAGATOR_T self;
  self.unit = genie_widening;
  self.source = p;
/* INT widenings. */
  if (COERCE_FROM_TO (p, MODE (INT), MODE (REAL)))
    {
      genie_widening_int_to_real (p);
      self.unit = genie_widening_int_to_real;
      make_constant_widening (p, MODE (REAL), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (INT), MODE (LONG_INT)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_int_to_long_mp (p);
      make_constant_widening (p, MODE (LONG_INT), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_INT), MODE (LONGLONG_INT)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_long_mp_to_longlong_mp (p);
      make_constant_widening (p, MODE (LONGLONG_INT), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_INT), MODE (LONG_REAL)))
    {
      EXECUTE_UNIT (SUB (p));
      /* 1-1 mapping. */ ;
      make_constant_widening (p, MODE (LONG_REAL), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONGLONG_INT), MODE (LONGLONG_REAL)))
    {
      EXECUTE_UNIT (SUB (p));
      /* 1-1 mapping. */ ;
      make_constant_widening (p, MODE (LONGLONG_REAL), &self);
    }
/* REAL widenings. */
  else if (COERCE_FROM_TO (p, MODE (REAL), MODE (LONG_REAL)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_real_to_long_mp (p);
      make_constant_widening (p, MODE (LONG_REAL), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_REAL), MODE (LONGLONG_REAL)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_long_mp_to_longlong_mp (p);
      make_constant_widening (p, MODE (LONGLONG_REAL), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (REAL), MODE (COMPLEX)))
    {
      EXECUTE_UNIT (SUB (p));
      PUSH_REAL (p, 0.0);
      make_constant_widening (p, MODE (COMPLEX), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_REAL), MODE (LONG_COMPLEX)))
    {
      mp_digit *z;
      int digits = get_mp_digits (MODE (LONG_REAL));
      EXECUTE_UNIT (SUB (p));
      z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
      make_constant_widening (p, MODE (LONG_COMPLEX), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONGLONG_REAL), MODE (LONGLONG_COMPLEX)))
    {
      mp_digit *z;
      int digits = get_mp_digits (MODE (LONGLONG_REAL));
      EXECUTE_UNIT (SUB (p));
      z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
      make_constant_widening (p, MODE (LONGLONG_COMPLEX), &self);
    }
/* COMPLEX widenings. */
  else if (COERCE_FROM_TO (p, MODE (COMPLEX), MODE (LONG_COMPLEX)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_complex_to_long_complex (p);
      make_constant_widening (p, MODE (LONG_COMPLEX), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_COMPLEX), MODE (LONGLONG_COMPLEX)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_long_complex_to_longlong_complex (p);
      make_constant_widening (p, MODE (LONGLONG_COMPLEX), &self);
    }
/* BITS widenings. */
  else if (COERCE_FROM_TO (p, MODE (BITS), MODE (LONG_BITS)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_int_to_long_mp (p);	/* Treat unsigned as int, but that's ok. */
      make_constant_widening (p, MODE (LONG_BITS), &self);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_BITS), MODE (LONGLONG_BITS)))
    {
      EXECUTE_UNIT (SUB (p));
      genie_lengthen_long_mp_to_longlong_mp (p);
      make_constant_widening (p, MODE (LONGLONG_BITS), &self);
    }
/* Miscellaneous widenings. */
  else if (COERCE_FROM_TO (p, MODE (BYTES), MODE (ROW_CHAR)))
    {
      A68_BYTES z;
      EXECUTE_UNIT (SUB (p));
      POP (p, &z, SIZE_OF (A68_BYTES));
      PUSH_REF (p, c_string_to_row_char (p, z.value, BYTES_WIDTH));
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_BYTES), MODE (ROW_CHAR)))
    {
      A68_LONG_BYTES z;
      EXECUTE_UNIT (SUB (p));
      POP (p, &z, SIZE_OF (A68_LONG_BYTES));
      PUSH_REF (p, c_string_to_row_char (p, z.value, LONG_BYTES_WIDTH));
    }
  else if (COERCE_FROM_TO (p, MODE (BITS), MODE (ROW_BOOL)))
    {
      A68_BITS x;
      A68_REF z, row;
      A68_ARRAY arr;
      A68_TUPLE tup;
      int k;
      unsigned bit;
      A68_BOOL *base;
      EXECUTE_UNIT (SUB (p));
      POP (p, &x, SIZE_OF (A68_BITS));
      z = heap_generator (p, MODE (ROW_BOOL), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
      protect_sweep_handle (&z);
      row = heap_generator (p, MODE (ROW_BOOL), BITS_WIDTH * SIZE_OF (A68_BOOL));
      protect_sweep_handle (&row);
      arr.dimensions = 1;
      arr.type = MODE (BOOL);
      arr.elem_size = SIZE_OF (A68_BOOL);
      arr.slice_offset = 0;
      arr.field_offset = 0;
      arr.array = row;
      tup.lower_bound = 1;
      tup.upper_bound = BITS_WIDTH;
      tup.shift = tup.lower_bound;
      tup.span = 1;
      PUT_DESCRIPTOR (arr, tup, &z);
      base = (A68_BOOL *) ADDRESS (&row);
      bit = 1;
      for (k = BITS_WIDTH - 1; k >= 0; k--)
	{
	  A68_BOOL b;
	  b.status = INITIALISED_MASK;
	  b.value = (x.value & bit) != 0 ? A_TRUE : A_FALSE;
	  base[k] = b;
	  bit <<= 1;
	}
      if (SUB (p)->genie.constant != NULL)
	{
	  self.unit = genie_constant;
	  protect_sweep_handle (&z);
	  p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_REF));
	  MOVE (p->genie.constant, &z, SIZE_OF (A68_REF));
	}
      else
	{
	  unprotect_sweep_handle (&z);
	}
      PUSH_REF (p, z);
      unprotect_sweep_handle (&row);
    }
  else if (COERCE_FROM_TO (p, MODE (LONG_BITS), MODE (ROW_BOOL)) || COERCE_FROM_TO (p, MODE (LONGLONG_BITS), MODE (ROW_BOOL)))
    {
      MOID_T *m = MOID (SUB (p));
      A68_REF z, row;
      A68_ARRAY arr;
      A68_TUPLE tup;
      int size = get_mp_size (m), k, width = get_mp_bits_width (m), words = get_mp_bits_words (m);
      unsigned *bits;
      A68_BOOL *base;
      mp_digit *x;
      int save_sp = stack_pointer;
/* Calculate and convert BITS value. */
      EXECUTE_UNIT (SUB (p));
      x = (mp_digit *) STACK_OFFSET (-size);
      bits = stack_mp_bits (p, x, m);
/* Make [] BOOL. */
      z = heap_generator (p, MODE (ROW_BOOL), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
      protect_sweep_handle (&z);
      row = heap_generator (p, MODE (ROW_BOOL), width * SIZE_OF (A68_BOOL));
      protect_sweep_handle (&row);
      arr.dimensions = 1;
      arr.type = MODE (BOOL);
      arr.elem_size = SIZE_OF (A68_BOOL);
      arr.slice_offset = 0;
      arr.field_offset = 0;
      arr.array = row;
      tup.lower_bound = 1;
      tup.upper_bound = width;
      tup.shift = tup.lower_bound;
      tup.span = 1;
      PUT_DESCRIPTOR (arr, tup, &z);
      base = (A68_BOOL *) ADDRESS (&row);
      k = width;
      while (k > 0)
	{
	  unsigned bit = 0x1;
	  int j;
	  for (j = 0; j < MP_BITS_BITS && k >= 0; j++)
	    {
	      A68_BOOL b;
	      b.status = INITIALISED_MASK;
	      b.value = (bits[words - 1] & bit) ? A_TRUE : A_FALSE;
	      base[--k] = b;
	      bit <<= 1;
	    }
	  words--;
	}
      if (SUB (p)->genie.constant != NULL)
	{
	  self.unit = genie_constant;
	  protect_sweep_handle (&z);
	  p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_REF));
	  MOVE (p->genie.constant, &z, SIZE_OF (A68_REF));
	}
      else
	{
	  unprotect_sweep_handle (&z);
	}
      stack_pointer = save_sp;
      PUSH_REF (p, z);
      unprotect_sweep_handle (&row);
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, "cannot widen M to M", MOID (SUB (p)), MOID (p));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  return (self);
#undef COERCE_FROM_TO
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_proceduring: cast a jump to a PROC VOID without executing the jump.     */

static void
genie_proceduring (NODE_T * p)
{
  A68_PROCEDURE z;
  NODE_T *jump = SUB (p);
  NODE_T *q = SUB (jump);
  NODE_T *label = WHETHER (q, GOTO_SYMBOL) ? NEXT (q) : q;
  z.body.status = INITIALISED_MASK;
  z.body.value = (void *) jump;
  z.environ.status = INITIALISED_MASK;
  z.environ.offset = static_link_for_frame (1 + LEX_LEVEL (TAX (label)));
  PUSH (p, &z, SIZE_OF (A68_PROCEDURE));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_dereferencing_quick: (optimised) dereference value of a unit            */

PROPAGATOR_T
genie_dereferencing_quick (NODE_T * p)
{
  A68_REF z;
  unsigned size = MOID_SIZE (MOID (p));
  EXECUTE_UNIT (SUB (p));
  POP (p, &z, SIZE_OF (A68_REF));
  TEST_NIL (p, z, MOID (SUB (p)));
  PUSH (p, ADDRESS (&z), size);
  if (!(z.status & ASSIGNED_MASK))
    {
      genie_check_initialisation (p, STACK_OFFSET (-size), MOID (p), NULL);
    }
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_dereference_loc_identifier: dereference a LOC local name.               */

PROPAGATOR_T
genie_dereference_loc_identifier (NODE_T * p)
{
  A68_REF *z = (A68_REF *) (FRAME_SHORTCUT (p));
  MOID_T *deref = SUB (MOID (p));
  unsigned size = MOID_SIZE (deref);
  PUSH (p, &frame_segment[z->offset], size);
  if (!(z->status & ASSIGNED_MASK))
    {
      genie_check_initialisation (p, STACK_OFFSET (-size), deref, NULL);
    }
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_dereference_identifier: dereference a local name.                  */

PROPAGATOR_T
genie_dereference_identifier (NODE_T * p)
{
  A68_REF *z = (A68_REF *) (FRAME_SHORTCUT (p));
  MOID_T *deref = SUB (MOID (p));
  unsigned size = MOID_SIZE (deref);
  TEST_NIL (p, *z, MOID (p));
  PUSH (p, ADDRESS (z), size);
  if (!(z->status & ASSIGNED_MASK))
    {
      genie_check_initialisation (p, STACK_OFFSET (-size), deref, NULL);
    }
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_dereferencing: dereference name in the stack.                           */

PROPAGATOR_T
genie_dereferencing (NODE_T * p)
{
  PROPAGATOR_T self;
  A68_REF z;
  self = EXECUTE_UNIT (SUB (p));
  if (self.unit == genie_loc_identifier)
    {
      if (TAX (self.source)->loc_assigned)
	{
	  self.unit = genie_dereference_loc_identifier;
	}
      else
	{
	  self.unit = genie_dereference_identifier;
	}
      self.source->genie.propagator.unit = self.unit;
    }
  else
    {
      self.unit = genie_dereferencing_quick;
      self.source = p;
    }
  POP (p, &z, SIZE_OF (A68_REF));
  TEST_NIL (p, z, MOID (SUB (p)));
  PUSH (p, ADDRESS (&z), MOID_SIZE (MOID (p)));
  if (!(z.status & ASSIGNED_MASK))
    {
      genie_check_initialisation (p, STACK_OFFSET (-MOID_SIZE (MOID (p))), MOID (p), NULL);
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_deproceduring: deprocedure PROC in the stack.                           */

PROPAGATOR_T
genie_deproceduring (NODE_T * p)
{
  PROPAGATOR_T self;
  A68_PROCEDURE z;
  self.unit = genie_deproceduring;
  self.source = p;
  EXECUTE_UNIT (SUB (p));
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  genie_check_initialisation (p, (BYTE_T *) & z, MOID (SUB (p)), NULL);
  if (z.body.status & STANDENV_PROCEDURE_MASK)
    {
      GENIE_PROCEDURE *pr = (GENIE_PROCEDURE *) (z.body.value);
      if (pr != NULL)
	{
	  (void) pr (p);
	}
    }
  else
    {
      NODE_T *body = ((NODE_T *) z.body.value);
      if (WHETHER (body, ROUTINE_TEXT))
	{
	  NODE_T *entry = SUB (body);
	  open_frame (entry, IS_PROCEDURE_PARM, z.environ.offset);
	  if (WHETHER (entry, PARAMETER_PACK))
	    {
	      entry = NEXT (entry);
	    }
	  EXECUTE_UNIT (NEXT (NEXT (entry)));
	  CLOSE_FRAME;
	}
      else
	{
	  open_frame (body, IS_PROCEDURE_PARM, z.environ.offset);
	  EXECUTE_UNIT (body);
	  CLOSE_FRAME;
	}
    }
  genie_scope_check (p, MOID (p));
  PROTECT_FROM_SWEEP (p);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_voiding: voiden value in the stack.                                     */

PROPAGATOR_T
genie_voiding (NODE_T * p)
{
  PROPAGATOR_T self, source;
  ADDR_T sp_for_voiding = stack_pointer;
  self.source = p;
  source = EXECUTE_UNIT (SUB (p));
  stack_pointer = sp_for_voiding;
  if (source.unit == genie_loc_assignation)
    {
      self.unit = genie_voiding_loc_assignation;
      self.source = source.source;
    }
  else if (source.unit == genie_loc_constant_assignation)
    {
      self.unit = genie_voiding_loc_constant_assignation;
      self.source = source.source;
    }
  else
    {
      self.unit = genie_voiding;
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_coercion:	coerce value in the stack.                                    */

PROPAGATOR_T
genie_coercion (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_coercion;
  self.source = p;
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case VOIDING:
	  {
	    self = genie_voiding (p);
	    break;
	  }
	case UNITING:
	  {
	    self = genie_uniting (p);
	    break;
	  }
	case WIDENING:
	  {
	    self = genie_widening (p);
	    break;
	  }
	case ROWING:
	  {
	    self = genie_rowing (p);
	    break;
	  }
	case DEREFERENCING:
	  {
	    self = genie_dereferencing (p);
	    break;
	  }
	case DEPROCEDURING:
	  {
	    self = genie_deproceduring (p);
	    break;
	  }
	case PROCEDURING:
	  {
	    genie_proceduring (p);
	    break;
	  }
	}
    }
  p->genie.propagator = self;
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_argument: push argument units.                                          */

static void
genie_argument (NODE_T * p, NODE_T ** seq)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, UNIT))
	{
	  EXECUTE_UNIT (p);
	  SEQUENCE (*seq) = p;
	  (*seq) = p;
	  return;
	}
      else
	{
	  genie_argument (SUB (p), seq);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_call_standenv_quick: (optimised) call of standard environ routine.      */

PROPAGATOR_T
genie_call_standenv_quick (NODE_T * p)
{
  NODE_T *prim = SUB (p), *q = SEQUENCE (p);
  TAG_T *x = TAX (prim->genie.propagator.source);
  for (; q != NULL; q = SEQUENCE (q))
    {
      EXECUTE_UNIT (q);
    }
  (void) (x->procedure) (p);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_call_quick: (optimised) call and push result.                           */

PROPAGATOR_T
genie_call_quick (NODE_T * p)
{
  NODE_T *id = SUB (SUB (p));
  A68_PROCEDURE *z = (A68_PROCEDURE *) FRAME_SHORTCUT (id);
  NODE_T *body = ((NODE_T *) z->body.value), *q = SEQUENCE (p);
  NODE_T *entry = SUB (body);
  PACK_T *args = PACK (MOID (id));
  ADDR_T fp0, sp0 = stack_pointer;
/* Calculate arguments. */
  for (; q != NULL; q = SEQUENCE (q))
    {
      EXECUTE_UNIT (q);
    }
/* Copy arguments from stack to frame. */
  open_frame (entry, IS_PROCEDURE_PARM, z->environ.offset);
  stack_pointer = sp0;
  fp0 = 0;
  for (; args != NULL; args = NEXT (args))
    {
      int size = MOID_SIZE (MOID (args));
      MOVE ((FRAME_OFFSET (FRAME_INFO_SIZE + fp0)), STACK_ADDRESS (sp0), (unsigned) size);
      sp0 += size;
      fp0 += size;
    }
/* Interpret routine text. */
  EXECUTE_UNIT (NEXT (NEXT (NEXT (entry))));
  CLOSE_FRAME;
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_call: call PROC with arguments and push result.                         */

PROPAGATOR_T
genie_call (NODE_T * p)
{
  PROPAGATOR_T self, primary;
  A68_PROCEDURE z;
  NODE_T *prim = SUB (p);
  ADDR_T sp0 = stack_pointer;
  self.unit = genie_call;
  self.source = p;
  if (SEQUENCE (p) == NULL && SEQUENCE_SET (p) == A_FALSE)
    {
      NODE_T top_seq;
      NODE_T *seq = &top_seq;
      genie_argument (NEXT (prim), &seq);
      SEQUENCE (p) = SEQUENCE (&top_seq);
      SEQUENCE_SET (p) = A_TRUE;
    }
  else
    {
      NODE_T *q = SEQUENCE (p);
      for (; q != NULL; q = SEQUENCE (q))
	{
	  EXECUTE_UNIT (q);
	}
    }
  primary = EXECUTE_UNIT (prim);
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  if (z.body.status & STANDENV_PROCEDURE_MASK)
    {
      GENIE_PROCEDURE *proc = (GENIE_PROCEDURE *) (z.body.value);
      if (proc != NULL)
	{
	  (void) proc (p);
	}
      if (primary.unit == genie_identifier_standenv_proc && p->protect_sweep == NULL)
	{
	  self.unit = genie_call_standenv_quick;
	}
    }
  else
    {
      NODE_T *body = ((NODE_T *) z.body.value);
      if (WHETHER (body, ROUTINE_TEXT))
	{
	  NODE_T *entry = SUB (body);
	  PACK_T *args = PACK (MOID (prim));
	  ADDR_T fp0;
/* Copy arguments from stack to frame. */
	  open_frame (entry, IS_PROCEDURE_PARM, z.environ.offset);
	  stack_pointer = sp0;
	  fp0 = 0;
	  for (; args != NULL; args = NEXT (args))
	    {
	      int size = MOID_SIZE (MOID (args));
	      MOVE ((FRAME_OFFSET (FRAME_INFO_SIZE + fp0)), STACK_ADDRESS (sp0), (unsigned) (unsigned) size);
	      sp0 += size;
	      fp0 += size;
	    }
/* Interpret routine text. */
	  EXECUTE_UNIT (NEXT (NEXT (NEXT (entry))));
	  CLOSE_FRAME;
	  if (prim->genie.propagator.unit == genie_loc_identifier && TAX (prim->genie.propagator.source)->loc_procedure && p->protect_sweep == NULL)
	    {
	      self.unit = genie_call_quick;
	    }
	}
      else
	{
	  EXECUTE_UNIT (body);
	}
    }
  genie_scope_check (p, MOID (p));
  PROTECT_FROM_SWEEP (p);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_trimmer: construct a descriptor "ref_new" for a trim of "ref_old".      */

static void
genie_trimmer (NODE_T * p, BYTE_T * *ref_new, BYTE_T * *ref_old, int *offset)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  A68_INT k;
	  A68_TUPLE *t;
	  EXECUTE_UNIT (p);
	  POP_INT (p, &k);
	  t = (A68_TUPLE *) * ref_old;
	  if (k.value < t->lower_bound || k.value > t->upper_bound)
	    {
	      diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  (*offset) += t->span * (k.value - t->shift);
	  (*ref_old) += SIZE_OF (A68_TUPLE);
	}
      else if (WHETHER (p, TRIMMER))
	{
	  A68_INT k;
	  A68_TUPLE *old_tup, *new_tup;
	  NODE_T *q;
	  int L, U, D;
	  old_tup = (A68_TUPLE *) * ref_old;
	  new_tup = (A68_TUPLE *) * ref_new;
/* TRIMMER is (l:u@r) with all units optional or (empty). */
	  q = SUB (p);
	  if (q == NULL)
	    {
	      L = old_tup->lower_bound;
	      U = old_tup->upper_bound;
	      D = 0;
	    }
	  else
	    {
	      BOOL_T absent = A_TRUE;
	      if (q != NULL && WHETHER (q, UNIT))
		{
		  EXECUTE_UNIT (q);
		  POP_INT (p, &k);
		  if (k.value < old_tup->lower_bound)
		    {
		      diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
		      exit_genie (p, A_RUNTIME_ERROR);
		    }
		  L = k.value;
		  q = NEXT (q);
		  absent = A_FALSE;
		}
	      else
		{
		  L = old_tup->lower_bound;
		}
	      if (q != NULL && (WHETHER (q, COLON_SYMBOL) || WHETHER (q, DOTDOT_SYMBOL)))
		{
		  q = NEXT (q);
		  absent = A_FALSE;
		}
	      if (q != NULL && WHETHER (q, UNIT))
		{
		  EXECUTE_UNIT (q);
		  POP_INT (p, &k);
		  if (k.value > old_tup->upper_bound)
		    {
		      diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
		      exit_genie (p, A_RUNTIME_ERROR);
		    }
		  U = k.value;
		  q = NEXT (q);
		  absent = 0;
		}
	      else
		{
		  U = old_tup->upper_bound;
		}
	      if (q != NULL && WHETHER (q, AT_SYMBOL))
		{
		  q = NEXT (q);
		}
	      if (q != NULL && WHETHER (q, UNIT))
		{
		  EXECUTE_UNIT (q);
		  POP_INT (p, &k);
		  D = L - k.value;
		  q = NEXT (q);
		}
	      else
		{
		  D = absent ? 0 : L - 1;
		}
	    }
	  new_tup->lower_bound = L - D;
	  new_tup->upper_bound = (L - D) + (U - L);
	  new_tup->span = old_tup->span;
	  new_tup->shift = old_tup->shift - D;
	  (*ref_old) += SIZE_OF (A68_TUPLE);
	  (*ref_new) += SIZE_OF (A68_TUPLE);
	}
      else
	{
	  genie_trimmer (SUB (p), ref_new, ref_old, offset);
	  genie_trimmer (NEXT (p), ref_new, ref_old, offset);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_subscript: calculation of subscript.                                    */

void
genie_subscript (NODE_T * p, ADDR_T * ref_heap, int *sum, NODE_T ** seq)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case UNIT:
	  {
	    A68_INT *k;
	    A68_TUPLE *t;
	    EXECUTE_UNIT (p);
	    POP_ADDRESS (p, k, A68_INT);
	    t = (A68_TUPLE *) HEAP_ADDRESS (*ref_heap);
	    if (k->value < t->lower_bound || k->value > t->upper_bound)
	      {
		diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
		exit_genie (p, A_RUNTIME_ERROR);
	      }
	    (*ref_heap) += SIZE_OF (A68_TUPLE);
	    (*sum) += t->span * (k->value - t->shift);
	    SEQUENCE (*seq) = p;
	    (*seq) = p;
	    return;
	  }
	case GENERIC_ARGUMENT:
	case GENERIC_ARGUMENT_LIST:
	  {
	    genie_subscript (SUB (p), ref_heap, sum, seq);
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_subscript_linear: (optimised) calculation of index for slice.           */

void
genie_subscript_linear (NODE_T * p, ADDR_T * ref_heap, int *sum)
{
  for (; p != NULL; p = SEQUENCE (p))
    {
      A68_INT *k;
      A68_TUPLE *t;
      EXECUTE_UNIT (p);
      POP_ADDRESS (p, k, A68_INT);
      t = (A68_TUPLE *) HEAP_ADDRESS (*ref_heap);
      if (k->value < t->lower_bound || k->value > t->upper_bound)
	{
	  diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      (*ref_heap) += SIZE_OF (A68_TUPLE);
      (*sum) += t->span * (k->value - t->shift);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_slice_name_quick: slice REF [] A to REF A.                              */

PROPAGATOR_T
genie_slice_name_quick (NODE_T * p)
{
  A68_ARRAY *x;
  ADDR_T ref_heap, index, address;
  A68_REF z, name;
/* Get row and save row from sweeper. */
  EXECUTE_UNIT (SUB (p));
  PROTECT_FROM_SWEEP (p);
/* Pop REF []. */
  POP (p, &z, SIZE_OF (A68_REF));
/* Dereference. */
  TEST_NIL (p, z, MOID (SUB (p)));
  z = *(A68_REF *) ADDRESS (&z);
  TEST_NIL (p, z, MOID (SUB (p)));
  x = (A68_ARRAY *) ADDRESS (&z);
/* Get indexer. */
  ref_heap = z.handle->offset + SIZE_OF (A68_ARRAY);
  index = 0;
  up_garbage_sema ();
  genie_subscript_linear (SEQUENCE (p), &ref_heap, &index);
  down_garbage_sema ();
/* Push reference to element. */
  address = ROW_ELEMENT (x, index);
  name = x->array;
  name.offset += address;
  PUSH (p, &name, SIZE_OF (A68_REF));
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_slice: push slice of a rowed object.                                    */

PROPAGATOR_T
genie_slice (NODE_T * p)
{
  PROPAGATOR_T self;
  BOOL_T slice_of_name = WHETHER (MOID (SUB (p)), REF_SYMBOL);
  MOID_T *result_moid = slice_of_name ? SUB (MOID (p)) : MOID (p);
  NODE_T *indexer = NEXT_SUB (p);
  self.unit = genie_slice;
  self.source = p;
/* Get row and save from sweeper. */
  EXECUTE_UNIT (SUB (p));
  PROTECT_FROM_SWEEP (p);
/* In case of slicing a REF [], we need the [] internally, so dereference. */
  if (slice_of_name)
    {
      A68_REF z;
      POP (p, &z, SIZE_OF (A68_REF));
      TEST_NIL (p, z, MOID (SUB (p)));
      PUSH (p, ADDRESS (&z), SIZE_OF (A68_REF));
    }
/* SLICING subscripts one element from an array. */
  if (ANNOTATION (indexer) == SLICE)
    {
      A68_REF z;
      A68_ARRAY *x;
      ADDR_T ref_heap, address;
      int index;
/* Get descriptor. */
      POP (p, &z, SIZE_OF (A68_REF));
      TEST_NIL (p, z, MOID (SUB (p)));
      x = (A68_ARRAY *) ADDRESS (&z);
/* Get indexer. */
      ref_heap = z.handle->offset + SIZE_OF (A68_ARRAY);
      index = 0;
      up_garbage_sema ();
      if (SEQUENCE (p) == NULL && SEQUENCE_SET (p) == A_FALSE)
	{
	  NODE_T top_seq;
	  NODE_T *seq = &top_seq;
	  genie_subscript (indexer, &ref_heap, &index, &seq);
	  SEQUENCE (p) = SEQUENCE (&top_seq);
	  SEQUENCE_SET (p) = A_TRUE;
	}
      else
	{
	  genie_subscript_linear (SEQUENCE (p), &ref_heap, &index);
	}
      down_garbage_sema ();
/* Slice of a name yields a name. */
      address = ROW_ELEMENT (x, index);
      if (slice_of_name)
	{
	  A68_REF name = x->array;
	  name.offset += address;
	  PUSH (p, &name, SIZE_OF (A68_REF));
	  if (SEQUENCE_SET (p) == A_TRUE)
	    {
	      self.unit = genie_slice_name_quick;
	      self.source = p;
	    }
	}
      else
	{
	  PUSH (p, ADDRESS (&(x->array)) + address, MOID_SIZE (result_moid));
	}
      return (self);
    }
/* Trimming selects a subarray from an array. */
  else if (ANNOTATION (indexer) == TRIMMER)
    {
      int new_size = SIZE_OF (A68_ARRAY) + DEFLEX (result_moid)->dimensions * SIZE_OF (A68_TUPLE);
      A68_REF ref_desc_copy = heap_generator (p, MOID (p), new_size);
      A68_ARRAY *old_des, *new_des;
      A68_REF z;
      BYTE_T *ref_new, *ref_old;
      int offset;
/* Get descriptor. */
      POP (p, &z, SIZE_OF (A68_REF));
/* Get indexer. */
      TEST_NIL (p, z, MOID (SUB (p)));
      old_des = (A68_ARRAY *) ADDRESS (&z);
      new_des = (A68_ARRAY *) ADDRESS (&ref_desc_copy);
      ref_old = ADDRESS (&z) + SIZE_OF (A68_ARRAY);
      ref_new = ADDRESS (&ref_desc_copy) + SIZE_OF (A68_ARRAY);
      new_des->dimensions = DEFLEX (result_moid)->dimensions;
      MOID (new_des) = MOID (old_des);
      new_des->elem_size = old_des->elem_size;
      offset = old_des->slice_offset;
      up_garbage_sema ();
      genie_trimmer (indexer, &ref_new, &ref_old, &offset);
      down_garbage_sema ();
      new_des->slice_offset = offset;
      new_des->field_offset = old_des->field_offset;
      new_des->array = old_des->array;
/* Trim of a name is a name. */
      if (slice_of_name)
	{
	  A68_REF ref_new = heap_generator (p, MOID (p), SIZE_OF (A68_REF));
	  *(A68_REF *) ADDRESS (&ref_new) = ref_desc_copy;
	  PUSH (p, &ref_new, SIZE_OF (A68_REF));
	}
      else
	{
	  PUSH (p, &ref_desc_copy, SIZE_OF (A68_REF));
	}
      return (self);
    }
  else
    {
      return (self);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_denoter: push value of denoter.                                         */

PROPAGATOR_T
genie_denoter (NODE_T * p)
{
  static PROPAGATOR_T self;
  MOID_T *moid = MOID (p);
  self.unit = genie_denoter;
  self.source = p;
  if (moid == MODE (INT))
    {
/* INT denoter. */
      A68_INT z;
      if (genie_string_to_value_internal (p, moid, SYMBOL (p), (BYTE_T *) & z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      self.unit = genie_constant;
      z.status = INITIALISED_MASK | CONSTANT_MASK;
      p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_INT));
      MOVE (p->genie.constant, &z, (unsigned) SIZE_OF (A68_INT));
      PUSH_INT (p, ((A68_INT *) (p->genie.constant))->value);
    }
  else if (moid == MODE (REAL))
    {
/* REAL denoter. */
      A68_REAL z;
      if (genie_string_to_value_internal (p, moid, SYMBOL (p), (BYTE_T *) & z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      z.status = INITIALISED_MASK | CONSTANT_MASK;
      self.unit = genie_constant;
      p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_REAL));
      MOVE (p->genie.constant, &z, (unsigned) SIZE_OF (A68_REAL));
      PUSH_REAL (p, ((A68_REAL *) (p->genie.constant))->value);
    }
  else if (moid == MODE (LONG_INT) || moid == MODE (LONGLONG_INT))
    {
/* [LONG] LONG INT denoter. */
      int digits = get_mp_digits (moid);
      mp_digit *z = stack_mp (p, digits);
      int size = get_mp_size (moid);
      if (genie_string_to_value_internal (p, moid, SYMBOL (NEXT_SUB (p)), (BYTE_T *) z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      z[0] = INITIALISED_MASK | CONSTANT_MASK;
      self.unit = genie_constant;
      p->genie.constant = (void *) get_heap_space (size);
      MOVE (p->genie.constant, z, (unsigned) size);
    }
  else if (moid == MODE (LONG_REAL) || moid == MODE (LONGLONG_REAL))
    {
/* [LONG] LONG REAL denoter. */
      int digits = get_mp_digits (moid);
      mp_digit *z = stack_mp (p, digits);
      int size = get_mp_size (moid);
      if (genie_string_to_value_internal (p, moid, SYMBOL (NEXT_SUB (p)), (BYTE_T *) z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      z[0] = INITIALISED_MASK | CONSTANT_MASK;
      self.unit = genie_constant;
      p->genie.constant = (void *) get_heap_space (size);
      MOVE (p->genie.constant, z, (unsigned) size);
    }
  else if (moid == MODE (BITS))
    {
/* BITS denoter. */
      A68_BITS z;
      if (genie_string_to_value_internal (p, moid, SYMBOL (p), (BYTE_T *) & z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      self.unit = genie_constant;
      z.status = INITIALISED_MASK | CONSTANT_MASK;
      p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_BITS));
      MOVE (p->genie.constant, &z, (unsigned) SIZE_OF (A68_BITS));
      PUSH_BITS (p, ((A68_BITS *) (p->genie.constant))->value);
    }
  else if (moid == MODE (LONG_BITS) || moid == MODE (LONGLONG_BITS))
    {
/* [LONG] LONG BITS denoter. */
      int digits = get_mp_digits (moid);
      mp_digit *z = stack_mp (p, digits);
      int size = get_mp_size (moid);
      if (genie_string_to_value_internal (p, moid, SYMBOL (NEXT_SUB (p)), (BYTE_T *) z) == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, moid);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      z[0] = INITIALISED_MASK | CONSTANT_MASK;
      self.unit = genie_constant;
      p->genie.constant = (void *) get_heap_space (size);
      MOVE (p->genie.constant, z, (unsigned) size);
    }
  else if (moid == MODE (BOOL))
    {
/* BOOL denoter. */
      A68_BOOL z;
      genie_string_to_value_internal (p, MODE (BOOL), SYMBOL (p), (BYTE_T *) & z);
      PUSH_BOOL (p, z.value);
    }
  else if (moid == MODE (CHAR))
    {
/* CHAR denoter. */
      PUSH_CHAR (p, SYMBOL (p)[0]);
    }
  else if (moid == MODE (ROW_CHAR))
    {
/* [] CHAR denoter. */
/* Make a permanent string in the heap. */
      A68_REF z;
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      z = c_to_a_string (p, SYMBOL (p));
      GET_DESCRIPTOR (arr, tup, &z);
      protect_sweep_handle (&z);
      protect_sweep_handle (&(arr->array));
      self.unit = genie_constant;
      p->genie.constant = (void *) get_heap_space (SIZE_OF (A68_REF));
      MOVE (p->genie.constant, &z, (unsigned) SIZE_OF (A68_REF));
      PUSH_REF (p, *(A68_REF *) (p->genie.constant));
    }
  else if (moid == MODE (VOID))
    {
/* VOID denoter. */
      /* EMPTY */ ;
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_loc_identifier: push a local identifier that is not a name.             */

PROPAGATOR_T
genie_loc_identifier (NODE_T * p)
{
  unsigned size = MOID_SIZE (MOID (p));
  PUSH (p, FRAME_SHORTCUT (p), size);
  genie_check_initialisation (p, STACK_OFFSET (-size), MOID (p), NULL);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_identifier_standenv_proc: push standard environ routine as PROC.        */

PROPAGATOR_T
genie_identifier_standenv_proc (NODE_T * p)
{
  static A68_PROCEDURE z;
  TAG_T *q = TAX (p);
  z.body.status = INITIALISED_MASK | STANDENV_PROCEDURE_MASK;
  z.body.value = (void *) q->procedure;
  z.environ.status = INITIALISED_MASK;
  z.environ.offset = 0;
  PUSH (p, &z, SIZE_OF (A68_PROCEDURE));
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_identifier_standenv: (optimised) push identifier from standard environ. */

PROPAGATOR_T
genie_identifier_standenv (NODE_T * p)
{
  TAG_T *q = TAX (p);
  (void) (q->procedure) (p);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_identifier: push identifier onto the stack.                             */

PROPAGATOR_T
genie_identifier (NODE_T * p)
{
  static PROPAGATOR_T self;
  TAG_T *q = TAX (p);
  self.source = p;
  if (q->stand_env_proc != 0)
    {
      if (WHETHER (MOID (q), PROC_SYMBOL))
	{
	  genie_identifier_standenv_proc (p);
	  self.unit = genie_identifier_standenv_proc;
	}
      else
	{
	  genie_identifier_standenv (p);
	  self.unit = genie_identifier_standenv;
	}
    }
  else
    {
      genie_loc_identifier (p);
      self.unit = genie_loc_identifier;
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_cast: push result of cast (coercions are deeper in the tree).           */

PROPAGATOR_T
genie_cast (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_cast;
  self.source = p;
  EXECUTE_UNIT (NEXT_SUB (p));
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_assertion: execute assertion.                                           */

PROPAGATOR_T
genie_assertion (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_assertion;
  self.source = p;
  if (MASK (p) & ASSERT_MASK)
    {
      A68_BOOL z;
      EXECUTE_UNIT (NEXT_SUB (p));
      POP_BOOL (p, &z);
      if (z.value == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "false assertion");
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_format_text: push format text.                                          */

PROPAGATOR_T
genie_format_text (NODE_T * p)
{
  static PROPAGATOR_T self;
  A68_FORMAT z = *(A68_FORMAT *) (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset));
  self.unit = genie_format_text;
  self.source = p;
  PUSH (p, &z, SIZE_OF (A68_FORMAT));
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static void
genie_generator_non_stowed (NODE_T * p, TAG_T * tag, int loc_or_heap, MOID_T * mode)
{
  if (loc_or_heap == HEAP_SYMBOL)
    {
/* HEAP non stowed. */
      ADDR_T sp_for_voiding = stack_pointer;
      A68_REF name = heap_generator (p, mode, MOID_SIZE (mode));
      stack_pointer = sp_for_voiding;
      PUSH_REF (p, name);
    }
  else if (loc_or_heap == LOC_SYMBOL)
    {
/* LOC non stowed. */
      ADDR_T sp_for_voiding = stack_pointer;
      A68_REF name;
      name.status = INITIALISED_MASK;
      name.segment = frame_segment;
      name.handle = &nil_handle;
      name.offset = frame_pointer + FRAME_INFO_SIZE + tag->offset;
      stack_pointer = sp_for_voiding;
      PUSH_REF (p, name);
    }
  else
    {
      abend (A_TRUE, INTERNAL_ERROR, "genie_generator_non_stowed");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_generator_internal: generate space and push a REF.
ref_mode: REF mode to be generated
tag: associated internal LOC for this generator
loc_or_heap: where to generate space.                                         */

static void
genie_generator_internal (NODE_T * p, MOID_T * ref_mode, TAG_T * tag, int loc_or_heap, BOOL_T declarer_prepared)
{
  MOID_T *mode = SUB (ref_mode);
  if (mode->has_rows)
    {
      if (WHETHER (mode, STRUCT_SYMBOL))
	{
	  if (loc_or_heap == HEAP_SYMBOL)
	    {
/* HEAP STRUCT with row. */
	      ADDR_T sp_for_voiding = stack_pointer;
	      A68_REF name, struct_ref;
	      struct_ref.status = INITIALISED_MASK;
	      struct_ref.segment = NULL;
	      struct_ref.offset = 0;
	      struct_ref.handle = &nil_handle;
	      /*  INITIALISED_MASK, 0, &nil_handle */
	      if (!declarer_prepared)
		{
		  genie_prepare_declarer (p);
		}
	      name = genie_allocate_declarer (p, &stack_pointer, struct_ref, A_FALSE);
	      stack_pointer = sp_for_voiding;
	      PUSH_REF (p, name);
	    }
	  else if (loc_or_heap == LOC_SYMBOL)
	    {
/* LOC STRUCT with row */
	      ADDR_T sp_for_voiding = stack_pointer;
	      A68_REF struct_ref;
	      struct_ref.status = INITIALISED_MASK;
	      struct_ref.segment = frame_segment;
	      struct_ref.offset = frame_pointer + FRAME_INFO_SIZE + tag->offset;
	      struct_ref.handle = &nil_handle;
	      if (!declarer_prepared)
		{
		  genie_prepare_declarer (p);
		}
	      (void) genie_allocate_declarer (p, &stack_pointer, struct_ref, A_TRUE);
	      stack_pointer = sp_for_voiding;
	      PUSH_REF (p, struct_ref);
	    }
	  else
	    {
	      abend (A_TRUE, INTERNAL_ERROR, "genie_generator_internal");
	    }
	}
      else if (WHETHER (mode, UNION_SYMBOL))
	{
	  genie_generator_non_stowed (p, tag, loc_or_heap, mode);
	}
      else
	{
/* Generators for rows. */
	  if (loc_or_heap == HEAP_SYMBOL)
	    {
/* HEAP row. */
	      ADDR_T sp_for_voiding = stack_pointer;
	      A68_REF descriptor, name, dummy_ref;
	      dummy_ref.status = INITIALISED_MASK;
	      dummy_ref.segment = NULL;
	      dummy_ref.offset = 0;
	      dummy_ref.handle = &nil_handle;
	      /*  INITIALISED_MASK, 0, &nil_handle */
	      if (!declarer_prepared)
		{
		  genie_prepare_declarer (p);
		}
	      descriptor = genie_allocate_declarer (p, &stack_pointer, dummy_ref, A_FALSE);
	      name = heap_generator (p, ref_mode, MOID_SIZE (ref_mode));
	      *(A68_REF *) ADDRESS (&name) = descriptor;
	      stack_pointer = sp_for_voiding;
	      PUSH_REF (p, name);
	    }
	  else if (loc_or_heap == LOC_SYMBOL)
	    {
	      /* LOC row */
	      ADDR_T sp_for_voiding = stack_pointer;
	      A68_REF descriptor, name, dummy_ref;
	      name.status = INITIALISED_MASK;
	      name.segment = frame_segment;
	      name.offset = frame_pointer + FRAME_INFO_SIZE + tag->offset;
	      name.handle = &nil_handle;
	      dummy_ref.status = INITIALISED_MASK;
	      dummy_ref.segment = frame_segment;
	      dummy_ref.offset = 0;
	      dummy_ref.handle = &nil_handle;
	      if (!declarer_prepared)
		{
		  genie_prepare_declarer (p);
		}
	      descriptor = genie_allocate_declarer (p, &stack_pointer, dummy_ref, A_FALSE);
	      *(A68_REF *) ADDRESS (&name) = descriptor;
	      stack_pointer = sp_for_voiding;
	      PUSH_REF (p, name);
	    }
	  else
	    {
	      abend (A_TRUE, INTERNAL_ERROR, "genie_generator_internal");
	    }
	}
    }
  else
    {
/* Generators for non stowed. */
      genie_generator_non_stowed (p, tag, loc_or_heap, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_generator: make and push a name.                                        */

PROPAGATOR_T
genie_generator (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_generator;
  self.source = p;
  genie_generator_internal (NEXT (SUB (p)), MOID (p), TAX (p), ATTRIBUTE (SUB (p)), A_FALSE);
  PROTECT_FROM_SWEEP (p);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_selection_value: SELECTION from a value.                                */

PROPAGATOR_T
genie_selection_value (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  MOID_T *result_mode = MOID (selector);
  ADDR_T old_stack_pointer = stack_pointer;
  int size = MOID_SIZE (result_mode);
  EXECUTE_UNIT (NEXT (selector));
  stack_pointer = old_stack_pointer;
  MOVE (STACK_TOP, STACK_OFFSET (PACK (SUB (selector))->offset), (unsigned) size);
  INCREMENT_STACK_POINTER (selector, size);
  PROTECT_FROM_SWEEP (p);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_selection_name: SELECTION from a name.                                  */

PROPAGATOR_T
genie_selection_name (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  MOID_T *struct_mode = MOID (NEXT (selector));
  A68_REF *z;
  EXECUTE_UNIT (NEXT (selector));
  z = (A68_REF *) (STACK_OFFSET (-SIZE_OF (A68_REF)));
  TEST_NIL (selector, *z, struct_mode);
  z->offset += PACK (SUB (selector))->offset;
  PROTECT_FROM_SWEEP (p);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_selection: push selection from secondary.                               */

PROPAGATOR_T
genie_selection (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  PROPAGATOR_T self;
  MOID_T *struct_mode = MOID (NEXT (selector)), *result_mode = MOID (selector);
  BOOL_T selection_of_name = (WHETHER (struct_mode, REF_SYMBOL));
  self.source = p;
  EXECUTE_UNIT (NEXT (selector));
/* Multiple selections. */
  if (selection_of_name && (WHETHER (SUB (struct_mode), FLEX_SYMBOL) || WHETHER (SUB (struct_mode), ROW_SYMBOL)))
    {
      A68_REF *row1, row2, row3;
      int dims, desc_size;
      POP_ADDRESS (selector, row1, A68_REF);
      row1 = (A68_REF *) ADDRESS (row1);
      dims = DEFLEX (SUB (struct_mode))->dimensions;
      desc_size = SIZE_OF (A68_ARRAY) + dims * SIZE_OF (A68_TUPLE);
      row2 = heap_generator (selector, result_mode, desc_size);
      MOVE (ADDRESS (&row2), (BYTE_T *) ADDRESS (row1), (unsigned) desc_size);
      MOID (((A68_ARRAY *) ADDRESS (&row2))) = SUB (SUB (result_mode));
      ((A68_ARRAY *) ADDRESS (&row2))->field_offset += PACK (SUB (selector))->offset;
      row3 = heap_generator (selector, result_mode, SIZE_OF (A68_REF));
      *(A68_REF *) ADDRESS (&row3) = row2;
      PUSH (selector, &row3, SIZE_OF (A68_REF));
      self.unit = genie_selection;
      PROTECT_FROM_SWEEP (p);
      return (self);
    }
  else if (struct_mode != NULL && (WHETHER (struct_mode, FLEX_SYMBOL) || WHETHER (struct_mode, ROW_SYMBOL)))
    {
      A68_REF *row1, row2;
      int dims, desc_size;
      POP_ADDRESS (selector, row1, A68_REF);
      dims = DEFLEX (struct_mode)->dimensions;
      desc_size = SIZE_OF (A68_ARRAY) + dims * SIZE_OF (A68_TUPLE);
      row2 = heap_generator (selector, result_mode, desc_size);
      MOVE (ADDRESS (&row2), (BYTE_T *) ADDRESS (row1), (unsigned) desc_size);
      MOID (((A68_ARRAY *) ADDRESS (&row2))) = SUB (result_mode);
      ((A68_ARRAY *) ADDRESS (&row2))->field_offset += PACK (SUB (selector))->offset;
      PUSH (selector, &row2, SIZE_OF (A68_REF));
      self.unit = genie_selection;
      PROTECT_FROM_SWEEP (p);
      return (self);
    }
/* Normal selections. */
  else if (selection_of_name && WHETHER (SUB (struct_mode), STRUCT_SYMBOL))
    {
      A68_REF *z = (A68_REF *) (STACK_OFFSET (-SIZE_OF (A68_REF)));
      TEST_NIL (selector, *z, struct_mode);
      z->offset += PACK (SUB (selector))->offset;
      self.unit = genie_selection_name;
      PROTECT_FROM_SWEEP (p);
      return (self);
    }
  else if (WHETHER (struct_mode, STRUCT_SYMBOL))
    {
      DECREMENT_STACK_POINTER (selector, MOID_SIZE (struct_mode));
      MOVE (STACK_TOP, STACK_OFFSET (PACK (SUB (selector))->offset), (unsigned) MOID_SIZE (result_mode));
      INCREMENT_STACK_POINTER (selector, MOID_SIZE (result_mode));
      self.unit = genie_selection_value;
      PROTECT_FROM_SWEEP (p);
      return (self);
    }
  else
    {
      abend (A_TRUE, "cannot select", moid_to_string (struct_mode, 80));
      return (self);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_call_operator: call operator.
Arguments are already on the stack.                                           */

void
genie_call_operator (NODE_T * p, ADDR_T sp0)
{
  A68_PROCEDURE z;
  MOID_T *oper_moid = MOID (TAX (p));
  z = *(A68_PROCEDURE *) FRAME_SHORTCUT (p);
  if (z.body.status & STANDENV_PROCEDURE_MASK)
    {
      GENIE_PROCEDURE *proc = (GENIE_PROCEDURE *) (z.body.value);
      if (proc != NULL)
	{
	  (void) proc (p);
	}
    }
  else
    {
      NODE_T *body = ((NODE_T *) z.body.value);
      NODE_T *entry = SUB (body);
      PACK_T *args = PACK (oper_moid);
      ADDR_T fp0;
      open_frame (entry, IS_PROCEDURE_PARM, z.environ.offset);
/* Copy arguments from stack to frame. */
      stack_pointer = sp0;
      fp0 = 0;
      for (; args != NULL; args = NEXT (args))
	{
	  int size = MOID_SIZE (MOID (args));
	  MOVE ((FRAME_OFFSET (FRAME_INFO_SIZE + fp0)), STACK_ADDRESS (sp0), (unsigned) size);
	  sp0 += size;
	  fp0 += size;
	}
/* Interpret routine text. */
      EXECUTE_UNIT (NEXT (NEXT (NEXT (entry))));
      CLOSE_FRAME;
    }
  genie_scope_check (p, MOID (p));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_monadic: push result of monadic formula OP "u".                         */

PROPAGATOR_T
genie_monadic (NODE_T * p)
{
  NODE_T *op = SUB (p);
  NODE_T *u = NEXT (op);
  PROPAGATOR_T self;
  ADDR_T sp = stack_pointer;
  self.unit = genie_monadic;
  self.source = p;
  EXECUTE_UNIT (u);
  if (TAX (op)->procedure != NULL)
    {
      (void) (TAX (op)->procedure) (op);
    }
  else
    {
      genie_call_operator (op, sp);
    }
  PROTECT_FROM_SWEEP (p);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_formula_standenv_quick: standard dyadic formula "u OP v".               */

PROPAGATOR_T
genie_formula_standenv_quick (NODE_T * p)
{
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  NODE_T *v = NEXT (op);
  EXECUTE_UNIT (u);
  EXECUTE_UNIT (v);
  (void) (TAX (op)->procedure) (op);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_formula_quick: (optimised) dyadic formula "u" OP "v".                   */

PROPAGATOR_T
genie_formula_quick (NODE_T * p)
{
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  NODE_T *v = NEXT (op);
  ADDR_T sp = stack_pointer;
  EXECUTE_UNIT (u);
  EXECUTE_UNIT (v);
  genie_call_operator (op, sp);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_formula: push result of formula.                                        */

PROPAGATOR_T
genie_formula (NODE_T * p)
{
  PROPAGATOR_T self;
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  ADDR_T sp = stack_pointer;
  self.unit = genie_formula;
  self.source = p;
  EXECUTE_UNIT (u);
  if (op != NULL)
    {
      NODE_T *v = NEXT (op);
      EXECUTE_UNIT (v);
/* Operate on top of stack and try to optimise the formula. */
      if (TAX (op)->procedure != NULL)
	{
	  (void) (TAX (op)->procedure) (op);
	  if (p->protect_sweep == NULL)
	    {
	      self.unit = genie_formula_standenv_quick;
	    }
	}
      else
	{
	  genie_call_operator (op, sp);
	  if (p->protect_sweep == NULL)
	    {
	      self.unit = genie_formula_quick;
	    }
	}
    }
  PROTECT_FROM_SWEEP (p);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_nihil: push NIL.                                                        */

PROPAGATOR_T
genie_nihil (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_nihil;
  self.source = p;
  PUSH_REF (p, nil_ref);
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_copy_union: copies union with stowed components on top of the stack.    */

static void
genie_copy_union (NODE_T * p)
{
  A68_POINTER *u = (A68_POINTER *) STACK_TOP;
  MOID_T *v = (MOID_T *) u->value;
  if (v != NULL)
    {
      unsigned v_size = MOID_SIZE (v);
      INCREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
      if (WHETHER (v, STRUCT_SYMBOL))
	{
	  A68_REF old, new_one;
	  old.status = INITIALISED_MASK;
	  old.segment = stack_segment;
	  old.offset = stack_pointer;
	  old.handle = &nil_handle;
	  new_one = genie_copy_stowed (old, p, v);
	  MOVE (STACK_TOP, ADDRESS (&old), v_size);
	}
      else if (WHETHER (v, ROW_SYMBOL) || WHETHER (v, FLEX_SYMBOL))
	{
	  A68_REF new_one, old = *(A68_REF *) STACK_TOP;
	  new_one = genie_copy_stowed (old, p, v);
	  MOVE (STACK_TOP, &new_one, SIZE_OF (A68_REF));
	}
      DECREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_assign_internal: internal workings of an assignment of stowed objects.  */

static void
genie_assign_internal (NODE_T * p, A68_REF * z, MOID_T * source_moid)
{
  if (WHETHER (source_moid, FLEX_SYMBOL) || source_moid == MODE (STRING))
    {
/* Assign to FLEX [] AMODE. */
      A68_REF old = *(A68_REF *) STACK_TOP, new_one;
      new_one = genie_copy_stowed (old, p, source_moid);
      if (source_moid->has_flex)
	{
	  *(A68_REF *) ADDRESS (z) = new_one;
	}
      else
	{
	  genie_assign_stowed (new_one, (A68_REF *) ADDRESS (z), p, source_moid);
	}
    }
  else if (WHETHER (source_moid, ROW_SYMBOL))
    {
/* Assign to [] AMODE. */
      A68_REF old = *(A68_REF *) STACK_TOP, new_one;
      new_one = genie_copy_stowed (old, p, source_moid);
      if (source_moid->has_flex)
	{
	  *(A68_REF *) ADDRESS (z) = new_one;
	}
      else
	{
	  genie_assign_stowed (new_one, (A68_REF *) ADDRESS (z), p, source_moid);
	}
    }
  else if (WHETHER (source_moid, STRUCT_SYMBOL))
    {
/* STRUCT with row */
      A68_REF w, src;
      w.status = INITIALISED_MASK;
      w.segment = stack_segment;
      w.offset = stack_pointer;
      w.handle = &nil_handle;
      src = genie_copy_stowed (w, p, source_moid);
      genie_assign_stowed (src, z, p, source_moid);
    }
  else if (WHETHER (source_moid, UNION_SYMBOL))
    {
/* UNION with row */
      genie_copy_union (p);
      MOVE (ADDRESS (z), STACK_TOP, MOID_SIZE (source_moid));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_loc_assignation: ASSIGNATION to LOC local name.                         */

PROPAGATOR_T
genie_loc_assignation (NODE_T * p)
{
  NODE_T *q = SUB (p)->genie.propagator.source;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  A68_REF *z = (A68_REF *) FRAME_SHORTCUT (q);
  TEST_NIL (p, *z, MOID (q));
  EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
  DECREMENT_STACK_POINTER (p, size);
  z->status |= ASSIGNED_MASK;
  MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
  PUSH (p, z, SIZE_OF (A68_REF));
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_voiding_loc_assignation: VOIDING assignation to a LOC local name.       */

PROPAGATOR_T
genie_voiding_loc_assignation (NODE_T * p)
{
  NODE_T *q = SUB (p)->genie.propagator.source;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  A68_REF *z = (A68_REF *) FRAME_SHORTCUT (q);
  TEST_NIL (p, *z, MOID (q));
  EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
  DECREMENT_STACK_POINTER (p, size);
  z->status |= ASSIGNED_MASK;
  MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_loc_constant_assignation: Assigning a constant to a LOC local name.     */

PROPAGATOR_T
genie_loc_constant_assignation (NODE_T * p)
{
  NODE_T *q = SUB (p)->genie.propagator.source;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  A68_REF *z = (A68_REF *) FRAME_SHORTCUT (q);
  TEST_NIL (p, *z, MOID (q));
  z->status |= ASSIGNED_MASK;
  MOVE (ADDRESS (z), (BYTE_T *) (NEXT (NEXT (SUB (p)))->genie.propagator.source->genie.constant), (unsigned) size);
  PUSH (p, z, SIZE_OF (A68_REF));
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_voiding_loc_constant_assignation: 
   VOIDING assignation from a constant to a LOC local name.                   */

PROPAGATOR_T
genie_voiding_loc_constant_assignation (NODE_T * p)
{
  NODE_T *q = SUB (p)->genie.propagator.source;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  A68_REF *z = (A68_REF *) FRAME_SHORTCUT (q);
  TEST_NIL (p, *z, MOID (q));
  z->status |= ASSIGNED_MASK;
  MOVE (ADDRESS (z), (BYTE_T *) (NEXT (NEXT (SUB (p)))->genie.propagator.source->genie.constant), (unsigned) size);
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_assignation_quick: (optimised) assignation.                             */

PROPAGATOR_T
genie_assignation_quick (NODE_T * p)
{
  PROPAGATOR_T dest, source;
  A68_REF *z;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  dest = EXECUTE_UNIT (SUB (p));
  source = EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
  DECREMENT_STACK_POINTER (p, size);
  z = (A68_REF *) (STACK_OFFSET (-SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (p));
  if (source_moid->has_rows)
    {
      genie_assign_internal (p, z, source_moid);
    }
  else
    {
      MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
      z->status |= ASSIGNED_MASK;
    }
  return (p->genie.propagator);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_assignation: assign a value to a name and push latter name.             */

PROPAGATOR_T
genie_assignation (NODE_T * p)
{
  PROPAGATOR_T self, dest, source;
  A68_REF *z;
  BOOL_T source_isnt_stowed;
  MOID_T *source_moid = SUB (MOID (p));
  int size = MOID_SIZE (source_moid);
  dest = EXECUTE_UNIT (SUB (p));
  source = EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
  DECREMENT_STACK_POINTER (p, size);
  z = (A68_REF *) (STACK_OFFSET (-SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (p));
  if (source_moid->has_rows)
    {
      source_isnt_stowed = A_FALSE;
      genie_assign_internal (p, z, source_moid);
    }
  else
    {
      source_isnt_stowed = A_TRUE;
      MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
      z->status |= ASSIGNED_MASK;
    }
  if (dest.unit == genie_loc_identifier && source_isnt_stowed)
    {
      if (source.unit == genie_constant)
	{
	  self.unit = genie_loc_constant_assignation;
	}
      else
	{
	  self.unit = genie_loc_assignation;
	}
    }
  else
    {
      self.unit = genie_assignation_quick;
    }
  self.source = p;
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_identity_relation: push equality of two REFs.                           */

PROPAGATOR_T
genie_identity_relation (NODE_T * p)
{
  PROPAGATOR_T self;
  A68_REF x, y;
  self.unit = genie_identity_relation;
  self.source = p;
  EXECUTE_UNIT (SUB (p));
  POP (p, &y, SIZE_OF (A68_REF));
  EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
  POP (p, &x, SIZE_OF (A68_REF));
  if (WHETHER (NEXT (SUB (p)), IS_SYMBOL))
    {
      PUSH_BOOL (p, ADDRESS (&x) == ADDRESS (&y));
    }
  else
    {
      PUSH_BOOL (p, ADDRESS (&x) != ADDRESS (&y));
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_and_function: push result of ANDF.                                      */

PROPAGATOR_T
genie_and_function (NODE_T * p)
{
  PROPAGATOR_T self;
  A68_BOOL x;
  self.unit = genie_and_function;
  self.source = p;
  EXECUTE_UNIT (SUB (p));
  POP_BOOL (p, &x);
  if (x.value == A_TRUE)
    {
      EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
    }
  else
    {
      PUSH_BOOL (p, A_FALSE);
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_or_function: push result of ORF.                                        */

PROPAGATOR_T
genie_or_function (NODE_T * p)
{
  PROPAGATOR_T self;
  A68_BOOL x;
  self.unit = genie_or_function;
  self.source = p;
  EXECUTE_UNIT (SUB (p));
  POP_BOOL (p, &x);
  if (x.value == A_FALSE)
    {
      EXECUTE_UNIT (NEXT (NEXT (SUB (p))));
    }
  else
    {
      PUSH_BOOL (p, A_TRUE);
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_routine_text: push routine text.                                        */

PROPAGATOR_T
genie_routine_text (NODE_T * p)
{
  static PROPAGATOR_T self;
  A68_PROCEDURE z = *(A68_PROCEDURE *) (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset));
  self.unit = genie_routine_text;
  self.source = p;
  PUSH (p, &z, SIZE_OF (A68_PROCEDURE));
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_push_undefined: push an undefined value of the required mode.           */

void
genie_push_undefined (NODE_T * p, MOID_T * u)
{
/* For primitiue modes we push an initialised value. */
  if (u == MODE (INT))
    {
      PUSH_INT (p, (int) (rng_53_bit () * MAX_INT));
    }
  else if (u == MODE (REAL))
    {
      PUSH_REAL (p, rng_53_bit ());
    }
  else if (u == MODE (BOOL))
    {
      PUSH_BOOL (p, rng_53_bit () < 0.5);
    }
  else if (u == MODE (CHAR))
    {
      PUSH_CHAR (p, (char) (32 + 96 * rng_53_bit ()));
    }
  else if (u == MODE (BITS))
    {
      PUSH_BITS (p, (unsigned) (rng_53_bit () * MAX_UNT));
    }
  else if (u == MODE (BYTES))
    {
      PUSH_BYTES (p, "SKIP");
    }
  else if (u == MODE (LONG_BYTES))
    {
      PUSH_LONG_BYTES (p, "SKIP");
    }
  else if (u == MODE (STRING))
    {
      PUSH_REF (p, empty_string (p));
    }
  else if (u == MODE (LONG_INT) || u == MODE (LONGLONG_INT))
    {
      int digits = get_mp_digits (u);
      mp_digit *z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
    }
  else if (u == MODE (LONG_REAL) || u == MODE (LONGLONG_REAL))
    {
      int digits = get_mp_digits (u);
      mp_digit *z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
    }
  else if (u == MODE (LONG_BITS) || u == MODE (LONGLONG_BITS))
    {
      int digits = get_mp_digits (u);
      mp_digit *z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
    }
  else if (u == MODE (LONG_COMPLEX) || u == MODE (LONGLONG_COMPLEX))
    {
      int digits = get_mp_digits (u);
      mp_digit *z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
      z = stack_mp (p, digits);
      SET_MP_ZERO (z, digits);
      z[0] = INITIALISED_MASK;
    }
  else if (WHETHER (u, REF_SYMBOL))
    {
/* All REFs are NIL. */
      PUSH_REF (p, nil_ref);
    }
  else if (WHETHER (u, ROW_SYMBOL) || WHETHER (u, FLEX_SYMBOL))
    {
/* [] AMODE or FLEX [] AMODE. */
      PUSH_REF (p, empty_row (p, u));
    }
  else if (WHETHER (u, STRUCT_SYMBOL))
    {
/* STRUCT */
      PACK_T *v;
      for (v = PACK (u); v != NULL; v = NEXT (v))
	{
	  genie_push_undefined (p, MOID (v));
	}
    }
  else if (WHETHER (u, UNION_SYMBOL))
    {
/* UNION */
      PUSH_POINTER (p, MODE (VOID));
    }
  else
    {
/* PROC: an uninitialised value. */
      BYTE_T *sp = STACK_TOP;
      INCREMENT_STACK_POINTER (p, MOID_SIZE (u));
      memset (sp, 0x00, (unsigned) MOID_SIZE (u));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_skip: push an undefined value of the required mode.                     */

PROPAGATOR_T
genie_skip (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_skip;
  self.source = p;
  if (MOID (p) != MODE (VOID))
    {
      genie_push_undefined (p, MOID (p));
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_jump: jump to the serial clause where the label is at. 
Stack pointer and frame pointer were saved at target serial clause.           */

static void
genie_jump (NODE_T * p)
{
  NODE_T *jump = SUB (p);
  NODE_T *label = (WHETHER (jump, GOTO_SYMBOL)) ? NEXT (jump) : jump;
  ADDR_T f = frame_pointer;
  jmp_buf *jump_stat;
/* Find the stack frame this jump points to. */
  BOOL_T found = A_FALSE;
  while (f > 0 && !found)
    {
      found = (SYMBOL_TABLE (TAX (label)) == SYMBOL_TABLE (FRAME_TREE (f))) && FRAME_JUMP_STAT (f) != NULL;
      if (!found)
	{
	  f = FRAME_STATIC_LINK (f);
	}
    }
  jump_stat = FRAME_JUMP_STAT (f);
/* Beam us up, Scotty! */
  SYMBOL_TABLE (TAX (label))->jump_to = TAX (label)->unit;
  longjmp (*(jump_stat), 1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_primary: execute primary.                                               */

static PROPAGATOR_T global_prop;

PROPAGATOR_T
genie_primary (NODE_T * p)
{
  if (p->genie.whether_coercion)
    {
      global_prop = genie_coercion (p);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case PRIMARY:
	  {
	    global_prop = genie_primary (SUB (p));
	    break;
	  }
	case ENCLOSED_CLAUSE:
	  {
	    global_prop = genie_enclosed (SUB (p));
	    break;
	  }
	case IDENTIFIER:
	  {
	    global_prop = genie_identifier (p);
	    break;
	  }
	case CALL:
	  {
	    global_prop = genie_call (p);
	    break;
	  }
	case SLICE:
	  {
	    global_prop = genie_slice (p);
	    break;
	  }
	case DENOTER:
	  {
	    global_prop = genie_denoter (p);
	    break;
	  }
	case CAST:
	  {
	    global_prop = genie_cast (p);
	    break;
	  }
	case FORMAT_TEXT:
	  {
	    global_prop = genie_format_text (p);
	    break;
	  }
	default:
	  {
	    diagnostic (A_RUNTIME_ERROR, p, INTERNAL_ERROR, "genie_primary");
	    exit_genie (p, A_RUNTIME_ERROR);
	  }
	}
    }
  p->genie.propagator = global_prop;
  return (global_prop);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_secondary: execute secondary.                                           */

PROPAGATOR_T
genie_secondary (NODE_T * p)
{
  if (p->genie.whether_coercion)
    {
      global_prop = genie_coercion (p);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case SECONDARY:
	  {
	    global_prop = genie_secondary (SUB (p));
	    break;
	  }
	case PRIMARY:
	  {
	    global_prop = genie_primary (SUB (p));
	    break;
	  }
	case GENERATOR:
	  {
	    global_prop = genie_generator (p);
	    break;
	  }
	case SELECTION:
	  {
	    global_prop = genie_selection (p);
	    break;
	  }
	default:
	  {
	    global_prop = genie_primary (p);
	    break;
	  }
	}
    }
  p->genie.propagator = global_prop;
  return (global_prop);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_tertiary: execute tertiary.                                             */

PROPAGATOR_T
genie_tertiary (NODE_T * p)
{
  if (p->genie.whether_coercion)
    {
      global_prop = genie_coercion (p);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case TERTIARY:
	  {
	    global_prop = genie_tertiary (SUB (p));
	    break;
	  }
	case SECONDARY:
	  {
	    global_prop = genie_secondary (SUB (p));
	    break;
	  }
	case PRIMARY:
	  {
	    global_prop = genie_primary (SUB (p));
	    break;
	  }
	case FORMULA:
	  {
	    global_prop = genie_formula (p);
	    break;
	  }
	case MONADIC_FORMULA:
	  {
	    global_prop = genie_monadic (p);
	    break;
	  }
	case NIHIL:
	  {
	    global_prop = genie_nihil (p);
	    break;
	  }
	case JUMP:
	  {
	    /* Leave this here */
	    global_prop.unit = genie_tertiary;
	    global_prop.source = p;
	    genie_jump (p);
	    break;
	  }
	default:
	  {
	    global_prop = genie_secondary (p);
	    break;
	  }
	}
    }
  p->genie.propagator = global_prop;
  return (global_prop);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_unit: execute a unit.                                                   */

PROPAGATOR_T
genie_unit (NODE_T * p)
{
  if (p->genie.whether_coercion)
    {
      global_prop = genie_coercion (p);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case UNIT:
	  {
	    global_prop = EXECUTE_UNIT (SUB (p));
	    break;
	  }
	case TERTIARY:
	  {
	    global_prop = genie_tertiary (SUB (p));
	    break;
	  }
	case SECONDARY:
	  {
	    global_prop = genie_secondary (SUB (p));
	    break;
	  }
	case PRIMARY:
	  {
	    global_prop = genie_primary (SUB (p));
	    break;
	  }
	case ASSIGNATION:
	  {
	    global_prop = genie_assignation (p);
	    break;
	  }
	case IDENTITY_RELATION:
	  {
	    global_prop = genie_identity_relation (p);
	    break;
	  }
	case AND_FUNCTION:
	  {
	    global_prop = genie_and_function (p);
	    break;
	  }
	case OR_FUNCTION:
	  {
	    global_prop = genie_or_function (p);
	    break;
	  }
	case ROUTINE_TEXT:
	  {
	    global_prop = genie_routine_text (p);
	    break;
	  }
	case SKIP:
	  {
	    global_prop = genie_skip (p);
	    break;
	  }
	case JUMP:
	  {
	    global_prop.unit = genie_unit;
	    global_prop.source = p;
	    genie_jump (p);
	    break;
	  }
	case ASSERTION:
	  {
	    global_prop = genie_assertion (p);
	    break;
	  }
	default:
	  {
	    global_prop = genie_tertiary (p);
	    break;
	  }
	}
    }
  p->genie.propagator = global_prop;
  return (global_prop);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_unit_trace: execute a unit, possibly in trace mood.                     */

static int unit_count = 0;

void
genie_unit_trace (NODE_T * p)
{
  BOOL_T trace_mood = MASK (p) & TRACE_MASK;
  if (p->info->module->options.time_limit > 0)
    {
      if (unit_count == 25000)
	{
	  A68_REAL cputime;
	  genie_cputime (p);
	  POP_REAL (p, &cputime);
	  if (cputime.value > p->info->module->options.time_limit)
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "time limit exceeded");
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  unit_count = 0;
	}
      else
	{
	  unit_count++;
	}
    }
  if (sys_request_flag && !trace_mood)
    {
      sprintf (output_line, "Entering monitor, type 'help' for help");
      io_write_string (STDOUT_FILENO, output_line);
      single_step (p);
      sys_request_flag = A_FALSE;
      EXECUTE_UNIT (p);
    }
  else if (trace_mood)
    {
      single_step (p);
      EXECUTE_UNIT (p);
    }
  else
    {
      EXECUTE_UNIT (p);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_serial_units_no_label_linear: (optimised) serial clause.                */

void
genie_serial_units_no_label_linear (NODE_T * p, int saved_stack_pointer)
{
  for (; p != NULL; p = SEQUENCE (p))
    {
      if (WHETHER (p, UNIT))
	{
	  EXECUTE_UNIT_TRACE (p);
	}
      else if (WHETHER (p, SEMI_SYMBOL))
	{
	  stack_pointer = saved_stack_pointer;
	}
      else if (WHETHER (p, DECLARATION_LIST))
	{
	  genie_declaration (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_serial_units_no_label: execution of serial clause without labels.       */

void
genie_serial_units_no_label (NODE_T * p, int saved_stack_pointer, NODE_T ** seq)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case UNIT:
	  {
	    EXECUTE_UNIT_TRACE (p);
	    SEQUENCE (*seq) = p;
	    (*seq) = p;
	    return;
	  }
	case SEMI_SYMBOL:
	  {
/* Voiden the expression stack. */
	    stack_pointer = saved_stack_pointer;
	    SEQUENCE (*seq) = p;
	    (*seq) = p;
	    break;
	  }
	case DECLARATION_LIST:
	  {
	    genie_declaration (SUB (p));
	    SEQUENCE (*seq) = p;
	    (*seq) = p;
	    return;
	  }
	default:
	  {
	    genie_serial_units_no_label (SUB (p), saved_stack_pointer, seq);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_serial_units: execution of serial clause with labels.                   */

void
genie_serial_units (NODE_T * p, NODE_T ** jump_to, jmp_buf * exit_buf, int saved_stack_pointer)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case UNIT:
	  {
	    if (*jump_to == NULL)
	      {
		EXECUTE_UNIT_TRACE (p);
	      }
/* If we dropped in this clause from a jump then this unit may be the target. */
	    else if (p == *jump_to)
	      {
		*jump_to = NULL;
		EXECUTE_UNIT_TRACE (p);
	      }
	    return;
	  }
	case EXIT_SYMBOL:
	  {
	    if (*jump_to == NULL)
	      {
		longjmp (*exit_buf, 1);
	      }
	    break;
	  }
	case SEMI_SYMBOL:
	  {
	    if (*jump_to == NULL)
	      {
/* Voiden the expression stack. */
		stack_pointer = saved_stack_pointer;
	      }
	    break;
	  }
	default:
	  {
	    if (WHETHER (p, DECLARATION_LIST) && *jump_to == NULL)
	      {
		genie_declaration (SUB (p));
		return;
	      }
	    else
	      {
		genie_serial_units (SUB (p), jump_to, exit_buf, saved_stack_pointer);
	      }
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_serial_clause: execute serial clause.                                   */

void
genie_serial_clause (NODE_T * p, jmp_buf * exit_buf)
{
/* Since the genie can suspend sweeping temporarily, you might not get heap 
   space at the moment you want it. The preemptive mechanism may save the day.*/
  double f = (double) heap_pointer / (double) heap_size;
  double h = (double) free_handle_count / (double) max_handle_count;
  if (f > 0.9 || h < 0.01)
    {
      sweep_heap (p, frame_pointer);
    }
/* Decide how to execute the clause */
  if (SYMBOL_TABLE (p)->labels == NULL)
    {
/* No labels in this clause. */
      if (SEQUENCE (p) == NULL && SEQUENCE_SET (p) == A_FALSE)
	{
	  NODE_T top_seq;
	  NODE_T *seq = &top_seq;
	  genie_serial_units_no_label (SUB (p), stack_pointer, &seq);
	  SEQUENCE (p) = SEQUENCE (&top_seq);
	  SEQUENCE_SET (p) = A_TRUE;
	  if (SEQUENCE (p) != NULL && SEQUENCE (SEQUENCE (p)) == NULL)
	    {
	      MASK (p) |= OPTIMAL_MASK;
	    }
	}
      else if (MASK (p) & OPTIMAL_MASK)
	{
	  EXECUTE_UNIT_TRACE (SEQUENCE (p));
	}
      else
	{
	  genie_serial_units_no_label_linear (SEQUENCE (p), stack_pointer);
	}
    }
  else
    {
/* Labels in this clause. */
      jmp_buf jump_stat;
      ADDR_T saved_sp = stack_pointer, saved_fp = frame_pointer;
      FRAME_JUMP_STAT (frame_pointer) = &jump_stat;
      if (!setjmp (jump_stat))
	{
	  NODE_T *jump_to = NULL;
	  genie_serial_units (SUB (p), &jump_to, exit_buf, stack_pointer);
	}
      else
	{
/* HIjol! Restore state and look for indicated unit */
	  NODE_T *jump_to = SYMBOL_TABLE (p)->jump_to;
	  stack_pointer = saved_sp;
	  frame_pointer = saved_fp;
	  genie_serial_units (SUB (p), &jump_to, exit_buf, stack_pointer);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_enquiry_clause: execute enquiry clause.                                 */

void
genie_enquiry_clause (NODE_T * p)
{
  if (SEQUENCE (p) == NULL && SEQUENCE_SET (p) == A_FALSE)
    {
      NODE_T top_seq;
      NODE_T *seq = &top_seq;
      genie_serial_units_no_label (SUB (p), stack_pointer, &seq);
      SEQUENCE (p) = SEQUENCE (&top_seq);
      SEQUENCE_SET (p) = A_TRUE;
      if (SEQUENCE (p) != NULL && SEQUENCE (SEQUENCE (p)) == NULL)
	{
	  MASK (p) |= OPTIMAL_MASK;
	}
    }
  else if (MASK (p) & OPTIMAL_MASK)
    {
      EXECUTE_UNIT_TRACE (SEQUENCE (p));
    }
  else
    {
      genie_serial_units_no_label_linear (SEQUENCE (p), stack_pointer);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_collateral_units: execute collateral units.                             */

static void
genie_collateral_units (NODE_T * p, int *count)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, UNIT))
	{
	  EXECUTE_UNIT_TRACE (p);
	  (*count)++;
	  return;
	}
      else
	{
	  genie_collateral_units (SUB (p), count);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_collateral: execute collateral clause.                                  */

PROPAGATOR_T genie_collateral (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_collateral;
  self.source = p;
/* VOID clause and STRUCT display */
  if (MOID (p) == MODE (VOID) || WHETHER (MOID (p), STRUCT_SYMBOL))
    {
      int count = 0;
      genie_collateral_units (SUB (p), &count);
    }
/* Row display */
  else
    {
      A68_REF new_display;
      int count = 0;
      ADDR_T sp = stack_pointer;
      MOID_T *m = MOID (p);
      genie_collateral_units (SUB (p), &count);
      if (DEFLEX (m)->dimensions == 1)
	{
/* [] AMODE display */
	  new_display = genie_make_row (p, DEFLEX (m)->slice, count, sp);
	  stack_pointer = sp;
	  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_REF));
	  *(A68_REF *) STACK_ADDRESS (sp) = new_display;
	}
      else
	{
/* [,,] AMODE display, we concatenate 1 + (n-1) to n dimensions */
	  new_display = genie_concatenate_rows (p, m, count, sp);
	  stack_pointer = sp;
	  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_REF));
	  *(A68_REF *) STACK_ADDRESS (sp) = new_display;
	}
    }
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_int_case_unit: execute unit from integral-case in-part.                 */

BOOL_T genie_int_case_unit (NODE_T * p, int k, int *count)
{
  if (p == NULL)
    {
      return (A_FALSE);
    }
  else
    {
      if (WHETHER (p, UNIT))
	{
	  if (k == *count)
	    {
	      EXECUTE_UNIT_TRACE (p);
	      return (A_TRUE);
	    }
	  else
	    {
	      (*count)++;
	      return (A_FALSE);
	    }
	}
      else
	{
	  if (genie_int_case_unit (SUB (p), k, count) != 0)
	    {
	      return (A_TRUE);
	    }
	  else
	    {
	      return (genie_int_case_unit (NEXT (p), k, count));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_united_case_unit: execute unit from united-case in-part.                */

BOOL_T genie_united_case_unit (NODE_T * p, MOID_T * m)
{
  if (p == NULL)
    {
      return (A_FALSE);
    }
  else
    {
      if (WHETHER (p, SPECIFIER))
	{
	  MOID_T *spec_moid = MOID (NEXT (SUB (p)));
	  BOOL_T equal_modes;
	  if (m != NULL)
	    {
	      if (WHETHER (spec_moid, UNION_SYMBOL))
		{
		  equal_modes = whether_unitable (m, spec_moid, NO_DEFLEXING);
		}
	      else
		{
		  equal_modes = (m == spec_moid);
		}
	    }
	  else
	    {
	      equal_modes = A_FALSE;
	    }
	  if (equal_modes)
	    {
	      NODE_T *q = NEXT (NEXT (SUB (p)));
	      open_frame (p, IS_NOT_PROCEDURE_PARM, frame_pointer);
	      if (WHETHER (q, IDENTIFIER))
		{
		  if (WHETHER (spec_moid, UNION_SYMBOL))
		    {
		      MOVE ((FRAME_OFFSET (FRAME_INFO_SIZE + TAX (q)->offset)), STACK_TOP, (unsigned) MOID_SIZE (spec_moid));
		    }
		  else
		    {
		      MOVE ((FRAME_OFFSET (FRAME_INFO_SIZE + TAX (q)->offset)), STACK_OFFSET (SIZE_OF (A68_UNION)), (unsigned) MOID_SIZE (spec_moid));
		    }
		}
	      EXECUTE_UNIT_TRACE (NEXT (NEXT (p)));
	      CLOSE_FRAME;
	      return (A_TRUE);
	    }
	  else
	    {
	      return (A_FALSE);
	    }
	}
      else
	{
	  if (genie_united_case_unit (SUB (p), m))
	    {
	      return (A_TRUE);
	    }
	  else
	    {
	      return (genie_united_case_unit (NEXT (p), m));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_enclosed: execute enclosed clause.                                      */

PROPAGATOR_T genie_enclosed (NODE_T * p)
{
  PROPAGATOR_T self;
  self.unit = genie_enclosed;
  self.source = p;
  switch (ATTRIBUTE (p))
    {
    case PARTICULAR_PROGRAM:
      {
	(void) genie_enclosed (SUB (p));
	break;
      }
    case ENCLOSED_CLAUSE:
      {
	(void) genie_enclosed (SUB (p));
	break;
      }
    case CLOSED_CLAUSE:
      {
	(void) genie_closed (NEXT_SUB (p));
	break;
      }
    case PARALLEL_CLAUSE:
      {
	(void) genie_collateral (NEXT_SUB (p));
	break;
      }
    case COLLATERAL_CLAUSE:
      {
	(void) genie_collateral (p);
	break;
      }
    case CONDITIONAL_CLAUSE:
      {
	genie_conditional (SUB (p), MOID (p));
	break;
      }
    case INTEGER_CASE_CLAUSE:
      {
	genie_int_case (SUB (p), MOID (p));
	break;
      }
    case UNITED_CASE_CLAUSE:
      {
	genie_united_case (SUB (p), MOID (p));
	break;
      }
    case LOOP_CLAUSE:
      {
	(void) genie_loop (SUB (p));
	break;
      }
    }
  genie_scope_check (p, MOID (p));
  PROTECT_FROM_SWEEP (p);
  p->genie.propagator = self;
  return (self);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
initialise_tuples: elaborate tuples elaborated by genie_prepare_bounds.       */

static int
initialise_tuples (ADDR_T sp, int dimensions)
{
  int elems = 1, k;
  sp -= (dimensions * SIZE_OF (A68_TUPLE));
  for (k = 0; k < dimensions; k++)
    {
      A68_TUPLE *tup = (A68_TUPLE *) STACK_ADDRESS (sp);
      int stride = ROW_SIZE (tup);
      tup->span = elems;
      abend ((stride > 0 && elems > MAX_INT / stride), INVALID_SIZE, NULL);
      elems *= stride;
      sp += SIZE_OF (A68_TUPLE);
    }
  return (elems);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_prepare_bounds: prepare bounds for a row.                               */

static void
genie_prepare_bounds (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  if (NEXT (p) != NULL && (WHETHER (NEXT (p), COLON_SYMBOL) || WHETHER (NEXT (p), DOTDOT_SYMBOL)))
	    {
	      A68_TUPLE t;
	      A68_INT j;
	      EXECUTE_UNIT (p);
	      POP_INT (p, &j);
	      t.shift = t.lower_bound = j.value;
	      p = NEXT (NEXT (p));
	      EXECUTE_UNIT (p);
	      POP_INT (p, &j);
	      t.upper_bound = j.value;
	      PUSH (p, &t, SIZE_OF (A68_TUPLE));
	    }
	  else
	    {
	      A68_TUPLE t;
	      A68_INT j;
	      t.shift = t.lower_bound = 1;
	      EXECUTE_UNIT (p);
	      POP_INT (p, &j);
	      t.upper_bound = j.value;
	      PUSH (p, &t, SIZE_OF (A68_TUPLE));
	    }
	}
      else
	{
	  genie_prepare_bounds (NEXT (p));
	  genie_prepare_bounds (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_prepare_struct: prepare dynamic declarers in a struct.                  */

static void
genie_prepare_struct (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, DECLARER))
	{
	  genie_prepare_declarer (SUB (p));
	}
      else
	{
	  genie_prepare_struct (NEXT (p));
	  genie_prepare_struct (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_prepare_declarer: execute what is needed for an object of dynamic size. */

void
genie_prepare_declarer (NODE_T * p)
{
  if (p != NULL)
    {
      if (MOID (p)->has_rows == A_TRUE)
	{
	  if (WHETHER (p, INDICANT))
	    {
	      if (!(TAX (p) == NULL && MOID (p) == MODE (STRING)))
		{
		  genie_prepare_declarer (NEXT (NEXT (NODE (TAX (p)))));
		}
	    }
	  else if (WHETHER (p, FLEX_SYMBOL))
	    {
	      genie_prepare_declarer (NEXT (p));
	    }
	  else if (WHETHER (p, DECLARER))
	    {
	      genie_prepare_declarer (SUB (p));
	    }
	  else if (WHETHER (p, BOUNDS))
	    {
	      genie_prepare_declarer (NEXT (p));
	      genie_prepare_bounds (SUB (p));
	    }
	  else if (WHETHER (p, STRUCT_SYMBOL))
	    {
	      genie_prepare_struct (p);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_allocate_struct: allocate declarers in a struct.                        */

static void
genie_allocate_struct (NODE_T * p, ADDR_T * sp, A68_REF ref_h, NODE_T ** declarer, PACK_T ** field)
{
  if (p != NULL)
    {
      if (WHETHER (p, DECLARER))
	{
	  *declarer = (MOID (p)->has_rows != 0) ? p : NULL;
	  genie_allocate_struct (NEXT (p), sp, ref_h, declarer, field);
	}
      else if (WHETHER (p, FIELD_IDENTIFIER))
	{
	  if (*declarer != NULL)
	    {
	      ref_h.offset += (*field)->offset;
	      if (WHETHER (MOID (*declarer), STRUCT_SYMBOL))
		{
		  (void) genie_allocate_declarer (SUB (*declarer), sp, ref_h, A_TRUE);
		}
	      else
		{
		  *(A68_REF *) ADDRESS (&ref_h) = genie_allocate_declarer (SUB (*declarer), sp, ref_h, A_TRUE);
		}
	    }
	  *field = NEXT (*field);
	}
      else
	{
	  genie_allocate_struct (SUB (p), sp, ref_h, declarer, field);
	  genie_allocate_struct (NEXT (p), sp, ref_h, declarer, field);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_allocate_declarer: allocate a stowed object of dynamic size.
Returns [..] AMODE or REF STRUCT (..).                                        */

A68_REF genie_allocate_declarer (NODE_T * p, ADDR_T * sp, A68_REF ref_h, BOOL_T struct_exists)
{
  if (p != NULL && MOID (p)->has_rows != 0)
    {
      if (WHETHER (p, INDICANT))
	{
	  if (TAX (p) == NULL && MOID (p) == MODE (STRING))
	    {
	      return (empty_string (p));
	    }
	  else
	    {
	      return (genie_allocate_declarer (NEXT (NEXT (NODE (TAX (p)))), sp, ref_h, struct_exists));
	    }
	}
      else if (WHETHER (p, FLEX_SYMBOL))
	{
	  return (genie_allocate_declarer (NEXT (p), sp, ref_h, struct_exists));
	}
      else if (WHETHER (p, DECLARER))
	{
	  return (genie_allocate_declarer (SUB (p), sp, ref_h, struct_exists));
	}
      else if (WHETHER (p, BOUNDS))
	{
/* [] AMODE */
	  A68_REF ref_desc, ref_row;
	  A68_ARRAY *arr;
	  A68_TUPLE *tup;
	  int k, dimensions = DEFLEX (MOID (p))->dimensions;
	  int elem_size = MOID_SIZE (MOID (NEXT (p)));
	  int row_size;
	  ADDR_T temp_sp;
	  up_garbage_sema ();
/* Initialise descriptor */
	  row_size = initialise_tuples (*sp, dimensions);
	  ref_desc = heap_generator (p, MOID (p), dimensions * SIZE_OF (A68_TUPLE) + SIZE_OF (A68_ARRAY));
	  ref_row = heap_generator (p, MOID (p), row_size * elem_size);
	  GET_DESCRIPTOR (arr, tup, &ref_desc);
	  arr->dimensions = dimensions;
	  arr->type = MOID (NEXT (p));
	  arr->elem_size = elem_size;
	  arr->slice_offset = 0;
	  arr->field_offset = 0;
	  arr->array = ref_row;
	  temp_sp = *sp;
	  for (k = 0; k < dimensions; k++)
	    {
	      temp_sp -= SIZE_OF (A68_TUPLE);
	      tup[k] = *(A68_TUPLE *) STACK_ADDRESS (temp_sp);
	    }
/* Initialise array elements */
	  *sp = temp_sp;
	  if (MOID (NEXT (p))->has_rows != 0)
	    {
	      int att = ATTRIBUTE (MOID (NEXT (p)));
	      A68_REF arr = ref_row;
	      for (k = 0; k < row_size; k++)
		{
		  A68_REF new_one;
		  *sp = temp_sp;
		  new_one = genie_allocate_declarer (NEXT (p), sp, arr, A_TRUE);
		  if (att == FLEX_SYMBOL || att == ROW_SYMBOL)
		    {
		      *(A68_REF *) ADDRESS (&arr) = new_one;
		    }
		  arr.offset += elem_size;
		}
	    }
	  down_garbage_sema ();
	  return (ref_desc);
	}
      else if (WHETHER (p, STRUCT_SYMBOL))
	{
/* STRUCT () */
	  NODE_T *declarer = NULL;
	  PACK_T *fields = PACK (MOID (p));
	  if (!struct_exists)
	    {
	      ADDR_T save_sp = *sp;
	      A68_REF ref_struct;
	      ref_struct = heap_generator (p, MOID (p), MOID_SIZE (MOID (p)));
	      protect_sweep_handle (&ref_struct);
	      genie_allocate_struct (p, sp, ref_struct, &declarer, &fields);
	      *sp = save_sp;
	      unprotect_sweep_handle (&ref_struct);
	      return (ref_struct);
	    }
	  else
	    {
	      ADDR_T save_sp = *sp;
	      genie_allocate_struct (p, sp, ref_h, &declarer, &fields);
	      *sp = save_sp;
	      return (ref_h);
	    }
	}
    }
  return (nil_ref);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_identity_dec: execute identity declaration.                             */

static void
genie_identity_dec (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case DEFINING_IDENTIFIER:
	case DEFINING_OPERATOR:
	  {
	    unsigned size = MOID_SIZE (MOID (p));
	    BYTE_T *z = (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset));
	    EXECUTE_UNIT (NEXT (NEXT (p)));
	    if (MOID (p)->has_rows)
	      {
		DECREMENT_STACK_POINTER (p, size);
		if (WHETHER (MOID (p), STRUCT_SYMBOL))
		  {
/* STRUCT with row */
		    A68_REF w, src;
		    w.status = INITIALISED_MASK;
		    w.segment = stack_segment;
		    w.offset = stack_pointer;
		    w.handle = &nil_handle;
		    src = genie_copy_stowed (w, p, MOID (p));
		    MOVE (z, ADDRESS (&src), size);
		  }
		else if (WHETHER (MOID (p), UNION_SYMBOL))
		  {
/* UNION with row */
		    genie_copy_union (p);
		    MOVE (z, STACK_TOP, size);
		  }
		else if (WHETHER (MOID (p), ROW_SYMBOL) || WHETHER (MOID (p), FLEX_SYMBOL))
		  {
/* (FLEX) ROW */
		    *(A68_REF *) z = genie_copy_stowed (*(A68_REF *) STACK_TOP, p, MOID (p));
		  }
	      }
	    else
	      {
		POP (p, z, size);
	      }
	    return;
	  }
	default:
	  {
	    genie_identity_dec (SUB (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_variable_dec: execute variable declaration.                             */

static void
genie_variable_dec (NODE_T * p, NODE_T ** declarer)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    *declarer = p;
	    if (SUB (MOID (p))->has_rows)
	      {
/* Prepare declarer so subsequent identifiers get identical bounds */
		genie_prepare_declarer (SUB (p));
	      }
	    break;
	  }
	case DEFINING_IDENTIFIER:
	  {
	    ADDR_T sp_for_voiding = stack_pointer;
	    MOID_T *ref_mode = MOID (*declarer);
	    TAG_T *tag = TAX (p);
	    int loc_or_heap = (HEAP (tag) == LOC_SYMBOL ? LOC_SYMBOL : HEAP_SYMBOL);
	    A68_REF *z = (A68_REF *) (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset));
	    genie_generator_internal (SUB (*declarer), ref_mode, tag->body, loc_or_heap, A_TRUE);
	    POP_REF (p, z);
	    if (NEXT (p) != NULL && WHETHER (NEXT (p), ASSIGN_SYMBOL))
	      {
		MOID_T *source_moid = SUB (MOID (p));
		int size = MOID_SIZE (source_moid);
		EXECUTE_UNIT (NEXT (NEXT (p)));
		DECREMENT_STACK_POINTER (p, size);
		if (source_moid->has_rows)
		  {
		    genie_assign_internal (p, z, source_moid);
		  }
		else
		  {
		    MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
		    z->status |= ASSIGNED_MASK;
		  }
	      }
	    stack_pointer = sp_for_voiding;	/* Voiding */
	    return;
	  }
	default:
	  {
	    genie_variable_dec (SUB (p), declarer);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_proc_variable_dec: execute PROC variable declaration.                   */

static void
genie_proc_variable_dec (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case DEFINING_IDENTIFIER:
	  {
	    ADDR_T sp_for_voiding = stack_pointer;
	    MOID_T *ref_mode = MOID (p);
	    TAG_T *tag = TAX (p);
	    int loc_or_heap = (HEAP (tag) == LOC_SYMBOL) ? LOC_SYMBOL : HEAP_SYMBOL;
	    A68_REF *z = (A68_REF *) (FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset));
	    genie_generator_internal (p, ref_mode, tag->body, loc_or_heap, A_TRUE);
	    POP_REF (p, z);
	    if (NEXT (p) != NULL && WHETHER (NEXT (p), ASSIGN_SYMBOL))
	      {
		MOID_T *source_moid = SUB (MOID (p));
		int size = MOID_SIZE (source_moid);
		EXECUTE_UNIT (NEXT (NEXT (p)));
		DECREMENT_STACK_POINTER (p, size);
		MOVE (ADDRESS (z), STACK_TOP, (unsigned) size);
		z->status |= ASSIGNED_MASK;
	      }
	    stack_pointer = sp_for_voiding;	/* Voiding */
	    return;
	  }
	default:
	  {
	    genie_proc_variable_dec (SUB (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_declaration: execute declaration.                                       */

void
genie_declaration (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case MODE_DECLARATION:
	case PROCEDURE_DECLARATION:
	case BRIEF_OPERATOR_DECLARATION:
	case OPERATOR_DECLARATION:
	case PRIORITY_DECLARATION:
	  {
	    return;		/* Already resolved. */
	  }
	case IDENTITY_DECLARATION:
	  {
	    genie_identity_dec (SUB (p));
	    break;
	  }
	case VARIABLE_DECLARATION:
	  {
	    NODE_T *declarer = NULL;
	    ADDR_T sp_for_voiding = stack_pointer;
	    genie_variable_dec (SUB (p), &declarer);
	    stack_pointer = sp_for_voiding;	/* Voiding to remove garbage from declarers. */
	    break;
	  }
	case PROCEDURE_VARIABLE_DECLARATION:
	  {
	    ADDR_T sp_for_voiding = stack_pointer;
	    genie_proc_variable_dec (SUB (p));
	    stack_pointer = sp_for_voiding;
	    break;
	  }
	default:
	  {
	    genie_declaration (SUB (p));
	    break;
	  }
	}
    }
}
