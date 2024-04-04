/*!
\file torrix.c
\brief vector and matrix support through GSL
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2006 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "algol68g.h"
#include "genie.h"
#include "gsl.h"

#if defined HAVE_GSL

#include <gsl/gsl_blas.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_const.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_vector.h>

#define VECTOR_OFFSET(a, t)\
  ((((t)->lower_bound - (t)->shift) * (t)->span + (a)->slice_offset) * (a)->elem_size + (a)->field_offset)

#define MATRIX_OFFSET(a, t1, t2)\
  ((((t1)->lower_bound - (t1)->shift) * (t1)->span + ((t2)->lower_bound - (t2)->shift) * (t2)->span + (a)->slice_offset ) * (a)->elem_size + (a)->field_offset)

static NODE_T *error_node = NULL;

/*!
\brief set permutation vector element - function fails in gsl
\param p permutation vector
\param i element index
\param j element value
**/

void gsl_permutation_set (const gsl_permutation * p, const size_t i, const size_t j)
{
  p->data[i] = j;
}

/*!
\brief map GSL error handler onto a68g error handler
\param reason error text
\param file gsl file where error occured
\param line line in above file
\param int gsl error number
**/

void error_handler (const char *reason, const char *file, int line, int gsl_errno)
{
  (void) file;
  (void) line;
  diagnostic_node (A_RUNTIME_ERROR, error_node, ERROR_TORRIX, reason, gsl_strerror (gsl_errno), NULL);
  exit_genie (error_node, A_RUNTIME_ERROR);
}

/*!
\brief detect math errors, mainly in BLAS functions
\param rc return code from function
**/

static void test_error (int rc)
{
  if (rc != 0) {
    error_handler ("math error", "", 0, rc);
  }
}

/*!
\brief pop [] INT on the stack as gsl_permutation
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return gsl_permutation_complex
**/

static gsl_permutation *pop_permutation (NODE_T * p, BOOL_T get)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int len, inc, index, k;
  BYTE_T *base;
  gsl_permutation *v;
/* Pop arguments. */
  POP (p, &desc, SIZE_OF (A68_REF));
  TEST_INIT (p, desc, MODE (ROW_INT));
  TEST_NIL (p, desc, MODE (ROW_INT));
  GET_DESCRIPTOR (arr, tup, &desc);
  len = ROW_SIZE (tup);
  v = gsl_permutation_alloc (len);
  if (get) {
    base = DEREF (BYTE_T, &(arr->array));
    index = VECTOR_OFFSET (arr, tup);
    inc = tup->span * arr->elem_size;
    for (k = 0; k < len; k++, index += inc) {
      A68_INT *x = (A68_INT *) (base + index);
      TEST_INIT (p, *x, MODE (INT));
      gsl_permutation_set (v, k, x->value);
    }
  }
  return (v);
}

/*!
\brief push gsl_permutation on the stack as [] INT
\param p position in the syntax tree, should not be NULL
**/

static void push_permutation (NODE_T * p, gsl_permutation * v)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int len, inc, index, k;
  BYTE_T *base;
  len = v->size;
  desc = heap_generator (p, MODE (ROW_INT), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_INT), len * SIZE_OF (A68_INT));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (INT);
  arr.elem_size = SIZE_OF (A68_INT);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_INT *x = (A68_INT *) (base + index);
    x->status = INITIALISED_MASK;
    x->value = gsl_permutation_get (v, k);
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop [] REAL on the stack as gsl_vector
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return gsl_vector_complex
**/

static gsl_vector *pop_vector (NODE_T * p, BOOL_T get)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int len, inc, index, k;
  BYTE_T *base;
  gsl_vector *v;
/* Pop arguments. */
  POP (p, &desc, SIZE_OF (A68_REF));
  TEST_INIT (p, desc, MODE (ROW_REAL));
  TEST_NIL (p, desc, MODE (ROW_REAL));
  GET_DESCRIPTOR (arr, tup, &desc);
  len = ROW_SIZE (tup);
  v = gsl_vector_alloc (len);
  if (get) {
    base = DEREF (BYTE_T, &(arr->array));
    index = VECTOR_OFFSET (arr, tup);
    inc = tup->span * arr->elem_size;
    for (k = 0; k < len; k++, index += inc) {
      A68_REAL *x = (A68_REAL *) (base + index);
      TEST_INIT (p, *x, MODE (REAL));
      gsl_vector_set (v, k, x->value);
    }
  }
  return (v);
}

/*!
\brief push gsl_vector on the stack as [] REAL
\param p position in the syntax tree, should not be NULL
**/

static void push_vector (NODE_T * p, gsl_vector * v)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int len, inc, index, k;
  BYTE_T *base;
  len = v->size;
  desc = heap_generator (p, MODE (ROW_REAL), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_REAL), len * SIZE_OF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (REAL);
  arr.elem_size = SIZE_OF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_REAL *x = (A68_REAL *) (base + index);
    x->status = INITIALISED_MASK;
    x->value = gsl_vector_get (v, k);
    TEST_REAL_REPRESENTATION (p, x->value);
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop [,] REAL on the stack as gsl_matrix
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return gsl_matrix
**/

static gsl_matrix *pop_matrix (NODE_T * p, BOOL_T get)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup1, *tup2;
  int len1, len2, inc1, inc2, index1, index2, k1, k2;
  BYTE_T *base;
  gsl_matrix *a;
/* Pop arguments. */
  POP (p, &desc, SIZE_OF (A68_REF));
  TEST_INIT (p, desc, MODE (ROWROW_REAL));
  TEST_NIL (p, desc, MODE (ROWROW_REAL));
  GET_DESCRIPTOR (arr, tup1, &desc);
  tup2 = &(tup1[1]);
  len1 = ROW_SIZE (tup1);
  len2 = ROW_SIZE (tup2);
  a = gsl_matrix_alloc (len1, len2);
  if (get) {
    base = DEREF (BYTE_T, &(arr->array));
    index1 = MATRIX_OFFSET (arr, tup1, tup2);
    inc1 = tup1->span * arr->elem_size;
    inc2 = tup2->span * arr->elem_size;
    for (k1 = 0; k1 < len1; k1++, index1 += inc1) {
      for (k2 = 0, index2 = index1; k2 < len2; k2++, index2 += inc2) {
        A68_REAL *x = (A68_REAL *) (base + index2);
        TEST_INIT (p, *x, MODE (REAL));
        gsl_matrix_set (a, k1, k2, x->value);
      }
    }
  }
  return (a);
}

