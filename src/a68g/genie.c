//! @file genie.c
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

//! @brief Set flags throughout tree.

void change_masks (NODE_T * p, unt mask, BOOL_T set)
{
  for (; p != NO_NODE; FORWARD (p)) {
    change_masks (SUB (p), mask, set);
    if (LINE_NUMBER (p) > 0) {
      if (set == A68_TRUE) {
        STATUS_SET (p, mask);
      } else {
        STATUS_CLEAR (p, mask);
      }
    }
  }
}

//! @brief Set flags throughout tree.

void change_gc_masks (NODE_T * p, BOOL_T set)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
      case CALL: {
          change_gc_masks (SUB (p), A68_TRUE);
          break;
        }
      case SLICE: {
          change_gc_masks (SUB (p), A68_TRUE);
          break;
        }
      default: {
        change_gc_masks (SUB (p), set);
        break;
      }
    }
    if (LINE_NUMBER (p) > 0) {
      if (set == A68_TRUE) {
        STATUS_SET (p, BLOCK_GC_MASK);
      } else {
        STATUS_CLEAR (p, BLOCK_GC_MASK);
      }
    }
  }
}

//! @brief Leave interpretation.

void exit_genie (NODE_T * p, int ret)
{
#if defined (HAVE_CURSES)
  genie_curses_end (p);
#endif
  A68 (close_tty_on_exit) = A68_TRUE;
  if (!A68 (in_execution)) {
    return;
  }
  if (ret == A68_RUNTIME_ERROR && A68 (in_monitor)) {
    return;
  } else if (ret == A68_RUNTIME_ERROR && OPTION_DEBUG (&A68_JOB)) {
    diagnostics_to_terminal (TOP_LINE (&A68_JOB), A68_RUNTIME_ERROR);
    single_step (p, (unt) BREAKPOINT_ERROR_MASK);
    A68 (in_execution) = A68_FALSE;
    A68 (ret_line_number) = LINE_NUMBER (p);
    A68 (ret_code) = ret;
    longjmp (A68 (genie_exit_label), 1);
  } else {
    if ((ret & A68_FORCE_QUIT) != NULL_MASK) {
      ret &= ~A68_FORCE_QUIT;
    }
#if defined (BUILD_PARALLEL_CLAUSE)
    if (!is_main_thread ()) {
      genie_set_exit_from_threads (ret);
    } else {
      A68 (in_execution) = A68_FALSE;
      A68 (ret_line_number) = LINE_NUMBER (p);
      A68 (ret_code) = ret;
      longjmp (A68 (genie_exit_label), 1);
    }
#else
    A68 (in_execution) = A68_FALSE;
    A68 (ret_line_number) = LINE_NUMBER (p);
    A68 (ret_code) = ret;
    longjmp (A68 (genie_exit_label), 1);
#endif
  }
}

//! @brief Genie init rng.

void genie_init_rng (void)
{
  time_t t;
  if (time (&t) != -1) {
    init_rng ((unt) t);
  }
}

//! @brief Tie label to the clause it is defined in.

void tie_label_to_serial (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, SERIAL_CLAUSE)) {
      BOOL_T valid_follow;
      if (NEXT (p) == NO_NODE) {
        valid_follow = A68_TRUE;
      } else if (IS (NEXT (p), CLOSE_SYMBOL)) {
        valid_follow = A68_TRUE;
      } else if (IS (NEXT (p), END_SYMBOL)) {
        valid_follow = A68_TRUE;
      } else if (IS (NEXT (p), EDOC_SYMBOL)) {
        valid_follow = A68_TRUE;
      } else if (IS (NEXT (p), OD_SYMBOL)) {
        valid_follow = A68_TRUE;
      } else {
        valid_follow = A68_FALSE;
      }
      if (valid_follow) {
        JUMP_TO (TABLE (SUB (p))) = NO_NODE;
      }
    }
    tie_label_to_serial (SUB (p));
  }
}

