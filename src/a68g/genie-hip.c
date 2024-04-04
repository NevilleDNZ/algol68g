//! @file genie-hip.c
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
//! Interpreter routines for jumps and SKIP.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"
#include "a68g-mp.h"
#include "a68g-double.h"

//! @brief Push an undefined value of the required mode.

void genie_push_undefined (NODE_T * p, MOID_T * u)
{
// For primitive modes we push an initialised value.
  if (u == M_VOID) {
    ;
  } else if (u == M_INT) {
    PUSH_VALUE (p, 1, A68_INT); // Because users write [~] INT !
  } else if (u == M_REAL) {
    PUSH_VALUE (p, (a68_unif_rand ()), A68_REAL);
  } else if (u == M_BOOL) {
    PUSH_VALUE (p, (BOOL_T) (a68_unif_rand () < 0.5), A68_BOOL);
  } else if (u == M_CHAR) {
    PUSH_VALUE (p, (char) (32 + 96 * a68_unif_rand ()), A68_CHAR);
  } else if (u == M_BITS) {
    PUSH_VALUE (p, (UNSIGNED_T) (a68_unif_rand () * (double) A68_MAX_BITS), A68_BITS);
  } else if (u == M_COMPLEX) {
    PUSH_COMPLEX (p, a68_unif_rand (), a68_unif_rand ());
  } else if (u == M_BYTES) {
    PUSH_BYTES (p, "SKIP");
  } else if (u == M_LONG_BYTES) {
    PUSH_LONG_BYTES (p, "SKIP");
  } else if (u == M_STRING) {
    PUSH_REF (p, empty_string (p));
  } else if (u == M_LONG_INT) {
#if (A68_LEVEL >= 3)
    DOUBLE_NUM_T w;
    set_lw (w, 1);
    PUSH_VALUE (p, w, A68_LONG_INT);    // Because users write [~] INT !
#else
    (void) nil_mp (p, DIGITS (u));
#endif
  } else if (u == M_LONG_REAL) {
#if (A68_LEVEL >= 3)
    genie_next_random_double (p);
#else
    (void) nil_mp (p, DIGITS (u));
#endif
  } else if (u == M_LONG_BITS) {
#if (A68_LEVEL >= 3)
    DOUBLE_NUM_T w;
    set_lw (w, 1);
    PUSH_VALUE (p, w, A68_LONG_BITS);   // Because users write [~] INT !
#else
    (void) nil_mp (p, DIGITS (u));
#endif
  } else if (u == M_LONG_LONG_INT) {
    (void) nil_mp (p, DIGITS (u));
  } else if (u == M_LONG_LONG_REAL) {
    (void) nil_mp (p, DIGITS (u));
  } else if (u == M_LONG_LONG_BITS) {
    (void) nil_mp (p, DIGITS (u));
  } else if (u == M_LONG_COMPLEX) {
#if (A68_LEVEL >= 3)
    genie_next_random_double (p);
    genie_next_random_double (p);
#else
    (void) nil_mp (p, DIGITS_COMPL (u));
    (void) nil_mp (p, DIGITS_COMPL (u));
#endif
  } else if (u == M_LONG_LONG_COMPLEX) {
    (void) nil_mp (p, DIGITS_COMPL (u));
    (void) nil_mp (p, DIGITS_COMPL (u));
  } else if (IS_REF (u)) {
// All REFs are NIL.
    PUSH_REF (p, nil_ref);
  } else if (IS_ROW (u) || IS_FLEX (u)) {
// [] AMODE or FLEX [] AMODE.
    A68_REF er = empty_row (p, u);
    STATUS (&er) |= SKIP_ROW_MASK;
    PUSH_REF (p, er);
  } else if (IS_STRUCT (u)) {
// STRUCT.
    for (PACK_T *v = PACK (u); v != NO_PACK; FORWARD (v)) {
      genie_push_undefined (p, MOID (v));
    }
  } else if (IS_UNION (u)) {
// UNION.
    ADDR_T pop_sp = A68_SP;
    PUSH_UNION (p, MOID (PACK (u)));
    genie_push_undefined (p, MOID (PACK (u)));
    A68_SP = pop_sp + SIZE (u);
  } else if (IS (u, PROC_SYMBOL)) {
// PROC.
    A68_PROCEDURE z;
    STATUS (&z) = (STATUS_MASK_T) (INIT_MASK | SKIP_PROCEDURE_MASK);
    (NODE (&BODY (&z))) = NO_NODE;
    ENVIRON (&z) = 0;
    LOCALE (&z) = NO_HANDLE;
    MOID (&z) = u;
    PUSH_PROCEDURE (p, z);
  } else if (u == M_FORMAT) {
// FORMAT etc. - what arbitrary FORMAT could mean anything at all?.
    A68_FORMAT z;
    STATUS (&z) = (STATUS_MASK_T) (INIT_MASK | SKIP_FORMAT_MASK);
    BODY (&z) = NO_NODE;
    ENVIRON (&z) = 0;
    PUSH_FORMAT (p, z);
  } else if (u == M_SIMPLOUT) {
    ADDR_T pop_sp = A68_SP;
    PUSH_UNION (p, M_STRING);
    PUSH_REF (p, c_to_a_string (p, "SKIP", DEFAULT_WIDTH));
    A68_SP = pop_sp + SIZE (u);
  } else if (u == M_SIMPLIN) {
    ADDR_T pop_sp = A68_SP;
    PUSH_UNION (p, M_REF_STRING);
    genie_push_undefined (p, M_REF_STRING);
    A68_SP = pop_sp + SIZE (u);
  } else if (u == M_REF_FILE) {
    PUSH_REF (p, A68 (skip_file));
  } else if (u == M_FILE) {
    A68_REF *z = (A68_REF *) STACK_TOP;
    int size = SIZE (M_FILE);
    ADDR_T pop_sp = A68_SP;
    PUSH_REF (p, A68 (skip_file));
    A68_SP = pop_sp;
    PUSH (p, ADDRESS (z), size);
  } else if (u == M_CHANNEL) {
    PUSH_OBJECT (p, A68 (skip_channel), A68_CHANNEL);
  } else if (u == M_PIPE) {
    genie_push_undefined (p, M_REF_FILE);
    genie_push_undefined (p, M_REF_FILE);
    genie_push_undefined (p, M_INT);
  } else if (u == M_SOUND) {
    A68_SOUND *z = (A68_SOUND *) STACK_TOP;
    int size = SIZE (M_SOUND);
    INCREMENT_STACK_POINTER (p, size);
    FILL (z, 0, size);
    STATUS (z) = INIT_MASK;
  } else {
    BYTE_T *tos = STACK_TOP;
    int size = SIZE_ALIGNED (u);
    INCREMENT_STACK_POINTER (p, size);
    FILL (tos, 0, size);
  }
}

