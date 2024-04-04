//! @file genie-coerce.c
//! @author J. Marcel van der Veer
//!
//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].
//!
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
//! Interpreter mode coercion routines.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"
#include "a68g-mp.h"
#include "a68g-double.h"
#include "a68g-parser.h"
#include "a68g-transput.h"

//! @brief Push result of cast (coercions are deeper in the tree).

PROP_T genie_cast (NODE_T * p)
{
  GENIE_UNIT (NEXT_SUB (p));
  PROP_T self;
  UNIT (&self) = genie_cast;
  SOURCE (&self) = p;
  return self;
}

//! @brief Unite value in the stack and push result.

PROP_T genie_uniting (NODE_T * p)
{
  ADDR_T sp = A68_SP;
  MOID_T *u = MOID (p), *v = MOID (SUB (p));
  int size = SIZE (u);
  if (ATTRIBUTE (v) != UNION_SYMBOL) {
    MOID_T *w = unites_to (v, u);
    PUSH_UNION (p, (void *) w);
    GENIE_UNIT (SUB (p));
    STACK_DNS (p, SUB (v), A68_FP);
  } else {
    A68_UNION *m = (A68_UNION *) STACK_TOP;
    GENIE_UNIT (SUB (p));
    STACK_DNS (p, SUB (v), A68_FP);
    VALUE (m) = (void *) unites_to ((MOID_T *) VALUE (m), u);
    if (!IS (u, ROWS_SYMBOL) && VALUE (m) == NO_MOID) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_EMPTY_VALUE_FROM, v);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
  }
  A68_SP = sp + size;
  PROP_T self;
  UNIT (&self) = genie_uniting;
  SOURCE (&self) = p;
  return self;
}

//! @brief Store widened constant as a constant.

void make_constant_widening (NODE_T * p, MOID_T * m, PROP_T * self)
{
  if (SUB (p) != NO_NODE && CONSTANT (GINFO (SUB (p))) != NO_CONSTANT) {
    int size = SIZE (m);
    UNIT (self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) size);
    SIZE (GINFO (p)) = size;
    COPY (CONSTANT (GINFO (p)), (void *) (STACK_OFFSET (-size)), size);
  }
}

//! @brief (optimised) push INT widened to REAL

PROP_T genie_widen_int_to_real (NODE_T * p)
{
  A68_INT *i = (A68_INT *) STACK_TOP;
  A68_REAL *z = (A68_REAL *) STACK_TOP;
  GENIE_UNIT (SUB (p));
  INCREMENT_STACK_POINTER (p, SIZE_ALIGNED (A68_REAL) - SIZE (M_INT));
  VALUE (z) = (REAL_T) VALUE (i);
  STATUS (z) = INIT_MASK;
  return GPROP (p);
}

//! @brief Widen value in the stack.

