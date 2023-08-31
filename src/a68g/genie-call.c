//! @file genie-call.c
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
//! Interpreter routines for procedure calls.

// Algol 68 Genie implements Charles Lindsey's proposal for partial parametrization.
// A procedure has a locale to store parameters until the pack is complete, and only
// then the procedure is actually called.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-frames.h"
#include "a68g-prelude.h"
#include "a68g-transput.h"

void genie_argument (NODE_T * p, NODE_T ** seq)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, UNIT)) {
      GENIE_UNIT_NO_GC (p);
      STACK_DNS (p, MOID (p), A68_FP);
      SEQUENCE (*seq) = p;
      (*seq) = p;
      return;
    } else if (IS (p, TRIMMER)) {
      return;
    } else {
      genie_argument (SUB (p), seq);
    }
  }
}

//! @brief Evaluate partial call.

void genie_partial_call (NODE_T * p, MOID_T * p_mode, MOID_T * pproc, MOID_T * pmap, A68_PROCEDURE z, ADDR_T pop_sp, ADDR_T pop_fp)
{
// Get or make locale for the new procedure descriptor.
  A68_REF ref; A68_HANDLE *locale;
  if (LOCALE (&z) == NO_HANDLE) {
    int size = 0;
    for (PACK_T *s = PACK (p_mode); s != NO_PACK; FORWARD (s)) {
      size += (SIZE (M_BOOL) + SIZE (MOID (s)));
    }
    ref = heap_generator (p, p_mode, size);
    locale = REF_HANDLE (&ref);
  } else {
    int size = SIZE (LOCALE (&z));
    ref = heap_generator (p, p_mode, size);
    locale = REF_HANDLE (&ref);
    COPY (POINTER (locale), POINTER (LOCALE (&z)), size);
  }
// Move arguments from stack to locale using pmap.
  BYTE_T *u = POINTER (locale), *v = STACK_ADDRESS (pop_sp);
// Uninitialised arguments are VOID.
  int voids = 0;
  PACK_T *s = PACK (p_mode);
  for (PACK_T *t = PACK (pmap); t != NO_PACK && s != NO_PACK; FORWARD (t)) {
// Skip already initialised arguments.
    while (u != NULL && VALUE ((A68_BOOL *) & u[0])) {
      u = &(u[SIZE (M_BOOL) + SIZE (MOID (s))]);
      FORWARD (s);
    }
    if (u != NULL && MOID (t) == M_VOID) {
// Move to next field in locale.
      voids++;
      u = &(u[SIZE (M_BOOL) + SIZE (MOID (s))]);
      FORWARD (s);
    } else {
// Move argument from stack to locale.
      A68_BOOL w;
      STATUS (&w) = INIT_MASK;
      VALUE (&w) = A68_TRUE;
      *(A68_BOOL *) & u[0] = w;
      COPY (&(u[SIZE (M_BOOL)]), v, SIZE (MOID (t)));
      u = &(u[SIZE (M_BOOL) + SIZE (MOID (s))]);
      v = &(v[SIZE (MOID (t))]);
      FORWARD (s);
    }
  }
  A68_SP = pop_sp;
  LOCALE (&z) = locale;
// When closure is complete, push locale onto the stack and call procedure body.
  if (voids == 0) {
    A68_SP = pop_sp;
    u = POINTER (locale);
    v = STACK_ADDRESS (A68_SP);
    for (s = PACK (p_mode); s != NO_PACK; FORWARD (s)) {
      int size = SIZE (MOID (s));
      COPY (v, &u[SIZE (M_BOOL)], size);
      u = &(u[SIZE (M_BOOL) + size]);
      v = &(v[SIZE (MOID (s))]);
      INCREMENT_STACK_POINTER (p, size);
    }
    genie_call_procedure (p, p_mode, pproc, M_VOID, &z, pop_sp, pop_fp);
  } else {
//  Closure is not complete. Return procedure body.
    PUSH_PROCEDURE (p, z);
  }
}

//! @brief Closure and deproceduring of routines with PARAMSETY.

void genie_call_procedure (NODE_T * p, MOID_T * p_mode, MOID_T * pproc, MOID_T * pmap, A68_PROCEDURE * z, ADDR_T pop_sp, ADDR_T pop_fp)
{
  if (pmap != M_VOID && p_mode != pmap) {
    genie_partial_call (p, p_mode, pproc, pmap, *z, pop_sp, pop_fp);
  } else if (STATUS (z) & STANDENV_PROC_MASK) {
    NODE_T *save = A68 (f_entry);
    A68 (f_entry) = p;
    (void) ((*(PROCEDURE (&(BODY (z))))) (p));
    A68 (f_entry) = save;
  } else if (STATUS (z) & SKIP_PROCEDURE_MASK) {
    A68_SP = pop_sp;
    genie_push_undefined (p, SUB ((MOID (z))));
  } else {
    NODE_T *body = NODE (&(BODY (z)));
    if (IS (body, ROUTINE_TEXT)) {
      NODE_T *entry = SUB (body);
      ADDR_T fp0 = 0;
// Copy arguments from stack to frame.
      OPEN_PROC_FRAME (entry, ENVIRON (z));
      INIT_STATIC_FRAME (entry);
      FRAME_DNS (A68_FP) = pop_fp;
      for (PACK_T *args = PACK (p_mode); args != NO_PACK; FORWARD (args)) {
        int size = SIZE (MOID (args));
        COPY ((FRAME_OBJECT (fp0)), STACK_ADDRESS (pop_sp + fp0), size);
        fp0 += size;
      }
      A68_SP = pop_sp;
      ARGSIZE (GINFO (p)) = fp0;
// Interpret routine text.
      if (DIM (p_mode) > 0) {
// With PARAMETERS.
        entry = NEXT (NEXT_NEXT (entry));
      } else {
// Without PARAMETERS.
        entry = NEXT_NEXT (entry);
      }
      GENIE_UNIT_TRACE (entry);
      if (A68_FP == A68_MON (finish_frame_pointer)) {
        change_masks (TOP_NODE (&A68_JOB), BREAKPOINT_INTERRUPT_MASK, A68_TRUE);
      }
      CLOSE_FRAME;
      STACK_DNS (p, SUB (p_mode), A68_FP);
    } else {
      OPEN_PROC_FRAME (body, ENVIRON (z));
      INIT_STATIC_FRAME (body);
      FRAME_DNS (A68_FP) = pop_fp;
      GENIE_UNIT_TRACE (body);
      if (A68_FP == A68_MON (finish_frame_pointer)) {
        change_masks (TOP_NODE (&A68_JOB), BREAKPOINT_INTERRUPT_MASK, A68_TRUE);
      }
      CLOSE_FRAME;
      STACK_DNS (p, SUB (p_mode), A68_FP);
    }
  }
}