//! @brief Tie label to the clause it is defined in.

void tie_label (NODE_T * p, NODE_T * unit)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, DEFINING_IDENTIFIER)) {
      UNIT (TAX (p)) = unit;
    }
    tie_label (SUB (p), unit);
  }
}

//! @brief Tie label to the clause it is defined in.

void tie_label_to_unit (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (IS (p, LABELED_UNIT)) {
      tie_label (SUB_SUB (p), NEXT_SUB (p));
    }
    tie_label_to_unit (SUB (p));
  }
}

//! @brief Fast way to indicate a mode.

int mode_attribute (MOID_T * p)
{
  if (IS_REF (p)) {
    return REF_SYMBOL;
  } else if (IS (p, PROC_SYMBOL)) {
    return PROC_SYMBOL;
  } else if (IS_UNION (p)) {
    return UNION_SYMBOL;
  } else if (p == M_INT) {
    return MODE_INT;
  } else if (p == M_LONG_INT) {
    return MODE_LONG_INT;
  } else if (p == M_LONG_LONG_INT) {
    return MODE_LONG_LONG_INT;
  } else if (p == M_REAL) {
    return MODE_REAL;
  } else if (p == M_LONG_REAL) {
    return MODE_LONG_REAL;
  } else if (p == M_LONG_LONG_REAL) {
    return MODE_LONG_LONG_REAL;
  } else if (p == M_COMPLEX) {
    return MODE_COMPLEX;
  } else if (p == M_LONG_COMPLEX) {
    return MODE_LONG_COMPLEX;
  } else if (p == M_LONG_LONG_COMPLEX) {
    return MODE_LONG_LONG_COMPLEX;
  } else if (p == M_BOOL) {
    return MODE_BOOL;
  } else if (p == M_CHAR) {
    return MODE_CHAR;
  } else if (p == M_BITS) {
    return MODE_BITS;
  } else if (p == M_LONG_BITS) {
    return MODE_LONG_BITS;
  } else if (p == M_LONG_LONG_BITS) {
    return MODE_LONG_LONG_BITS;
  } else if (p == M_BYTES) {
    return MODE_BYTES;
  } else if (p == M_LONG_BYTES) {
    return MODE_LONG_BYTES;
  } else if (p == M_FILE) {
    return MODE_FILE;
  } else if (p == M_FORMAT) {
    return MODE_FORMAT;
  } else if (p == M_PIPE) {
    return MODE_PIPE;
  } else if (p == M_SOUND) {
    return MODE_SOUND;
  } else {
    return MODE_NO_CHECK;
  }
}

//! @brief Perform tasks before interpretation.

