//! @file genie-misc.c
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
//! Miscellaneous interpreter routines.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"

#define VECTOR_SIZE 512
#define FD_READ 0
#define FD_WRITE 1

//! @brief Nop for the genie, for instance '+' for INT or REAL.

void genie_idle (NODE_T * p)
{
  (void) p;
}

//! @brief Unimplemented feature handler.

void genie_unimplemented (NODE_T * p)
{
  diagnostic (A68_RUNTIME_ERROR, p, ERROR_UNIMPLEMENTED);
  exit_genie (p, A68_RUNTIME_ERROR);
}

//! @brief PROC sleep = (INT) INT

void genie_sleep (NODE_T * p)
{
  A68_INT secs;
  POP_OBJECT (p, &secs, A68_INT);
  int wait = VALUE (&secs);
  PRELUDE_ERROR (wait < 0, p, ERROR_INVALID_ARGUMENT, M_INT);
  while (wait > 0) {
    wait = (int) sleep ((unt) wait);
  }
  PUSH_VALUE (p, (INT_T) 0, A68_INT);
}

//! @brief PROC system = (STRING) INT

void genie_system (NODE_T * p)
{
  A68_REF cmd;
  POP_REF (p, &cmd);
  CHECK_INIT (p, INITIALISED (&cmd), M_STRING);
  int size = 1 + a68_string_size (p, cmd);
  A68_REF ref_z = heap_generator (p, M_C_STRING, 1 + size);
  PUSH_VALUE (p, system (a_to_c_string (p, DEREF (char, &ref_z), cmd)), A68_INT);
}

//! @brief PROC (PROC VOID) VOID on gc event

void genie_on_gc_event (NODE_T * p)
{
  POP_PROCEDURE (p, &A68 (on_gc_event));
}

//! @brief Generic procedure for OP AND BECOMES (+:=, -:=, ...).

void genie_f_and_becomes (NODE_T * p, MOID_T * ref, GPROC * f)
{
  MOID_T *mode = SUB (ref);
  int size = SIZE (mode);
  BYTE_T *src = STACK_OFFSET (-size), *addr;
  A68_REF *dst = (A68_REF *) STACK_OFFSET (-size - A68_REF_SIZE);
  CHECK_REF (p, *dst, ref);
  addr = ADDRESS (dst);
  PUSH (p, addr, size);
  genie_check_initialisation (p, STACK_OFFSET (-size), mode);
  PUSH (p, src, size);
  (*f) (p);
  POP (p, addr, size);
  DECREMENT_STACK_POINTER (p, size);
}

//! @brief INT system heap pointer

void genie_system_heap_pointer (NODE_T * p)
{
  PUSH_VALUE (p, (int) (A68_HP), A68_INT);
}

//! @brief INT system stack pointer

void genie_system_stack_pointer (NODE_T * p)
{
  BYTE_T stack_offset;
  PUSH_VALUE (p, (int) (A68 (system_stack_offset) - &stack_offset), A68_INT);
}