//! @brief Call event routine.

void genie_call_event_routine (NODE_T * p, MOID_T * m, A68_PROCEDURE * proc, ADDR_T pop_sp, ADDR_T pop_fp)
{
  if (NODE (&(BODY (proc))) != NO_NODE) {
    A68_PROCEDURE save = *proc;
    set_default_event_procedure (proc);
    genie_call_procedure (p, MOID (&save), m, m, &save, pop_sp, pop_fp);
    (*proc) = save;
  }
}

//! @brief Call PROC with arguments and push result.

PROP_T genie_call_standenv_quick (NODE_T * p)
{
  NODE_T *save = A68 (f_entry);
  A68 (f_entry) = p;
  NODE_T *pr = SUB (p);
  TAG_T *proc = TAX (SOURCE (&GPROP (pr)));
// Get arguments.
  for (NODE_T *q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
    GENIE_UNIT_NO_GC (q);
    STACK_DNS (p, MOID (q), A68_FP);
  }
  (void) ((*(PROCEDURE (proc))) (p));
  A68 (f_entry) = save;
  return GPROP (p);
}

//! @brief Call PROC with arguments and push result.

PROP_T genie_call_quick (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
// Get procedure.
  NODE_T *proc = SUB (p);
  GENIE_UNIT_NO_GC (proc);
  A68_PROCEDURE z;
  POP_OBJECT (proc, &z, A68_PROCEDURE);
  genie_check_initialisation (p, (BYTE_T *) & z, MOID (proc));
// Get arguments.
  if (SEQUENCE (p) == NO_NODE && !STATUS_TEST (p, SEQUENCE_MASK)) {
    NODE_T top_seq;
    NODE_T *seq = &top_seq;
    GINFO_T g;
    GINFO (&top_seq) = &g;
    genie_argument (NEXT (proc), &seq);
    SEQUENCE (p) = SEQUENCE (&top_seq);
    STATUS_SET (p, SEQUENCE_MASK);
  } else {
    for (NODE_T *q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
      GENIE_UNIT_NO_GC (q);
      STACK_DNS (p, MOID (q), A68_FP);
    }
  }
  genie_call_procedure (p, MOID (&z), PARTIAL_PROC (GINFO (proc)), PARTIAL_LOCALE (GINFO (proc)), &z, pop_sp, pop_fp);
  return GPROP (p);
}

//! @brief Call PROC with arguments and push result.

PROP_T genie_call (NODE_T * p)
{
  ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
  PROP_T self;
  UNIT (&self) = genie_call_quick;
  SOURCE (&self) = p;
// Get procedure.
  NODE_T *proc = SUB (p);
  GENIE_UNIT_NO_GC (proc);
  A68_PROCEDURE z;
  POP_OBJECT (proc, &z, A68_PROCEDURE);
  genie_check_initialisation (p, (BYTE_T *) & z, MOID (proc));
// Get arguments.
  if (SEQUENCE (p) == NO_NODE && !STATUS_TEST (p, SEQUENCE_MASK)) {
    NODE_T top_seq;
    NODE_T *seq = &top_seq;
    GINFO_T g;
    GINFO (&top_seq) = &g;
    genie_argument (NEXT (proc), &seq);
    SEQUENCE (p) = SEQUENCE (&top_seq);
    STATUS_SET (p, SEQUENCE_MASK);
  } else {
    for (NODE_T *q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
      GENIE_UNIT_NO_GC (q);
    }
  }
  genie_call_procedure (p, MOID (&z), PARTIAL_PROC (GINFO (proc)), PARTIAL_LOCALE (GINFO (proc)), &z, pop_sp, pop_fp);
  if (PARTIAL_LOCALE (GINFO (proc)) != M_VOID && MOID (&z) != PARTIAL_LOCALE (GINFO (proc))) {
    ;
  } else if (STATUS (&z) & STANDENV_PROC_MASK) {
    if (UNIT (&GPROP (proc)) == genie_identifier_standenv_proc) {
      UNIT (&self) = genie_call_standenv_quick;
    }
  }
  return self;
}
