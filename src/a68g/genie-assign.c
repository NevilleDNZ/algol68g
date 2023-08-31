//! @file genie-assign.c
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
//! Interpreter routines for assignations.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"

//! @brief Assign a value to a name and voiden.

PROP_T genie_voiding_assignation_constant (NODE_T * p)
{
  NODE_T *dst = SUB (p);
  NODE_T *src = SOURCE (&PROP (GINFO (NEXT_NEXT (dst))));
  ADDR_T pop_sp = A68_SP;
  A68_REF *z = (A68_REF *) STACK_TOP;
  PROP_T self;
  UNIT (&self) = genie_voiding_assignation_constant;
  SOURCE (&self) = p;
  GENIE_UNIT (dst);
  CHECK_REF (p, *z, MOID (p));
  COPY (ADDRESS (z), CONSTANT (GINFO (src)), SIZE (GINFO (src)));
  A68_SP = pop_sp;
  return self;
}

//! @brief Assign a value to a name and voiden.

PROP_T genie_voiding_assignation (NODE_T * p)
{
  NODE_T *dst = SUB (p);
  NODE_T *src = NEXT_NEXT (dst);
  MOID_T *src_mode = SUB_MOID (dst);
  ADDR_T pop_sp = A68_SP, pop_fp = FRAME_DNS (A68_FP);
  A68_REF z;
  PROP_T self;
  UNIT (&self) = genie_voiding_assignation;
  SOURCE (&self) = p;
  GENIE_UNIT (dst);
  POP_OBJECT (p, &z, A68_REF);
  CHECK_REF (p, z, MOID (p));
  FRAME_DNS (A68_FP) = REF_SCOPE (&z);
  GENIE_UNIT (src);
  STACK_DNS (src, src_mode, REF_SCOPE (&z));
  FRAME_DNS (A68_FP) = pop_fp;
  A68_SP = pop_sp;
  if (HAS_ROWS (src_mode)) {
    genie_clone_stack (p, src_mode, &z, &z);
  } else {
    COPY_ALIGNED (ADDRESS (&z), STACK_TOP, SIZE (src_mode));
  }
  return self;
}

//! @brief Assign a value to a name and push the name.

PROP_T genie_assignation_constant (NODE_T * p)
{
  NODE_T *dst = SUB (p);
  NODE_T *src = SOURCE (&PROP (GINFO (NEXT_NEXT (dst))));
  A68_REF *z = (A68_REF *) STACK_TOP;
  PROP_T self;
  UNIT (&self) = genie_assignation_constant;
  SOURCE (&self) = p;
  GENIE_UNIT (dst);
  CHECK_REF (p, *z, MOID (p));
  COPY (ADDRESS (z), CONSTANT (GINFO (src)), SIZE (GINFO (src)));
  return self;
}

//! @brief Assign a value to a name and push the name.

PROP_T genie_assignation_quick (NODE_T * p)
{
  NODE_T *dst = SUB (p);
  NODE_T *src = NEXT_NEXT (dst);
  MOID_T *src_mode = SUB_MOID (dst);
  int size = SIZE (src_mode);
  ADDR_T pop_fp = FRAME_DNS (A68_FP);
  A68_REF *z = (A68_REF *) STACK_TOP;
  GENIE_UNIT (dst);
  CHECK_REF (p, *z, MOID (p));
  FRAME_DNS (A68_FP) = REF_SCOPE (z);
  GENIE_UNIT (src);
  STACK_DNS (src, src_mode, REF_SCOPE (z));
  FRAME_DNS (A68_FP) = pop_fp;
  DECREMENT_STACK_POINTER (p, size);
  if (HAS_ROWS (src_mode)) {
    genie_clone_stack (p, src_mode, z, z);
  } else {
    COPY (ADDRESS (z), STACK_TOP, size);
  }
  PROP_T self;
  UNIT (&self) = genie_assignation_quick;
  SOURCE (&self) = p;
  return self;
}

//! @brief Assign a value to a name and push the name.

PROP_T genie_assignation (NODE_T * p)
{
  PROP_T self, srp;
  NODE_T *dst = SUB (p);
  NODE_T *src = NEXT_NEXT (dst);
  MOID_T *src_mode = SUB_MOID (dst);
  int size = SIZE (src_mode);
  ADDR_T pop_fp = FRAME_DNS (A68_FP);
  A68_REF *z = (A68_REF *) STACK_TOP;
  GENIE_UNIT (dst);
  CHECK_REF (p, *z, MOID (p));
  FRAME_DNS (A68_FP) = REF_SCOPE (z);
  GENIE_UNIT_2 (src, srp);
  STACK_DNS (src, src_mode, REF_SCOPE (z));
  FRAME_DNS (A68_FP) = pop_fp;
  DECREMENT_STACK_POINTER (p, size);
  if (HAS_ROWS (src_mode)) {
    genie_clone_stack (p, src_mode, z, z);
    UNIT (&self) = genie_assignation;
  } else {
    COPY (ADDRESS (z), STACK_TOP, size);
    if (UNIT (&srp) == genie_constant) {
      UNIT (&self) = genie_assignation_constant;
    } else {
      UNIT (&self) = genie_assignation_quick;
    }
  }
  SOURCE (&self) = p;
  return self;
}