PROP_T genie_widen (NODE_T * p)
{
#define COERCE_FROM_TO(p, a, b) (MOID (p) == (b) && MOID (SUB (p)) == (a))
  PROP_T self;
  UNIT (&self) = genie_widen;
  SOURCE (&self) = p;
// INT widenings.
  if (COERCE_FROM_TO (p, M_INT, M_REAL)) {
    (void) genie_widen_int_to_real (p);
    UNIT (&self) = genie_widen_int_to_real;
    make_constant_widening (p, M_REAL, &self);
  } else if (COERCE_FROM_TO (p, M_INT, M_LONG_INT)) {
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_int_to_double_int (p);
#else
    genie_lengthen_int_to_mp (p);
#endif
    make_constant_widening (p, M_LONG_INT, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_INT, M_LONG_LONG_INT)) {
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_double_int_to_mp (p);
#else
    genie_lengthen_mp_to_long_mp (p);
#endif
    make_constant_widening (p, M_LONG_LONG_INT, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_INT, M_LONG_REAL)) {
#if (A68_LEVEL >= 3)
    (void) genie_widen_double_int_to_double (p);
#else
// 1-1 mapping.
    GENIE_UNIT (SUB (p));
#endif
    make_constant_widening (p, M_LONG_REAL, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_LONG_INT, M_LONG_LONG_REAL)) {
    GENIE_UNIT (SUB (p));
// 1-1 mapping.
    make_constant_widening (p, M_LONG_LONG_REAL, &self);
  }
// REAL widenings.
  else if (COERCE_FROM_TO (p, M_REAL, M_LONG_REAL)) {
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_real_to_double (p);
#else
    genie_lengthen_real_to_mp (p);
#endif
    make_constant_widening (p, M_LONG_REAL, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_REAL, M_LONG_LONG_REAL)) {
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_double_to_mp (p);
#else
    genie_lengthen_mp_to_long_mp (p);
#endif
    make_constant_widening (p, M_LONG_LONG_REAL, &self);
  } else if (COERCE_FROM_TO (p, M_REAL, M_COMPLEX)) {
    GENIE_UNIT (SUB (p));
    PUSH_VALUE (p, 0.0, A68_REAL);
    make_constant_widening (p, M_COMPLEX, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_REAL, M_LONG_COMPLEX)) {
#if (A68_LEVEL >= 3)
    DOUBLE_NUM_T z;
    z.f = 0.0q;
    GENIE_UNIT (SUB (p));
    PUSH_VALUE (p, z, A68_LONG_REAL);
#else
    GENIE_UNIT (SUB (p));
    (void) nil_mp (p, DIGITS (M_LONG_REAL));
    make_constant_widening (p, M_LONG_COMPLEX, &self);
#endif
  } else if (COERCE_FROM_TO (p, M_LONG_LONG_REAL, M_LONG_LONG_COMPLEX)) {
    GENIE_UNIT (SUB (p));
    (void) nil_mp (p, DIGITS (M_LONG_LONG_REAL));
    make_constant_widening (p, M_LONG_LONG_COMPLEX, &self);
  } else if (COERCE_FROM_TO (p, M_COMPLEX, M_LONG_COMPLEX)) {
// COMPLEX widenings.
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_complex_to_double_compl (p);
#else
    genie_lengthen_complex_to_mp_complex (p);
#endif
    make_constant_widening (p, M_LONG_COMPLEX, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_COMPLEX, M_LONG_LONG_COMPLEX)) {
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_double_compl_to_long_mp_complex (p);
#else
    genie_lengthen_mp_complex_to_long_mp_complex (p);
#endif
    make_constant_widening (p, M_LONG_LONG_COMPLEX, &self);
  } else if (COERCE_FROM_TO (p, M_BITS, M_LONG_BITS)) {
// BITS widenings.
    GENIE_UNIT (SUB (p));
#if (A68_LEVEL >= 3)
    genie_lengthen_bits_to_double_bits (p);
#else
    genie_lengthen_int_to_mp (p);
#endif
    make_constant_widening (p, M_LONG_BITS, &self);
  } else if (COERCE_FROM_TO (p, M_LONG_BITS, M_LONG_LONG_BITS)) {
#if (A68_LEVEL >= 3)
    ABEND (A68_TRUE, ERROR_INTERNAL_CONSISTENCY, __func__);
#else
    GENIE_UNIT (SUB (p));
    genie_lengthen_mp_to_long_mp (p);
    make_constant_widening (p, M_LONG_LONG_BITS, &self);
#endif
  } else if (COERCE_FROM_TO (p, M_BITS, M_ROW_BOOL) || COERCE_FROM_TO (p, M_BITS, M_FLEX_ROW_BOOL)) {
    GENIE_UNIT (SUB (p));
    A68_BITS x;
    POP_OBJECT (p, &x, A68_BITS);
    A68_REF z, row; A68_ARRAY arr; A68_TUPLE tup;
    NEW_ROW_1D (z, row, arr, tup, M_ROW_BOOL, M_BOOL, BITS_WIDTH);
    BYTE_T *base = ADDRESS (&row) + SIZE (M_BOOL) * (BITS_WIDTH - 1);
    UNSIGNED_T bit = 1;
    for (int k = BITS_WIDTH - 1; k >= 0; k--, base -= SIZE (M_BOOL), bit <<= 1) {
      STATUS ((A68_BOOL *) base) = INIT_MASK;
      VALUE ((A68_BOOL *) base) = (BOOL_T) ((VALUE (&x) & bit) != 0 ? A68_TRUE : A68_FALSE);
    }
    PUSH_REF (p, z);
  } else if (COERCE_FROM_TO (p, M_LONG_BITS, M_ROW_BOOL) || COERCE_FROM_TO (p, M_LONG_BITS, M_FLEX_ROW_BOOL)) {
#if (A68_LEVEL >= 3)
    GENIE_UNIT (SUB (p));
    A68_LONG_BITS x;
    POP_OBJECT (p, &x, A68_LONG_BITS);
    A68_REF z, row; A68_ARRAY arr; A68_TUPLE tup;
    NEW_ROW_1D (z, row, arr, tup, M_ROW_BOOL, M_BOOL, LONG_BITS_WIDTH);
    BYTE_T *base = ADDRESS (&row) + SIZE (M_BOOL) * (LONG_BITS_WIDTH - 1);
    UNSIGNED_T bit = 1;
    for (int k = BITS_WIDTH - 1; k >= 0; k--, base -= SIZE (M_BOOL), bit <<= 1) {
      STATUS ((A68_BOOL *) base) = INIT_MASK;
      VALUE ((A68_BOOL *) base) = (BOOL_T) ((LW (VALUE (&x)) & bit) != 0 ? A68_TRUE : A68_FALSE);
    }
    bit = 1;
    for (int k = BITS_WIDTH - 1; k >= 0; k--, base -= SIZE (M_BOOL), bit <<= 1) {
      STATUS ((A68_BOOL *) base) = INIT_MASK;
      VALUE ((A68_BOOL *) base) = (BOOL_T) ((HW (VALUE (&x)) & bit) != 0 ? A68_TRUE : A68_FALSE);
    }
    PUSH_REF (p, z);
#else
    GENIE_UNIT (SUB (p));
    genie_lengthen_long_bits_to_row_bool (p);
#endif
  } else if (COERCE_FROM_TO (p, M_LONG_LONG_BITS, M_ROW_BOOL) || COERCE_FROM_TO (p, M_LONG_LONG_BITS, M_FLEX_ROW_BOOL)) {
#if (A68_LEVEL <= 2)
    GENIE_UNIT (SUB (p));
    genie_lengthen_long_bits_to_row_bool (p);
#endif
  } else if (COERCE_FROM_TO (p, M_BYTES, M_ROW_CHAR) || COERCE_FROM_TO (p, M_BYTES, M_FLEX_ROW_CHAR)) {
    GENIE_UNIT (SUB (p));
    A68_BYTES z;
    POP_OBJECT (p, &z, A68_BYTES);
    PUSH_REF (p, c_string_to_row_char (p, VALUE (&z), BYTES_WIDTH));
  } else if (COERCE_FROM_TO (p, M_LONG_BYTES, M_ROW_CHAR) || COERCE_FROM_TO (p, M_LONG_BYTES, M_FLEX_ROW_CHAR)) {
    GENIE_UNIT (SUB (p));
    A68_LONG_BYTES z;
    POP_OBJECT (p, &z, A68_LONG_BYTES);
    PUSH_REF (p, c_string_to_row_char (p, VALUE (&z), LONG_BYTES_WIDTH));
  } else {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_CANNOT_WIDEN, MOID (SUB (p)), MOID (p));
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  return self;
#undef COERCE_FROM_TO
}

