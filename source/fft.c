/*!
\file fft.c
\brief fft support through GSL
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2007 J. Marcel van der Veer <algol68g@xs4all.nl>.

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

#if defined ENABLE_NUMERICAL

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

/*
#include <gsl/gsl_blas.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_const.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_vector.h>
*/

#define VECTOR_OFFSET(a, t)\
  (((t)->lower_bound * (t)->span -(t)->shift + (a)->slice_offset) * (a)->elem_size + (a)->field_offset)

#define MATRIX_OFFSET(a, t1, t2)\
  (((t1)->lower_bound * (t1)->span -(t1)->shift + (t2)->lower_bound * (t2)->span -(t2)->shift + (a)->slice_offset) * (a)->elem_size + (a)->field_offset)

static NODE_T *error_node = NULL;

/*!
\brief map GSL error handler onto a68g error handler
\param reason error text
\param file gsl file where error occured
\param line line in above file
\param int gsl error number
**/

void fft_error_handler (const char *reason, const char *file, int line, int gsl_errno)
{
  if (line != 0) {
    snprintf (edit_line, BUFFER_SIZE, "%s in line %d of file %s", reason, line, file);
  } else {
    snprintf (edit_line, BUFFER_SIZE, "%s", reason);
  }
  diagnostic_node (A68_RUNTIME_ERROR, error_node, ERROR_FFT, edit_line, gsl_strerror (gsl_errno), NULL);
  exit_genie (error_node, A68_RUNTIME_ERROR);
}

/*!
\brief detect math errors
\param rc return code from function
**/

static void fft_test_error (int rc)
{
  if (rc != 0) {
    fft_error_handler ("math error", "", 0, rc);
  }
}

/*!
\brief pop [] REAL on the stack as complex double []
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return double []
**/

static double *pop_array_real (NODE_T * p, int *len)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int inc, index, k;
  BYTE_T *base;
  double *v;
  error_node = p;
/* Pop arguments. */
  POP_REF (p, &desc);
  CHECK_INIT (p, INITIALISED (&desc), MODE (ROW_REAL));
  CHECK_NIL (p, desc, MODE (ROW_REAL));
  GET_DESCRIPTOR (arr, tup, &desc);
  *len = ROW_SIZE (tup);
  if ((*len) == 0) {
    return (NULL);
  }
  v = malloc (2 * (*len) * sizeof (double));
  fft_test_error (v == NULL ? GSL_ENOMEM : GSL_SUCCESS);
  base = DEREF (BYTE_T, &(arr->array));
  index = VECTOR_OFFSET (arr, tup);
  inc = tup->span * arr->elem_size;
  for (k = 0; k < (*len); k++, index += inc) {
    A68_REAL *x = (A68_REAL *) (base + index);
    CHECK_INIT (p, INITIALISED (x), MODE (REAL));
    v[2 * k] = x->value;
    v[2 * k + 1] = 0.0;
  }
  return (v);
}

/*!
\brief push double [] on the stack as [] REAL
\param p position in the syntax tree, should not be NULL
**/

static void push_array_real (NODE_T * p, double *v, int len)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int inc, index, k;
  BYTE_T *base;
  error_node = p;
  desc = heap_generator (p, MODE (ROW_REAL), ALIGNED_SIZEOF (A68_ARRAY) + ALIGNED_SIZEOF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_REAL), len * ALIGNED_SIZEOF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (REAL);
  arr.elem_size = ALIGNED_SIZEOF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  tup.k = 0;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_REAL *x = (A68_REAL *) (base + index);
    x->status = INITIALISED_MASK;
    x->value = v[2 * k];
    TEST_REAL_REPRESENTATION (p, x->value);
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief pop [] COMPLEX on the stack as double []
\param p position in the syntax tree, should not be NULL
\param get whether to get elements from row in the stack
\return double []
**/

static double *pop_array_complex (NODE_T * p, int *len)
{
  A68_REF desc;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int inc, index, k;
  BYTE_T *base;
  double *v;
  error_node = p;
/* Pop arguments. */
  POP_REF (p, &desc);
  CHECK_INIT (p, INITIALISED (&desc), MODE (ROW_COMPLEX));
  CHECK_NIL (p, desc, MODE (ROW_COMPLEX));
  GET_DESCRIPTOR (arr, tup, &desc);
  *len = ROW_SIZE (tup);
  if ((*len) == 0) {
    return (NULL);
  }
  v = malloc (2 * (*len) * sizeof (double));
  fft_test_error (v == NULL ? GSL_ENOMEM : GSL_SUCCESS);
  base = DEREF (BYTE_T, &(arr->array));
  index = VECTOR_OFFSET (arr, tup);
  inc = tup->span * arr->elem_size;
  for (k = 0; k < (*len); k++, index += inc) {
    A68_REAL *re = (A68_REAL *) (base + index);
    A68_REAL *im = (A68_REAL *) (base + index + ALIGNED_SIZEOF (A68_REAL));
    CHECK_INIT (p, INITIALISED (re), MODE (COMPLEX));
    CHECK_INIT (p, INITIALISED (im), MODE (COMPLEX));
    v[2 * k] = re->value;
    v[2 * k + 1] = im->value;
  }
  return (v);
}

