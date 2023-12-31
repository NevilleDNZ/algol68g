//! @file genie-rows.c
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
//! Interpreter routines for ROW values.

// An A68G row is a reference to a descriptor in the heap:
//
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

// Operators for ROW values.

//! @brief OP ELEMS = (ROWS) INT

void genie_monad_elems (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  PUSH_VALUE (p, get_row_size (tup, DIM (arr)), A68_INT);
}

//! @brief OP LWB = (ROWS) INT

void genie_monad_lwb (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  PUSH_VALUE (p, LWB (tup), A68_INT);
}

//! @brief OP UPB = (ROWS) INT

void genie_monad_upb (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  PUSH_VALUE (p, UPB (tup), A68_INT);
}

//! @brief OP ELEMS = (INT, ROWS) INT

void genie_dyad_elems (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_INT k;
  POP_OBJECT (p, &k, A68_INT);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  if (VALUE (&k) < 1 || VALUE (&k) > DIM (arr)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_INVALID_DIMENSION, (int) VALUE (&k));
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  A68_TUPLE *u = &(tup[VALUE (&k) - 1]);
  PUSH_VALUE (p, ROW_SIZE (u), A68_INT);
}

//! @brief OP LWB = (INT, ROWS) INT

void genie_dyad_lwb (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_INT k;
  POP_OBJECT (p, &k, A68_INT);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  if (VALUE (&k) < 1 || VALUE (&k) > DIM (arr)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_INVALID_DIMENSION, (int) VALUE (&k));
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  PUSH_VALUE (p, LWB (&(tup[VALUE (&k) - 1])), A68_INT);
}

//! @brief OP UPB = (INT, ROWS) INT

void genie_dyad_upb (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  DECREMENT_STACK_POINTER (p, A68_UNION_SIZE); // Pop UNION.
  CHECK_REF (p, row, M_ROWS);
  A68_INT k;
  POP_OBJECT (p, &k, A68_INT);
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  if (VALUE (&k) < 1 || VALUE (&k) > DIM (arr)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_INVALID_DIMENSION, (int) VALUE (&k));
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  PUSH_VALUE (p, UPB (&(tup[VALUE (&k) - 1])), A68_INT);
}
