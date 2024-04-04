//! @file rts-stowed.c
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
//! Interpreter routines for STOWED values.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"

// Routines for handling stowed objects.
// 
// An A68G row is a reference to a descriptor in the heap:
// 
//                ...
// A68_REF row -> A68_ARRAY ----+   ARRAY: Description of row, ref to elements.
//                A68_TUPLE 1   |   TUPLE: Bounds, one for every dimension.
//                ...           |
//                A68_TUPLE dim |
//                ...           |
//                ...           |
//                Element 1 <---+   Sequential row elements in the heap.
//                ...
//                Element n

//! @brief Size of a row.

int get_row_size (A68_TUPLE * tup, int dim)
{
  int span = 1;
  for (int k = 0; k < dim; k++) {
    int stride = ROW_SIZE (&tup[k]);
    ABEND ((stride > 0 && span > A68_MAX_INT / stride), ERROR_INVALID_SIZE, __func__);
    span *= stride;
  }
  return span;
}

//! @brief Initialise index for FORALL constructs.

void initialise_internal_index (A68_TUPLE * tup, int dim)
{
  for (int k = 0; k < dim; k++) {
    A68_TUPLE *ref = &tup[k];
    K (ref) = LWB (ref);
  }
}

//! @brief Calculate index.

ADDR_T calculate_internal_index (A68_TUPLE * tup, int dim)
{
  ADDR_T idx = 0;
  for (int k = 0; k < dim; k++) {
    A68_TUPLE *ref = &tup[k];
// Only consider non-empty rows.
    if (ROW_SIZE (ref) > 0) {
      idx += (SPAN (ref) * K (ref) - SHIFT (ref));
    }
  }
  return idx;
}

//! @brief Increment index for FORALL constructs.

BOOL_T increment_internal_index (A68_TUPLE * tup, int dim)
{
  BOOL_T carry = A68_TRUE;
  for (int k = dim - 1; k >= 0 && carry; k--) {
    A68_TUPLE *ref = &tup[k];
    if (K (ref) < UPB (ref)) {
      (K (ref))++;
      carry = A68_FALSE;
    } else {
      K (ref) = LWB (ref);
    }
  }
  return carry;
}

//! @brief Print index.

void print_internal_index (FILE_T f, A68_TUPLE * tup, int dim)
{
  for (int k = 0; k < dim; k++) {
    A68_TUPLE *ref = &tup[k];
    BUFFER buf;
    BUFCLR (buf);
    ASSERT (snprintf (buf, SNPRINTF_SIZE, A68_LD, K (ref)) >= 0);
    WRITE (f, buf);
    if (k < dim - 1) {
      WRITE (f, ", ");
    }
  }
}

//! @brief Convert C string to A68 [] CHAR.

A68_REF c_string_to_row_char (NODE_T * p, char *str, int width)
{
  A68_REF z, row; A68_ARRAY arr; A68_TUPLE tup;
  NEW_ROW_1D (z, row, arr, tup, M_ROW_CHAR, M_CHAR, width);
  BYTE_T *base = ADDRESS (&row);
  int len = strlen (str);
  for (int k = 0; k < width; k++) {
    A68_CHAR *ch = (A68_CHAR *) & (base[k * SIZE_ALIGNED (A68_CHAR)]);
    STATUS (ch) = INIT_MASK;
    VALUE (ch) = (k < len ? TO_UCHAR (str[k]) : NULL_CHAR);
  }
  return z;
}

//! @brief Convert C string to A68 string.

A68_REF c_to_a_string (NODE_T * p, char *str, int width)
{
  if (str == NO_TEXT) {
    return empty_string (p);
  } else {
    if (width == DEFAULT_WIDTH) {
      return c_string_to_row_char (p, str, (int) strlen (str));
    } else {
      return c_string_to_row_char (p, str, (int) width);
    }
  }
}

//! @brief Size of a string.

int a68_string_size (NODE_T * p, A68_REF row)
{
  (void) p;
  if (INITIALISED (&row)) {
    A68_ARRAY *arr; A68_TUPLE *tup;
    GET_DESCRIPTOR (arr, tup, &row);
    return ROW_SIZE (tup);
  } else {
    return 0;
  }
}

//! @brief Convert A68 string to C string.