/*!
\brief push gsl_matrix on the stack as [,] REAL
\param p position in the syntax tree, should not be NULL
**/

static void push_matrix (NODE_T * p, gsl_matrix * a)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup1, tup2;
  int len1, len2, inc1, inc2, index1, index2, k1, k2;
  BYTE_T *base;
  len1 = a->size1;
  len2 = a->size2;
  desc = heap_generator (p, MODE (ROWROW_REAL), SIZE_OF (A68_ARRAY) + 2 * SIZE_OF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROWROW_REAL), len1 * len2 * SIZE_OF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 2;
  arr.type = MODE (REAL);
  arr.elem_size = SIZE_OF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup1.lower_bound = 1;
  tup1.upper_bound = len1;
  tup1.shift = tup1.lower_bound;
  tup1.span = 1;
  tup2.lower_bound = 1;
  tup2.upper_bound = len2;
  tup2.shift = tup2.lower_bound;
  tup2.span = ROW_SIZE (&tup1);
  PUT_DESCRIPTOR2 (arr, tup1, tup2, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index1 = MATRIX_OFFSET (&arr, &tup1, &tup2);
  inc1 = tup1.span * arr.elem_size;
  inc2 = tup2.span * arr.elem_size;
  for (k1 = 0; k1 < len1; k1++, index1 += inc1) {
    for (k2 = 0, index2 = index1; k2 < len2; k2++, index2 += inc2) {
      A68_REAL *x = (A68_REAL *) (base + index2);
      x->status = INITIALISED_MASK;
      x->value = gsl_matrix_get (a, k1, k2);
      TEST_REAL_REPRESENTATION (p, x->value);
    }
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop [] COMPLEX on the stack as gsl_vector_complex
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return gsl_vector_complex
**/

static gsl_vector_complex *pop_vector_complex (NODE_T * p, BOOL_T get)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int len, inc, index, k;
  BYTE_T *base;
  gsl_vector_complex *v;
/* Pop arguments. */
  POP (p, &desc, SIZE_OF (A68_REF));
  TEST_INIT (p, desc, MODE (ROW_COMPLEX));
  TEST_NIL (p, desc, MODE (ROW_COMPLEX));
  GET_DESCRIPTOR (arr, tup, &desc);
  len = ROW_SIZE (tup);
  v = gsl_vector_complex_alloc (len);
  if (get) {
    base = DEREF (BYTE_T, &(arr->array));
    index = VECTOR_OFFSET (arr, tup);
    inc = tup->span * arr->elem_size;
    for (k = 0; k < len; k++, index += inc) {
      A68_REAL *re = (A68_REAL *) (base + index);
      A68_REAL *im = &(re[1]);
      gsl_complex z;
      TEST_INIT (p, *re, MODE (COMPLEX));
      TEST_INIT (p, *im, MODE (COMPLEX));
      GSL_SET_COMPLEX (&z, re->value, im->value);
      gsl_vector_complex_set (v, k, z);
    }
  }
  return (v);
}

/*!
\brief push gsl_vector_complex on the stack as [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

static void push_vector_complex (NODE_T * p, gsl_vector_complex * v)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int len, inc, index, k;
  BYTE_T *base;
  len = v->size;
  desc = heap_generator (p, MODE (ROW_COMPLEX), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_COMPLEX), len * 2 * SIZE_OF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (COMPLEX);
  arr.elem_size = 2 * SIZE_OF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_REAL *re = (A68_REAL *) (base + index);
    A68_REAL *im = &(re[1]);
    gsl_complex z = gsl_vector_complex_get (v, k);
    re->status = INITIALISED_MASK;
    re->value = GSL_REAL (z);
    im->status = INITIALISED_MASK;
    im->value = GSL_IMAG (z);
    TEST_COMPLEX_REPRESENTATION (p, re->value, im->value);
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop [,] COMPLEX on the stack as gsl_matrix_complex
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return gsl_matrix_complex
**/

static gsl_matrix_complex *pop_matrix_complex (NODE_T * p, BOOL_T get)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup1, *tup2;
  int len1, len2;
  gsl_matrix_complex *a;
/* Pop arguments. */
  POP (p, &desc, SIZE_OF (A68_REF));
  TEST_INIT (p, desc, MODE (ROWROW_COMPLEX));
  TEST_NIL (p, desc, MODE (ROWROW_COMPLEX));
  GET_DESCRIPTOR (arr, tup1, &desc);
  tup2 = &(tup1[1]);
  len1 = ROW_SIZE (tup1);
  len2 = ROW_SIZE (tup2);
  a = gsl_matrix_complex_alloc (len1, len2);
  if (get) {
    BYTE_T *base = DEREF (BYTE_T, &(arr->array));
    int index1 = MATRIX_OFFSET (arr, tup1, tup2);
    int inc1 = tup1->span * arr->elem_size, inc2 = tup2->span * arr->elem_size, k1;
    for (k1 = 0; k1 < len1; k1++, index1 += inc1) {
      int index2, k2;
      for (k2 = 0, index2 = index1; k2 < len2; k2++, index2 += inc2) {
        A68_REAL *re = (A68_REAL *) (base + index2);
        A68_REAL *im = &(re[1]);
        gsl_complex z;
        TEST_INIT (p, *re, MODE (COMPLEX));
        TEST_INIT (p, *im, MODE (COMPLEX));
        GSL_SET_COMPLEX (&z, re->value, im->value);
        gsl_matrix_complex_set (a, k1, k2, z);
      }
    }
  }
  return (a);
}

