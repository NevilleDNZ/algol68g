//! @file genie-declaration.c
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
//! Interpreter routines for declarations.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"

//! @brief Execute identity declaration.

void genie_identity_dec (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (ISNT (p, DEFINING_IDENTIFIER)) {
      genie_identity_dec (SUB (p));
    } else {
      A68_REF loc;
      NODE_T *src = NEXT_NEXT (p);
      MOID_T *src_mode = MOID (p);
      unt size = (unt) SIZE (src_mode);
      BYTE_T *tos = STACK_TOP;
      ADDR_T pop_sp = A68_SP;
      ADDR_T pop_dns = FRAME_DNS (A68_FP);
      FRAME_DNS (A68_FP) = A68_FP;
      GENIE_UNIT_TRACE (src);
      genie_check_initialisation (src, tos, src_mode);
      STACK_DNS (src, src_mode, A68_FP);
      FRAME_DNS (A68_FP) = pop_dns;
// Make a temporary REF to the object in the frame.
      STATUS (&loc) = (STATUS_MASK_T) (INIT_MASK | IN_FRAME_MASK);
      REF_HANDLE (&loc) = (A68_HANDLE *) & nil_handle;
      OFFSET (&loc) = A68_FP + FRAME_INFO_SIZE + OFFSET (TAX (p));
      REF_SCOPE (&loc) = A68_FP;
      ABEND (ADDRESS (&loc) != FRAME_OBJECT (OFFSET (TAX (p))), ERROR_INTERNAL_CONSISTENCY, __func__);
// Initialise the tag, value is in the stack.
      if (HAS_ROWS (src_mode)) {
        A68_SP = pop_sp;
        genie_clone_stack (p, src_mode, &loc, (A68_REF *) & nil_ref);
      } else if (UNIT (&GPROP (src)) == genie_constant) {
        STATUS_SET (TAX (p), CONSTANT_MASK);
        POP_ALIGNED (p, ADDRESS (&loc), size);
      } else {
        POP_ALIGNED (p, ADDRESS (&loc), size);
      }
      return;
    }
  }
}

//! @brief Execute variable declaration.

void genie_variable_dec (NODE_T * p, NODE_T ** declarer, ADDR_T sp)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, VARIABLE_DECLARATION)) {
      genie_variable_dec (SUB (p), declarer, sp);
    } else {
      if (IS (p, DECLARER)) {
        (*declarer) = SUB (p);
        genie_generator_bounds (*declarer);
        FORWARD (p);
      }
      if (IS (p, DEFINING_IDENTIFIER)) {
        MOID_T *ref_mode = MOID (p);
        TAG_T *tag = TAX (p);
        LEAP_T leap = (HEAP (tag) == LOC_SYMBOL ? LOC_SYMBOL : HEAP_SYMBOL);
        A68_REF *z;
        MOID_T *src_mode = SUB_MOID (p);
        z = (A68_REF *) (FRAME_OBJECT (OFFSET (TAX (p))));
        genie_generator_internal (*declarer, ref_mode, BODY (tag), leap, sp);
        POP_REF (p, z);
        if (NEXT (p) != NO_NODE && IS (NEXT (p), ASSIGN_SYMBOL)) {
          NODE_T *src = NEXT_NEXT (p);
          ADDR_T pop_sp = A68_SP;
          ADDR_T pop_dns = FRAME_DNS (A68_FP);
          FRAME_DNS (A68_FP) = A68_FP;
          GENIE_UNIT_TRACE (src);
          STACK_DNS (src, src_mode, A68_FP);
          FRAME_DNS (A68_FP) = pop_dns;
          A68_SP = pop_sp;
          if (HAS_ROWS (src_mode)) {
            genie_clone_stack (p, src_mode, z, z);
          } else {
            MOVE (ADDRESS (z), STACK_TOP, (unt) SIZE (src_mode));
          }
        }
      }
    }
  }
}

//! @brief Execute PROC variable declaration.

void genie_proc_variable_dec (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case DEFINING_IDENTIFIER: {
        ADDR_T sp_for_voiding = A68_SP;
        MOID_T *ref_mode = MOID (p);
        TAG_T *tag = TAX (p);
        LEAP_T leap = (HEAP (tag) == LOC_SYMBOL ? LOC_SYMBOL : HEAP_SYMBOL);
        A68_REF *z = (A68_REF *) (FRAME_OBJECT (OFFSET (TAX (p))));
        genie_generator_internal (p, ref_mode, BODY (tag), leap, A68_SP);
        POP_REF (p, z);
        if (NEXT (p) != NO_NODE && IS (NEXT (p), ASSIGN_SYMBOL)) {
          MOID_T *src_mode = SUB_MOID (p);
          ADDR_T pop_sp = A68_SP;
          ADDR_T pop_dns = FRAME_DNS (A68_FP);
          FRAME_DNS (A68_FP) = A68_FP;
          GENIE_UNIT_TRACE (NEXT_NEXT (p));
          STACK_DNS (p, SUB (ref_mode), A68_FP);
          FRAME_DNS (A68_FP) = pop_dns;
          A68_SP = pop_sp;
          MOVE (ADDRESS (z), STACK_TOP, (unt) SIZE (src_mode));
        }
        A68_SP = sp_for_voiding;        // Voiding
        return;
      }
    default: {
        genie_proc_variable_dec (SUB (p));
        break;
      }
    }
  }
}

//! @brief Execute operator declaration.

void genie_operator_dec (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case DEFINING_OPERATOR: {
        A68_PROCEDURE *z = (A68_PROCEDURE *) (FRAME_OBJECT (OFFSET (TAX (p))));
        ADDR_T pop_dns = FRAME_DNS (A68_FP);
        FRAME_DNS (A68_FP) = A68_FP;
        GENIE_UNIT_TRACE (NEXT_NEXT (p));
        STACK_DNS (p, MOID (p), A68_FP);
        FRAME_DNS (A68_FP) = pop_dns;
        POP_PROCEDURE (p, z);
        return;
      }
    default: {
        genie_operator_dec (SUB (p));
        break;
      }
    }
  }
}

//! @brief Execute declaration.

void genie_declaration (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case MODE_DECLARATION:
    case PROCEDURE_DECLARATION:
    case BRIEF_OPERATOR_DECLARATION:
    case PRIORITY_DECLARATION: {
// Already resolved.
        return;
      }
    case IDENTITY_DECLARATION: {
        genie_identity_dec (SUB (p));
        break;
      }
    case OPERATOR_DECLARATION: {
        genie_operator_dec (SUB (p));
        break;
      }
    case VARIABLE_DECLARATION: {
        NODE_T *declarer = NO_NODE;
        ADDR_T pop_sp = A68_SP;
        genie_variable_dec (SUB (p), &declarer, A68_SP);
// Voiding to remove garbage from declarers.
        A68_SP = pop_sp;
        break;
      }
    case PROCEDURE_VARIABLE_DECLARATION: {
        ADDR_T pop_sp = A68_SP;
        genie_proc_variable_dec (SUB (p));
        A68_SP = pop_sp;
        break;
      }
    default: {
        genie_declaration (SUB (p));
        break;
      }
    }
  }
}
