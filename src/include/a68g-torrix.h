//! @file a68g-torrix.h
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
//! REAL vector and matrix support.

#if ! defined (__A68G_TORRIX_H__)
#define __A68G_TORRIX_H__

#include "a68g-genie.h"
#include "a68g-prelude.h"

#if defined (HAVE_GSL)

#define NO_REAL_MATRIX ((gsl_matrix *) NULL)
#define NO_REF_MATRIX ((gsl_matrix **) NULL)
#define NO_REAL_VECTOR ((gsl_vector *) NULL)
#define NO_REF_VECTOR ((gsl_vector **) NULL)

#define ASSERT_GSL(f) {\
  int _rc_ = (f);\
  if (_rc_ != 0) {\
    BUFFER txt;\
    ASSERT (a68_bufprt (txt, SNPRINTF_SIZE, "%s: %d: math error", __FILE__, __LINE__) >= 0);\
    torrix_error_handler (txt, "", 0, _rc_);\
  }}

A68_ROW matrix_to_row (NODE_T *, gsl_matrix *);
A68_ROW vector_to_row (NODE_T *, gsl_vector *);
gsl_matrix_complex *pop_matrix_complex (NODE_T *, BOOL_T);
gsl_matrix *compute_pca_cv (NODE_T *, gsl_vector **, gsl_matrix *);
gsl_matrix *compute_pca_svd (NODE_T *, gsl_vector **, gsl_matrix *);
gsl_matrix *compute_pca_svd_pad (NODE_T *, gsl_vector **, gsl_matrix *);
gsl_matrix *matrix_hcat (NODE_T *, gsl_matrix *, gsl_matrix *);
gsl_matrix *matrix_vcat (NODE_T *, gsl_matrix *, gsl_matrix *);
gsl_matrix *pop_matrix (NODE_T *, BOOL_T);
gsl_permutation *pop_permutation (NODE_T *, BOOL_T);
gsl_vector_complex *pop_vector_complex (NODE_T *, BOOL_T);
gsl_vector *pop_vector (NODE_T *, BOOL_T);
REAL_T matrix_norm (gsl_matrix *);
void compute_pseudo_inverse (NODE_T *, gsl_matrix **, gsl_matrix *, REAL_T);
void print_matrix (gsl_matrix *, unt);
void print_vector (gsl_vector *, unt);
void push_matrix_complex (NODE_T *, gsl_matrix_complex *);
void push_matrix (NODE_T *, gsl_matrix *);
void push_permutation (NODE_T *, gsl_permutation *);
void push_vector_complex (NODE_T *, gsl_vector_complex *);
void push_vector (NODE_T *, gsl_vector *);
void torrix_error_handler (const char *, const char *, int, int);
void torrix_test_error (int);

// BLAS support

#define FLIP ((CBLAS_TRANSPOSE_t) CblasTrans)
#define SELF ((CBLAS_TRANSPOSE_t) CblasNoTrans)

void a68_vector_free (gsl_vector *);
void a68_matrix_free (gsl_matrix *);
void a68_dgemm (CBLAS_TRANSPOSE_t, CBLAS_TRANSPOSE_t, double, gsl_matrix *, gsl_matrix *, double, gsl_matrix **);
gsl_matrix *mat_before_ab (NODE_T *, gsl_matrix *, gsl_matrix *);
gsl_matrix *mat_over_ab (NODE_T *, gsl_matrix *, gsl_matrix *);

#endif

#endif