/*!
\brief push gsl_matrix_complex on the stack as [,] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

static void push_matrix_complex (NODE_T * p, gsl_matrix_complex * a)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup1, tup2;
  int len1, len2, inc1, inc2, index1, index2, k1, k2;
  BYTE_T *base;
  len1 = a->size1;
  len2 = a->size2;
  desc = heap_generator (p, MODE (ROWROW_COMPLEX), SIZE_OF (A68_ARRAY) + 2 * SIZE_OF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROWROW_COMPLEX), len1 * len2 * 2 * SIZE_OF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 2;
  arr.type = MODE (COMPLEX);
  arr.elem_size = 2 * SIZE_OF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup1.lower_bound = 1;
  tup1.upper_bound = len1;
  tup1.shift = tup1.lower_bound;
  tup1.span = 1;
  tup2.lower_bound = 1;
  tup2.upper_bound = len2;
  tup2.shift = tup2.lower_bound;
  tup2.span = ROW_SIZE (&tup1);
  PUT_DESCRIPTOR2 (arr, tup1, tup2, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index1 = MATRIX_OFFSET (&arr, &tup1, &tup2);
  inc1 = tup1.span * arr.elem_size;
  inc2 = tup2.span * arr.elem_size;
  for (k1 = 0; k1 < len1; k1++, index1 += inc1) {
    for (k2 = 0, index2 = index1; k2 < len2; k2++, index2 += inc2) {
      A68_REAL *re = (A68_REAL *) (base + index2);
      A68_REAL *im = &(re[1]);
      gsl_complex z = gsl_matrix_complex_get (a, k1, k2);
      re->status = INITIALISED_MASK;
      re->value = GSL_REAL (z);
      im->status = INITIALISED_MASK;
      im->value = GSL_IMAG (z);
      TEST_COMPLEX_REPRESENTATION (p, re->value, im->value);
    }
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop REF [...] M and derefence to [...] M
\param p position in the syntax tree, should not be NULL
\param m mode of REF [...] M
\param par_size size of parameters in the stack
\return the undereferenced REF
**/

static A68_REF dereference_ref_row (NODE_T * p, MOID_T * m, ADDR_T par_size)
{
  A68_REF *u, v;
  u = (A68_REF *) STACK_OFFSET (-par_size);
  v = *u;
  TEST_NIL (p, v, m);
  *u = *DEREF (A68_ROW, &v);
  return (v);
}

/*!
\brief generically perform operation and assign result (+:=, -:=, ...) 
\param p position in the syntax tree, should not be NULL
\param m mode of REF [...] M
\param n mode of right operand M
\param op operation to be performed
**/

static void op_ab (NODE_T * p, MOID_T * m, MOID_T * n, GENIE_PROCEDURE * op)
{
  unsigned par_size = MOID_SIZE (m) + MOID_SIZE (n);
  A68_REF u, *v;
  error_node = p;
  u = dereference_ref_row (p, m, par_size);
  v = (A68_REF *) STACK_OFFSET (-par_size);
  op (p);
  *DEREF (A68_ROW, &u) = *v;
  *v = u;
}