void genie_preprocess (NODE_T * p, int *max_lev, void *compile_plugin)
{
#if defined (BUILD_A68_COMPILER)
  static char *last_compile_name = NO_TEXT;
  static PROP_PROC *last_compile_unit = NO_PPROC;
#endif
  for (; p != NO_NODE; FORWARD (p)) {
    if (STATUS_TEST (p, BREAKPOINT_MASK)) {
      if (!(STATUS_TEST (p, INTERRUPTIBLE_MASK))) {
        STATUS_CLEAR (p, BREAKPOINT_MASK);
      }
    }
    if (GINFO (p) != NO_GINFO) {
      IS_COERCION (GINFO (p)) = is_coercion (p);
      IS_NEW_LEXICAL_LEVEL (GINFO (p)) = is_new_lexical_level (p);
// The default.
      UNIT (&GPROP (p)) = genie_unit;
      SOURCE (&GPROP (p)) = p;
#if defined (BUILD_A68_COMPILER)
      if (OPTION_OPT_LEVEL (&A68_JOB) > 0 && COMPILE_NAME (GINFO (p)) != NO_TEXT && compile_plugin != NULL) {
        if (COMPILE_NAME (GINFO (p)) == last_compile_name) {
// We copy.
          UNIT (&GPROP (p)) = last_compile_unit;
        } else {
// We look up.
// Next line may provoke a warning even with this POSIX workaround. Tant pis.
          *(void **) &(UNIT (&GPROP (p))) = dlsym (compile_plugin, COMPILE_NAME (GINFO (p)));
          ABEND (UNIT (&GPROP (p)) == NULL, ERROR_INTERNAL_CONSISTENCY, dlerror ());
          last_compile_name = COMPILE_NAME (GINFO (p));
          last_compile_unit = UNIT (&GPROP (p));
        }
      }
#endif
    }
    if (MOID (p) != NO_MOID) {
      SIZE (MOID (p)) = moid_size (MOID (p));
      DIGITS (MOID (p)) = moid_digits (MOID (p));
      SHORT_ID (MOID (p)) = mode_attribute (MOID (p));
      if (GINFO (p) != NO_GINFO) {
        NEED_DNS (GINFO (p)) = A68_FALSE;
        if (IS_REF (MOID (p))) {
          NEED_DNS (GINFO (p)) = A68_TRUE;
        } else if (IS (MOID (p), PROC_SYMBOL)) {
          NEED_DNS (GINFO (p)) = A68_TRUE;
        } else if (IS (MOID (p), FORMAT_SYMBOL)) {
          NEED_DNS (GINFO (p)) = A68_TRUE;
        }
      }
    }
    if (TABLE (p) != NO_TABLE) {
      if (LEX_LEVEL (p) > *max_lev) {
        *max_lev = LEX_LEVEL (p);
      }
    }
    if (IS (p, FORMAT_TEXT)) {
      TAG_T *q = TAX (p);
      if (q != NO_TAG && NODE (q) != NO_NODE) {
        NODE (q) = p;
      }
    } else if (IS (p, DEFINING_IDENTIFIER)) {
      TAG_T *q = TAX (p);
      if (q != NO_TAG && NODE (q) != NO_NODE && TABLE (NODE (q)) != NO_TABLE) {
        LEVEL (GINFO (p)) = LEX_LEVEL (NODE (q));
      }
    } else if (IS (p, IDENTIFIER)) {
      TAG_T *q = TAX (p);
      if (q != NO_TAG && NODE (q) != NO_NODE && TABLE (NODE (q)) != NO_TABLE) {
        LEVEL (GINFO (p)) = LEX_LEVEL (NODE (q));
        OFFSET (GINFO (p)) = &(A68_STACK[FRAME_INFO_SIZE + OFFSET (q)]);
      }
    } else if (IS (p, OPERATOR)) {
      TAG_T *q = TAX (p);
      if (q != NO_TAG && NODE (q) != NO_NODE && TABLE (NODE (q)) != NO_TABLE) {
        LEVEL (GINFO (p)) = LEX_LEVEL (NODE (q));
        OFFSET (GINFO (p)) = &(A68_STACK[FRAME_INFO_SIZE + OFFSET (q)]);
      }
    }
    if (SUB (p) != NO_NODE) {
      if (GINFO (p) != NO_GINFO) {
        GPARENT (SUB (p)) = p;
      }
      genie_preprocess (SUB (p), max_lev, compile_plugin);
    }
  }
}

//! @brief Get outermost lexical level in the user program.

void get_global_level (NODE_T * p)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (LINE_NUMBER (p) != 0 && IS (p, UNIT)) {
      if (LEX_LEVEL (p) < A68 (global_level)) {
        A68 (global_level) = LEX_LEVEL (p);
      }
    }
    get_global_level (SUB (p));
  }
}

//! @brief Driver for the interpreter.