/*!
\brief push double [] on the stack as [] COMPLEX
\param p position in the syntax tree, should not be NULL
**/

static void push_array_complex (NODE_T * p, double *v, int len)
{
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int inc, index, k;
  BYTE_T *base;
  error_node = p;
  desc = heap_generator (p, MODE (ROW_COMPLEX), ALIGNED_SIZEOF (A68_ARRAY) + ALIGNED_SIZEOF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_COMPLEX), len * 2 * ALIGNED_SIZEOF (A68_REAL));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (COMPLEX);
  arr.elem_size = 2 * ALIGNED_SIZEOF (A68_REAL);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  tup.k = 0;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_REAL *re = (A68_REAL *) (base + index);
    A68_REAL *im = (A68_REAL *) (base + index + ALIGNED_SIZEOF (A68_REAL));
    re->status = INITIALISED_MASK;
    re->value = v[2 * k];
    im->status = INITIALISED_MASK;
    im->value = v[2 * k + 1];
    TEST_COMPLEX_REPRESENTATION (p, re->value, im->value);
  }
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
}

/*!
\brief push prime factorisation on the stack as [] INT
\param p position in the syntax tree, should not be NULL
**/

void genie_prime_factors (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  A68_INT n;
  A68_REF desc, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  int len, inc, index, k;
  BYTE_T *base;
  gsl_fft_complex_wavetable *wt;
  error_node = p;
  POP_OBJECT (p, &n, A68_INT);
  CHECK_INIT (p, INITIALISED (&n), MODE (INT));
  wt = gsl_fft_complex_wavetable_alloc (n.value);
  len = wt->nf;
  desc = heap_generator (p, MODE (ROW_INT), ALIGNED_SIZEOF (A68_ARRAY) + ALIGNED_SIZEOF (A68_TUPLE));
  PROTECT_SWEEP_HANDLE (&desc);
  row = heap_generator (p, MODE (ROW_INT), len * ALIGNED_SIZEOF (A68_INT));
  PROTECT_SWEEP_HANDLE (&row);
  arr.dimensions = 1;
  arr.type = MODE (INT);
  arr.elem_size = ALIGNED_SIZEOF (A68_INT);
  arr.slice_offset = arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = len;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  tup.k = 0;
  PUT_DESCRIPTOR (arr, tup, &desc);
  base = DEREF (BYTE_T, &(arr.array));
  index = VECTOR_OFFSET (&arr, &tup);
  inc = tup.span * arr.elem_size;
  for (k = 0; k < len; k++, index += inc) {
    A68_INT *x = (A68_INT *) (base + index);
    x->status = INITIALISED_MASK;
    x->value = (wt->factor)[k];
  }
  gsl_fft_complex_wavetable_free (wt);
  UNPROTECT_SWEEP_HANDLE (&desc);
  UNPROTECT_SWEEP_HANDLE (&row);
  PUSH_REF (p, desc);
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] COMPLEX) [] COMPLEX fft complex forward
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_complex_forward (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_complex (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_forward (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_complex (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] COMPLEX) [] COMPLEX fft complex backward
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_complex_backward (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_complex (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_backward (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_complex (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] COMPLEX) [] COMPLEX fft complex inverse
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_complex_inverse (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_complex (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_inverse (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_complex (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] REAL) [] COMPLEX fft forward
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_forward (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_real (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_forward (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_complex (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] COMPLEX) [] REAL fft backward
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_backward (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_complex (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_backward (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_real (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

/*!
\brief PROC ([] COMPLEX) [] REAL fft inverse
\param p position in the syntax tree, should not be NULL
**/

void genie_fft_inverse (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (fft_error_handler);
  int len, rc;
  double *data;
  gsl_fft_complex_wavetable *wt;
  gsl_fft_complex_workspace *ws;
  error_node = p;
  data = pop_array_complex (p, &len);
  fft_test_error (len == 0 ? GSL_EDOM : GSL_SUCCESS);
  wt = gsl_fft_complex_wavetable_alloc (len);
  ws = gsl_fft_complex_workspace_alloc (len);
  rc = gsl_fft_complex_inverse (data, 1, len, wt, ws);
  fft_test_error (rc);
  push_array_real (p, data, len);
  gsl_fft_complex_wavetable_free (wt);
  gsl_fft_complex_workspace_free (ws);
  if (data != NULL) {
    free (data);
  }
  (void) gsl_set_error_handler (save_handler);
}

#endif