char *a_to_c_string (NODE_T * p, char *str, A68_REF row)
{
// Assume "str" to be long enough - caller's responsibility!.
  (void) p;
  if (INITIALISED (&row)) {
    A68_ARRAY *arr; A68_TUPLE *tup;
    GET_DESCRIPTOR (arr, tup, &row);
    int size = ROW_SIZE (tup), n = 0;
    if (size > 0) {
      BYTE_T *base_address = ADDRESS (&ARRAY (arr));
      for (int k = LWB (tup); k <= UPB (tup); k++) {
        int addr = INDEX_1_DIM (arr, tup, k);
        A68_CHAR *ch = (A68_CHAR *) & (base_address[addr]);
        CHECK_INIT (p, INITIALISED (ch), M_CHAR);
        str[n++] = (char) VALUE (ch);
      }
    }
    str[n] = NULL_CHAR;
    return str;
  } else {
    return NO_TEXT;
  }
}

//! @brief Return an empty row.

A68_REF empty_row (NODE_T * p, MOID_T * m_row)
{
  if (IS_FLEX (m_row)) {
    m_row = SUB (m_row);
  }
  MOID_T *m_elem = SUB (m_row);
  int dim = DIM (m_row);
  A68_REF dsc; A68_ARRAY *arr; A68_TUPLE *tup;
  dsc = heap_generator (p, m_row, DESCRIPTOR_SIZE (dim));
  GET_DESCRIPTOR (arr, tup, &dsc);
  DIM (arr) = dim;
  MOID (arr) = SLICE (m_row);
  ELEM_SIZE (arr) = moid_size (SLICE (m_row));
  SLICE_OFFSET (arr) = 0;
  FIELD_OFFSET (arr) = 0;
  if (IS_ROW (m_elem) || IS_FLEX (m_elem)) {
// [] AMODE or FLEX [] AMODE 
    ARRAY (arr) = heap_generator (p, m_elem, A68_REF_SIZE);
    *DEREF (A68_REF, &ARRAY (arr)) = empty_row (p, m_elem);
  } else {
    ARRAY (arr) = nil_ref;
  }
  STATUS (&ARRAY (arr)) = (STATUS_MASK_T) (INIT_MASK | IN_HEAP_MASK);
  for (int k = 0; k < dim; k++) {
    LWB (&tup[k]) = 1;
    UPB (&tup[k]) = 0;
    SPAN (&tup[k]) = 1;
    SHIFT (&tup[k]) = LWB (tup);
  }
  return dsc;
}

//! @brief An empty string, FLEX [1 : 0] CHAR.

A68_REF empty_string (NODE_T * p)
{
  return empty_row (p, M_STRING);
}

//! @brief Make [,, ..] MODE  from [, ..] MODE.