//! @brief Cast a jump to a PROC VOID without executing the jump.

void genie_proceduring (NODE_T * p)
{
  NODE_T *jump = SUB (p);
  NODE_T *q = SUB (jump);
  NODE_T *label = (IS (q, GOTO_SYMBOL) ? NEXT (q) : q);
  A68_PROCEDURE z;
  STATUS (&z) = INIT_MASK;
  NODE (&(BODY (&z))) = jump;
  STATIC_LINK_FOR_FRAME (ENVIRON (&z), 1 + TAG_LEX_LEVEL (TAX (label)));
  LOCALE (&z) = NO_HANDLE;
  MOID (&z) = M_PROC_VOID;
  PUSH_PROCEDURE (p, z);
}

//! @brief (optimised) dereference value of a unit

PROP_T genie_dereferencing_quick (NODE_T * p)
{
  BYTE_T *stack_top = STACK_TOP;
  A68_REF *z = (A68_REF *) stack_top;
  ADDR_T pop_sp = A68_SP;
  GENIE_UNIT (SUB (p));
  A68_SP = pop_sp;
  CHECK_REF (p, *z, MOID (SUB (p)));
  PUSH (p, ADDRESS (z), SIZE (MOID (p)));
  genie_check_initialisation (p, stack_top, MOID (p));
  return GPROP (p);
}