void genie (void *compile_plugin)
{
// Fill in final info for modes.
  for (MOID_T *m = TOP_MOID (&A68_JOB); m != NO_MOID; FORWARD (m)) {
    SIZE (m) = moid_size (m);
    DIGITS (m) = moid_digits (m);
    SHORT_ID (m) = mode_attribute (m);
  }
// Preprocessing.
  A68 (max_lex_lvl) = 0;
//  genie_lex_levels (TOP_NODE (&A68_JOB), 1);.
  genie_preprocess (TOP_NODE (&A68_JOB), &A68 (max_lex_lvl), compile_plugin);
  change_masks (TOP_NODE (&A68_JOB), BREAKPOINT_INTERRUPT_MASK, A68_FALSE);
  change_gc_masks (TOP_NODE (&A68_JOB), A68_FALSE);
  A68_MON (watchpoint_expression) = NO_TEXT;
  A68 (frame_stack_limit) = A68 (frame_end) - A68 (storage_overhead);
  A68 (expr_stack_limit) = A68 (stack_end) - A68 (storage_overhead);
  if (OPTION_REGRESSION_TEST (&A68_JOB)) {
    init_rng (1);
  } else {
    genie_init_rng ();
  }
  io_close_tty_line ();
  if (OPTION_TRACE (&A68_JOB)) {
    ASSERT (snprintf (A68 (output_line), SNPRINTF_SIZE, "genie: frame stack %uk, expression stack %uk, heap %uk, handles %uk\n", A68 (frame_stack_size) / KILOBYTE, A68 (expr_stack_size) / KILOBYTE, A68 (heap_size) / KILOBYTE, A68 (handle_pool_size) / KILOBYTE) >= 0);
    WRITE (STDOUT_FILENO, A68 (output_line));
  }
  install_signal_handlers ();
  set_default_event_procedure (&A68 (on_gc_event));
  A68 (do_confirm_exit) = A68_TRUE;
#if defined (BUILD_PARALLEL_CLAUSE)
  ASSERT (pthread_mutex_init (&A68_PAR (unit_sema), NULL) == 0);
#endif
// Dive into the program.
  if (setjmp (A68 (genie_exit_label)) == 0) {
    NODE_T *p = SUB (TOP_NODE (&A68_JOB));
// If we are to stop in the monitor, set a breakpoint on the first unit.
    if (OPTION_DEBUG (&A68_JOB)) {
      change_masks (TOP_NODE (&A68_JOB), BREAKPOINT_TEMPORARY_MASK, A68_TRUE);
      WRITE (STDOUT_FILENO, "Execution begins ...");
    }
    errno = 0;
    A68 (ret_code) = 0;
    A68 (global_level) = INT_MAX;
    A68_GLOBALS = 0;
    get_global_level (p);
    A68_FP = A68 (frame_start);
    A68_SP = A68 (stack_start);
    FRAME_DYNAMIC_LINK (A68_FP) = 0;
    FRAME_DNS (A68_FP) = 0;
    FRAME_STATIC_LINK (A68_FP) = 0;
    FRAME_NUMBER (A68_FP) = 0;
    FRAME_TREE (A68_FP) = (NODE_T *) p;
    FRAME_LEXICAL_LEVEL (A68_FP) = LEX_LEVEL (p);
    FRAME_PARAMETER_LEVEL (A68_FP) = LEX_LEVEL (p);
    FRAME_PARAMETERS (A68_FP) = A68_FP;
    initialise_frame (p);
    genie_init_heap (p);
    genie_init_transput (TOP_NODE (&A68_JOB));
    A68 (cputime_0) = seconds ();
    A68_GC (sema) = 0;
// Here we go ...
    A68 (in_execution) = A68_TRUE;
    A68 (f_entry) = TOP_NODE (&A68_JOB);
#if defined (BUILD_UNIX)
    (void) alarm (1);
#endif
    if (OPTION_TRACE (&A68_JOB)) {
      WIS (TOP_NODE (&A68_JOB));
    }
    (void) genie_enclosed (TOP_NODE (&A68_JOB));
  } else {
// Here we have jumped out of the interpreter. What happened?.
    if (OPTION_DEBUG (&A68_JOB)) {
      WRITE (STDOUT_FILENO, "Execution discontinued");
    }
    if (A68 (ret_code) == A68_RERUN) {
      diagnostics_to_terminal (TOP_LINE (&A68_JOB), A68_RUNTIME_ERROR);
      genie (compile_plugin);
    } else if (A68 (ret_code) == A68_RUNTIME_ERROR) {
      if (OPTION_BACKTRACE (&A68_JOB)) {
        int printed = 0;
        ASSERT (snprintf (A68 (output_line), SNPRINTF_SIZE, "\nStack backtrace") >= 0);
        WRITE (STDOUT_FILENO, A68 (output_line));
        stack_dump (STDOUT_FILENO, A68_FP, 16, &printed);
        WRITE (STDOUT_FILENO, NEWLINE_STRING);
      }
      if (FILE_LISTING_OPENED (&A68_JOB)) {
        int printed = 0;
        ASSERT (snprintf (A68 (output_line), SNPRINTF_SIZE, "\nStack backtrace") >= 0);
        WRITE (FILE_LISTING_FD (&A68_JOB), A68 (output_line));
        stack_dump (FILE_LISTING_FD (&A68_JOB), A68_FP, 32, &printed);
      }
    }
  }
  A68 (in_execution) = A68_FALSE;
}