A68_REF genie_make_rowrow (NODE_T *p, MOID_T * m_row, int len, ADDR_T pop_sp)
{
  MOID_T *m_deflex = IS_FLEX (m_row) ? SUB (m_row) : m_row;
  int old_dim = DIM (m_deflex) - 1;
// Make the new descriptor.
  A68_ARRAY *new_arr; A68_TUPLE *new_tup;
  A68_REF new_row = heap_generator (p, m_row, DESCRIPTOR_SIZE (DIM (m_deflex)));
  GET_DESCRIPTOR (new_arr, new_tup, &new_row);
  DIM (new_arr) = DIM (m_deflex);
  MOID_T *m_elem = SUB (m_deflex);
  MOID (new_arr) = m_elem;
  ELEM_SIZE (new_arr) = SIZE (m_elem);
  SLICE_OFFSET (new_arr) = 0;
  FIELD_OFFSET (new_arr) = 0;
  if (len == 0) {
// There is a vacuum on the stack.
    for (int k = 0; k < old_dim; k++) {
      LWB (&new_tup[k + 1]) = 1;
      UPB (&new_tup[k + 1]) = 0;
      SPAN (&new_tup[k + 1]) = 1;
      SHIFT (&new_tup[k + 1]) = LWB (&new_tup[k + 1]);
    }
    LWB (new_tup) = 1;
    UPB (new_tup) = 0;
    SPAN (new_tup) = 0;
    SHIFT (new_tup) = 0;
    ARRAY (new_arr) = nil_ref;
    return new_row;
  } else if (len > 0) {
    A68_ARRAY *tmp = NO_ARRAY;
// Arrays in the stack must have equal bounds.
    A68_REF row_0 = *(A68_REF *) STACK_ADDRESS (pop_sp);
    A68_TUPLE *tup_0;
    GET_DESCRIPTOR (tmp, tup_0, &row_0);
    for (int j = 1; j < len; j++) {
      A68_REF row_j = *(A68_REF *) STACK_ADDRESS (pop_sp + j * A68_REF_SIZE);
      A68_TUPLE *tup_j;
      GET_DESCRIPTOR (tmp, tup_j, &row_j);
      for (int k = 0; k < old_dim; k++) {
        if ((UPB (&tup_0[k]) != UPB (&tup_j[k])) || (LWB (&tup_0[k]) != LWB (&tup_j[k]))) {
          diagnostic (A68_RUNTIME_ERROR, p, ERROR_DIFFERENT_BOUNDS);
          exit_genie (p, A68_RUNTIME_ERROR);
        }
      }
    }
// Fill descriptor of new row with info from (arbitrary) first one.
    A68_ARRAY *old_arr; A68_TUPLE *old_tup;
    A68_REF old_row = *(A68_REF *) STACK_ADDRESS (pop_sp);
    GET_DESCRIPTOR (tmp, old_tup, &old_row);
    int span = 1;
    for (int k = 0; k < old_dim; k++) {
      A68_TUPLE *tup = &new_tup[k + 1];
      LWB (tup) = LWB (&old_tup[k]);
      UPB (tup) = UPB (&old_tup[k]);
      SPAN (tup) = span;
      SHIFT (tup) = LWB (tup) * SPAN (tup);
      span *= ROW_SIZE (tup);
    }
    LWB (new_tup) = 1;
    UPB (new_tup) = len;
    SPAN (new_tup) = span;
    SHIFT (new_tup) = LWB (new_tup) * SPAN (new_tup);
    ARRAY (new_arr) = heap_generator_2 (p, m_row, len, span * ELEM_SIZE (new_arr));
    for (int j = 0; j < len; j++) {
// Copy new[j,, ] := old[, ].
      GET_DESCRIPTOR (old_arr, old_tup, (A68_REF *) STACK_ADDRESS (pop_sp + j * A68_REF_SIZE));
      if (LWB (old_tup) > UPB (old_tup)) {
        A68_REF dst = ARRAY (new_arr);
        ADDR_T new_k = j * SPAN (new_tup) + calculate_internal_index (&new_tup[1], old_dim);
        OFFSET (&dst) += ROW_ELEMENT (new_arr, new_k);
        A68_REF clone = empty_row (p, SLICE (m_row));
        MOVE (ADDRESS (&dst), ADDRESS (&clone), SIZE (m_elem));
      } else {
        initialise_internal_index (old_tup, old_dim);
        initialise_internal_index (&new_tup[1], old_dim);
        BOOL_T done = A68_FALSE;
        while (!done) {
          A68_REF src = ARRAY (old_arr), dst = ARRAY (new_arr);
          ADDR_T old_k = calculate_internal_index (old_tup, old_dim);
          ADDR_T new_k = j * SPAN (new_tup) + calculate_internal_index (&new_tup[1], old_dim);
          OFFSET (&src) += ROW_ELEMENT (old_arr, old_k);
          OFFSET (&dst) += ROW_ELEMENT (new_arr, new_k);
          if (HAS_ROWS (m_elem)) {
            A68_REF clone = genie_clone (p, m_elem, (A68_REF *) & nil_ref, &src);
            MOVE (ADDRESS (&dst), ADDRESS (&clone), SIZE (m_elem));
          } else {
            MOVE (ADDRESS (&dst), ADDRESS (&src), SIZE (m_elem));
          }
          done = increment_internal_index (old_tup, old_dim) | increment_internal_index (&new_tup[1], old_dim);
        }
      }
    }
  }
  return new_row;
}

//! @brief Make a row of 'len' objects that are in the stack.

