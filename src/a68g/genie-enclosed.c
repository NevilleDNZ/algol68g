//! @file genie-enclosed.c
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

#define LABEL_FREE(_p_) {\
  NODE_T *_m_q; ADDR_T pop_sp_lf = A68_SP;\
  for (_m_q = SEQUENCE (_p_); _m_q != NO_NODE; _m_q = SEQUENCE (_m_q)) {\
    if (IS (_m_q, UNIT) || IS (_m_q, DECLARATION_LIST)) {\
      GENIE_UNIT_TRACE (_m_q);\
    }\
    if (SEQUENCE (_m_q) != NO_NODE) {\
      A68_SP = pop_sp_lf;\
      _m_q = SEQUENCE (_m_q);\
    }\
  }}

#define SERIAL_CLAUSE(_p_)\
  genie_preemptive_gc_heap ((NODE_T *) (_p_));\
  if (STATUS_TEST ((_p_), OPTIMAL_MASK)) {\
    GENIE_UNIT_TRACE (SEQUENCE (_p_));\
  } else if (STATUS_TEST ((_p_), SERIAL_MASK)) {\
    LABEL_FREE (_p_);\
  } else {\
    if (!setjmp (exit_buf)) {\
      genie_serial_clause ((NODE_T *) (_p_), (jmp_buf *) exit_buf);\
  }}

#define ENQUIRY_CLAUSE(_p_)\
  genie_preemptive_gc_heap ((NODE_T *) (_p_));\
  if (STATUS_TEST ((_p_), OPTIMAL_MASK)) {\
    GENIE_UNIT (SEQUENCE (_p_));\
  } else if (STATUS_TEST ((_p_), SERIAL_MASK)) {\
    LABEL_FREE (_p_);\
  } else {\
    genie_enquiry_clause ((NODE_T *) (_p_));\
  }

//! @brief Execute assertion.

PROP_T genie_assertion (NODE_T * p)
{
  PROP_T self;
  if (STATUS_TEST (p, ASSERT_MASK)) {
    A68_BOOL z;
    GENIE_UNIT (NEXT_SUB (p));
    POP_OBJECT (p, &z, A68_BOOL);
    if (VALUE (&z) == A68_FALSE) {
      diagnostic (A68_RUNTIME_ERROR, p, ERROR_FALSE_ASSERTION);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
  }
  UNIT (&self) = genie_assertion;
  SOURCE (&self) = p;
  return self;
}

//! @brief Execute a unit, tertiary, secondary or primary.

PROP_T genie_unit (NODE_T * p)
{
  if (IS_COERCION (GINFO (p))) {
    GLOBAL_PROP (&A68_JOB) = genie_coercion (p);
  } else {
    switch (ATTRIBUTE (p)) {
    case DECLARATION_LIST: {
        genie_declaration (SUB (p));
        UNIT (&GLOBAL_PROP (&A68_JOB)) = genie_unit;
        SOURCE (&GLOBAL_PROP (&A68_JOB)) = p;
        break;
      }
    case UNIT: {
        GENIE_UNIT_2 (SUB (p), GLOBAL_PROP (&A68_JOB));
        break;
      }
    case TERTIARY:
    case SECONDARY:
    case PRIMARY: {
        GLOBAL_PROP (&A68_JOB) = genie_unit (SUB (p));
        break;
      }
// Ex primary.
    case ENCLOSED_CLAUSE: {
        GLOBAL_PROP (&A68_JOB) = genie_enclosed ((volatile NODE_T *) p);
        break;
      }
    case IDENTIFIER: {
        GLOBAL_PROP (&A68_JOB) = genie_identifier (p);
        break;
      }
    case CALL: {
        GLOBAL_PROP (&A68_JOB) = genie_call (p);
        break;
      }
    case SLICE: {
        GLOBAL_PROP (&A68_JOB) = genie_slice (p);
        break;
      }
    case DENOTATION: {
        GLOBAL_PROP (&A68_JOB) = genie_denotation (p);
        break;
      }
    case CAST: {
        GLOBAL_PROP (&A68_JOB) = genie_cast (p);
        break;
      }
    case FORMAT_TEXT: {
        GLOBAL_PROP (&A68_JOB) = genie_format_text (p);
        break;
      }
// Ex secondary.
    case GENERATOR: {
        GLOBAL_PROP (&A68_JOB) = genie_generator (p);
        break;
      }
    case SELECTION: {
        GLOBAL_PROP (&A68_JOB) = genie_selection (p);
        break;
      }
// Ex tertiary.
    case FORMULA: {
        GLOBAL_PROP (&A68_JOB) = genie_formula (p);
        break;
      }
    case MONADIC_FORMULA: {
        GLOBAL_PROP (&A68_JOB) = genie_monadic (p);
        break;
      }
    case NIHIL: {
        GLOBAL_PROP (&A68_JOB) = genie_nihil (p);
        break;
      }
    case DIAGONAL_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_diagonal_function (p);
        break;
      }
    case TRANSPOSE_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_transpose_function (p);
        break;
      }
    case ROW_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_row_function (p);
        break;
      }
    case COLUMN_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_column_function (p);
        break;
      }
