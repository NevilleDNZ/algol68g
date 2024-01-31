//! @file genie-stowed.c
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
//! Interpreter routines for STOWED values.

// An A68G row is a reference to a descriptor in the heap:
// A68_REF row -> A68_ARRAY ----+   ARRAY: Description of row, ref to elements.
//                A68_TUPLE 1   |   TUPLE: Bounds, one for every dimension.
//                ...           |
//                A68_TUPLE dim |
//                ...           |
//                ...           |
//                Element 1 <---+   Sequential row elements in the heap.
//                ...
//                Element n

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"

//! @brief Construct a descriptor "ref_new" for a trim of "ref_old".

void genie_trimmer (NODE_T * p, BYTE_T * *ref_new, BYTE_T * *ref_old, INT_T * offset)
{
  if (p != NO_NODE) {
    if (IS (p, UNIT)) {
      GENIE_UNIT_NO_GC (p);
      A68_INT k;
      POP_OBJECT (p, &k, A68_INT);
      A68_TUPLE *tup = (A68_TUPLE *) * ref_old;
      CHECK_INDEX (p, &k, tup);
      (*offset) += SPAN (tup) * VALUE (&k) - SHIFT (tup);
      (*ref_old) += sizeof (A68_TUPLE);
    } else if (IS (p, TRIMMER)) {
      A68_TUPLE *old_tup = (A68_TUPLE *) * ref_old;
      A68_TUPLE *new_tup = (A68_TUPLE *) * ref_new;
// TRIMMER is (l:u@r) with all units optional or (empty).
      INT_T L, U, D;
      NODE_T *q = SUB (p);
      if (q == NO_NODE) {
        L = LWB (old_tup);
        U = UPB (old_tup);
        D = 0;
      } else {
        BOOL_T absent = A68_TRUE;
// Lower index.
        if (q != NO_NODE && IS (q, UNIT)) {
          GENIE_UNIT_NO_GC (q);
          A68_INT k;
          POP_OBJECT (p, &k, A68_INT);
          if (VALUE (&k) < LWB (old_tup)) {
            diagnostic (A68_RUNTIME_ERROR, p, ERROR_INDEX_OUT_OF_BOUNDS);
            exit_genie (p, A68_RUNTIME_ERROR);
          }
          L = VALUE (&k);
          FORWARD (q);
          absent = A68_FALSE;
        } else {
          L = LWB (old_tup);
        }
        if (q != NO_NODE && (IS (q, COLON_SYMBOL) || IS (q, DOTDOT_SYMBOL))) {
          FORWARD (q);
          absent = A68_FALSE;
        }
// Upper index.
        if (q != NO_NODE && IS (q, UNIT)) {
          GENIE_UNIT_NO_GC (q);
          A68_INT k;
          POP_OBJECT (p, &k, A68_INT);
          if (VALUE (&k) > UPB (old_tup)) {
            diagnostic (A68_RUNTIME_ERROR, p, ERROR_INDEX_OUT_OF_BOUNDS);
            exit_genie (p, A68_RUNTIME_ERROR);
          }
          U = VALUE (&k);
          FORWARD (q);
          absent = A68_FALSE;
        } else {
          U = UPB (old_tup);
        }
        if (q != NO_NODE && IS (q, AT_SYMBOL)) {
          FORWARD (q);
        }
// Revised lower bound.
        if (q != NO_NODE && IS (q, UNIT)) {
          GENIE_UNIT_NO_GC (q);
          A68_INT k;
          POP_OBJECT (p, &k, A68_INT);
          D = L - VALUE (&k);
          FORWARD (q);
        } else {
          D = (absent ? 0 : L - 1);
        }
      }
      LWB (new_tup) = L - D;
      UPB (new_tup) = U - D;    // (L - D) + (U - L)
      SPAN (new_tup) = SPAN (old_tup);
      SHIFT (new_tup) = SHIFT (old_tup) - D * SPAN (new_tup);
      (*ref_old) += sizeof (A68_TUPLE);
      (*ref_new) += sizeof (A68_TUPLE);
    } else {
      genie_trimmer (SUB (p), ref_new, ref_old, offset);
      genie_trimmer (NEXT (p), ref_new, ref_old, offset);
    }
  }
}

//! @brief Calculation of subscript.