A68_REF genie_make_row (NODE_T * p, MOID_T * m_elem, int len, ADDR_T pop_sp)
{
  A68_REF new_row, new_arr; A68_ARRAY arr; A68_TUPLE tup;
  NEW_ROW_1D (new_row, new_arr, arr, tup, MOID (p), m_elem, len);
  for (int k = 0; k < len * ELEM_SIZE (&arr); k += ELEM_SIZE (&arr)) {
    A68_REF dst = new_arr, src;
    OFFSET (&dst) += k;
    STATUS (&src) = (STATUS_MASK_T) (INIT_MASK | IN_STACK_MASK);
    OFFSET (&src) = pop_sp + k;
    REF_HANDLE (&src) = (A68_HANDLE *) & nil_handle;
    if (HAS_ROWS (m_elem)) {
      A68_REF clone = genie_clone (p, m_elem, (A68_REF *) & nil_ref, &src);
      MOVE (ADDRESS (&dst), ADDRESS (&clone), SIZE (m_elem));
    } else {
      MOVE (ADDRESS (&dst), ADDRESS (&src), SIZE (m_elem));
    }
  }
  return new_row;
}

//! @brief Make REF [1 : 1] [] MODE from REF [] MODE.

A68_REF genie_make_ref_row_of_row (NODE_T * p, MOID_T * m_dst, MOID_T * m_src, ADDR_T pop_sp)
{
  m_dst = DEFLEX (m_dst);
  m_src = DEFLEX (m_src);
  A68_REF array = *(A68_REF *) STACK_ADDRESS (pop_sp);
// ROWING NIL yields NIL.
  if (IS_NIL (array)) {
    return nil_ref;
  } else {
    A68_REF new_row = heap_generator (p, SUB (m_dst), DESCRIPTOR_SIZE (1));
    A68_REF name = heap_generator (p, m_dst, A68_REF_SIZE);
    A68_ARRAY *arr; A68_TUPLE *tup;
    GET_DESCRIPTOR (arr, tup, &new_row);
    DIM (arr) = 1;
    MOID (arr) = m_src;
    ELEM_SIZE (arr) = SIZE (m_src);
    SLICE_OFFSET (arr) = 0;
    FIELD_OFFSET (arr) = 0;
    ARRAY (arr) = array;
    LWB (tup) = 1;
    UPB (tup) = 1;
    SPAN (tup) = 1;
    SHIFT (tup) = LWB (tup);
    *DEREF (A68_REF, &name) = new_row;
    return name;
  }
}

//! @brief Make REF [1 : 1, ..] MODE from REF [..] MODE.

A68_REF genie_make_ref_row_row (NODE_T * p, MOID_T * m_dst, MOID_T * m_src, ADDR_T pop_sp)
{
  m_dst = DEFLEX (m_dst);
  m_src = DEFLEX (m_src);
  A68_REF name = *(A68_REF *) STACK_ADDRESS (pop_sp);
// ROWING NIL yields NIL.
  if (IS_NIL (name)) {
    return nil_ref;
  }
  A68_REF old_row = *DEREF (A68_REF, &name); A68_TUPLE *new_tup, *old_tup;
  A68_ARRAY *old_arr;
  GET_DESCRIPTOR (old_arr, old_tup, &old_row);
// Make new descriptor.
  A68_REF new_row = heap_generator (p, m_dst, DESCRIPTOR_SIZE (DIM (SUB (m_dst))));
  A68_ARRAY *new_arr;
  name = heap_generator (p, m_dst, A68_REF_SIZE);
  GET_DESCRIPTOR (new_arr, new_tup, &new_row);
  DIM (new_arr) = DIM (SUB (m_dst));
  MOID (new_arr) = MOID (old_arr);
  ELEM_SIZE (new_arr) = ELEM_SIZE (old_arr);
  SLICE_OFFSET (new_arr) = 0;
  FIELD_OFFSET (new_arr) = 0;
  ARRAY (new_arr) = ARRAY (old_arr);
// Fill out the descriptor.
  LWB (&(new_tup[0])) = 1;
  UPB (&(new_tup[0])) = 1;
  SPAN (&(new_tup[0])) = 1;
  SHIFT (&(new_tup[0])) = LWB (&(new_tup[0]));
  for (int k = 0; k < DIM (SUB (m_src)); k++) {
    new_tup[k + 1] = old_tup[k];
  }
// Yield the new name.
  *DEREF (A68_REF, &name) = new_row;
  return name;
}

//! @brief Coercion to [1 : 1, ] MODE.