// Ex unit.
    case ASSIGNATION: {
        GLOBAL_PROP (&A68_JOB) = genie_assignation (p);
        break;
      }
    case IDENTITY_RELATION: {
        GLOBAL_PROP (&A68_JOB) = genie_identity_relation (p);
        break;
      }
    case ROUTINE_TEXT: {
        GLOBAL_PROP (&A68_JOB) = genie_routine_text (p);
        break;
      }
    case SKIP: {
        GLOBAL_PROP (&A68_JOB) = genie_skip (p);
        break;
      }
    case JUMP: {
        UNIT (&GLOBAL_PROP (&A68_JOB)) = genie_unit;
        SOURCE (&GLOBAL_PROP (&A68_JOB)) = p;
        genie_jump (p);
        break;
      }
    case AND_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_and_function (p);
        break;
      }
    case OR_FUNCTION: {
        GLOBAL_PROP (&A68_JOB) = genie_or_function (p);
        break;
      }
    case ASSERTION: {
        GLOBAL_PROP (&A68_JOB) = genie_assertion (p);
        break;
      }
    case CODE_CLAUSE: {
        diagnostic (A68_RUNTIME_ERROR, p, ERROR_CODE);
        exit_genie (p, A68_RUNTIME_ERROR);
        break;
      }
    }
  }
  return GPROP (p) = GLOBAL_PROP (&A68_JOB);
}

//! @brief Execution of serial clause without labels.

void genie_serial_units_no_label (NODE_T * p, ADDR_T pop_sp, NODE_T ** seq)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case DECLARATION_LIST:
    case UNIT: {
        GENIE_UNIT_TRACE (p);
        SEQUENCE (*seq) = p;
        (*seq) = p;
        return;
      }
    case SEMI_SYMBOL: {
// Voiden the expression stack.
        A68_SP = pop_sp;
        SEQUENCE (*seq) = p;
        (*seq) = p;
        break;
      }
    default: {
        genie_serial_units_no_label (SUB (p), pop_sp, seq);
        break;
      }
    }
  }
}

//! @brief Execution of serial clause with labels.

void genie_serial_units (NODE_T * p, NODE_T ** jump_to, jmp_buf * exit_buf, ADDR_T pop_sp)
{
  LOW_STACK_ALERT (p);
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case DECLARATION_LIST:
    case UNIT: {
        if (*jump_to == NO_NODE) {
          GENIE_UNIT_TRACE (p);
        } else if (p == *jump_to) {
// If we dropped in this clause from a jump then this unit is the target.
          *jump_to = NO_NODE;
          GENIE_UNIT_TRACE (p);
        }
        return;
      }
    case EXIT_SYMBOL: {
        if (*jump_to == NO_NODE) {
          longjmp (*exit_buf, 1);
        }
        break;
      }
    case SEMI_SYMBOL: {
        if (*jump_to == NO_NODE) {
// Voiden the expression stack.
          A68_SP = pop_sp;
        }
        break;
      }
    default: {
        genie_serial_units (SUB (p), jump_to, exit_buf, pop_sp);
        break;
      }
    }
  }
}

