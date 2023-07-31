//! @file single-laplace.c
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
//! REAL GSL laplace routines.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"

#if defined (HAVE_GSL)

//! @brief Map GSL error handler onto a68g error handler.

static void laplace_error_handler (const char *reason, const char *file, int line, int gsl_errno)
{
  if (line != 0) {
    ASSERT (snprintf (A68 (edit_line), SNPRINTF_SIZE, "%s in line %d of file %s", reason, line, file) >= 0);
  } else {
    ASSERT (snprintf (A68 (edit_line), SNPRINTF_SIZE, "%s", reason) >= 0);
  }
  diagnostic (A68_RUNTIME_ERROR, A68 (f_entry), ERROR_LAPLACE, A68 (edit_line), gsl_strerror (gsl_errno));
  exit_genie (A68 (f_entry), A68_RUNTIME_ERROR);
}

//! @brief Detect math errors.

static void laplace_test_error (int ret)
{
  if (ret != 0) {
    laplace_error_handler ("math error", "", 0, ret);
  }
}

//! @brief PROC (PROC (REAL) REAL, REAL, REF REAL) REAL laplace

#define LAPLACE_DIVISIONS 1024

typedef struct A68_LAPLACE A68_LAPLACE;

struct A68_LAPLACE
{
  NODE_T *p;
  A68_PROCEDURE f;
  REAL_T s;
};

//! @brief Evaluate function for Laplace transform.

static REAL_T laplace_f (REAL_T t, void *z)
{
  A68_LAPLACE *l = (A68_LAPLACE *) z;
  ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
  MOID_T *u = M_PROC_REAL_REAL;
  A68_REAL *ft = (A68_REAL *) STACK_TOP;
  PUSH_VALUE (P (l), t, A68_REAL);
  genie_call_procedure (P (l), MOID (&(F (l))), u, u, &(F (l)), pop_sp, pop_fp);
  A68_SP = pop_sp;
  return VALUE (ft) * a68_exp (-(S (l)) * t);
}

//! @brief PROC laplace = (PROC (REAL) REAL, REAL, REF REAL) REAL

void genie_laplace (NODE_T * p)
{
  gsl_error_handler_t *save_handler = gsl_set_error_handler (laplace_error_handler);
  A68_REF ref_err;
  POP_REF (p, &ref_err);
  CHECK_REF (p, ref_err, M_REF_REAL);
  A68_REAL *err = (A68_REAL *) ADDRESS (&ref_err);
  A68_REAL s;
  POP_OBJECT (p, &s, A68_REAL);
  A68_PROCEDURE f;
  POP_PROCEDURE (p, &f);
  A68_LAPLACE l;
  P (&l) = p;
  F (&l) = f;
  S (&l) = VALUE (&s);
  gsl_function g;
  FUNCTION (&g) = &laplace_f;
  GSL_PARAMS (&g) = &l;
  gsl_integration_workspace *w = gsl_integration_workspace_alloc (LAPLACE_DIVISIONS);
  REAL_T res, estimated_err; int ret;
  if (VALUE (err) >= 0.0) {
    ret = gsl_integration_qagiu (&g, 0.0, VALUE (err), 0.0, LAPLACE_DIVISIONS, w, &res, &estimated_err);
  } else {
    ret = gsl_integration_qagiu (&g, 0.0, 0.0, -VALUE (err), LAPLACE_DIVISIONS, w, &res, &estimated_err);
  }
  laplace_test_error (ret);
  VALUE (err) = estimated_err;
  PUSH_VALUE (p, res, A68_REAL);
  gsl_integration_workspace_free (w);
  (void) gsl_set_error_handler (save_handler);
}

#endif