PROP_T genie_rowing_row_row (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP;
  GENIE_UNIT_NO_GC (SUB (p));
  STACK_DNS (p, MOID (SUB (p)), A68_FP);
  A68_REF row = genie_make_rowrow (p, MOID (p), 1, pop_sp);
  A68_SP = pop_sp;
  PUSH_REF (p, row);
  return GPROP (p);
}

//! @brief Coercion to [1 : 1] [] MODE.

PROP_T genie_rowing_row_of_row (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP;
  GENIE_UNIT_NO_GC (SUB (p));
  STACK_DNS (p, MOID (SUB (p)), A68_FP);
  A68_REF row = genie_make_row (p, SLICE (MOID (p)), 1, pop_sp);
  A68_SP = pop_sp;
  PUSH_REF (p, row);
  return GPROP (p);
}

//! @brief Coercion to REF [1 : 1, ..] MODE.

PROP_T genie_rowing_ref_row_row (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP;
  MOID_T *dst = MOID (p), *src = MOID (SUB (p));
  GENIE_UNIT_NO_GC (SUB (p));
  STACK_DNS (p, MOID (SUB (p)), A68_FP);
  A68_SP = pop_sp;
  A68_REF name = genie_make_ref_row_row (p, dst, src, pop_sp);
  PUSH_REF (p, name);
  return GPROP (p);
}

//! @brief REF [1 : 1] [] MODE from [] MODE

PROP_T genie_rowing_ref_row_of_row (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP;
  MOID_T *m_dst = MOID (p), *src = MOID (SUB (p));
  GENIE_UNIT_NO_GC (SUB (p));
  STACK_DNS (p, MOID (SUB (p)), A68_FP);
  A68_SP = pop_sp;
  A68_REF name = genie_make_ref_row_of_row (p, m_dst, src, pop_sp);
  PUSH_REF (p, name);
  return GPROP (p);
}

//! @brief Rowing coercion.

PROP_T genie_rowing (NODE_T * p)
{
  PROP_T self;
  if (IS_REF (MOID (p))) {
// REF ROW, decide whether we want A->[] A or [] A->[,] A.
    MOID_T *mode = SUB_MOID (p);
    if (DIM (DEFLEX (mode)) >= 2) {
      (void) genie_rowing_ref_row_row (p);
      UNIT (&self) = genie_rowing_ref_row_row;
      SOURCE (&self) = p;
    } else {
      (void) genie_rowing_ref_row_of_row (p);
      UNIT (&self) = genie_rowing_ref_row_of_row;
      SOURCE (&self) = p;
    }
  } else {
// ROW, decide whether we want A->[] A or [] A->[,] A.
    if (DIM (DEFLEX (MOID (p))) >= 2) {
      (void) genie_rowing_row_row (p);
      UNIT (&self) = genie_rowing_row_row;
      SOURCE (&self) = p;
    } else {
      (void) genie_rowing_row_of_row (p);
      UNIT (&self) = genie_rowing_row_of_row;
      SOURCE (&self) = p;
    }
  }
  return self;
}

//! @brief Clone a compounded value referred to by 'old'.