//! @brief Execute serial clause.

void genie_serial_clause (NODE_T * p, jmp_buf * exit_buf)
{
  if (LABELS (TABLE (p)) == NO_TAG) {
// No labels in this clause.
    if (SEQUENCE (p) == NO_NODE && !STATUS_TEST (p, SEQUENCE_MASK)) {
      NODE_T top_seq;
      NODE_T *seq = &top_seq;
      GINFO_T g;
      GINFO (&top_seq) = &g;
      genie_serial_units_no_label (SUB (p), A68_SP, &seq);
      SEQUENCE (p) = SEQUENCE (&top_seq);
      STATUS_SET (p, SEQUENCE_MASK);
      STATUS_SET (p, SERIAL_MASK);
      if (SEQUENCE (p) != NO_NODE && SEQUENCE (SEQUENCE (p)) == NO_NODE) {
        STATUS_SET (p, OPTIMAL_MASK);
      }
    } else {
// A linear list without labels.
      NODE_T *q;
      ADDR_T pop_sp = A68_SP;
      STATUS_SET (p, SERIAL_CLAUSE);
      for (q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
        switch (ATTRIBUTE (q)) {
        case DECLARATION_LIST:
        case UNIT: {
            GENIE_UNIT_TRACE (q);
            break;
          }
        case SEMI_SYMBOL: {
            A68_SP = pop_sp;
            break;
          }
        }
      }
    }
  } else {
// Labels in this clause.
    jmp_buf jump_stat;
    ADDR_T pop_sp = A68_SP, pop_fp = A68_FP;
    ADDR_T pop_dns = FRAME_DNS (A68_FP);
    FRAME_JUMP_STAT (A68_FP) = &jump_stat;
    if (!setjmp (jump_stat)) {
      NODE_T *jump_to = NO_NODE;
      genie_serial_units (SUB (p), &jump_to, exit_buf, A68_SP);
    } else {
// HIjol! Restore state and look for indicated unit.
      NODE_T *jump_to = JUMP_TO (TABLE (p));
      A68_SP = pop_sp;
      A68_FP = pop_fp;
      FRAME_DNS (A68_FP) = pop_dns;
      genie_serial_units (SUB (p), &jump_to, exit_buf, A68_SP);
    }
  }
}

//! @brief Execute enquiry clause.

void genie_enquiry_clause (NODE_T * p)
{
  if (SEQUENCE (p) == NO_NODE && !STATUS_TEST (p, SEQUENCE_MASK)) {
    NODE_T top_seq;
    NODE_T *seq = &top_seq;
    GINFO_T g;
    GINFO (&top_seq) = &g;
    genie_serial_units_no_label (SUB (p), A68_SP, &seq);
    SEQUENCE (p) = SEQUENCE (&top_seq);
    STATUS_SET (p, SEQUENCE_MASK);
    if (SEQUENCE (p) != NO_NODE && SEQUENCE (SEQUENCE (p)) == NO_NODE) {
      STATUS_SET (p, OPTIMAL_MASK);
    }
  } else {
// A linear list without labels (of course, it's an enquiry clause).
    NODE_T *q;
    ADDR_T pop_sp = A68_SP;
    STATUS_SET (p, SERIAL_MASK);
    for (q = SEQUENCE (p); q != NO_NODE; q = SEQUENCE (q)) {
      switch (ATTRIBUTE (q)) {
      case DECLARATION_LIST:
      case UNIT: {
          GENIE_UNIT_TRACE (q);
          break;
        }
      case SEMI_SYMBOL: {
          A68_SP = pop_sp;
          break;
        }
      }
    }
  }
}

