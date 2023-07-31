//! @file genie-identifier.c
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

//! @brief Push a local identifier.

PROP_T genie_frame_identifier (NODE_T * p)
{
  BYTE_T *z;
  FRAME_GET (z, BYTE_T, p);
  PUSH (p, z, SIZE (MOID (p)));
  return GPROP (p);
}

//! @brief Push standard environ routine as PROC.

PROP_T genie_identifier_standenv_proc (NODE_T * p)
{
  A68_PROCEDURE z;
  TAG_T *q = TAX (p);
  STATUS (&z) = (STATUS_MASK_T) (INIT_MASK | STANDENV_PROC_MASK);
  PROCEDURE (&(BODY (&z))) = PROCEDURE (q);
  ENVIRON (&z) = 0;
  LOCALE (&z) = NO_HANDLE;
  MOID (&z) = MOID (p);
  PUSH_PROCEDURE (p, z);
  return GPROP (p);
}

//! @brief (optimised) push identifier from standard environ

PROP_T genie_identifier_standenv (NODE_T * p)
{
  (void) ((*(PROCEDURE (TAX (p)))) (p));
  return GPROP (p);
}

//! @brief Push identifier onto the stack.

PROP_T genie_identifier (NODE_T * p)
{
  static PROP_T self;
  TAG_T *q = TAX (p);
  SOURCE (&self) = p;
  if (A68_STANDENV_PROC (q)) {
    if (IS (MOID (q), PROC_SYMBOL)) {
      (void) genie_identifier_standenv_proc (p);
      UNIT (&self) = genie_identifier_standenv_proc;
    } else {
      (void) genie_identifier_standenv (p);
      UNIT (&self) = genie_identifier_standenv;
    }
  } else if (STATUS_TEST (q, CONSTANT_MASK)) {
    int size = SIZE (MOID (p));
    BYTE_T *sp_0 = STACK_TOP;
    (void) genie_frame_identifier (p);
    CONSTANT (GINFO (p)) = (void *) get_heap_space ((size_t) size);
    SIZE (GINFO (p)) = size;
    COPY (CONSTANT (GINFO (p)), (void *) sp_0, size);
    UNIT (&self) = genie_constant;
  } else {
    (void) genie_frame_identifier (p);
    UNIT (&self) = genie_frame_identifier;
  }
  return self;
}