A68_REF genie_clone (NODE_T * p, MOID_T * m, A68_REF * tmp, A68_REF * old)
{
// This complex routine is needed as arrays are not always contiguous.
// The routine takes a REF to the value and returns a REF to the clone.
  if (m == M_SOUND) {
// REF SOUND.
    A68_REF new_snd = heap_generator (p, m, SIZE (m));
    A68_SOUND *w = DEREF (A68_SOUND, &new_snd);
    int size = A68_SOUND_DATA_SIZE (w);
    COPY ((BYTE_T *) w, ADDRESS (old), SIZE (M_SOUND));
    BYTE_T *owd = ADDRESS (&(DATA (w)));
    DATA (w) = heap_generator (p, M_SOUND_DATA, size);
    COPY (ADDRESS (&(DATA (w))), owd, size);
    return new_snd;
  } else if (IS_STRUCT (m)) {
// REF STRUCT.
    A68_REF new_str = heap_generator (p, m, SIZE (m));
    for (PACK_T *field = PACK (m); field != NO_PACK; FORWARD (field)) {
      MOID_T *m_f = MOID (field);
      A68_REF old_f = *old, new_f = new_str;
      OFFSET (&old_f) += OFFSET (field);
      OFFSET (&new_f) += OFFSET (field);
      A68_REF tmp_f = *tmp;
      if (!IS_NIL (tmp_f)) {
        OFFSET (&tmp_f) += OFFSET (field);
      }
      if (HAS_ROWS (m_f)) {
        A68_REF clone = genie_clone (p, m_f, &tmp_f, &old_f);
        MOVE (ADDRESS (&new_f), ADDRESS (&clone), SIZE (m_f));
      } else {
        MOVE (ADDRESS (&new_f), ADDRESS (&old_f), SIZE (m_f));
      }
    }
    return new_str;
  } else if (IS_UNION (m)) {
// REF UNION.
    A68_REF new_uni = heap_generator (p, m, SIZE (m));
    A68_REF src = *old;
    A68_UNION *u = DEREF (A68_UNION, &src);
    MOID_T *m_u = (MOID_T *) VALUE (u);
    OFFSET (&src) += UNION_OFFSET;
    A68_REF dst = new_uni;
    *DEREF (A68_UNION, &dst) = *u;
    OFFSET (&dst) += UNION_OFFSET;
// A union has formal members, so 'tmp' is irrelevant.
    A68_REF tmp_u = nil_ref;
    if (m_u != NO_MOID && HAS_ROWS (m_u)) {
      A68_REF clone = genie_clone (p, m_u, &tmp_u, &src);
      MOVE (ADDRESS (&dst), ADDRESS (&clone), SIZE (m_u));
    } else if (m_u != NO_MOID) {
      MOVE (ADDRESS (&dst), ADDRESS (&src), SIZE (m_u));
    }
    return new_uni;
  } else if (IF_ROW (m)) {
// REF [FLEX] [].
    MOID_T *em = SUB (IS_FLEX (m) ? SUB (m) : m);
// Make new array.
    A68_ARRAY *old_arr; A68_TUPLE *old_tup;
    GET_DESCRIPTOR (old_arr, old_tup, DEREF (A68_REF, old));
    A68_ARRAY *new_arr; A68_TUPLE *new_tup;
    A68_REF nrow = heap_generator (p, m, DESCRIPTOR_SIZE (DIM (old_arr)));
    GET_DESCRIPTOR (new_arr, new_tup, &nrow);
    DIM (new_arr) = DIM (old_arr);
    MOID (new_arr) = MOID (old_arr);
    ELEM_SIZE (new_arr) = ELEM_SIZE (old_arr);
    SLICE_OFFSET (new_arr) = 0;
    FIELD_OFFSET (new_arr) = 0;
// Get size and copy bounds; check in case of a row.
// This is just song and dance to comply with the RR.
    BOOL_T check_bounds = A68_FALSE;
    A68_REF ntmp; A68_ARRAY *tarr; A68_TUPLE *ttup = NO_TUPLE;
    if (IS_NIL (*tmp)) {
      ntmp = nil_ref;
    } else {
      A68_REF *z = DEREF (A68_REF, tmp);
      if (!IS_NIL (*z)) {
        GET_DESCRIPTOR (tarr, ttup, z);
        ntmp = ARRAY (tarr);
        check_bounds = IS_ROW (m);
      }
    }
    int span = 1;
    for (int k = 0; k < DIM (old_arr); k++) {
      A68_TUPLE *op = &old_tup[k], *np = &new_tup[k];
      if (check_bounds) {
        A68_TUPLE *tp = &ttup[k];
        if (UPB (tp) >= LWB (tp) && UPB (op) >= LWB (op)) {
          if (UPB (tp) != UPB (op) || LWB (tp) != LWB (op)) {
            diagnostic (A68_RUNTIME_ERROR, p, ERROR_DIFFERENT_BOUNDS);
            exit_genie (p, A68_RUNTIME_ERROR);
          }
        }
      }
      LWB (np) = LWB (op);
      UPB (np) = UPB (op);
      SPAN (np) = span;
      SHIFT (np) = LWB (np) * SPAN (np);
      span *= ROW_SIZE (np);
    }
// Make a new array with at least a ghost element.
    if (span == 0) {
      ARRAY (new_arr) = heap_generator (p, em, ELEM_SIZE (new_arr));
    } else {
      ARRAY (new_arr) = heap_generator_2 (p, em, span, ELEM_SIZE (new_arr));
    }
// Copy the ghost element if there are no elements.
    if (span == 0) {
      if (IS_UNION (em)) {
// UNION has formal members.
      } else if (HAS_ROWS (em)) {
        A68_REF old_ref, dst_ref, clone;
        old_ref = ARRAY (old_arr);
        OFFSET (&old_ref) += ROW_ELEMENT (old_arr, 0);
        dst_ref = ARRAY (new_arr);
        OFFSET (&dst_ref) += ROW_ELEMENT (new_arr, 0);
        clone = genie_clone (p, em, &ntmp, &old_ref);
        MOVE (ADDRESS (&dst_ref), ADDRESS (&clone), SIZE (em));
      }
    } else if (span > 0) {
// The n-dimensional copier.
      initialise_internal_index (old_tup, DIM (old_arr));
      initialise_internal_index (new_tup, DIM (new_arr));
      BOOL_T done = A68_FALSE;
      while (!done) {
        A68_REF old_ref = ARRAY (old_arr), dst_ref = ARRAY (new_arr);
        ADDR_T old_k = calculate_internal_index (old_tup, DIM (old_arr));
        ADDR_T new_k = calculate_internal_index (new_tup, DIM (new_arr));
        OFFSET (&old_ref) += ROW_ELEMENT (old_arr, old_k);
        OFFSET (&dst_ref) += ROW_ELEMENT (new_arr, new_k);
        if (HAS_ROWS (em)) {
          A68_REF clone;
          clone = genie_clone (p, em, &ntmp, &old_ref);
          MOVE (ADDRESS (&dst_ref), ADDRESS (&clone), SIZE (em));
        } else {
          MOVE (ADDRESS (&dst_ref), ADDRESS (&old_ref), SIZE (em));
        }
// Increase pointers.
        done = increment_internal_index (old_tup, DIM (old_arr)) | increment_internal_index (new_tup, DIM (new_arr));
      }
    }
    A68_REF heap = heap_generator (p, m, A68_REF_SIZE);
    *DEREF (A68_REF, &heap) = nrow;
    return heap;
  }
  return nil_ref;
}