// This file contains interpreter ("genie") routines related to executing primitive
// A68 actions.
// 
// The genie is self-optimising as when it traverses the tree, it stores terminals
// it ends up in at the root where traversing for that terminal started.
// Such piece of information is called a PROP.

//! @brief Shows line where 'p' is at and draws a '-' beneath the position.

void where_in_source (FILE_T f, NODE_T * p)
{
  write_source_line (f, LINE (INFO (p)), p, A68_NO_DIAGNOSTICS);
}

// Since Algol 68 can pass procedures as parameters, we use static links rather
// than a display.

//! @brief Initialise PROC and OP identities.

void genie_init_proc_op (NODE_T * p, NODE_T ** seq, int *count)
{
  for (; p != NO_NODE; FORWARD (p)) {
    switch (ATTRIBUTE (p)) {
    case OP_SYMBOL:
    case PROC_SYMBOL:
    case OPERATOR_PLAN:
    case DECLARER: {
        break;
      }
    case DEFINING_IDENTIFIER:
    case DEFINING_OPERATOR: {
// Store position so we need not search again.
        NODE_T *save = *seq;
        (*seq) = p;
        SEQUENCE (*seq) = save;
        (*count)++;
        return;
      }
    default: {
        genie_init_proc_op (SUB (p), seq, count);
        break;
      }
    }
  }
}

//! @brief Initialise PROC and OP identity declarations.

void genie_find_proc_op (NODE_T * p, int *count)
{
  for (; p != NO_NODE; FORWARD (p)) {
    if (GINFO (p) != NO_GINFO && IS_NEW_LEXICAL_LEVEL (GINFO (p))) {
// Don't enter a new lexical level - it will have its own initialisation.
      return;
    } else if (IS (p, PROCEDURE_DECLARATION) || IS (p, BRIEF_OPERATOR_DECLARATION)) {
      genie_init_proc_op (SUB (p), &(SEQUENCE (TABLE (p))), count);
      return;
    } else {
      genie_find_proc_op (SUB (p), count);
    }
  }
}

//! @brief Initialise stack frame.