/*!
\brief PROC vector echo = ([] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_echo (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  error_node = p;
  u = pop_vector (p, A_TRUE);
  push_vector (p, u);
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC matrix echo = ([,] REAL) [,] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_echo (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  error_node = p;
  a = pop_matrix (p, A_TRUE);
  push_matrix (p, a);
  gsl_matrix_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex vector echo = ([] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_echo (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  error_node = p;
  u = pop_vector_complex (p, A_TRUE);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex matrix echo = ([,] COMPLEX) [,] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_echo (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *a;
  error_node = p;
  a = pop_matrix_complex (p, A_TRUE);
  push_matrix_complex (p, a);
  gsl_matrix_complex_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_minus (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  int rc;
  error_node = p;
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_scale (u, -1);
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([,] REAL) [,] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_minus (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  int rc;
  error_node = p;
  a = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_scale (a, -1);
  test_error (rc);
  push_matrix (p, a);
  gsl_matrix_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP T = ([,] REAL) [,] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_transpose (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  int rc;
  error_node = p;
  a = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_transpose (a);
  test_error (rc);
  push_matrix (p, a);
  gsl_matrix_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP T = ([,] COMPLEX) [,] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_transpose (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *a;
  int rc;
  error_node = p;
  a = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_transpose (a);
  test_error (rc);
  push_matrix_complex (p, a);
  gsl_matrix_complex_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP INV = ([,] REAL) [,] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_inv (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix *u, *inv;
  int rc, signum;
  error_node = p;
  u = pop_matrix (p, A_TRUE);
  q = gsl_permutation_alloc (u->size1);
  rc = gsl_linalg_LU_decomp (u, q, &signum);
  test_error (rc);
  inv = gsl_matrix_alloc (u->size1, u->size2);
  rc = gsl_linalg_LU_invert (u, q, inv);
  test_error (rc);
  push_matrix (p, inv);
  gsl_matrix_free (inv);
  gsl_matrix_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP INV = ([,] COMPLEX) [,] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_inv (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix_complex *u, *inv;
  int rc, signum;
  error_node = p;
  u = pop_matrix_complex (p, A_TRUE);
  q = gsl_permutation_alloc (u->size1);
  rc = gsl_linalg_complex_LU_decomp (u, q, &signum);
  test_error (rc);
  inv = gsl_matrix_complex_alloc (u->size1, u->size2);
  rc = gsl_linalg_complex_LU_invert (u, q, inv);
  test_error (rc);
  push_matrix_complex (p, inv);
  gsl_matrix_complex_free (inv);
  gsl_matrix_complex_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP DET = ([,] REAL) REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_det (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix *u;
  int rc, signum;
  error_node = p;
  u = pop_matrix (p, A_TRUE);
  q = gsl_permutation_alloc (u->size1);
  rc = gsl_linalg_LU_decomp (u, q, &signum);
  test_error (rc);
  PUSH_REAL (p, gsl_linalg_LU_det (u, signum));
  gsl_matrix_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP DET = ([,] COMPLEX) COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_det (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix_complex *u;
  int rc, signum;
  gsl_complex det;
  error_node = p;
  u = pop_matrix_complex (p, A_TRUE);
  q = gsl_permutation_alloc (u->size1);
  rc = gsl_linalg_complex_LU_decomp (u, q, &signum);
  test_error (rc);
  det = gsl_linalg_complex_LU_det (u, signum);
  PUSH_REAL (p, GSL_REAL (det));
  PUSH_REAL (p, GSL_IMAG (det));
  gsl_matrix_complex_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP TRACE = ([,] REAL) REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_trace (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  double sum;
  int len1, len2, k;
  error_node = p;
  a = pop_matrix (p, A_TRUE);
  len1 = a->size1;
  len2 = a->size2;
  if (len1 != len2) {
    error_handler ("cannot calculate trace", __FILE__, __LINE__, GSL_ENOTSQR);
  }
  sum = 0.0;
  for (k = 0; k < len1; k++) {
    sum += gsl_matrix_get (a, k, k);
  }
  PUSH_REAL (p, sum);
  gsl_matrix_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP TRACE = ([,] COMPLEX) COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_trace (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *a;
  gsl_complex sum;
  int len1, len2, k;
  error_node = p;
  a = pop_matrix_complex (p, A_TRUE);
  len1 = a->size1;
  len2 = a->size2;
  if (len1 != len2) {
    error_handler ("cannot calculate trace", __FILE__, __LINE__, GSL_ENOTSQR);
  }
  GSL_SET_COMPLEX (&sum, 0.0, 0.0);
  for (k = 0; k < len1; k++) {
    sum = gsl_complex_add (sum, gsl_matrix_complex_get (a, k, k));
  }
  PUSH_REAL (p, GSL_REAL (sum));
  PUSH_REAL (p, GSL_IMAG (sum));
  gsl_matrix_complex_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_minus (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  error_node = p;
  u = pop_vector_complex (p, A_TRUE);
  gsl_blas_zdscal (-1, u);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([,] COMPLEX) [,] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_minus (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *a;
  gsl_complex one;
  int rc;
  error_node = p;
  GSL_SET_COMPLEX (&one, -1.0, 0.0);
  a = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_scale (a, one);
  test_error (rc);
  push_matrix_complex (p, a);
  gsl_matrix_complex_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP + = ([] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_add (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u, *v;
  int rc;
  error_node = p;
  v = pop_vector (p, A_TRUE);
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_add (u, v);
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  gsl_vector_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_sub (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u, *v;
  int rc;
  error_node = p;
  v = pop_vector (p, A_TRUE);
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_sub (u, v);
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  gsl_vector_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP = = ([] REAL, [] REAL) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_eq (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u, *v;
  int rc;
  error_node = p;
  v = pop_vector (p, A_TRUE);
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_sub (u, v);
  test_error (rc);
  PUSH_BOOL (p, (gsl_vector_isnull (u) ? A_TRUE : A_FALSE));
  gsl_vector_free (u);
  gsl_vector_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP /= = ([] REAL, [] REAL) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_ne (NODE_T * p)
{
  genie_vector_eq (p);
  genie_not_bool (p);
}

/*!
\brief OP +:= = (REF [] REAL, [] REAL) REF [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_plusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_REAL), MODE (ROW_REAL), genie_vector_add);
}

/*!
\brief OP -:= = (REF [] REAL, [] REAL) REF [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_minusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_REAL), MODE (ROW_REAL), genie_vector_sub);
}

/*!
\brief OP + = ([, ] REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_add (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix (p, A_TRUE);
  u = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_add (u, v);
  test_error (rc);
  push_matrix (p, u);
  gsl_matrix_free (u);
  gsl_matrix_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([, ] REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_sub (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix (p, A_TRUE);
  u = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_sub (u, v);
  test_error (rc);
  push_matrix (p, u);
  gsl_matrix_free (u);
  gsl_matrix_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP = = ([, ] REAL, [, ] REAL) [, ] BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_eq (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix (p, A_TRUE);
  u = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_sub (u, v);
  test_error (rc);
  PUSH_BOOL (p, (gsl_matrix_isnull (u) ? A_TRUE : A_FALSE));
  gsl_matrix_free (u);
  gsl_matrix_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP /= = ([, ] REAL, [, ] REAL) [, ] BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_ne (NODE_T * p)
{
  genie_matrix_eq (p);
  genie_not_bool (p);
}

/*!
\brief OP +:= = (REF [, ] REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_plusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_REAL), MODE (ROWROW_REAL), genie_matrix_add);
}

/*!
\brief OP -:= = (REF [, ] REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_minusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_REAL), MODE (ROWROW_REAL), genie_matrix_sub);
}

/*!
\brief OP + = ([] COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_add (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u, *v;
  gsl_complex one;
  int rc;
  error_node = p;
  GSL_SET_COMPLEX (&one, 1.0, 0.0);
  v = pop_vector_complex (p, A_TRUE);
  u = pop_vector_complex (p, A_TRUE);
  rc = gsl_blas_zaxpy (one, u, v);
  test_error (rc);
  push_vector_complex (p, v);
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([] COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_sub (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u, *v;
  gsl_complex one;
  error_node = p;
  int rc;
  GSL_SET_COMPLEX (&one, -1.0, 0.0);
  v = pop_vector_complex (p, A_TRUE);
  u = pop_vector_complex (p, A_TRUE);
  rc = gsl_blas_zaxpy (one, v, u);
  test_error (rc);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP = = ([] COMPLEX, [] COMPLEX) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_eq (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u, *v;
  gsl_complex one;
  error_node = p;
  int rc;
  GSL_SET_COMPLEX (&one, -1.0, 0.0);
  v = pop_vector_complex (p, A_TRUE);
  u = pop_vector_complex (p, A_TRUE);
  rc = gsl_blas_zaxpy (one, v, u);
  test_error (rc);
  PUSH_BOOL (p, (gsl_vector_complex_isnull (u) ? A_TRUE : A_FALSE));
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP /= = ([] COMPLEX, [] COMPLEX) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_ne (NODE_T * p)
{
  genie_vector_complex_eq (p);
  genie_not_bool (p);
}

/*!
\brief OP +:= = (REF [] COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_plusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_COMPLEX), MODE (ROW_COMPLEX), genie_vector_complex_add);
}

/*!
\brief OP -:= = (REF [] COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_minusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_COMPLEX), MODE (ROW_COMPLEX), genie_vector_complex_sub);
}

/*!
\brief OP + = ([, ] COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_add (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix_complex (p, A_TRUE);
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_add (u, v);
  test_error (rc);
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  gsl_matrix_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP - = ([, ] COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_sub (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix_complex (p, A_TRUE);
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_sub (u, v);
  test_error (rc);
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  gsl_matrix_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP = = ([, ] COMPLEX, [, ] COMPLEX) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_eq (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u, *v;
  int rc;
  error_node = p;
  v = pop_matrix_complex (p, A_TRUE);
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_sub (u, v);
  test_error (rc);
  PUSH_BOOL (p, (gsl_matrix_complex_isnull (u) ? A_TRUE : A_FALSE));
  gsl_matrix_complex_free (u);
  gsl_matrix_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP /= = ([, ] COMPLEX, [, ] COMPLEX) BOOL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_ne (NODE_T * p)
{
  genie_matrix_complex_eq (p);
  genie_not_bool (p);
}

/*!
\brief OP +:= = (REF [, ] COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_plusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_COMPLEX), MODE (ROWROW_COMPLEX), genie_matrix_complex_add);
}

/*!
\brief OP -:= = (REF [, ] COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_minusab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_COMPLEX), MODE (ROWROW_COMPLEX), genie_matrix_complex_sub);
}

/*!
\brief OP * = ([] REAL, REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_scale_real (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  A68_REAL v;
  int rc;
  error_node = p;
  POP_REAL (p, &v);
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_scale (u, VALUE (&v));
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = (REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_real_scale_vector (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  A68_REAL v;
  int rc;
  error_node = p;
  u = pop_vector (p, A_TRUE);
  POP_REAL (p, &v);
  rc = gsl_vector_scale (u, VALUE (&v));
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] REAL, REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_scale_real (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u;
  A68_REAL v;
  int rc;
  error_node = p;
  POP_REAL (p, &v);
  u = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_scale (u, VALUE (&v));
  test_error (rc);
  push_matrix (p, u);
  gsl_matrix_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = (REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_real_scale_matrix (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u;
  A68_REAL v;
  int rc;
  error_node = p;
  u = pop_matrix (p, A_TRUE);
  POP_REAL (p, &v);
  rc = gsl_matrix_scale (u, VALUE (&v));
  test_error (rc);
  push_matrix (p, u);
  gsl_matrix_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([] COMPLEX, COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_scale_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  error_node = p;
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  u = pop_vector_complex (p, A_TRUE);
  gsl_blas_zscal (v, u);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = (COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_complex_scale_vector_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  error_node = p;
  u = pop_vector_complex (p, A_TRUE);
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  gsl_blas_zscal (v, u);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] COMPLEX, COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_scale_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  int rc;
  error_node = p;
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_scale (u, v);
  test_error (rc);
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = (COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_complex_scale_matrix_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  int rc;
  error_node = p;
  u = pop_matrix_complex (p, A_TRUE);
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  rc = gsl_matrix_complex_scale (u, v);
  test_error (rc);
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP *:= (REF [] REAL, REAL) REF [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_scale_real_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_REAL), MODE (REAL), genie_vector_scale_real);
}

/*!
\brief OP *:= (REF [, ] REAL, REAL) REF [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_scale_real_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_REAL), MODE (REAL), genie_matrix_scale_real);
}

/*!
\brief OP *:= (REF [] COMPLEX, COMPLEX) REF [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_scale_complex_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_COMPLEX), MODE (COMPLEX), genie_vector_complex_scale_complex);
}

/*!
\brief OP *:= (REF [, ] COMPLEX, COMPLEX) REF [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_scale_complex_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_COMPLEX), MODE (COMPLEX), genie_matrix_complex_scale_complex);
}

/*!
\brief OP / = ([] REAL, REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_div_real (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  A68_REAL v;
  int rc;
  error_node = p;
  POP_REAL (p, &v);
  if (VALUE (&v) == 0.0) {
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_DIVISION_BY_ZERO, MODE (ROW_REAL));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  u = pop_vector (p, A_TRUE);
  rc = gsl_vector_scale (u, 1.0 / VALUE (&v));
  test_error (rc);
  push_vector (p, u);
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP / = ([, ] REAL, REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_div_real (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u;
  A68_REAL v;
  int rc;
  error_node = p;
  POP_REAL (p, &v);
  if (VALUE (&v) == 0.0) {
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_DIVISION_BY_ZERO, MODE (ROWROW_REAL));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  u = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_scale (u, 1.0 / VALUE (&v));
  test_error (rc);
  push_matrix (p, u);
  gsl_matrix_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP / = ([] COMPLEX, COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_div_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  error_node = p;
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  if (VALUE (&re) == 0.0 && VALUE (&im) == 0.0) {
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_DIVISION_BY_ZERO, MODE (ROW_COMPLEX));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  u = pop_vector_complex (p, A_TRUE);
  v = gsl_complex_inverse (v);
  gsl_blas_zscal (v, u);
  push_vector_complex (p, u);
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP / = ([, ] COMPLEX, COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_div_complex (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *u;
  A68_REAL re, im;
  gsl_complex v;
  int rc;
  error_node = p;
  POP_REAL (p, &im);
  POP_REAL (p, &re);
  if (VALUE (&re) == 0.0 && VALUE (&im) == 0.0) {
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_DIVISION_BY_ZERO, MODE (ROWROW_COMPLEX));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  GSL_SET_COMPLEX (&v, VALUE (&re), VALUE (&im));
  v = gsl_complex_inverse (v);
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_scale (u, v);
  test_error (rc);
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP /:= (REF [] REAL, REAL) REF [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_div_real_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_REAL), MODE (REAL), genie_vector_div_real);
}

/*!
\brief OP /:= (REF [, ] REAL, REAL) REF [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_div_real_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_REAL), MODE (REAL), genie_matrix_div_real);
}

/*!
\brief OP /:= (REF [] COMPLEX, COMPLEX) REF [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_div_complex_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROW_COMPLEX), MODE (COMPLEX), genie_vector_complex_div_complex);
}

/*!
\brief OP /:= (REF [, ] COMPLEX, COMPLEX) REF [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_div_complex_ab (NODE_T * p)
{
  op_ab (p, MODE (REF_ROWROW_COMPLEX), MODE (COMPLEX), genie_matrix_complex_div_complex);
}

/*!
\brief OP * = ([] REAL, [] REAL) REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_dot (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u, *v;
  double w;
  int rc;
  error_node = p;
  v = pop_vector (p, A_TRUE);
  u = pop_vector (p, A_TRUE);
  rc = gsl_blas_ddot (u, v, &w);
  test_error (rc);
  PUSH_REAL (p, w);
  gsl_vector_free (u);
  gsl_vector_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([] COMPLEX, [] COMPLEX) COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_dot (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u, *v;
  gsl_complex w;
  int rc;
  error_node = p;
  v = pop_vector_complex (p, A_TRUE);
  u = pop_vector_complex (p, A_TRUE);
  rc = gsl_blas_zdotc (u, v, &w);
  test_error (rc);
  PUSH_REAL (p, GSL_REAL (w));
  PUSH_REAL (p, GSL_IMAG (w));
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP NORM = ([] REAL) REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_norm (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u;
  error_node = p;
  u = pop_vector (p, A_TRUE);
  PUSH_REAL (p, gsl_blas_dnrm2 (u));
  gsl_vector_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP NORM = ([] COMPLEX) COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_norm (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u;
  error_node = p;
  u = pop_vector_complex (p, A_TRUE);
  PUSH_REAL (p, gsl_blas_dznrm2 (u));
  gsl_vector_complex_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP DYAD = ([] REAL, [] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_dyad (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector *u, *v;
  gsl_matrix *w;
  int len1, len2, j, k;
  error_node = p;
  v = pop_vector (p, A_TRUE);
  u = pop_vector (p, A_TRUE);
  len1 = u->size;
  len2 = v->size;
  w = gsl_matrix_alloc (len1, len2);
  for (j = 0; j < len1; j++) {
    double uj = gsl_vector_get (u, j);
    for (k = 0; k < len2; k++) {
      double vk = gsl_vector_get (v, k);
      gsl_matrix_set (w, j, k, uj * vk);
    }
  }
  push_matrix (p, w);
  gsl_vector_free (u);
  gsl_vector_free (v);
  gsl_matrix_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP DYAD = ([] COMPLEX, [] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_dyad (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_vector_complex *u, *v;
  gsl_matrix_complex *w;
  int len1, len2, j, k;
  error_node = p;
  v = pop_vector_complex (p, A_TRUE);
  u = pop_vector_complex (p, A_TRUE);
  len1 = u->size;
  len2 = v->size;
  w = gsl_matrix_complex_alloc (len1, len2);
  for (j = 0; j < len1; j++) {
    gsl_complex uj = gsl_vector_complex_get (u, j);
    for (k = 0; k < len2; k++) {
      gsl_complex vk = gsl_vector_complex_get (u, k);
      gsl_matrix_complex_set (w, j, k, gsl_complex_mul (uj, vk));
    }
  }
  push_matrix_complex (p, w);
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  gsl_matrix_complex_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_times_vector (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len;
  int rc;
  gsl_vector *u, *v;
  gsl_matrix *w;
  error_node = p;
  u = pop_vector (p, A_TRUE);
  w = pop_matrix (p, A_TRUE);
  len = u->size;
  v = gsl_vector_alloc (len);
  gsl_vector_set_zero (v);
  rc = gsl_blas_dgemv (CblasNoTrans, 1.0, w, u, 0.0, v);
  test_error (rc);
  push_vector (p, v);
  gsl_vector_free (u);
  gsl_vector_free (v);
  gsl_matrix_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([] REAL, [, ] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_times_matrix (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len;
  int rc;
  gsl_vector *u, *v;
  gsl_matrix *w;
  error_node = p;
  w = pop_matrix (p, A_TRUE);
  rc = gsl_matrix_transpose (w);
  test_error (rc);
  u = pop_vector (p, A_TRUE);
  len = u->size;
  v = gsl_vector_alloc (len);
  gsl_vector_set_zero (v);
  rc = gsl_blas_dgemv (CblasNoTrans, 1.0, w, u, 0.0, v);
  test_error (rc);
  push_vector (p, v);
  gsl_vector_free (u);
  gsl_vector_free (v);
  gsl_matrix_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] REAL, [, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_times_matrix (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len1, len2;
  int rc;
  gsl_matrix *u, *v, *w;
  error_node = p;
  v = pop_matrix (p, A_TRUE);
  u = pop_matrix (p, A_TRUE);
  len1 = v->size2;
  len2 = u->size1;
  w = gsl_matrix_alloc (len1, len2);
  gsl_matrix_set_zero (w);
  rc = gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, u, v, 0.0, w);
  test_error (rc);
  push_matrix (p, w);
  gsl_matrix_free (u);
  gsl_matrix_free (v);
  gsl_matrix_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] COMPLEX, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_times_vector (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len, rc;
  gsl_vector_complex *u, *v;
  gsl_matrix_complex *w;
  gsl_complex zero, one;
  error_node = p;
  GSL_SET_COMPLEX (&zero, 0.0, 0.0);
  GSL_SET_COMPLEX (&one, 1.0, 0.0);
  u = pop_vector_complex (p, A_TRUE);
  w = pop_matrix_complex (p, A_TRUE);
  len = u->size;
  v = gsl_vector_complex_alloc (len);
  gsl_vector_complex_set_zero (v);
  rc = gsl_blas_zgemv (CblasNoTrans, one, w, u, zero, v);
  test_error (rc);
  push_vector_complex (p, v);
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  gsl_matrix_complex_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([] COMPLEX, [, ] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_vector_complex_times_matrix (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len, rc;
  gsl_vector_complex *u, *v;
  gsl_matrix_complex *w;
  gsl_complex zero, one;
  error_node = p;
  GSL_SET_COMPLEX (&zero, 0.0, 0.0);
  GSL_SET_COMPLEX (&one, 1.0, 0.0);
  w = pop_matrix_complex (p, A_TRUE);
  rc = gsl_matrix_complex_transpose (w);
  test_error (rc);
  u = pop_vector_complex (p, A_TRUE);
  len = u->size;
  v = gsl_vector_complex_alloc (len);
  gsl_vector_complex_set_zero (v);
  rc = gsl_blas_zgemv (CblasNoTrans, one, w, u, zero, v);
  test_error (rc);
  push_vector_complex (p, v);
  gsl_vector_complex_free (u);
  gsl_vector_complex_free (v);
  gsl_matrix_complex_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief OP * = ([, ] COMPLEX, [, ] COMPLEX) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_times_matrix (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  int len1, len2, rc;
  gsl_matrix_complex *u, *v, *w;
  gsl_complex zero, one;
  error_node = p;
  GSL_SET_COMPLEX (&zero, 0.0, 0.0);
  GSL_SET_COMPLEX (&one, 1.0, 0.0);
  v = pop_matrix_complex (p, A_TRUE);
  u = pop_matrix_complex (p, A_TRUE);
  len1 = v->size2;
  len2 = u->size1;
  w = gsl_matrix_complex_alloc (len1, len2);
  gsl_matrix_complex_set_zero (w);
  rc = gsl_blas_zgemm (CblasNoTrans, CblasNoTrans, one, u, v, zero, w);
  test_error (rc);
  push_matrix_complex (p, w);
  gsl_matrix_complex_free (u);
  gsl_matrix_complex_free (v);
  gsl_matrix_complex_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC lu decomp = ([, ] REAL, REF [] INT, REF INT) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_lu (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  A68_REF ref_signum, ref_q;
  gsl_permutation *q;
  gsl_matrix *u;
  int rc;
  A68_INT signum;
  error_node = p;
  POP_REF (p, &ref_signum);
  TEST_NIL (p, ref_signum, MODE (REF_INT));
  POP_REF (p, &ref_q);
  TEST_NIL (p, ref_q, MODE (REF_ROW_INT));
  PUSH_ROW (p, *DEREF (A68_ROW, &ref_q));
  q = pop_permutation (p, A_FALSE);
  u = pop_matrix (p, A_TRUE);
  rc = gsl_linalg_LU_decomp (u, q, &(signum.value));
  test_error (rc);
  signum.status = INITIALISED_MASK;
  *DEREF (A68_INT, &ref_signum) = signum;
  push_permutation (p, q);
  POP_ROW (p, DEREF (A68_ROW, &ref_q));
  push_matrix (p, u);
  gsl_matrix_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC lu det = ([, ] REAL, INT) REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_lu_det (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *lu;
  A68_INT signum;
  error_node = p;
  POP_INT (p, &signum);
  lu = pop_matrix (p, A_TRUE);
  PUSH_REAL (p, gsl_linalg_LU_det (lu, signum.value));
  gsl_matrix_free (lu);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC lu inv = ([, ] REAL, [] INT) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_lu_inv (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix *lu, *inv;
  int rc;
  error_node = p;
  q = pop_permutation (p, A_TRUE);
  lu = pop_matrix (p, A_TRUE);
  inv = gsl_matrix_alloc (lu->size1, lu->size2);
  rc = gsl_linalg_LU_invert (lu, q, inv);
  test_error (rc);
  push_matrix (p, inv);
  gsl_matrix_free (lu);
  gsl_matrix_free (inv);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC lu solve ([, ] REAL, [, ] REAL, [] INT, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_lu_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix *a, *lu;
  gsl_vector *b, *x, *r;
  int rc;
  error_node = p;
  b = pop_vector (p, A_TRUE);
  q = pop_permutation (p, A_TRUE);
  lu = pop_matrix (p, A_TRUE);
  a = pop_matrix (p, A_TRUE);
  x = gsl_vector_alloc (b->size);
  r = gsl_vector_alloc (b->size);
  rc = gsl_linalg_LU_solve (lu, q, b, x);
  test_error (rc);
  rc = gsl_linalg_LU_refine (a, lu, q, b, x, r);
  test_error (rc);
  push_vector (p, x);
  gsl_matrix_free (a);
  gsl_matrix_free (lu);
  gsl_vector_free (b);
  gsl_vector_free (r);
  gsl_vector_free (x);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex lu decomp = ([, ] COMPLEX, REF [] INT, REF INT) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_lu (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  A68_REF ref_signum, ref_q;
  gsl_permutation *q;
  gsl_matrix_complex *u;
  int rc;
  A68_INT signum;
  error_node = p;
  POP_REF (p, &ref_signum);
  TEST_NIL (p, ref_signum, MODE (REF_INT));
  POP_REF (p, &ref_q);
  TEST_NIL (p, ref_q, MODE (REF_ROW_INT));
  PUSH_ROW (p, *DEREF (A68_ROW, &ref_q));
  q = pop_permutation (p, A_FALSE);
  u = pop_matrix_complex (p, A_TRUE);
  rc = gsl_linalg_complex_LU_decomp (u, q, &(signum.value));
  test_error (rc);
  signum.status = INITIALISED_MASK;
  *DEREF (A68_INT, &ref_signum) = signum;
  push_permutation (p, q);
  POP_ROW (p, DEREF (A68_ROW, &ref_q));
  push_matrix_complex (p, u);
  gsl_matrix_complex_free (u);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex lu det = ([, ] COMPLEX, INT) COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_lu_det (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix_complex *lu;
  A68_INT signum;
  gsl_complex det;
  error_node = p;
  POP_INT (p, &signum);
  lu = pop_matrix_complex (p, A_TRUE);
  det = gsl_linalg_complex_LU_det (lu, signum.value);
  PUSH_REAL (p, GSL_REAL (det));
  PUSH_REAL (p, GSL_IMAG (det));
  gsl_matrix_complex_free (lu);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex lu inv = ([, ] COMPLEX, [] INT) [, ] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_lu_inv (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix_complex *lu, *inv;
  int rc;
  error_node = p;
  q = pop_permutation (p, A_TRUE);
  lu = pop_matrix_complex (p, A_TRUE);
  inv = gsl_matrix_complex_alloc (lu->size1, lu->size2);
  rc = gsl_linalg_complex_LU_invert (lu, q, inv);
  test_error (rc);
  push_matrix_complex (p, inv);
  gsl_matrix_complex_free (lu);
  gsl_matrix_complex_free (inv);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC complex lu solve ([, ] COMPLEX, [, ] COMPLEX, [] INT, [] COMPLEX) [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_complex_lu_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_permutation *q;
  gsl_matrix_complex *a, *lu;
  gsl_vector_complex *b, *x, *r;
  int rc;
  error_node = p;
  b = pop_vector_complex (p, A_TRUE);
  q = pop_permutation (p, A_TRUE);
  lu = pop_matrix_complex (p, A_TRUE);
  a = pop_matrix_complex (p, A_TRUE);
  x = gsl_vector_complex_alloc (b->size);
  r = gsl_vector_complex_alloc (b->size);
  rc = gsl_linalg_complex_LU_solve (lu, q, b, x);
  test_error (rc);
  rc = gsl_linalg_complex_LU_refine (a, lu, q, b, x, r);
  test_error (rc);
  push_vector_complex (p, x);
  gsl_matrix_complex_free (a);
  gsl_matrix_complex_free (lu);
  gsl_vector_complex_free (b);
  gsl_vector_complex_free (r);
  gsl_vector_complex_free (x);
  gsl_permutation_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC svd decomp = ([, ] REAL, REF [, ] REAL, REF [] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_svd (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a, *v;
  gsl_vector *s, *w;
  A68_REF ref_s, ref_v;
  int rc;
  error_node = p;
  POP_REF (p, &ref_s);
  TEST_NIL (p, ref_s, MODE (REF_ROW_REAL));
  PUSH_ROW (p, *DEREF (A68_ROW, &ref_s));
  s = pop_vector (p, A_FALSE);
  POP_REF (p, &ref_v);
  TEST_NIL (p, ref_v, MODE (REF_ROWROW_REAL));
  PUSH_ROW (p, *DEREF (A68_ROW, &ref_v));
  v = pop_matrix (p, A_FALSE);
  a = pop_matrix (p, A_TRUE);
  w = gsl_vector_alloc (v->size2);
  rc = gsl_linalg_SV_decomp (a, v, s, w);
  test_error (rc);
  push_vector (p, s);
  POP_ROW (p, DEREF (A68_ROW, &ref_s));
  push_matrix (p, v);
  POP_ROW (p, DEREF (A68_ROW, &ref_v));
  push_matrix (p, a);
  gsl_matrix_free (a);
  gsl_matrix_free (v);
  gsl_vector_free (s);
  gsl_vector_free (w);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC svd solve = ([, ] REAL, [, ] REAL, [] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_svd_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *u, *v;
  gsl_vector *s, *b, *x;
  int rc;
  error_node = p;
  b = pop_vector (p, A_TRUE);
  s = pop_vector (p, A_TRUE);
  v = pop_matrix (p, A_TRUE);
  u = pop_matrix (p, A_TRUE);
  x = gsl_vector_alloc (b->size);
  rc = gsl_linalg_SV_solve (u, v, s, b, x);
  push_vector (p, x);
  gsl_vector_free (x);
  gsl_vector_free (b);
  gsl_vector_free (s);
  gsl_matrix_free (v);
  gsl_matrix_free (u);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC qr decomp = ([, ] REAL, [] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_qr (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  gsl_vector *t;
  A68_REF ref_t;
  int rc;
  error_node = p;
  POP_REF (p, &ref_t);
  TEST_NIL (p, ref_t, MODE (REF_ROW_REAL));
  PUSH_ROW (p, *DEREF (A68_ROW, &ref_t));
  t = pop_vector (p, A_FALSE);
  a = pop_matrix (p, A_TRUE);
  rc = gsl_linalg_QR_decomp (a, t);
  test_error (rc);
  push_vector (p, t);
  POP_ROW (p, DEREF (A68_ROW, &ref_t));
  push_matrix (p, a);
  gsl_matrix_free (a);
  gsl_vector_free (t);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC qr solve = ([, ] REAL, [] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_qr_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *q;
  gsl_vector *t, *b, *x;
  int rc;
  error_node = p;
  b = pop_vector (p, A_TRUE);
  t = pop_vector (p, A_TRUE);
  q = pop_matrix (p, A_TRUE);
  x = gsl_vector_alloc (b->size);
  rc = gsl_linalg_QR_solve (q, t, b, x);
  push_vector (p, x);
  gsl_vector_free (x);
  gsl_vector_free (b);
  gsl_vector_free (t);
  gsl_matrix_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC qr ls solve = ([, ] REAL, [] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_qr_ls_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *q;
  gsl_vector *t, *b, *x, *r;
  int rc;
  error_node = p;
  b = pop_vector (p, A_TRUE);
  t = pop_vector (p, A_TRUE);
  q = pop_matrix (p, A_TRUE);
  r = gsl_vector_alloc (b->size);
  x = gsl_vector_alloc (b->size);
  rc = gsl_linalg_QR_lssolve (q, t, b, x, r);
  push_vector (p, x);
  gsl_vector_free (x);
  gsl_vector_free (r);
  gsl_vector_free (b);
  gsl_vector_free (t);
  gsl_matrix_free (q);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC cholesky decomp = ([, ] REAL) [, ] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_ch (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *a;
  int rc;
  error_node = p;
  a = pop_matrix (p, A_TRUE);
  rc = gsl_linalg_cholesky_decomp (a);
  test_error (rc);
  push_matrix (p, a);
  gsl_matrix_free (a);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC cholesky solve = ([, ] REAL, [] REAL) [] REAL
\param p position in the syntax tree, should not be NULL
**/

void genie_matrix_ch_solve (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (error_handler);
  gsl_matrix *c;
  gsl_vector *b, *x;
  int rc;
  error_node = p;
  b = pop_vector (p, A_TRUE);
  c = pop_matrix (p, A_TRUE);
  x = gsl_vector_alloc (b->size);
  rc = gsl_linalg_cholesky_solve (c, b, x);
  push_vector (p, x);
  gsl_vector_free (x);
  gsl_vector_free (b);
  gsl_matrix_free (c);
  (void) gsl_set_error_handler (save_handler);
}

#endif