//! @brief Store into a row, fi. trimmed destinations.

A68_REF genie_store (NODE_T * p, MOID_T * m, A68_REF * dst, A68_REF * old)
{
// This complex routine is needed as arrays are not always contiguous.
// The routine takes a REF to the value and returns a REF to the clone.
  if (IF_ROW (m)) {
// REF [FLEX] [].
    A68_TUPLE *old_tup, *new_tup, *old_p, *new_p;
    MOID_T *em = SUB (IS_FLEX (m) ? SUB (m) : m);
    BOOL_T done = A68_FALSE;
    A68_ARRAY *old_arr, *new_arr;
    GET_DESCRIPTOR (old_arr, old_tup, DEREF (A68_REF, old));
    GET_DESCRIPTOR (new_arr, new_tup, DEREF (A68_REF, dst));
// Get size and check bounds.
// This is just song and dance to comply with the RR.
    int span = 1;
    for (int k = 0; k < DIM (old_arr); k++) {
      old_p = &old_tup[k];
      new_p = &new_tup[k];
      if ((UPB (new_p) >= LWB (new_p) && UPB (old_p) >= LWB (old_p))) {
        if ((UPB (new_p) != UPB (old_p) || LWB (new_p) != LWB (old_p))) {
          diagnostic (A68_RUNTIME_ERROR, p, ERROR_DIFFERENT_BOUNDS);
          exit_genie (p, A68_RUNTIME_ERROR);
        }
      }
      span *= ROW_SIZE (new_p);
    }
// Destination is an empty row, inspect if the source has elements.
    if (span == 0) {
      span = 1;
      for (int k = 0; k < DIM (old_arr); k++) {
        span *= ROW_SIZE (old_p);
      }
      if (span > 0) {
        span = 1;
        for (int k = 0; k < DIM (old_arr); k++) {
          new_tup[k] = old_tup[k];
        }
        ARRAY (new_arr) = heap_generator_2 (p, em, span, ELEM_SIZE (new_arr));
      }
    } 
    if (span > 0) {
      initialise_internal_index (old_tup, DIM (old_arr));
      initialise_internal_index (new_tup, DIM (new_arr));
      while (!done) {
        A68_REF new_old = ARRAY (old_arr), new_dst = ARRAY (new_arr);
        ADDR_T old_index = calculate_internal_index (old_tup, DIM (old_arr));
        ADDR_T new_index = calculate_internal_index (new_tup, DIM (new_arr));
        OFFSET (&new_old) += ROW_ELEMENT (old_arr, old_index);
        OFFSET (&new_dst) += ROW_ELEMENT (new_arr, new_index);
        MOVE (ADDRESS (&new_dst), ADDRESS (&new_old), SIZE (em));
        done = increment_internal_index (old_tup, DIM (old_arr)) | increment_internal_index (new_tup, DIM (new_arr));
      }
    }
    return *dst;
  }
  return nil_ref;
}

