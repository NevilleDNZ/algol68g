//! @file genie-denotation.c
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].

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
//! Interpreter routines for denotations.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"
#include "a68g-mp.h"
#include "a68g-transput.h"

//! @brief Push routine text.

PROP_T genie_routine_text (NODE_T * p)
{
  static PROP_T self;
  A68_PROCEDURE z = *(A68_PROCEDURE *) (FRAME_OBJECT (OFFSET (TAX (p))));
  PUSH_PROCEDURE (p, z);
  UNIT (&self) = genie_routine_text;
  SOURCE (&self) = p;
  return self;
}

//! @brief Push format text.

PROP_T genie_format_text (NODE_T * p)
{
  static PROP_T self;
  A68_FORMAT z = *(A68_FORMAT *) (FRAME_OBJECT (OFFSET (TAX (p))));
  PUSH_FORMAT (p, z);
  UNIT (&self) = genie_format_text;
  SOURCE (&self) = p;
  return self;
}

//! @brief Push NIL.

PROP_T genie_nihil (NODE_T * p)
{
  PROP_T self;
  PUSH_REF (p, nil_ref);
  UNIT (&self) = genie_nihil;
  SOURCE (&self) = p;
  return self;
}

//! @brief Push constant stored in the tree.

PROP_T genie_constant (NODE_T * p)
{
  PUSH (p, CONSTANT (GINFO (p)), SIZE (GINFO (p)));
  return GPROP (p);
}

//! @brief Push value of denotation.

PROP_T genie_denotation (NODE_T * p)
{
  MOID_T *moid = MOID (p);
  PROP_T self;
  UNIT (&self) = genie_denotation;
  SOURCE (&self) = p;
  if (moid == M_INT) {
// INT denotation.
    A68_INT z;
    NODE_T *s = IS (SUB (p), SHORTETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    UNIT (&self) = genie_constant;
    STATUS (&z) = INIT_MASK;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) SIZE (M_INT));
    SIZE (GINFO (p)) = SIZE (M_INT);
    COPY (CONSTANT (GINFO (p)), &z, SIZE (M_INT));
    PUSH_VALUE (p, VALUE ((A68_INT *) (CONSTANT (GINFO (p)))), A68_INT);
    return self;
  }
  if (moid == M_REAL) {
// REAL denotation.
    A68_REAL z;
    NODE_T *s = IS (SUB (p), SHORTETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    STATUS (&z) = INIT_MASK;
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) SIZE_ALIGNED (A68_REAL));
    SIZE (GINFO (p)) = SIZE_ALIGNED (A68_REAL);
    COPY (CONSTANT (GINFO (p)), &z, SIZE_ALIGNED (A68_REAL));
    PUSH_VALUE (p, VALUE ((A68_REAL *) (CONSTANT (GINFO (p)))), A68_REAL);
    return self;
  }
#if (A68_LEVEL >= 3)
  if (moid == M_LONG_INT) {
// LONG INT denotation.
    A68_LONG_INT z;
    size_t len = (size_t) SIZE_ALIGNED (A68_LONG_INT);
    NODE_T *s = IS (SUB (p), LONGETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    UNIT (&self) = genie_constant;
    STATUS (&z) = INIT_MASK;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) len);
    SIZE (GINFO (p)) = len;
    COPY (CONSTANT (GINFO (p)), &z, len);
    PUSH_VALUE (p, VALUE ((A68_LONG_INT *) (CONSTANT (GINFO (p)))), A68_LONG_INT);
    return self;
  }
  if (moid == M_LONG_REAL) {
// LONG REAL denotation.
    A68_LONG_REAL z;
    NODE_T *s = IS (SUB (p), LONGETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    STATUS (&z) = INIT_MASK;
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) SIZE_ALIGNED (A68_LONG_REAL));
    SIZE (GINFO (p)) = SIZE_ALIGNED (A68_LONG_REAL);
    COPY (CONSTANT (GINFO (p)), &z, SIZE_ALIGNED (A68_LONG_REAL));
    PUSH_VALUE (p, VALUE ((A68_LONG_REAL *) (CONSTANT (GINFO (p)))), A68_LONG_REAL);
    return self;
  }
// LONG BITS denotation.
  if (moid == M_LONG_BITS) {
    A68_LONG_BITS z;
    NODE_T *s = IS (SUB (p), LONGETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    UNIT (&self) = genie_constant;
    STATUS (&z) = INIT_MASK;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) SIZE_ALIGNED (A68_LONG_BITS));
    SIZE (GINFO (p)) = SIZE_ALIGNED (A68_LONG_BITS);
    COPY (CONSTANT (GINFO (p)), &z, SIZE_ALIGNED (A68_LONG_BITS));
    PUSH_VALUE (p, VALUE ((A68_LONG_BITS *) (CONSTANT (GINFO (p)))), A68_LONG_BITS);
    return self;
  }