//! @brief Execute collateral units.

void genie_collateral_units (NODE_T * p, int *count)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, UNIT)) {
      GENIE_UNIT_TRACE (p);
      STACK_DNS (p, MOID (p), FRAME_DNS (A68_FP));
      (*count)++;
      return;
    } else {
      genie_collateral_units (SUB (p), count);
    }
  }
}

//! @brief Execute collateral clause.

PROP_T genie_collateral (NODE_T * p)
{
  PROP_T self;
// VOID clause and STRUCT display.
  if (MOID (p) == M_VOID || IS_STRUCT (MOID (p))) {
    int count = 0;
    genie_collateral_units (SUB (p), &count);
  } else {
// Row display.
    A68_REF new_display;
    int count = 0;
    ADDR_T sp = A68_SP;
    MOID_T *m = MOID (p);
    genie_collateral_units (SUB (p), &count);
// [] AMODE vacuum.
    if (count == 0) {
      A68_SP = sp;
      INCREMENT_STACK_POINTER (p, A68_REF_SIZE);
      *(A68_REF *) STACK_ADDRESS (sp) = empty_row (p, m);
    } else if (DIM (DEFLEX (m)) == 1) {
// [] AMODE display.
      new_display = genie_make_row (p, SLICE (DEFLEX (m)), count, sp);
      A68_SP = sp;
      INCREMENT_STACK_POINTER (p, A68_REF_SIZE);
      *(A68_REF *) STACK_ADDRESS (sp) = new_display;
    } else {
// [,,] AMODE display, we concatenate 1 + (n-1) to n dimensions.
      new_display = genie_make_rowrow (p, m, count, sp);
      A68_SP = sp;
      INCREMENT_STACK_POINTER (p, A68_REF_SIZE);
      *(A68_REF *) STACK_ADDRESS (sp) = new_display;
    }
  }
  UNIT (&self) = genie_collateral;
  SOURCE (&self) = p;
  return self;
}

//! @brief Execute unit from integral-case in-part.

BOOL_T genie_int_case_unit (NODE_T * p, int k, int *count)
{
  if (p == NO_NODE) {
    return A68_FALSE;
  } else {
    if (IS (p, UNIT)) {
      if (k == *count) {
        GENIE_UNIT_TRACE (p);
        return A68_TRUE;
      } else {
        (*count)++;
        return A68_FALSE;
      }
    } else {
      if (genie_int_case_unit (SUB (p), k, count)) {
        return A68_TRUE;
      } else {
        return genie_int_case_unit (NEXT (p), k, count);
      }
    }
  }
}

//! @brief Execute unit from united-case in-part.

BOOL_T genie_united_case_unit (NODE_T * p, MOID_T * m)
{
  if (p == NO_NODE) {
    return A68_FALSE;
  } else {
    if (IS (p, SPECIFIER)) {
      MOID_T *spec_moid = MOID (NEXT_SUB (p));
      BOOL_T equal_modes;
      if (m != NO_MOID) {
        if (IS_UNION (spec_moid)) {
          equal_modes = is_unitable (m, spec_moid, SAFE_DEFLEXING);
        } else {
          equal_modes = (BOOL_T) (m == spec_moid);
        }
      } else {
        equal_modes = A68_FALSE;
      }
      if (equal_modes) {
        NODE_T *q = NEXT_NEXT (SUB (p));
        OPEN_STATIC_FRAME (p);
        INIT_STATIC_FRAME (p);
        if (IS (q, IDENTIFIER)) {
          if (IS_UNION (spec_moid)) {
            COPY ((FRAME_OBJECT (OFFSET (TAX (q)))), STACK_TOP, SIZE (spec_moid));
          } else {
            COPY ((FRAME_OBJECT (OFFSET (TAX (q)))), STACK_OFFSET (A68_UNION_SIZE), SIZE (spec_moid));
          }
        }
        GENIE_UNIT_TRACE (NEXT_NEXT (p));
        CLOSE_FRAME;
        return A68_TRUE;
      } else {
        return A68_FALSE;
      }
    } else {
      if (genie_united_case_unit (SUB (p), m)) {
        return A68_TRUE;
      } else {
        return genie_united_case_unit (NEXT (p), m);
      }
    }
  }
}