//! @brief Push an undefined value of the required mode.

PROP_T genie_skip (NODE_T * p)
{
  PROP_T self;
  if (MOID (p) != M_VOID) {
    genie_push_undefined (p, MOID (p));
  }
  UNIT (&self) = genie_skip;
  SOURCE (&self) = p;
  return self;
}

//! @brief Jump to the serial clause where the label is at.

void genie_jump (NODE_T * p)
{
// Stack pointer and frame pointer were saved at target serial clause.
  NODE_T *jump = SUB (p);
  NODE_T *label = (IS (jump, GOTO_SYMBOL)) ? NEXT (jump) : jump;
  ADDR_T target_frame_pointer = A68_FP;
  jmp_buf *jump_stat = NO_JMP_BUF;
// Find the stack frame this jump points to.
  BOOL_T found = A68_FALSE;
  while (target_frame_pointer > 0 && !found) {
    found = (BOOL_T) ((TAG_TABLE (TAX (label)) == TABLE (FRAME_TREE (target_frame_pointer))) && FRAME_JUMP_STAT (target_frame_pointer) != NO_JMP_BUF);
    if (!found) {
      target_frame_pointer = FRAME_STATIC_LINK (target_frame_pointer);
    }
  }
// Beam us up, Scotty!.
#if defined (BUILD_PARALLEL_CLAUSE)
  {
    pthread_t target_id = FRAME_THREAD_ID (target_frame_pointer);
    if (SAME_THREAD (target_id, pthread_self ())) {
      jump_stat = FRAME_JUMP_STAT (target_frame_pointer);
      JUMP_TO (TAG_TABLE (TAX (label))) = UNIT (TAX (label));
      longjmp (*(jump_stat), 1);
    } else if (SAME_THREAD (target_id, A68_PAR (main_thread_id))) {
// A jump out of all parallel clauses back into the main program.
      genie_abend_all_threads (p, FRAME_JUMP_STAT (target_frame_pointer), label);
      ABEND (A68_TRUE, ERROR_INTERNAL_CONSISTENCY, __func__);
    } else {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_PARALLEL_JUMP);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
  }
#else
  jump_stat = FRAME_JUMP_STAT (target_frame_pointer);
  JUMP_TO (TAG_TABLE (TAX (label))) = UNIT (TAX (label));
  longjmp (*(jump_stat), 1);
#endif
}