void genie_subscript (NODE_T * p, A68_TUPLE ** tup, INT_T * sum, NODE_T ** seq)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case UNIT: {
        GENIE_UNIT_NO_GC (p);
        A68_INT *k;
        POP_ADDRESS (p, k, A68_INT);
        CHECK_INDEX (p, k, *tup);
        (*sum) += (SPAN (*tup) * VALUE (k) - SHIFT (*tup));
        (*tup)++;
        SEQUENCE (*seq) = p;
        (*seq) = p;
        return;
      }
    case GENERIC_ARGUMENT:
    case GENERIC_ARGUMENT_LIST: {
        genie_subscript (SUB (p), tup, sum, seq);
      }
    }
  }
}

//! @brief Slice REF [] A to REF A.

PROP_T genie_slice_name_quick (NODE_T * p)
{
  A68_REF *z = (A68_REF *) STACK_TOP;
  GENIE_UNIT_NO_GC (SUB (p));
  CHECK_REF (p, *z, MOID (SUB (p)));
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, DEREF (A68_ROW, z));
  ADDR_T pop_sp = A68_SP;
  INT_T index = 0;
  for (NODE_T *q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
    A68_INT *j = (A68_INT *) STACK_TOP;
    GENIE_UNIT_NO_GC (q);
    INT_T k = VALUE (j);
    if (k < LWB (tup) || k > UPB (tup)) {
      diagnostic (A68_RUNTIME_ERROR, q, ERROR_INDEX_OUT_OF_BOUNDS);
      exit_genie (q, A68_RUNTIME_ERROR);
    }
    index += (SPAN (tup) * k - SHIFT (tup));
    tup++;
    A68_SP = pop_sp;
  }
// Leave reference to element on the stack, preserving scope.
  ADDR_T scope = REF_SCOPE (z);
  *z = ARRAY (arr);
  OFFSET (z) += ROW_ELEMENT (arr, index);
  REF_SCOPE (z) = scope;
  return GPROP (p);
}

//! @brief Push slice of a rowed object.

PROP_T genie_slice (NODE_T * p)
{
  BOOL_T slice_name = (BOOL_T) (IS_REF (MOID (SUB (p))));
  MOID_T *m_slice = slice_name ? SUB_MOID (p) : MOID (p);
  PROP_T self;
  UNIT (&self) = genie_slice;
  SOURCE (&self) = p;
  ADDR_T pop_sp = A68_SP;
// Get row.
  PROP_T primary;
  GENIE_UNIT_NO_GC_2 (SUB (p), primary);
  (void) primary;
// In case of slicing a REF [], we need the [] internally, so dereference.
  ADDR_T scope = PRIMAL_SCOPE;
  if (slice_name) {
    A68_REF z;
    POP_REF (p, &z);
    CHECK_REF (p, z, MOID (SUB (p)));
    scope = REF_SCOPE (&z);
    PUSH_REF (p, *DEREF (A68_REF, &z));
  }
  NODE_T *indexer = NEXT_SUB (p);
  if (ANNOTATION (indexer) == SLICE) {
// SLICING subscripts one element from an array.
    A68_REF z;
    POP_REF (p, &z);
    CHECK_REF (p, z, MOID (SUB (p)));
    A68_ARRAY *arr; A68_TUPLE *tup;
    GET_DESCRIPTOR (arr, tup, &z);
    INT_T index = 0;
    if (SEQUENCE (p) == NO_NODE && !STATUS_TEST (p, SEQUENCE_MASK)) {
      NODE_T top_seq;
      GINFO_T g;
      NODE_T *seq = &top_seq;
      GINFO (seq) = &g;
      SEQUENCE (seq) = NO_NODE;
      genie_subscript (indexer, &tup, &index, &seq);
      SEQUENCE (p) = SEQUENCE (&top_seq);
      STATUS_SET (p, SEQUENCE_MASK);
    } else {
      for (NODE_T *q = SEQUENCE (p); q != NO_NODE; tup++, q = SEQUENCE (q)) {
        A68_INT *j = (A68_INT *) STACK_TOP;
        GENIE_UNIT_NO_GC (q);
        INT_T k = VALUE (j);
        if (k < LWB (tup) || k > UPB (tup)) {
          diagnostic (A68_RUNTIME_ERROR, q, ERROR_INDEX_OUT_OF_BOUNDS);
          exit_genie (q, A68_RUNTIME_ERROR);
        }
        index += (SPAN (tup) * k - SHIFT (tup));
      }
    }
// Slice of a name yields a name.
    A68_SP = pop_sp;
    if (slice_name) {
      A68_REF name = ARRAY (arr);
      OFFSET (&name) += ROW_ELEMENT (arr, index);
      REF_SCOPE (&name) = scope;
      PUSH_REF (p, name);
      if (STATUS_TEST (p, SEQUENCE_MASK)) {
        UNIT (&self) = genie_slice_name_quick;
        SOURCE (&self) = p;
      }
    } else {
      BYTE_T *tos = STACK_TOP;
      PUSH (p, &((ADDRESS (&(ARRAY (arr))))[ROW_ELEMENT (arr, index)]), SIZE (m_slice));
      genie_check_initialisation (p, tos, m_slice);
    }
    return self;
  } else if (ANNOTATION (indexer) == TRIMMER) {
// Trimming selects a subarray from an array.
    int dim = DIM (DEFLEX (m_slice));
    A68_REF ref_desc_copy = heap_generator (p, MOID (p), DESCRIPTOR_SIZE (dim));
// Get descriptor.
    A68_REF z;
    POP_REF (p, &z);
// Get indexer.
    CHECK_REF (p, z, MOID (SUB (p)));
    A68_ARRAY *old_des = DEREF (A68_ARRAY, &z), *new_des = DEREF (A68_ARRAY, &ref_desc_copy);
    BYTE_T *ref_old = ADDRESS (&z) + SIZE_ALIGNED (A68_ARRAY);
    BYTE_T *ref_new = ADDRESS (&ref_desc_copy) + SIZE_ALIGNED (A68_ARRAY);
    DIM (new_des) = dim;
    MOID (new_des) = MOID (old_des);
    ELEM_SIZE (new_des) = ELEM_SIZE (old_des);
    INT_T offset = SLICE_OFFSET (old_des);
    genie_trimmer (indexer, &ref_new, &ref_old, &offset);
    SLICE_OFFSET (new_des) = offset;
    FIELD_OFFSET (new_des) = FIELD_OFFSET (old_des);
    ARRAY (new_des) = ARRAY (old_des);
// Trim of a name is a name.
    if (slice_name) {
      A68_REF ref_trim = heap_generator (p, MOID (p), A68_REF_SIZE);
      *DEREF (A68_REF, &ref_trim) = ref_desc_copy;
      REF_SCOPE (&ref_trim) = scope;
      PUSH_REF (p, ref_trim);
    } else {
      PUSH_REF (p, ref_desc_copy);
    }
    return self;
  } else {
    ABEND (A68_TRUE, ERROR_INTERNAL_CONSISTENCY, __func__);
    return self;
  }
}