#endif
  if (moid == M_LONG_INT || moid == M_LONG_LONG_INT) {
// [LONG] LONG INT denotation.
    int digits = DIGITS (moid);
    int size = SIZE (moid);
    NODE_T *number;
    if (IS (SUB (p), SHORTETY) || IS (SUB (p), LONGETY)) {
      number = NEXT_SUB (p);
    } else {
      number = SUB (p);
    }
    MP_T *z = nil_mp (p, digits);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (number), (BYTE_T *) z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    MP_STATUS (z) = (MP_T) INIT_MASK;
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) size);
    SIZE (GINFO (p)) = size;
    COPY (CONSTANT (GINFO (p)), z, size);
    return self;
  }
  if (moid == M_LONG_REAL || moid == M_LONG_LONG_REAL) {
// [LONG] LONG REAL denotation.
    int digits = DIGITS (moid);
    int size = SIZE (moid);
    NODE_T *number;
    if (IS (SUB (p), SHORTETY) || IS (SUB (p), LONGETY)) {
      number = NEXT_SUB (p);
    } else {
      number = SUB (p);
    }
    MP_T *z = nil_mp (p, digits);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (number), (BYTE_T *) z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    MP_STATUS (z) = (MP_T) INIT_MASK;
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) size);
    SIZE (GINFO (p)) = size;
    COPY (CONSTANT (GINFO (p)), z, size);
    return self;
  }
  if (moid == M_BITS) {
// BITS denotation.
    A68_BITS z;
    NODE_T *s = IS (SUB (p), SHORTETY) ? NEXT_SUB (p) : SUB (p);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (s), (BYTE_T *) & z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    UNIT (&self) = genie_constant;
    STATUS (&z) = INIT_MASK;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) SIZE_ALIGNED (A68_BITS));
    SIZE (GINFO (p)) = SIZE_ALIGNED (A68_BITS);
    COPY (CONSTANT (GINFO (p)), &z, SIZE_ALIGNED (A68_BITS));
    PUSH_VALUE (p, VALUE ((A68_BITS *) (CONSTANT (GINFO (p)))), A68_BITS);
  }
  if (moid == M_LONG_BITS || moid == M_LONG_LONG_BITS) {
// [LONG] LONG BITS denotation.
    int digits = DIGITS (moid);
    int size = SIZE (moid);
    NODE_T *number;
    if (IS (SUB (p), SHORTETY) || IS (SUB (p), LONGETY)) {
      number = NEXT_SUB (p);
    } else {
      number = SUB (p);
    }
    MP_T *z = nil_mp (p, digits);
    if (genie_string_to_value_internal (p, moid, NSYMBOL (number), (BYTE_T *) z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTATION, moid);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    MP_STATUS (z) = (MP_T) INIT_MASK;
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) size);
    SIZE (GINFO (p)) = size;
    COPY (CONSTANT (GINFO (p)), z, size);
    return self;
  }
  if (moid == M_BOOL) {
// BOOL denotation.
    A68_BOOL z;
    ASSERT (genie_string_to_value_internal (p, M_BOOL, NSYMBOL (p), (BYTE_T *) & z) == A68_TRUE);
    PUSH_VALUE (p, VALUE (&z), A68_BOOL);
    return self;
  } else if (moid == M_CHAR) {
// CHAR denotation.
    PUSH_VALUE (p, TO_UCHAR (NSYMBOL (p)[0]), A68_CHAR);
    return self;
  } else if (moid == M_ROW_CHAR) {
// [] CHAR denotation - permanent string in the heap.
    A68_REF z;
    A68_ARRAY *arr;
    A68_TUPLE *tup;
    z = c_to_a_string (p, NSYMBOL (p), DEFAULT_WIDTH);
    GET_DESCRIPTOR (arr, tup, &z);
    BLOCK_GC_HANDLE (&z);
    BLOCK_GC_HANDLE (&(ARRAY (arr)));
    UNIT (&self) = genie_constant;
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) A68_REF_SIZE);
    SIZE (GINFO (p)) = A68_REF_SIZE;
    COPY (CONSTANT (GINFO (p)), &z, A68_REF_SIZE);
    PUSH_REF (p, *(A68_REF *) (CONSTANT (GINFO (p))));
    (void) tup;
    return self;
  }
  if (moid == M_VOID) {
// VOID denotation: EMPTY.
    return self;
  }
  return self;
}
