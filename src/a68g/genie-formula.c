//! @file genie-formula.c
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
//! Interpreter driver.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"
#include "a68g-mp.h"
#include "a68g-double.h"
#include "a68g-parser.h"
#include "a68g-transput.h"

//! @brief Push equality of two REFs.

PROP_T genie_identity_relation (NODE_T * p)
{
  PROP_T self;
  NODE_T *lhs = SUB (p), *rhs = NEXT_NEXT (lhs);
  A68_REF x, y;
  UNIT (&self) = genie_identity_relation;
  SOURCE (&self) = p;
  GENIE_UNIT (lhs);
  POP_REF (p, &y);
  GENIE_UNIT (rhs);
  POP_REF (p, &x);
  if (IS (NEXT_SUB (p), IS_SYMBOL)) {
    PUSH_VALUE (p, (BOOL_T) (ADDRESS (&x) == ADDRESS (&y)), A68_BOOL);
  } else {
    PUSH_VALUE (p, (BOOL_T) (ADDRESS (&x) != ADDRESS (&y)), A68_BOOL);
  }
  return self;
}

//! @brief Push result of ANDF.

PROP_T genie_and_function (NODE_T * p)
{
  PROP_T self;
  A68_BOOL x;
  GENIE_UNIT (SUB (p));
  POP_OBJECT (p, &x, A68_BOOL);
  if (VALUE (&x) == A68_TRUE) {
    GENIE_UNIT (NEXT_NEXT (SUB (p)));
  } else {
    PUSH_VALUE (p, A68_FALSE, A68_BOOL);
  }
  UNIT (&self) = genie_and_function;
  SOURCE (&self) = p;
  return self;
}

//! @brief Push result of ORF.

PROP_T genie_or_function (NODE_T * p)
{
  PROP_T self;
  A68_BOOL x;
  GENIE_UNIT (SUB (p));
  POP_OBJECT (p, &x, A68_BOOL);
  if (VALUE (&x) == A68_FALSE) {
    GENIE_UNIT (NEXT_NEXT (SUB (p)));
  } else {
    PUSH_VALUE (p, A68_TRUE, A68_BOOL);
  }
  UNIT (&self) = genie_or_function;
  SOURCE (&self) = p;
  return self;
}

//! @brief Call operator.

void genie_call_operator (NODE_T * p, ADDR_T pop_sp)
{
  A68_PROCEDURE *z;
  ADDR_T pop_fp = A68_FP;
  MOID_T *pr_mode = MOID (TAX (p));
  FRAME_GET (z, A68_PROCEDURE, p);
  genie_call_procedure (p, pr_mode, MOID (z), pr_mode, z, pop_sp, pop_fp);
  STACK_DNS (p, SUB (pr_mode), A68_FP);
}

//! @brief Push result of monadic formula OP "u".

PROP_T genie_monadic (NODE_T * p)
{
  NODE_T *op = SUB (p);
  NODE_T *u = NEXT (op);
  PROP_T self;
  ADDR_T sp = A68_SP;
  GENIE_UNIT (u);
  STACK_DNS (u, MOID (u), A68_FP);
  if (PROCEDURE (TAX (op)) != NO_GPROC) {
    (void) ((*(PROCEDURE (TAX (op)))) (op));
  } else {
    genie_call_operator (op, sp);
  }
  UNIT (&self) = genie_monadic;
  SOURCE (&self) = p;
  return self;
}

//! @brief Push result of formula.

PROP_T genie_dyadic_quick (NODE_T * p)
{
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  NODE_T *v = NEXT (op);
  GENIE_UNIT (u);
  STACK_DNS (u, MOID (u), A68_FP);
  GENIE_UNIT (v);
  STACK_DNS (v, MOID (v), A68_FP);
  (void) ((*(PROCEDURE (TAX (op)))) (op));
  return GPROP (p);
}

//! @brief Push result of formula.

PROP_T genie_dyadic (NODE_T * p)
{
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  NODE_T *v = NEXT (op);
  ADDR_T pop_sp = A68_SP;
  GENIE_UNIT (u);
  STACK_DNS (u, MOID (u), A68_FP);
  GENIE_UNIT (v);
  STACK_DNS (v, MOID (v), A68_FP);
  if (PROCEDURE (TAX (op)) != NO_GPROC) {
    (void) ((*(PROCEDURE (TAX (op)))) (op));
  } else {
    genie_call_operator (op, pop_sp);
  }
  return GPROP (p);
}

//! @brief Push result of formula.

PROP_T genie_formula (NODE_T * p)
{
  PROP_T self, lhs, rhs;
  NODE_T *u = SUB (p);
  NODE_T *op = NEXT (u);
  ADDR_T pop_sp = A68_SP;
  UNIT (&self) = genie_formula;
  SOURCE (&self) = p;
  GENIE_UNIT_2 (u, lhs);
  STACK_DNS (u, MOID (u), A68_FP);
  if (op != NO_NODE) {
    NODE_T *v = NEXT (op);
    GPROC *proc = PROCEDURE (TAX (op));
    GENIE_UNIT_2 (v, rhs);
    STACK_DNS (v, MOID (v), A68_FP);
    UNIT (&self) = genie_dyadic;
    if (proc != NO_GPROC) {
      (void) ((*(proc)) (op));
      UNIT (&self) = genie_dyadic_quick;
    } else {
      genie_call_operator (op, pop_sp);
    }
    return self;
  } else if (UNIT (&lhs) == genie_monadic) {
    return lhs;
  }
  (void) rhs;
  return self;
}