//! @brief Dereference an identifier.

PROP_T genie_dereference_frame_identifier (NODE_T * p)
{
  MOID_T *deref = SUB_MOID (p);
  BYTE_T *stack_top = STACK_TOP;
  A68_REF *z;
  FRAME_GET (z, A68_REF, p);
  PUSH (p, ADDRESS (z), SIZE (deref));
  genie_check_initialisation (p, stack_top, deref);
  return GPROP (p);
}

//! @brief Dereference an identifier.

PROP_T genie_dereference_generic_identifier (NODE_T * p)
{
  MOID_T *deref = SUB_MOID (p);
  BYTE_T *stack_top = STACK_TOP;
  A68_REF *z;
  FRAME_GET (z, A68_REF, p);
  CHECK_REF (p, *z, MOID (SUB (p)));
  PUSH (p, ADDRESS (z), SIZE (deref));
  genie_check_initialisation (p, stack_top, deref);
  return GPROP (p);
}

//! @brief Slice REF [] A to A.

PROP_T genie_dereference_slice_name_quick (NODE_T * p)
{
  MOID_T *ref_m = MOID (p); MOID_T *deref_m = SUB (ref_m);
  ADDR_T pop_sp = A68_SP;
// Get REF [] and [].
  BYTE_T *stack_top = STACK_TOP;
  A68_REF *z = (A68_REF *) stack_top;
  GENIE_UNIT (SUB (p));
  CHECK_REF (p, *z, ref_m);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, DEREF (A68_ROW, z));
// Compute index.
  A68_SP = pop_sp;
  int index = 0;
  for (NODE_T *q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
    A68_INT *j = (A68_INT *) STACK_TOP;
    GENIE_UNIT (q);
    int k = VALUE (j);
    if (k < LWB (tup) || k > UPB (tup)) {
      diagnostic (A68_RUNTIME_ERROR, q, ERROR_INDEX_OUT_OF_BOUNDS);
      exit_genie (q, A68_RUNTIME_ERROR);
    }
    index += (SPAN (tup) * k - SHIFT (tup));
    tup++;
    A68_SP = pop_sp;
  }
// Push element.
  PUSH (p, &((ADDRESS (&(ARRAY (arr))))[ROW_ELEMENT (arr, index)]), SIZE (deref_m));
  genie_check_initialisation (p, stack_top, deref_m);
  return GPROP (p);
}

//! @brief Dereference SELECTION from a name.

PROP_T genie_dereference_selection_name_quick (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  MOID_T *struct_m = MOID (NEXT (selector));
  MOID_T *result_m = SUB_MOID (selector);
  BYTE_T *stack_top = STACK_TOP;
  A68_REF *z = (A68_REF *) stack_top;
  ADDR_T pop_sp = A68_SP;
  GENIE_UNIT (NEXT (selector));
  CHECK_REF (selector, *z, struct_m);
  OFFSET (z) += OFFSET (NODE_PACK (SUB (selector)));
  A68_SP = pop_sp;
  PUSH (p, ADDRESS (z), SIZE (result_m));
  genie_check_initialisation (p, stack_top, result_m);
  return GPROP (p);
}