//! @brief Execute integral-case-clause.

PROP_T genie_int_case (volatile NODE_T * p)
{
  volatile int unit_count;
  volatile BOOL_T found_unit;
  jmp_buf exit_buf;
  A68_INT k;
  volatile NODE_T *q = SUB (p);
  volatile MOID_T *yield = MOID (q);
// CASE or OUSE.
  OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
  INIT_GLOBAL_POINTER ((NODE_T *) SUB (q));
  INIT_STATIC_FRAME ((NODE_T *) SUB (q));
  ENQUIRY_CLAUSE (NEXT_SUB (q));
  POP_OBJECT (q, &k, A68_INT);
// IN.
  FORWARD (q);
  OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
  INIT_STATIC_FRAME ((NODE_T *) SUB (q));
  unit_count = 1;
  found_unit = genie_int_case_unit (NEXT_SUB ((NODE_T *) q), (int) VALUE (&k), (int *) &unit_count);
  CLOSE_FRAME;
// OUT.
  if (!found_unit) {
    FORWARD (q);
    switch (ATTRIBUTE (q)) {
    case CHOICE:
    case OUT_PART: {
        OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
        INIT_STATIC_FRAME ((NODE_T *) SUB (q));
        SERIAL_CLAUSE (NEXT_SUB (q));
        CLOSE_FRAME;
        break;
      }
    case CLOSE_SYMBOL:
    case ESAC_SYMBOL: {
        if (yield != M_VOID) {
          genie_push_undefined ((NODE_T *) q, (MOID_T *) yield);
        }
        break;
      }
    default: {
        MOID (SUB ((NODE_T *) q)) = (MOID_T *) yield;
        (void) genie_int_case (q);
        break;
      }
    }
  }
// ESAC.
  CLOSE_FRAME;
  return GPROP (p);
}

//! @brief Execute united-case-clause.

PROP_T genie_united_case (volatile NODE_T * p)
{
  volatile BOOL_T found_unit = A68_FALSE;
  volatile MOID_T *um;
  volatile ADDR_T pop_sp;
  jmp_buf exit_buf;
  volatile NODE_T *q = SUB (p);
  volatile MOID_T *yield = MOID (q);
// CASE or OUSE.
  OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
  INIT_GLOBAL_POINTER ((NODE_T *) SUB (q));
  INIT_STATIC_FRAME ((NODE_T *) SUB (q));
  pop_sp = A68_SP;
  ENQUIRY_CLAUSE (NEXT_SUB (q));
  A68_SP = pop_sp;
  um = (volatile MOID_T *) VALUE ((A68_UNION *) STACK_TOP);
// IN.
  FORWARD (q);
  if (um != NO_MOID) {
    OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
    INIT_STATIC_FRAME ((NODE_T *) SUB (q));
    found_unit = genie_united_case_unit (NEXT_SUB ((NODE_T *) q), (MOID_T *) um);
    CLOSE_FRAME;
  } else {
    found_unit = A68_FALSE;
  }
// OUT.
  if (!found_unit) {
    FORWARD (q);
    switch (ATTRIBUTE (q)) {
    case CHOICE:
    case OUT_PART: {
        OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
        INIT_STATIC_FRAME ((NODE_T *) SUB (q));
        SERIAL_CLAUSE (NEXT_SUB (q));
        CLOSE_FRAME;
        break;
      }
    case CLOSE_SYMBOL:
    case ESAC_SYMBOL: {
        if (yield != M_VOID) {
          genie_push_undefined ((NODE_T *) q, (MOID_T *) yield);
        }
        break;
      }
    default: {
        MOID (SUB ((NODE_T *) q)) = (MOID_T *) yield;
        (void) genie_united_case (q);
        break;
      }
    }
  }
// ESAC.
  CLOSE_FRAME;
  return GPROP (p);
}