//! @brief SELECTION from a value

PROP_T genie_selection_value_quick (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  MOID_T *result_mode = MOID (selector);
  ADDR_T pop_sp = A68_SP;
  int size = SIZE (result_mode);
  INT_T offset = OFFSET (NODE_PACK (SUB (selector)));
  GENIE_UNIT_NO_GC (NEXT (selector));
  A68_SP = pop_sp;
  if (offset > 0) {
    MOVE (STACK_TOP, STACK_OFFSET (offset), (unt) size);
    genie_check_initialisation (p, STACK_TOP, result_mode);
  }
  INCREMENT_STACK_POINTER (selector, size);
  return GPROP (p);
}

//! @brief SELECTION from a name

PROP_T genie_selection_name_quick (NODE_T * p)
{
  NODE_T *selector = SUB (p);
  MOID_T *struct_mode = MOID (NEXT (selector));
  A68_REF *z = (A68_REF *) STACK_TOP;
  GENIE_UNIT_NO_GC (NEXT (selector));
  CHECK_REF (selector, *z, struct_mode);
  OFFSET (z) += OFFSET (NODE_PACK (SUB (selector)));
  return GPROP (p);
}

//! @brief Push selection from secondary.

PROP_T genie_selection (NODE_T * p)
{
  NODE_T *select = SUB (p);
  MOID_T *m_str = MOID (NEXT (select)), *m_sel = MOID (select);
  BOOL_T select_name = (BOOL_T) (IS_REF (m_str));
  PROP_T self;
  SOURCE (&self) = p;
  UNIT (&self) = genie_selection;
  GENIE_UNIT_NO_GC (NEXT (select));
// Multiple selections.
  if (select_name && (IS_FLEX (SUB (m_str)) || IS_ROW (SUB (m_str)))) {
    A68_REF *r_src;
    POP_ADDRESS (select, r_src, A68_REF);
    CHECK_REF (p, *r_src, m_str);
    r_src = DEREF (A68_REF, r_src);
    int dim = DIM (DEFLEX (SUB (m_str)));
    int d_size = DESCRIPTOR_SIZE (dim);
    A68_REF r_dst = heap_generator (select, m_sel, d_size);
    MOVE (ADDRESS (&r_dst), DEREF (BYTE_T, r_src), (unt) d_size);
    MOID ((DEREF (A68_ARRAY, &r_dst))) = SUB_SUB (m_sel);
    FIELD_OFFSET (DEREF (A68_ARRAY, &r_dst)) += OFFSET (NODE_PACK (SUB (select)));
    A68_REF r_sel = heap_generator (select, m_sel, A68_REF_SIZE);
    *DEREF (A68_REF, &r_sel) = r_dst;
    PUSH_REF (select, r_sel);
    UNIT (&self) = genie_selection;
  } else if (m_str != NO_MOID && (IS_FLEX (m_str) || IS_ROW (m_str))) {
    A68_REF *r_src;
    POP_ADDRESS (select, r_src, A68_REF);
    int dim = DIM (DEFLEX (m_str));
    int d_size = DESCRIPTOR_SIZE (dim);
    A68_REF r_dst = heap_generator (select, m_sel, d_size);
    MOVE (ADDRESS (&r_dst), DEREF (BYTE_T, r_src), (unt) d_size);
    MOID ((DEREF (A68_ARRAY, &r_dst))) = SUB (m_sel);
    FIELD_OFFSET (DEREF (A68_ARRAY, &r_dst)) += OFFSET (NODE_PACK (SUB (select)));
    PUSH_REF (select, r_dst);
    UNIT (&self) = genie_selection;
  }
// Normal selections.
  else if (select_name && IS_STRUCT (SUB (m_str))) {
    A68_REF *z = (A68_REF *) (STACK_OFFSET (-A68_REF_SIZE));
    CHECK_REF (select, *z, m_str);
    OFFSET (z) += OFFSET (NODE_PACK (SUB (select)));
    UNIT (&self) = genie_selection_name_quick;
  } else if (IS_STRUCT (m_str)) {
    DECREMENT_STACK_POINTER (select, SIZE (m_str));
    MOVE (STACK_TOP, STACK_OFFSET (OFFSET (NODE_PACK (SUB (select)))), (unt) SIZE (m_sel));
    genie_check_initialisation (p, STACK_TOP, m_sel);
    INCREMENT_STACK_POINTER (select, SIZE (m_sel));
    UNIT (&self) = genie_selection_value_quick;
  }
  return self;
}