void initialise_frame (NODE_T * p)
{
  if (INITIALISE_ANON (TABLE (p))) {
    TAG_T *_a_;
    INITIALISE_ANON (TABLE (p)) = A68_FALSE;
    for (_a_ = ANONYMOUS (TABLE (p)); _a_ != NO_TAG; FORWARD (_a_)) {
      if (PRIO (_a_) == ROUTINE_TEXT) {
        int youngest = YOUNGEST_ENVIRON (TAX (NODE (_a_)));
        A68_PROCEDURE *_z_ = (A68_PROCEDURE *) (FRAME_OBJECT (OFFSET (_a_)));
        STATUS (_z_) = INIT_MASK;
        NODE (&(BODY (_z_))) = NODE (_a_);
        if (youngest > 0) {
          STATIC_LINK_FOR_FRAME (ENVIRON (_z_), 1 + youngest);
        } else {
          ENVIRON (_z_) = 0;
        }
        LOCALE (_z_) = NO_HANDLE;
        MOID (_z_) = MOID (_a_);
        INITIALISE_ANON (TABLE (p)) = A68_TRUE;
      } else if (PRIO (_a_) == FORMAT_TEXT) {
        int youngest = YOUNGEST_ENVIRON (TAX (NODE (_a_)));
        A68_FORMAT *_z_ = (A68_FORMAT *) (FRAME_OBJECT (OFFSET (_a_)));
        STATUS (_z_) = INIT_MASK;
        BODY (_z_) = NODE (_a_);
        if (youngest > 0) {
          STATIC_LINK_FOR_FRAME (ENVIRON (_z_), 1 + youngest);
        } else {
          ENVIRON (_z_) = 0;
        }
        INITIALISE_ANON (TABLE (p)) = A68_TRUE;
      }
    }
  }
  if (PROC_OPS (TABLE (p))) {
    NODE_T *_q_;
    if (SEQUENCE (TABLE (p)) == NO_NODE) {
      int count = 0;
      genie_find_proc_op (p, &count);
      PROC_OPS (TABLE (p)) = (BOOL_T) (count > 0);
    }
    for (_q_ = SEQUENCE (TABLE (p)); _q_ != NO_NODE; _q_ = SEQUENCE (_q_)) {
      NODE_T *u = NEXT_NEXT (_q_);
      if (IS (u, ROUTINE_TEXT)) {
        NODE_T *src = SOURCE (&(GPROP (u)));
        *(A68_PROCEDURE *) FRAME_OBJECT (OFFSET (TAX (_q_))) = *(A68_PROCEDURE *) (FRAME_OBJECT (OFFSET (TAX (src))));
      } else if ((IS (u, UNIT) && IS (SUB (u), ROUTINE_TEXT))) {
        NODE_T *src = SOURCE (&(GPROP (SUB (u))));
        *(A68_PROCEDURE *) FRAME_OBJECT (OFFSET (TAX (_q_))) = *(A68_PROCEDURE *) (FRAME_OBJECT (OFFSET (TAX (src))));
      }
    }
  }
  INITIALISE_FRAME (TABLE (p)) = (BOOL_T) (INITIALISE_ANON (TABLE (p)) || PROC_OPS (TABLE (p)));
}

//! @brief Whether item at "w" of mode "q" is initialised.