//! @brief Execute conditional-clause.

PROP_T genie_conditional (volatile NODE_T * p)
{
  volatile ADDR_T pop_sp = A68_SP;
  jmp_buf exit_buf;
  volatile NODE_T *q = SUB (p);
  volatile MOID_T *yield = MOID (q);
// IF or ELIF.
  OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
  INIT_GLOBAL_POINTER ((NODE_T *) SUB (q));
  INIT_STATIC_FRAME ((NODE_T *) SUB (q));
  ENQUIRY_CLAUSE (NEXT_SUB (q));
  A68_SP = pop_sp;
  FORWARD (q);
  if (VALUE ((A68_BOOL *) STACK_TOP) == A68_TRUE) {
// THEN.
    OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
    INIT_STATIC_FRAME ((NODE_T *) SUB (q));
    SERIAL_CLAUSE (NEXT_SUB (q));
    CLOSE_FRAME;
  } else {
// ELSE.
    FORWARD (q);
    switch (ATTRIBUTE (q)) {
    case CHOICE:
    case ELSE_PART: {
        OPEN_STATIC_FRAME ((NODE_T *) SUB (q));
        INIT_STATIC_FRAME ((NODE_T *) SUB (q));
        SERIAL_CLAUSE (NEXT_SUB (q));
        CLOSE_FRAME;
        break;
      }
    case CLOSE_SYMBOL:
    case FI_SYMBOL: {
        if (yield != M_VOID) {
          genie_push_undefined ((NODE_T *) q, (MOID_T *) yield);
        }
        break;
      }
    default: {
        MOID (SUB ((NODE_T *) q)) = (MOID_T *) yield;
        (void) genie_conditional (q);
        break;
      }
    }
  }
// FI.
  CLOSE_FRAME;
  return GPROP (p);
}

// INCREMENT_COUNTER procures that the counter only increments if there is
// a for-part or a to-part. Otherwise an infinite loop would trigger overflow
// when the anonymous counter reaches max int, which is strange behaviour.
// This is less relevant using 64-bit integers.

#define INCREMENT_COUNTER\
  if (!(for_part == NO_NODE && to_part == NO_NODE)) {\
    CHECK_INT_ADDITION ((NODE_T *) p, counter, by);\
    counter += by;\
  }

//! @brief Execute loop-clause.