//! @brief Push selection from primary.

PROP_T genie_field_selection (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
  NODE_T *entry = p;
  A68_REF *z = (A68_REF *) STACK_TOP;
  A68_PROCEDURE *w = (A68_PROCEDURE *) STACK_TOP;
  PROP_T self;
  SOURCE (&self) = entry;
  UNIT (&self) = genie_field_selection;
  GENIE_UNIT_NO_GC (SUB (p));
  for (p = SEQUENCE (SUB (p)); p != NO_NODE; p = SEQUENCE (p)) {
    MOID_T *m = MOID (p);
    MOID_T *m_sel = MOID (NODE_PACK (p));
    BOOL_T coerce = A68_TRUE;
    while (coerce) {
      if (IS_REF (m) && ISNT (SUB (m), STRUCT_SYMBOL)) {
        int size = SIZE (SUB (m));
        A68_SP = pop_sp;
        CHECK_REF (p, *z, m);
        PUSH (p, ADDRESS (z), size);
        genie_check_initialisation (p, STACK_OFFSET (-size), MOID (p));
        m = SUB (m);
      } else if (IS (m, PROC_SYMBOL)) {
        genie_check_initialisation (p, (BYTE_T *) w, m);
        genie_call_procedure (p, m, m, M_VOID, w, pop_sp, pop_fp);
        STACK_DNS (p, MOID (p), A68_FP);
        m = SUB (m);
      } else {
        coerce = A68_FALSE;
      }
    }
    if (IS_REF (m) && IS (SUB (m), STRUCT_SYMBOL)) {
      CHECK_REF (p, *z, m);
      OFFSET (z) += OFFSET (NODE_PACK (p));
    } else if (IS_STRUCT (m)) {
      A68_SP = pop_sp;
      MOVE (STACK_TOP, STACK_OFFSET (OFFSET (NODE_PACK (p))), (unt) SIZE (m_sel));
      INCREMENT_STACK_POINTER (p, SIZE (m_sel));
    }
  }
  return self;
}