void genie_check_initialisation (NODE_T * p, BYTE_T * w, MOID_T * q)
{
  switch (SHORT_ID (q)) {
  case REF_SYMBOL: {
      A68_REF *z = (A68_REF *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case PROC_SYMBOL: {
      A68_PROCEDURE *z = (A68_PROCEDURE *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_INT: {
      A68_INT *z = (A68_INT *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_REAL: {
      A68_REAL *z = (A68_REAL *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_COMPLEX: {
      A68_REAL *r = (A68_REAL *) w;
      A68_REAL *i = (A68_REAL *) (w + SIZE_ALIGNED (A68_REAL));
      CHECK_INIT (p, INITIALISED (r), q);
      CHECK_INIT (p, INITIALISED (i), q);
      return;
    }
#if (A68_LEVEL >= 3)
  case MODE_LONG_INT:
  case MODE_LONG_REAL:
  case MODE_LONG_BITS: {
      A68_DOUBLE *z = (A68_DOUBLE *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_LONG_COMPLEX: {
      A68_LONG_REAL *r = (A68_LONG_REAL *) w;
      A68_LONG_REAL *i = (A68_LONG_REAL *) (w + SIZE_ALIGNED (A68_LONG_REAL));
      CHECK_INIT (p, INITIALISED (r), q);
      CHECK_INIT (p, INITIALISED (i), q);
      return;
    }
  case MODE_LONG_LONG_INT:
  case MODE_LONG_LONG_REAL:
  case MODE_LONG_LONG_BITS: {
      MP_T *z = (MP_T *) w;
      CHECK_INIT (p, (unt) MP_STATUS (z) & INIT_MASK, q);
      return;
    }
#else
  case MODE_LONG_INT:
  case MODE_LONG_LONG_INT:
  case MODE_LONG_REAL:
  case MODE_LONG_LONG_REAL:
  case MODE_LONG_BITS:
  case MODE_LONG_LONG_BITS: {
      MP_T *z = (MP_T *) w;
      CHECK_INIT (p, (unt) MP_STATUS (z) & INIT_MASK, q);
      return;
    }
  case MODE_LONG_COMPLEX: {
      MP_T *r = (MP_T *) w;
      MP_T *i = (MP_T *) (w + size_mp ());
      CHECK_INIT (p, (unt) r[0] & INIT_MASK, q);
      CHECK_INIT (p, (unt) i[0] & INIT_MASK, q);
      return;
    }
#endif
  case MODE_LONG_LONG_COMPLEX: {
      MP_T *r = (MP_T *) w;
      MP_T *i = (MP_T *) (w + size_long_mp ());
      CHECK_INIT (p, (unt) r[0] & INIT_MASK, q);
      CHECK_INIT (p, (unt) i[0] & INIT_MASK, q);
      return;
    }
  case MODE_BOOL: {
      A68_BOOL *z = (A68_BOOL *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_CHAR: {
      A68_CHAR *z = (A68_CHAR *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_BITS: {
      A68_BITS *z = (A68_BITS *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_BYTES: {
      A68_BYTES *z = (A68_BYTES *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_LONG_BYTES: {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_FILE: {
      A68_FILE *z = (A68_FILE *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_FORMAT: {
      A68_FORMAT *z = (A68_FORMAT *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  case MODE_PIPE: {
      A68_REF *pipe_read = (A68_REF *) w;
      A68_REF *pipe_write = (A68_REF *) (w + A68_REF_SIZE);
      A68_INT *pid = (A68_INT *) (w + 2 * A68_REF_SIZE);
      CHECK_INIT (p, INITIALISED (pipe_read), q);
      CHECK_INIT (p, INITIALISED (pipe_write), q);
      CHECK_INIT (p, INITIALISED (pid), q);
      return;
    }
  case MODE_SOUND: {
      A68_SOUND *z = (A68_SOUND *) w;
      CHECK_INIT (p, INITIALISED (z), q);
      return;
    }
  }
}

//! @brief Propagator_name.

char *propagator_name (PROP_PROC * p)
{
  if (p == genie_and_function) {
    return "genie_and_function";
  }
  if (p == genie_assertion) {
    return "genie_assertion";
  }
  if (p == genie_assignation) {
    return "genie_assignation";
  }
  if (p == genie_assignation_constant) {
    return "genie_assignation_constant";
  }
  if (p == genie_call) {
    return "genie_call";
  }
  if (p == genie_cast) {
    return "genie_cast";
  }
  if (p == (PROP_PROC *) genie_closed) {
    return "genie_closed";
  }
  if (p == genie_coercion) {
    return "genie_coercion";
  }
  if (p == genie_collateral) {
    return "genie_collateral";
  }
  if (p == genie_column_function) {
    return "genie_column_function";
  }
  if (p == (PROP_PROC *) genie_conditional) {
    return "genie_conditional";
  }
  if (p == genie_constant) {
    return "genie_constant";
  }
  if (p == genie_denotation) {
    return "genie_denotation";
  }
  if (p == genie_deproceduring) {
    return "genie_deproceduring";
  }
  if (p == genie_dereference_frame_identifier) {
    return "genie_dereference_frame_identifier";
  }
  if (p == genie_dereference_selection_name_quick) {
    return "genie_dereference_selection_name_quick";
  }
  if (p == genie_dereference_slice_name_quick) {
    return "genie_dereference_slice_name_quick";
  }
  if (p == genie_dereferencing) {
    return "genie_dereferencing";
  }
  if (p == genie_dereferencing_quick) {
    return "genie_dereferencing_quick";
  }
  if (p == genie_diagonal_function) {
    return "genie_diagonal_function";
  }
  if (p == genie_dyadic) {
    return "genie_dyadic";
  }
  if (p == genie_dyadic_quick) {
    return "genie_dyadic_quick";
  }
  if (p == (PROP_PROC *) genie_enclosed) {
    return "genie_enclosed";
  }
  if (p == genie_format_text) {
    return "genie_format_text";
  }
  if (p == genie_formula) {
    return "genie_formula";
  }
  if (p == genie_generator) {
    return "genie_generator";
  }
  if (p == genie_identifier) {
    return "genie_identifier";
  }
  if (p == genie_identifier_standenv) {
    return "genie_identifier_standenv";
  }
  if (p == genie_identifier_standenv_proc) {
    return "genie_identifier_standenv_proc";
  }
  if (p == genie_identity_relation) {
    return "genie_identity_relation";
  }
  if (p == (PROP_PROC *) genie_int_case) {
    return "genie_int_case";
  }
  if (p == genie_field_selection) {
    return "genie_field_selection";
  }
  if (p == genie_frame_identifier) {
    return "genie_frame_identifier";
  }
  if (p == (PROP_PROC *) genie_loop) {
    return "genie_loop";
  }
  if (p == genie_monadic) {
    return "genie_monadic";
  }
  if (p == genie_nihil) {
    return "genie_nihil";
  }
  if (p == genie_or_function) {
    return "genie_or_function";
  }
#if defined (BUILD_PARALLEL_CLAUSE)
  if (p == genie_parallel) {
    return "genie_parallel";
  }
#endif
  if (p == genie_routine_text) {
    return "genie_routine_text";
  }
  if (p == genie_row_function) {
    return "genie_row_function";
  }
  if (p == genie_rowing) {
    return "genie_rowing";
  }
  if (p == genie_rowing_ref_row_of_row) {
    return "genie_rowing_ref_row_of_row";
  }
  if (p == genie_rowing_ref_row_row) {
    return "genie_rowing_ref_row_row";
  }
  if (p == genie_rowing_row_of_row) {
    return "genie_rowing_row_of_row";
  }
  if (p == genie_rowing_row_row) {
    return "genie_rowing_row_row";
  }
  if (p == genie_selection) {
    return "genie_selection";
  }
  if (p == genie_selection_name_quick) {
    return "genie_selection_name_quick";
  }
  if (p == genie_selection_value_quick) {
    return "genie_selection_value_quick";
  }
  if (p == genie_skip) {
    return "genie_skip";
  }
  if (p == genie_slice) {
    return "genie_slice";
  }
  if (p == genie_slice_name_quick) {
    return "genie_slice_name_quick";
  }
  if (p == genie_transpose_function) {
    return "genie_transpose_function";
  }
  if (p == genie_unit) {
    return "genie_unit";
  }
  if (p == (PROP_PROC *) genie_united_case) {
    return "genie_united_case";
  }
  if (p == genie_uniting) {
    return "genie_uniting";
  }
  if (p == genie_voiding) {
    return "genie_voiding";
  }
  if (p == genie_voiding_assignation) {
    return "genie_voiding_assignation";
  }
  if (p == genie_voiding_assignation_constant) {
    return "genie_voiding_assignation_constant";
  }
  if (p == genie_widen) {
    return "genie_widen";
  }
  if (p == genie_widen_int_to_real) {
    return "genie_widen_int_to_real";
  }
  return NO_TEXT;
}