PROP_T genie_loop (volatile NODE_T * p)
{
  volatile ADDR_T pop_sp = A68_SP;
  volatile INT_T from, by, to, counter;
  volatile BOOL_T siga, conditional;
  volatile NODE_T *for_part = NO_NODE, *to_part = NO_NODE, *q = NO_NODE;
  jmp_buf exit_buf;
// FOR  identifier.
  if (IS (p, FOR_PART)) {
    for_part = NEXT_SUB (p);
    FORWARD (p);
  }
// FROM unit.
  if (IS (p, FROM_PART)) {
    GENIE_UNIT (NEXT_SUB (p));
    A68_SP = pop_sp;
    from = VALUE ((A68_INT *) STACK_TOP);
    FORWARD (p);
  } else {
    from = 1;
  }
// BY unit.
  if (IS (p, BY_PART)) {
    GENIE_UNIT (NEXT_SUB (p));
    A68_SP = pop_sp;
    by = VALUE ((A68_INT *) STACK_TOP);
    FORWARD (p);
  } else {
    by = 1;
  }
// TO unit, DOWNTO unit.
  if (IS (p, TO_PART)) {
    if (IS (SUB (p), DOWNTO_SYMBOL)) {
      by = -by;
    }
    GENIE_UNIT (NEXT_SUB (p));
    A68_SP = pop_sp;
    to = VALUE ((A68_INT *) STACK_TOP);
    to_part = p;
    FORWARD (p);
  } else {
    to = (by >= 0 ? A68_MAX_INT : -A68_MAX_INT);
  }
  q = NEXT_SUB (p);
// Here the loop part starts.
// We open the frame only once and reinitialise if necessary
  OPEN_STATIC_FRAME ((NODE_T *) q);
  INIT_GLOBAL_POINTER ((NODE_T *) q);
  INIT_STATIC_FRAME ((NODE_T *) q);
  counter = from;
// Does the loop contain conditionals?.
  if (IS (p, WHILE_PART)) {
    conditional = A68_TRUE;
  } else if (IS (p, DO_PART) || IS (p, ALT_DO_PART)) {
    NODE_T *until_part = NEXT_SUB (p);
    if (IS (until_part, SERIAL_CLAUSE)) {
      until_part = NEXT (until_part);
    }
    conditional = (BOOL_T) (until_part != NO_NODE && IS (until_part, UNTIL_PART));
  } else {
    conditional = A68_FALSE;
  }
  if (conditional) {
// [FOR ...] [WHILE ...] DO [...] [UNTIL ...] OD.
    siga = (BOOL_T) ((by > 0 && counter <= to) || (by < 0 && counter >= to) || (by == 0));
    while (siga) {
      if (for_part != NO_NODE) {
        A68_INT *z = (A68_INT *) (FRAME_OBJECT (OFFSET (TAX (for_part))));
        STATUS (z) = INIT_MASK;
        VALUE (z) = counter;
      }
      A68_SP = pop_sp;
      if (IS (p, WHILE_PART)) {
        ENQUIRY_CLAUSE (q);
        A68_SP = pop_sp;
        siga = (BOOL_T) (VALUE ((A68_BOOL *) STACK_TOP) != A68_FALSE);
      }
      if (siga) {
        volatile NODE_T *do_part = p, *until_part;
        if (IS (p, WHILE_PART)) {
          do_part = NEXT_SUB (NEXT (p));
          OPEN_STATIC_FRAME ((NODE_T *) do_part);
          INIT_STATIC_FRAME ((NODE_T *) do_part);
        } else {
          do_part = NEXT_SUB (p);
        }
        if (IS (do_part, SERIAL_CLAUSE)) {
          SERIAL_CLAUSE (do_part);
          until_part = NEXT (do_part);
        } else {
          until_part = do_part;
        }
// UNTIL part.
        if (until_part != NO_NODE && IS (until_part, UNTIL_PART)) {
          NODE_T *v = NEXT_SUB (until_part);
          OPEN_STATIC_FRAME ((NODE_T *) v);
          INIT_STATIC_FRAME ((NODE_T *) v);
          A68_SP = pop_sp;
          ENQUIRY_CLAUSE (v);
          A68_SP = pop_sp;
          siga = (BOOL_T) (VALUE ((A68_BOOL *) STACK_TOP) == A68_FALSE);
          CLOSE_FRAME;
        }
        if (IS (p, WHILE_PART)) {
          CLOSE_FRAME;
        }
// Increment counter.
        if (siga) {
          INCREMENT_COUNTER;
          siga = (BOOL_T) ((by > 0 && counter <= to) || (by < 0 && counter >= to) || (by == 0));
        }
// The genie cannot take things to next iteration: re-initialise stack frame.
        if (siga) {
          FRAME_CLEAR (AP_INCREMENT (TABLE (q)));
          if (INITIALISE_FRAME (TABLE (q))) {
            initialise_frame ((NODE_T *) q);
          }
        }
      }
    }
  } else {
// [FOR ...] DO ... OD.
    siga = (BOOL_T) ((by > 0 && counter <= to) || (by < 0 && counter >= to) || (by == 0));
    while (siga) {
      if (for_part != NO_NODE) {
        A68_INT *z = (A68_INT *) (FRAME_OBJECT (OFFSET (TAX (for_part))));
        STATUS (z) = INIT_MASK;
        VALUE (z) = counter;
      }
      A68_SP = pop_sp;
      SERIAL_CLAUSE (q);
      INCREMENT_COUNTER;
      siga = (BOOL_T) ((by > 0 && counter <= to) || (by < 0 && counter >= to) || (by == 0));
// The genie cannot take things to next iteration: re-initialise stack frame.
      if (siga) {
        FRAME_CLEAR (AP_INCREMENT (TABLE (q)));
        if (INITIALISE_FRAME (TABLE (q))) {
          initialise_frame ((NODE_T *) q);
        }
      }
    }
  }
// OD.
  CLOSE_FRAME;
  A68_SP = pop_sp;
  return GPROP (p);
}