//! @brief Assignment of complex objects in the stack.

void genie_clone_stack (NODE_T * p, MOID_T * srcm, A68_REF * dst, A68_REF * tmp)
{
// STRUCT, UNION, [FLEX] [] or SOUND.
  A68_REF stack;
  STATUS (&stack) = (STATUS_MASK_T) (INIT_MASK | IN_STACK_MASK);
  OFFSET (&stack) = A68_SP;
  REF_HANDLE (&stack) = (A68_HANDLE *) & nil_handle;
  A68_REF *src = DEREF (A68_REF, &stack);
  if (IS_ROW (srcm) && !IS_NIL (*tmp)) {
    if (STATUS (src) & SKIP_ROW_MASK) {
      return;
    }
    A68_REF clone = genie_clone (p, srcm, tmp, &stack);
    (void) genie_store (p, srcm, dst, &clone);
  } else {
    A68_REF clone = genie_clone (p, srcm, tmp, &stack);
    MOVE (ADDRESS (dst), ADDRESS (&clone), SIZE (srcm));
  }
}

//! @brief Strcmp for qsort.

int qstrcmp (const void *a, const void *b)
{
  return strcmp (*(char *const *) a, *(char *const *) b);
}

//! @brief Sort row of string.

void genie_sort_row_string (NODE_T * p)
{
  A68_REF z; A68_ARRAY *arr; A68_TUPLE *tup;
  POP_REF (p, &z);
  ADDR_T pop_sp = A68_SP;
  CHECK_REF (p, z, M_ROW_STRING);
  GET_DESCRIPTOR (arr, tup, &z);
  int size = ROW_SIZE (tup);
  if (size > 0) {
    BYTE_T *base = ADDRESS (&ARRAY (arr));
    char **ptrs = (char **) a68_alloc ((size_t) (size * (int) sizeof (char *)), __func__, __LINE__);
    if (ptrs == NO_VAR) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
// Copy C-strings into the stack and sort.
    for (int j = 0, k = LWB (tup); k <= UPB (tup); j++, k++) {
      int addr = INDEX_1_DIM (arr, tup, k);
      A68_REF ref = *(A68_REF *) & (base[addr]);
      CHECK_REF (p, ref, M_STRING);
      int len = A68_ALIGN (a68_string_size (p, ref) + 1);
      if (A68_SP + len > A68 (expr_stack_limit)) {
        diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
        exit_genie (p, A68_RUNTIME_ERROR);
      }
      ptrs[j] = (char *) STACK_TOP;
      ASSERT (a_to_c_string (p, (char *) STACK_TOP, ref) != NO_TEXT);
      INCREMENT_STACK_POINTER (p, len);
    }
    qsort (ptrs, (size_t) size, sizeof (char *), qstrcmp);
// Construct an array of sorted strings.
    A68_REF row; A68_ARRAY arrn; A68_TUPLE tupn;
    NEW_ROW_1D (z, row, arrn, tupn, M_ROW_STRING, M_STRING, size);
    A68_REF *base_ref = DEREF (A68_REF, &row);
    for (int k = 0; k < size; k++) {
      base_ref[k] = c_to_a_string (p, ptrs[k], DEFAULT_WIDTH);
    }
    a68_free (ptrs);
    A68_SP = pop_sp;
    PUSH_REF (p, z);
  } else {
// This is how we sort an empty row of strings ...
    A68_SP = pop_sp;
    PUSH_REF (p, empty_row (p, M_ROW_STRING));
  }
}