//! @brief Dereference name in the stack.

PROP_T genie_dereferencing (NODE_T * p)
{
  PROP_T self;
  GENIE_UNIT_2 (SUB (p), self);
  A68_REF z;
  POP_REF (p, &z);
  CHECK_REF (p, z, MOID (SUB (p)));
  PUSH (p, ADDRESS (&z), SIZE (MOID (p)));
  genie_check_initialisation (p, STACK_OFFSET (-SIZE (MOID (p))), MOID (p));
  if (UNIT (&self) == genie_frame_identifier) {
    if (IS_IN_FRAME (&z)) {
      UNIT (&self) = genie_dereference_frame_identifier;
    } else {
      UNIT (&self) = genie_dereference_generic_identifier;
    }
    UNIT (&PROP (GINFO (SOURCE (&self)))) = UNIT (&self);
  } else if (UNIT (&self) == genie_slice_name_quick) {
    UNIT (&self) = genie_dereference_slice_name_quick;
    UNIT (&PROP (GINFO (SOURCE (&self)))) = UNIT (&self);
  } else if (UNIT (&self) == genie_selection_name_quick) {
    UNIT (&self) = genie_dereference_selection_name_quick;
    UNIT (&PROP (GINFO (SOURCE (&self)))) = UNIT (&self);
  } else {
    UNIT (&self) = genie_dereferencing_quick;
    SOURCE (&self) = p;
  }
  return self;
}

//! @brief Deprocedure PROC in the stack.

PROP_T genie_deproceduring (NODE_T * p)
{
  NODE_T *proc = SUB (p);
  MOID_T *proc_m = MOID (proc);
  PROP_T self;
  UNIT (&self) = genie_deproceduring;
  SOURCE (&self) = p;
// Get procedure.
  ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
  A68_PROCEDURE *z = (A68_PROCEDURE *) STACK_TOP;
  GENIE_UNIT (proc);
  A68_SP = pop_sp;
  genie_check_initialisation (p, (BYTE_T *) z, proc_m);
  genie_call_procedure (p, proc_m, proc_m, M_VOID, z, pop_sp, pop_fp);
  STACK_DNS (p, MOID (p), A68_FP);
  return self;
}

//! @brief Voiden value in the stack.

PROP_T genie_voiding (NODE_T * p)
{
  PROP_T self, source;
  ADDR_T sp_for_voiding = A68_SP;
  SOURCE (&self) = p;
  GENIE_UNIT_2 (SUB (p), source);
  A68_SP = sp_for_voiding;
  if (UNIT (&source) == genie_assignation_quick) {
    UNIT (&self) = genie_voiding_assignation;
    SOURCE (&self) = SOURCE (&source);
  } else if (UNIT (&source) == genie_assignation_constant) {
    UNIT (&self) = genie_voiding_assignation_constant;
    SOURCE (&self) = SOURCE (&source);
  } else {
    UNIT (&self) = genie_voiding;
  }
  return self;
}

//! @brief Coerce value in the stack.

PROP_T genie_coercion (NODE_T * p)
{
  PROP_T self;
  UNIT (&self) = genie_coercion;
  SOURCE (&self) = p;
  switch (ATTRIBUTE (p)) {
  case VOIDING: {
      self = genie_voiding (p);
      break;
    }
  case UNITING: {
      self = genie_uniting (p);
      break;
    }
  case WIDENING: {
      self = genie_widen (p);
      break;
    }
  case ROWING: {
      self = genie_rowing (p);
      break;
    }
  case DEREFERENCING: {
      self = genie_dereferencing (p);
      break;
    }
  case DEPROCEDURING: {
      self = genie_deproceduring (p);
      break;
    }
  case PROCEDURING: {
      genie_proceduring (p);
      break;
    }
  }
  GPROP (p) = self;
  return self;
}