#undef INCREMENT_COUNTER
#undef LOOP_OVERFLOW

//! @brief Execute closed clause.

PROP_T genie_closed (volatile NODE_T * p)
{
  jmp_buf exit_buf;
  volatile NODE_T *q = NEXT_SUB (p);
  OPEN_STATIC_FRAME ((NODE_T *) q);
  INIT_GLOBAL_POINTER ((NODE_T *) q);
  INIT_STATIC_FRAME ((NODE_T *) q);
  SERIAL_CLAUSE (q);
  CLOSE_FRAME;
  return GPROP (p);
}

//! @brief Execute enclosed clause.

PROP_T genie_enclosed (volatile NODE_T * p)
{
  PROP_T self;
  UNIT (&self) = (PROP_PROC *) genie_enclosed;
  SOURCE (&self) = (NODE_T *) p;
  switch (ATTRIBUTE (p)) {
  case PARTICULAR_PROGRAM: {
      self = genie_enclosed (SUB (p));
      break;
    }
  case ENCLOSED_CLAUSE: {
      self = genie_enclosed (SUB (p));
      break;
    }
  case CLOSED_CLAUSE: {
      self = genie_closed ((NODE_T *) p);
      if (UNIT (&self) == genie_unit) {
        UNIT (&self) = (PROP_PROC *) genie_closed;
        SOURCE (&self) = (NODE_T *) p;
      }
      break;
    }
#if defined (BUILD_PARALLEL_CLAUSE)
  case PARALLEL_CLAUSE: {
      (void) genie_parallel ((NODE_T *) NEXT_SUB (p));
      break;
    }
#endif
  case COLLATERAL_CLAUSE: {
      (void) genie_collateral ((NODE_T *) p);
      break;
    }
  case CONDITIONAL_CLAUSE: {
      MOID (SUB ((NODE_T *) p)) = MOID (p);
      (void) genie_conditional (p);
      UNIT (&self) = (PROP_PROC *) genie_conditional;
      SOURCE (&self) = (NODE_T *) p;
      break;
    }
  case CASE_CLAUSE: {
      MOID (SUB ((NODE_T *) p)) = MOID (p);
      (void) genie_int_case (p);
      UNIT (&self) = (PROP_PROC *) genie_int_case;
      SOURCE (&self) = (NODE_T *) p;
      break;
    }
  case CONFORMITY_CLAUSE: {
      MOID (SUB ((NODE_T *) p)) = MOID (p);
      (void) genie_united_case (p);
      UNIT (&self) = (PROP_PROC *) genie_united_case;
      SOURCE (&self) = (NODE_T *) p;
      break;
    }
  case LOOP_CLAUSE: {
      (void) genie_loop (SUB ((NODE_T *) p));
      UNIT (&self) = (PROP_PROC *) genie_loop;
      SOURCE (&self) = SUB ((NODE_T *) p);
      break;
    }
  }
  GPROP (p) = self;
  return self;
}
