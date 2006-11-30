/*!
\file genie.h
\brief contains genie related definitions
**/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2006 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#if ! defined A68G_GENIE_H
#define A68G_GENIE_H

/* Macros. */

#define INITIALISED(z) ((z)->status & INITIALISED_MASK)

#define BITS_WIDTH ((int) (1 + ceil (log (MAX_INT) / log(2))))
#define INT_WIDTH ((int) (1 + floor (log (MAX_INT) / log (10))))

#define CHAR_WIDTH (1 + (int) log10 ((double) SCHAR_MAX))
#define REAL_WIDTH (DBL_DIG)
#define EXP_WIDTH ((int) (1 + log10 ((double) DBL_MAX_10_EXP)))

#define HEAP_ADDRESS(n) ((BYTE_T *) & (heap_segment[n]))

#define LHS_MODE(p) (MOID (PACK (MOID (p))))
#define RHS_MODE(p) (MOID (NEXT (PACK (MOID (p)))))

/* ADDRESS calculates the effective address of fat pointer z. */

#define ADDRESS(z)\
  (IS_IN_HEAP (z) ? &(heap_segment[REF_OFFSET (z) + REF_OFFSET (REF_HANDLE(z))])\
                  : &((IS_IN_FRAME (z) ? frame_segment : stack_segment)[REF_OFFSET (z)]))

#define DEREF(mode, expr) ((mode *) ADDRESS (expr))

/* Check on a NIL name. */

#define CHECK_NIL(p, z, m)\
  if (IS_NIL (z)) {\
    genie_check_initialisation (p, (BYTE_T *) &z, m);\
    diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_ACCESSING_NIL, (m));\
    exit_genie ((p), A_RUNTIME_ERROR);\
  }

/* Activation records in the frame stack. */

typedef struct ACTIVATION_RECORD ACTIVATION_RECORD;

struct ACTIVATION_RECORD
{
  ADDR_T static_link, dynamic_link, dynamic_scope;
  NODE_T *node;
  jmp_buf *jump_stat;
  BOOL_T proc_frame;
};

#define FRAME_DYNAMIC_LINK(n) (((ACTIVATION_RECORD *) FRAME_ADDRESS(n))->dynamic_link)
#define FRAME_DYNAMIC_SCOPE(n) (((ACTIVATION_RECORD *) FRAME_ADDRESS(n))->dynamic_scope)
#define FRAME_PROC_FRAME(n) (((ACTIVATION_RECORD *) FRAME_ADDRESS(n))->proc_frame)
#define FRAME_INCREMENT(n) (SYMBOL_TABLE (FRAME_TREE(n))->ap_increment)
#define FRAME_JUMP_STAT(n) (((ACTIVATION_RECORD *) FRAME_ADDRESS(n))->jump_stat)
#define FRAME_LEXICAL_LEVEL(n) (SYMBOL_TABLE (FRAME_TREE(n))->level)
#define FRAME_STATIC_LINK(n) (((ACTIVATION_RECORD *) FRAME_ADDRESS(n))->static_link)
#define FRAME_TREE(n) (NODE ((ACTIVATION_RECORD *) FRAME_ADDRESS(n)))
#define FRAME_INFO_SIZE (ALIGN (SIZE_OF (ACTIVATION_RECORD)))
#define FRAME_ADDRESS(n) ((BYTE_T *) &frame_segment[n])
#define FRAME_LOCAL(n, m) (FRAME_ADDRESS ((n) + FRAME_INFO_SIZE + (m)))
#define FRAME_OFFSET(n) (FRAME_ADDRESS (frame_pointer + (n)))
#define FRAME_TOP (FRAME_OFFSET (0))
#define FRAME_CLEAR(m) FILL ((BYTE_T *) FRAME_OFFSET (FRAME_INFO_SIZE), 0, (m))
#define FRAME_SIZE(fp) (FRAME_INFO_SIZE + FRAME_INCREMENT (fp))

#define FOLLOW_STATIC_LINK(dest, l) {\
  if ((l) == global_level) {\
    (dest) = global_pointer;\
  } else {\
    ADDR_T m_sl = frame_pointer;\
    while ((l) != FRAME_LEXICAL_LEVEL (m_sl)) {\
      m_sl = FRAME_STATIC_LINK (m_sl);\
    }\
    (dest) = m_sl;\
  }}

#define FRAME_GET(dest, cast, p) {\
  ADDR_T m_z;\
  FOLLOW_STATIC_LINK (m_z, (p)->genie.level);\
  (dest) = (cast *) & ((p)->genie.offset[m_z]);\
  }

/* Macros for row-handling. */

#define GET_DESCRIPTOR(a, t, p)\
  a = (A68_ARRAY *) ADDRESS (p);\
  t = (A68_TUPLE *) & (((BYTE_T *) (a)) [SIZE_OF (A68_ARRAY)]);

#define PUT_DESCRIPTOR(a, t1, p) {\
  BYTE_T *a_p = ADDRESS (p);\
  *(A68_ARRAY *) a_p = (a);\
  *(A68_TUPLE *) &(((BYTE_T *) (a_p)) [SIZE_OF (A68_ARRAY)]) = (t1);\
  }

#define PUT_DESCRIPTOR2(a, t1, t2, p) {\
  /* ABNORMAL_END (IS_NIL (*p), ERROR_NIL_DESCRIPTOR, NULL); */\
  BYTE_T *a_p = ADDRESS (p);\
  *(A68_ARRAY *) a_p = (a);\
  *(A68_TUPLE *) &(((BYTE_T *) (a_p)) [SIZE_OF (A68_ARRAY)]) = (t1);\
  *(A68_TUPLE *) &(((BYTE_T *) (a_p)) [SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE)]) = (t2);\
  }

#define ROW_SIZE(t) (((t)->upper_bound >= (t)->lower_bound) ? ((t)->upper_bound - (t)->lower_bound + 1) : 0)

#define ROW_ELEMENT(a, k) (((ADDR_T) k + (a)->slice_offset) * (a)->elem_size + (a)->field_offset)

#define INDEX_1_DIM(a, t, k) ROW_ELEMENT (a, ((t)->span * ((int) k - (t)->shift)))

/* Macros for execution. */

extern unsigned check_time_limit_count;

#define CHECK_TIME_LIMIT(p) {\
  double m_t = p->info->module->options.time_limit;\
  BOOL_T m_trace_mood = (MASK (p) & TRACE_MASK) != 0;\
  if (m_t > 0 && (((check_time_limit_count++) % 100) == 0)) {\
    if ((seconds () - cputime_0) > m_t) {\
      diagnostic_node (A_RUNTIME_ERROR, (NODE_T *) p, ERROR_TIME_LIMIT_EXCEEDED);\
      exit_genie ((NODE_T *) p, A_RUNTIME_ERROR);\
    }\
  } else if (sys_request_flag) {\
    single_step ((NODE_T *) p, A_TRUE, A_FALSE);\
  } else if (m_trace_mood) {\
    where (STDOUT_FILENO, (NODE_T *) p);\
  }}

#define EXECUTE_UNIT_2(p, dest) {\
  PROPAGATOR_T *prop = &((p)->genie.propagator);\
  last_unit = p;\
  dest = prop->unit (prop->source);\
  }

#define EXECUTE_UNIT(p) {\
  PROPAGATOR_T *prop = &((p)->genie.propagator);\
  last_unit = p;\
  prop->unit (prop->source);\
  }

#define EXECUTE_UNIT_TRACE(p) {\
  PROPAGATOR_T *prop = &((p)->genie.propagator);\
  if (sys_request_flag) {\
    single_step ((p), A_TRUE, A_FALSE);\
  } else if (MASK (p) & BREAKPOINT_MASK) {\
    if (INFO (p)->expr == NULL) {\
      sys_request_flag = A_FALSE;\
      single_step ((p), A_FALSE, A_TRUE);\
    } else if (breakpoint_expression (p)) {\
      sys_request_flag = A_FALSE;\
      single_step ((p), A_FALSE, A_TRUE);\
    }\
  } else if (MASK (p) & TRACE_MASK) {\
    where (STDOUT_FILENO, (p));\
  }\
  last_unit = p;\
  prop->unit (prop->source);\
  }

/* Macro's for the garbage collector. */

/* Store intermediate REF to save it from the GC. */

#define PROTECT_FROM_SWEEP(p, z)\
  if ((p)->protect_sweep != NULL) {\
    *(A68_REF *) FRAME_LOCAL (frame_pointer, (p)->protect_sweep->offset) = *(A68_REF *) (z);\
  }

#define PROTECT_FROM_SWEEP_STACK(p)\
  if ((p)->protect_sweep != NULL) {\
    *(A68_REF *) FRAME_LOCAL (frame_pointer, (p)->protect_sweep->offset) =\
    *(A68_REF *) (STACK_OFFSET (- SIZE_OF (A68_REF)));\
  }

#define PREEMPTIVE_SWEEP {\
  double f = (double) heap_pointer / (double) heap_size;\
  double h = (double) free_handle_count / (double) max_handle_count;\
  if (f > 0.8 || h < 0.2) {\
    sweep_heap ((NODE_T *) p, frame_pointer);\
  }}

extern int block_heap_compacter;

#define UP_SWEEP_SEMA {block_heap_compacter++;}
#define DOWN_SWEEP_SEMA {block_heap_compacter--;}

#define PROTECT_SWEEP_HANDLE(z) { if (IS_IN_HEAP (z)) {(REF_HANDLE(z))->status |= NO_SWEEP_MASK;} }
#define UNPROTECT_SWEEP_HANDLE(z) { if (IS_IN_HEAP (z)) {(REF_HANDLE (z))->status &= ~NO_SWEEP_MASK;} }

/* Tests for objects of mode INT. */

#if defined HAVE_IEEE_754
#if INT_MAX == 2147483647
#define TEST_INT_ADDITION(p, i, j) {\
  double _sum_ = (double) (i) + (double) (j);\
  if (ABS (_sum_) > INT_MAX) {\
    errno = ERANGE;\
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (INT), NULL);\
    exit_genie (p, A_RUNTIME_ERROR);\
  }}
#else
#define TEST_INT_ADDITION(p, i, j)\
  if (((i ^ j) & MIN_INT) == 0 && ABS (i) > INT_MAX - ABS (j)) {\
    errno = ERANGE;\
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (INT), NULL);\
    exit_genie (p, A_RUNTIME_ERROR);\
    }
#endif
#define TEST_INT_MULTIPLICATION(p, i, j) {\
  double _prod_ = (double ) (i) * (double) (j);\
  if (ABS (_prod_) > MAX_INT) {\
    errno = ERANGE;\
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (INT), NULL);\
    exit_genie (p, A_RUNTIME_ERROR);\
  }}
#else
#define TEST_INT_ADDITION(p, i, j) {;}
#define TEST_TIMES_OVERFLOW_INT(p, i, j) {;}
#endif

/* Tests for objects of mode REAL. */

#if defined HAVE_IEEE_754
#define NOT_A_REAL(x) (!finite (x))
#define TEST_REAL_REPRESENTATION(p, u)\
  if (NOT_A_REAL (u)) {\
    errno = ERANGE;\
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (REAL), NULL);\
    exit_genie (p, A_RUNTIME_ERROR);\
  }
#else
#define TEST_REAL_REPRESENTATION(p, u) {;}
#endif

#if defined HAVE_IEEE_754
#define TEST_COMPLEX_REPRESENTATION(p, u, v)\
  if (NOT_A_REAL (u) || NOT_A_REAL (v)) {\
    errno = ERANGE;\
    diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (COMPLEX), NULL);\
    exit_genie (p, A_RUNTIME_ERROR);\
  }
#else
#define TEST_COMPLEX_REPRESENTATION(p, u, v) {;}
#endif

#if defined HAVE_IEEE_754
#define TEST_TIMES_OVERFLOW_REAL(p, u, v) {;}
#else
#define TEST_TIMES_OVERFLOW_REAL(p, u, v)\
  if (v != 0.0) {\
    if ((u >= 0 ? u : -u) > DBL_MAX / (v >= 0 ? v : -v)) {\
      errno = ERANGE;\
      diagnostic_node (A_RUNTIME_ERROR, p, ERROR_MATH, MODE (REAL), NULL);\
      exit_genie (p, A_RUNTIME_ERROR);\
      }\
  }
#endif

/*
Macro's for stack checking. Since the stacks grow by small amounts at a time
(A68 rows are in the heap), we check the stacks only at certain points: where
A68 recursion may set in, or in the garbage collector. We check whether there
still is sufficient overhead to make it to the next check.
*/

#define TOO_COMPLEX "program too complex"

#if defined HAVE_SYSTEM_STACK_CHECK
#define SYSTEM_STACK_USED (ABS ((int) system_stack_offset - (int) &stack_offset))
#define LOW_STACK_ALERT(p) {\
  BYTE_T stack_offset;\
  if (stack_size > 0 && SYSTEM_STACK_USED > stack_limit) {\
    errno = 0;\
    if ((p) == NULL) {\
      ABNORMAL_END (A_TRUE, TOO_COMPLEX, ERROR_STACK_OVERFLOW);\
    } else {\
      diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_STACK_OVERFLOW);\
      exit_genie ((p), A_RUNTIME_ERROR);\
    }\
  }\
  if ((p) != NULL && stack_pointer > expr_stack_limit) {\
    errno = 0;\
    diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_STACK_OVERFLOW);\
    exit_genie ((p), A_RUNTIME_ERROR);\
  }\
  if ((p) != NULL && frame_pointer > frame_stack_limit) { \
    errno = 0;\
    diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_STACK_OVERFLOW);\
    exit_genie ((p), A_RUNTIME_ERROR);\
  }}
#else
#define LOW_STACK_ALERT(p) {\
  (void) (p);\
  errno = 0;\
  ABNORMAL_END (stack_pointer > expr_stack_limit, TOO_COMPLEX, ERROR_STACK_OVERFLOW);\
  ABNORMAL_END (frame_pointer > frame_stack_limit, TOO_COMPLEX, ERROR_STACK_OVERFLOW);\
  }
#endif

/* Opening of stack frames is in-line. */

/* 
STATIC_LINK_FOR_FRAME: determine static link for stack frame.
new_lex_lvl: lexical level of new stack frame.
returns: static link for stack frame at 'new_lex_lvl'. 
*/

#define STATIC_LINK_FOR_FRAME(dest, new_lex_lvl) {\
  int m_cur_lex_lvl = FRAME_LEXICAL_LEVEL (frame_pointer);\
  if (m_cur_lex_lvl == (new_lex_lvl)) {\
    (dest) = FRAME_STATIC_LINK (frame_pointer);\
  } else if (m_cur_lex_lvl > (new_lex_lvl)) {\
    ADDR_T m_static_link = frame_pointer;\
    while (FRAME_LEXICAL_LEVEL (m_static_link) >= (new_lex_lvl)) {\
      m_static_link = FRAME_STATIC_LINK (m_static_link);\
    }\
    (dest) = m_static_link;\
  } else {\
    (dest) = frame_pointer;\
  }}

#define OPEN_STATIC_FRAME(p) {\
  ADDR_T dynamic_link = frame_pointer, static_link;\
  ACTIVATION_RECORD *act;\
  /* LOW_STACK_ALERT (p); */\
  STATIC_LINK_FOR_FRAME (static_link, LEX_LEVEL (p));\
  frame_pointer += FRAME_SIZE (dynamic_link);\
  act = (ACTIVATION_RECORD *) FRAME_ADDRESS (frame_pointer);\
  act->static_link = static_link;\
  act->dynamic_link = dynamic_link;\
  act->dynamic_scope = frame_pointer;\
  act->node = p;\
  act->jump_stat = NULL;\
  act->proc_frame = A_FALSE;\
  FRAME_CLEAR (SYMBOL_TABLE (p)->ap_increment);\
  if (SYMBOL_TABLE (p)->initialise_frame) {\
    initialise_frame (p);\
  }}

#define OPEN_PROC_FRAME(p, environ) {\
  ADDR_T dynamic_link = frame_pointer, static_link;\
  ACTIVATION_RECORD *act;\
  LOW_STACK_ALERT (p);\
  static_link = (environ > 0 ? environ : frame_pointer);\
  if (frame_pointer < static_link) {\
    diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_SCOPE_DYNAMIC_0);\
    exit_genie (p, A_RUNTIME_ERROR);\
  }\
  frame_pointer += FRAME_SIZE (dynamic_link);\
  act = (ACTIVATION_RECORD *) FRAME_ADDRESS (frame_pointer);\
  act->static_link = static_link;\
  act->dynamic_link = dynamic_link;\
  act->dynamic_scope = frame_pointer;\
  act->node = p;\
  act->jump_stat = NULL;\
  act->proc_frame = A_TRUE;\
  FRAME_CLEAR (SYMBOL_TABLE (p)->ap_increment);\
  if (SYMBOL_TABLE (p)->initialise_frame) {\
    initialise_frame (p);\
  }}

#define CLOSE_FRAME {\
  frame_pointer = FRAME_DYNAMIC_LINK (frame_pointer);\
  }

/* Macros for check on initialisation of values. */

#define CHECK_INIT(p, c, q)\
  if (!(c)) {\
    diagnostic_node (A_RUNTIME_ERROR, (p), ERROR_EMPTY_VALUE_FROM, (q));\
    exit_genie ((p), A_RUNTIME_ERROR);\
  }

#define CHECK_INIT_GENERIC(p, w, q) {\
  switch ((q)->short_id) {\
  case MODE_INT: {\
      CHECK_INIT ((p), INITIALISED ((A68_INT *) (w)), (q));\
      break;\
    }\
  case MODE_REAL: {\
      CHECK_INIT ((p), INITIALISED ((A68_REAL *) (w)), (q));\
      break;\
    }\
  case MODE_BOOL: {\
      CHECK_INIT ((p), INITIALISED ((A68_BOOL *) (w)), (q));\
      break;\
    }\
  case MODE_CHAR: {\
      CHECK_INIT ((p), INITIALISED ((A68_CHAR *) (w)), (q));\
      break;\
    }\
  case MODE_BITS: {\
      CHECK_INIT ((p), INITIALISED ((A68_BITS *) (w)), (q));\
      break;\
    }\
  case MODE_COMPLEX: {\
      A68_REAL *_r_ = (A68_REAL *) (w);\
      A68_REAL *_i_ = &(_r_[1]);\
      CHECK_INIT ((p), INITIALISED (_r_), (q));\
      CHECK_INIT ((p), INITIALISED (_i_), (q));\
      break;\
    }\
  case ROW_SYMBOL:\
  case REF_SYMBOL: {\
      CHECK_INIT ((p), INITIALISED ((A68_REF *) (w)), (q));\
      break;\
    }\
  case PROC_SYMBOL: {\
      CHECK_INIT ((p), INITIALISED ((A68_PROCEDURE *) (w)), (q));\
      break;\
    }\
  default: {\
      genie_check_initialisation ((p), (BYTE_T *) (w), (q));\
      break;\
    }\
  }\
}

/* Stack manipulation. */

#define STACK_ADDRESS(n) ((BYTE_T *) &(stack_segment[(n)]))
#define STACK_OFFSET(n) (STACK_ADDRESS (stack_pointer + (n)))
#define STACK_TOP (STACK_ADDRESS (stack_pointer))

#define INCREMENT_STACK_POINTER(err, i) {stack_pointer += ALIGN (i); (void) (err);}

#define DECREMENT_STACK_POINTER(err, i) {\
  stack_pointer -= ALIGN (i);\
  (void) (err);\
  }

#define PUSH(p, addr, size) {\
  BYTE_T *sp = STACK_TOP;\
  INCREMENT_STACK_POINTER ((p), (size));\
  COPY (sp, (BYTE_T *) (addr), (unsigned) (size));\
  }

#define POP(p, addr, size) {\
  DECREMENT_STACK_POINTER((p), (size));\
  COPY ((BYTE_T *) (addr), STACK_TOP, (unsigned) (size));\
  }

#define POP_ADDRESS(p, addr, type) {\
  DECREMENT_STACK_POINTER((p), SIZE_OF (type));\
  (addr) = (type *) STACK_TOP;\
  }

#define PUSH_PRIMITIVE(p, z, mode) {\
  mode *_x_ = (mode *) STACK_TOP;\
  _x_->status = INITIALISED_MASK;\
  _x_->value = (z);\
  INCREMENT_STACK_POINTER((p), SIZE_OF (mode));\
  }

#define POP_PRIMITIVE(p, z, mode) {\
  DECREMENT_STACK_POINTER((p), SIZE_OF (mode));\
  (*(z)) = *((mode *) STACK_TOP);\
  }

#define PUSH_COMPLEX(p, re, im) {\
  PUSH_PRIMITIVE (p, re, A68_REAL);\
  PUSH_PRIMITIVE (p, im, A68_REAL);\
  }

#define POP_COMPLEX(p, re, im) {\
  POP_PRIMITIVE (p, im, A68_REAL);\
  POP_PRIMITIVE (p, re, A68_REAL);\
  }

#define PUSH_BYTES(p, k) {\
  A68_BYTES *_z_ = (A68_BYTES *) STACK_TOP;\
  _z_->status = INITIALISED_MASK;\
  strncpy (_z_->value, k, BYTES_WIDTH);\
  INCREMENT_STACK_POINTER((p), SIZE_OF (A68_BYTES));\
  }

#define POP_BYTES(p, k) {\
  DECREMENT_STACK_POINTER((p), SIZE_OF (A68_BYTES));\
  (*(k)) = *((A68_BYTES *) STACK_TOP);\
  }

#define PUSH_LONG_BYTES(p, k) {\
  A68_LONG_BYTES *_z_ = (A68_LONG_BYTES *) STACK_TOP;\
  _z_->status = INITIALISED_MASK;\
  strncpy (_z_->value, k, LONG_BYTES_WIDTH);\
  INCREMENT_STACK_POINTER((p), SIZE_OF (A68_LONG_BYTES));\
  }

#define POP_LONG_BYTES(p, k) {\
  DECREMENT_STACK_POINTER((p), SIZE_OF (A68_LONG_BYTES));\
  (*(k)) = *((A68_LONG_BYTES *) STACK_TOP);\
  }

#define PUSH_REF(p, z) {\
  * (A68_REF *) STACK_TOP = (z);\
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_REF));\
  }

#define PUSH_ROW PUSH_REF

#define PUSH_REF_FILE PUSH_REF

#define POP_REF(p, z) {\
  DECREMENT_STACK_POINTER((p), SIZE_OF (A68_REF));\
  (*(z)) = *((A68_REF *) STACK_TOP);\
  }

#define POP_ROW POP_REF

#define PUSH_CHANNEL(p, z) {\
  * (A68_CHANNEL *) STACK_TOP = (z);\
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_CHANNEL));\
  }

#define PUSH_UNION(p, z) {\
  A68_UNION *_w_ = (A68_UNION *) STACK_TOP;\
  _w_->status = INITIALISED_MASK;\
  _w_->value = z;\
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_UNION));\
  }

#define POP_OPERAND_ADDRESS(p, i, type) {\
  (void) (p);\
  (i) = (type *) (STACK_OFFSET (-SIZE_OF (type)));\
  }

#define POP_OPERAND_ADDRESSES(p, i, j, type) {\
  DECREMENT_STACK_POINTER ((p), SIZE_OF (type));\
  (j) = (type *) STACK_TOP;\
  (i) = (type *) (STACK_OFFSET (-SIZE_OF (type)));\
  }

/* Macro's for standard environ. */

#define A68_ENV_INT(n, k) void n (NODE_T *p) {PUSH_PRIMITIVE (p, (k), A68_INT);}
#define A68_ENV_REAL(n, z) void n (NODE_T *p) {PUSH_PRIMITIVE (p, (z), A68_REAL);}

/* External symbols. */

extern ADDR_T frame_pointer, stack_pointer, heap_pointer, handle_pointer, global_pointer;
extern A68_FORMAT nil_format;
extern A68_HANDLE nil_handle, *free_handles, *busy_handles;
extern A68_REF nil_ref;
extern A68_REF stand_in, stand_out;
extern BOOL_T in_monitor;
extern BYTE_T *frame_segment, *stack_segment, *heap_segment, *handle_segment;
extern MOID_T *top_expr_moid;
extern double cputime_0;
extern int global_level, max_lex_lvl, ret_code;
extern jmp_buf genie_exit_label, monitor_exit_label;

extern int frame_stack_size, expr_stack_size, heap_size, handle_pool_size;
extern int stack_limit, frame_stack_limit, expr_stack_limit;
extern int storage_overhead;

/* External symbols. */

#if defined HAVE_POSIX_THREADS
extern int parallel_clauses;
extern pthread_t main_thread_id;
extern BOOL_T in_par_clause;
extern BOOL_T is_main_thread (void);
extern void count_parallel_clauses (NODE_T *);
extern void genie_abend_thread (void);
extern void zap_all_threads (NODE_T *, jmp_buf *, NODE_T *);
#endif

extern int free_handle_count, max_handle_count;
extern void sweep_heap (NODE_T *, ADDR_T);

extern GENIE_PROCEDURE genie_abs_bool;
extern GENIE_PROCEDURE genie_abs_char;
extern GENIE_PROCEDURE genie_abs_complex;
extern GENIE_PROCEDURE genie_abs_int;
extern GENIE_PROCEDURE genie_abs_long_complex;
extern GENIE_PROCEDURE genie_abs_long_mp;
extern GENIE_PROCEDURE genie_abs_real;
extern GENIE_PROCEDURE genie_acos_long_complex;
extern GENIE_PROCEDURE genie_acos_long_mp;
extern GENIE_PROCEDURE genie_acronym;
extern GENIE_PROCEDURE genie_add_bytes;
extern GENIE_PROCEDURE genie_add_char;
extern GENIE_PROCEDURE genie_add_complex;
extern GENIE_PROCEDURE genie_add_int;
extern GENIE_PROCEDURE genie_add_long_bytes;
extern GENIE_PROCEDURE genie_add_long_complex;
extern GENIE_PROCEDURE genie_add_long_int;
extern GENIE_PROCEDURE genie_add_long_mp;
extern GENIE_PROCEDURE genie_add_real;
extern GENIE_PROCEDURE genie_add_string;
extern GENIE_PROCEDURE genie_and_bits;
extern GENIE_PROCEDURE genie_and_bool;
extern GENIE_PROCEDURE genie_and_long_mp;
extern GENIE_PROCEDURE genie_arccos_real;
extern GENIE_PROCEDURE genie_arccosh_long_mp;
extern GENIE_PROCEDURE genie_arcsin_real;
extern GENIE_PROCEDURE genie_arcsinh_long_mp;
extern GENIE_PROCEDURE genie_arctan_real;
extern GENIE_PROCEDURE genie_arctanh_long_mp;
extern GENIE_PROCEDURE genie_arg_complex;
extern GENIE_PROCEDURE genie_arg_long_complex;
extern GENIE_PROCEDURE genie_asin_long_complex;
extern GENIE_PROCEDURE genie_asin_long_mp;
extern GENIE_PROCEDURE genie_atan2_real;
extern GENIE_PROCEDURE genie_atan_long_complex;
extern GENIE_PROCEDURE genie_atan_long_mp;
extern GENIE_PROCEDURE genie_atan2_long_mp;
extern GENIE_PROCEDURE genie_bin_int;
extern GENIE_PROCEDURE genie_bin_long_mp;
extern GENIE_PROCEDURE genie_bits_lengths;
extern GENIE_PROCEDURE genie_bits_pack;
extern GENIE_PROCEDURE genie_bits_shorths;
extern GENIE_PROCEDURE genie_bits_width;
extern GENIE_PROCEDURE genie_break;
extern GENIE_PROCEDURE genie_debug;
extern GENIE_PROCEDURE genie_bytes_lengths;
extern GENIE_PROCEDURE genie_bytes_shorths;
extern GENIE_PROCEDURE genie_bytes_width;
extern GENIE_PROCEDURE genie_bytespack;
extern GENIE_PROCEDURE genie_char_in_string;
extern GENIE_PROCEDURE genie_complex_lengths;
extern GENIE_PROCEDURE genie_complex_shorths;
extern GENIE_PROCEDURE genie_conj_complex;
extern GENIE_PROCEDURE genie_conj_long_complex;
extern GENIE_PROCEDURE genie_cos_long_complex;
extern GENIE_PROCEDURE genie_cos_long_mp;
extern GENIE_PROCEDURE genie_cos_real;
extern GENIE_PROCEDURE genie_cosh_long_mp;
extern GENIE_PROCEDURE genie_cputime;
extern GENIE_PROCEDURE genie_curt_long_mp;
extern GENIE_PROCEDURE genie_curt_real;
extern GENIE_PROCEDURE genie_div_complex;
extern GENIE_PROCEDURE genie_div_int;
extern GENIE_PROCEDURE genie_div_long_complex;
extern GENIE_PROCEDURE genie_div_long_mp;
extern GENIE_PROCEDURE genie_div_real;
extern GENIE_PROCEDURE genie_divab_complex;
extern GENIE_PROCEDURE genie_divab_long_complex;
extern GENIE_PROCEDURE genie_divab_long_mp;
extern GENIE_PROCEDURE genie_down_sema;
extern GENIE_PROCEDURE genie_dyad_elems;
extern GENIE_PROCEDURE genie_dyad_lwb;
extern GENIE_PROCEDURE genie_dyad_upb;
extern GENIE_PROCEDURE genie_elem_bits;
extern GENIE_PROCEDURE genie_elem_bytes;
extern GENIE_PROCEDURE genie_elem_long_bits;
extern GENIE_PROCEDURE genie_elem_long_bytes;
extern GENIE_PROCEDURE genie_elem_longlong_bits;
extern GENIE_PROCEDURE genie_elem_string;
extern GENIE_PROCEDURE genie_entier_long_mp;
extern GENIE_PROCEDURE genie_entier_real;
extern GENIE_PROCEDURE genie_eq_bits;
extern GENIE_PROCEDURE genie_eq_bool;
extern GENIE_PROCEDURE genie_eq_bytes;
extern GENIE_PROCEDURE genie_eq_char;
extern GENIE_PROCEDURE genie_eq_complex;
extern GENIE_PROCEDURE genie_eq_int;
extern GENIE_PROCEDURE genie_eq_long_bytes;
extern GENIE_PROCEDURE genie_eq_long_complex;
extern GENIE_PROCEDURE genie_eq_long_mp;
extern GENIE_PROCEDURE genie_eq_real;
extern GENIE_PROCEDURE genie_eq_string;
extern GENIE_PROCEDURE genie_evaluate;
extern GENIE_PROCEDURE genie_exp_long_complex;
extern GENIE_PROCEDURE genie_exp_long_mp;
extern GENIE_PROCEDURE genie_exp_real;
extern GENIE_PROCEDURE genie_exp_width;
extern GENIE_PROCEDURE genie_first_random;
extern GENIE_PROCEDURE genie_garbage_collections;
extern GENIE_PROCEDURE genie_garbage_freed;
extern GENIE_PROCEDURE genie_garbage_seconds;
extern GENIE_PROCEDURE genie_ge_bits;
extern GENIE_PROCEDURE genie_ge_bytes;
extern GENIE_PROCEDURE genie_ge_char;
extern GENIE_PROCEDURE genie_ge_int;
extern GENIE_PROCEDURE genie_ge_long_bytes;
extern GENIE_PROCEDURE genie_ge_long_mp;
extern GENIE_PROCEDURE genie_ge_real;
extern GENIE_PROCEDURE genie_ge_string;
extern GENIE_PROCEDURE genie_gt_bits;
extern GENIE_PROCEDURE genie_gt_bytes;
extern GENIE_PROCEDURE genie_gt_char;
extern GENIE_PROCEDURE genie_gt_int;
extern GENIE_PROCEDURE genie_gt_long_bytes;
extern GENIE_PROCEDURE genie_gt_long_mp;
extern GENIE_PROCEDURE genie_gt_real;
extern GENIE_PROCEDURE genie_gt_string;
extern GENIE_PROCEDURE genie_icomplex;
extern GENIE_PROCEDURE genie_idf;
extern GENIE_PROCEDURE genie_idle;
extern GENIE_PROCEDURE genie_iint_complex;
extern GENIE_PROCEDURE genie_im_complex;
extern GENIE_PROCEDURE genie_im_long_complex;
extern GENIE_PROCEDURE genie_int_lengths;
extern GENIE_PROCEDURE genie_int_shorths;
extern GENIE_PROCEDURE genie_int_width;
extern GENIE_PROCEDURE genie_last_char_in_string;
extern GENIE_PROCEDURE genie_le_bits;
extern GENIE_PROCEDURE genie_le_bytes;
extern GENIE_PROCEDURE genie_le_char;
extern GENIE_PROCEDURE genie_le_int;
extern GENIE_PROCEDURE genie_le_long_bytes;
extern GENIE_PROCEDURE genie_le_long_mp;
extern GENIE_PROCEDURE genie_le_real;
extern GENIE_PROCEDURE genie_le_string;
extern GENIE_PROCEDURE genie_leng_bytes;
extern GENIE_PROCEDURE genie_lengthen_complex_to_long_complex;
extern GENIE_PROCEDURE genie_lengthen_int_to_long_mp;
extern GENIE_PROCEDURE genie_lengthen_long_complex_to_longlong_complex;
extern GENIE_PROCEDURE genie_lengthen_long_mp_to_longlong_mp;
extern GENIE_PROCEDURE genie_lengthen_real_to_long_mp;
extern GENIE_PROCEDURE genie_lengthen_unsigned_to_long_mp;
extern GENIE_PROCEDURE genie_level_int_sema;
extern GENIE_PROCEDURE genie_level_sema_int;
extern GENIE_PROCEDURE genie_ln_long_complex;
extern GENIE_PROCEDURE genie_ln_long_mp;
extern GENIE_PROCEDURE genie_ln_real;
extern GENIE_PROCEDURE genie_log_long_mp;
extern GENIE_PROCEDURE genie_log_real;
extern GENIE_PROCEDURE genie_long_bits_pack;
extern GENIE_PROCEDURE genie_long_bits_width;
extern GENIE_PROCEDURE genie_long_bytes_width;
extern GENIE_PROCEDURE genie_long_bytespack;
extern GENIE_PROCEDURE genie_long_exp_width;
extern GENIE_PROCEDURE genie_long_int_width;
extern GENIE_PROCEDURE genie_long_max_bits;
extern GENIE_PROCEDURE genie_long_max_int;
extern GENIE_PROCEDURE genie_long_max_real;
extern GENIE_PROCEDURE genie_long_next_random;
extern GENIE_PROCEDURE genie_long_real_width;
extern GENIE_PROCEDURE genie_long_small_real;
extern GENIE_PROCEDURE genie_longlong_bits_width;
extern GENIE_PROCEDURE genie_longlong_exp_width;
extern GENIE_PROCEDURE genie_longlong_int_width;
extern GENIE_PROCEDURE genie_longlong_max_bits;
extern GENIE_PROCEDURE genie_longlong_max_int;
extern GENIE_PROCEDURE genie_longlong_max_real;
extern GENIE_PROCEDURE genie_longlong_real_width;
extern GENIE_PROCEDURE genie_longlong_small_real;
extern GENIE_PROCEDURE genie_lt_bits;
extern GENIE_PROCEDURE genie_lt_bytes;
extern GENIE_PROCEDURE genie_lt_char;
extern GENIE_PROCEDURE genie_lt_int;
extern GENIE_PROCEDURE genie_lt_long_bytes;
extern GENIE_PROCEDURE genie_lt_long_mp;
extern GENIE_PROCEDURE genie_lt_real;
extern GENIE_PROCEDURE genie_lt_string;
extern GENIE_PROCEDURE genie_make_term;
extern GENIE_PROCEDURE genie_max_abs_char;
extern GENIE_PROCEDURE genie_max_bits;
extern GENIE_PROCEDURE genie_max_int;
extern GENIE_PROCEDURE genie_max_real;
extern GENIE_PROCEDURE genie_minus_complex;
extern GENIE_PROCEDURE genie_minus_int;
extern GENIE_PROCEDURE genie_minus_long_complex;
extern GENIE_PROCEDURE genie_minus_long_int;
extern GENIE_PROCEDURE genie_minus_long_mp;
extern GENIE_PROCEDURE genie_minus_real;
extern GENIE_PROCEDURE genie_minusab_complex;
extern GENIE_PROCEDURE genie_minusab_int;
extern GENIE_PROCEDURE genie_minusab_long_complex;
extern GENIE_PROCEDURE genie_minusab_long_int;
extern GENIE_PROCEDURE genie_minusab_long_mp;
extern GENIE_PROCEDURE genie_minusab_real;
extern GENIE_PROCEDURE genie_mod_int;
extern GENIE_PROCEDURE genie_mod_long_mp;
extern GENIE_PROCEDURE genie_modab_int;
extern GENIE_PROCEDURE genie_modab_long_mp;
extern GENIE_PROCEDURE genie_monad_elems;
extern GENIE_PROCEDURE genie_monad_lwb;
extern GENIE_PROCEDURE genie_monad_upb;
extern GENIE_PROCEDURE genie_mul_complex;
extern GENIE_PROCEDURE genie_mul_int;
extern GENIE_PROCEDURE genie_mul_long_complex;
extern GENIE_PROCEDURE genie_mul_long_int;
extern GENIE_PROCEDURE genie_mul_long_mp;
extern GENIE_PROCEDURE genie_mul_real;
extern GENIE_PROCEDURE genie_ne_bits;
extern GENIE_PROCEDURE genie_ne_bool;
extern GENIE_PROCEDURE genie_ne_bytes;
extern GENIE_PROCEDURE genie_ne_char;
extern GENIE_PROCEDURE genie_ne_complex;
extern GENIE_PROCEDURE genie_ne_int;
extern GENIE_PROCEDURE genie_ne_long_bytes;
extern GENIE_PROCEDURE genie_ne_long_complex;
extern GENIE_PROCEDURE genie_ne_long_mp;
extern GENIE_PROCEDURE genie_ne_real;
extern GENIE_PROCEDURE genie_ne_string;
extern GENIE_PROCEDURE genie_next_random;
extern GENIE_PROCEDURE genie_nint_real;
extern GENIE_PROCEDURE genie_not_bits;
extern GENIE_PROCEDURE genie_not_bool;
extern GENIE_PROCEDURE genie_not_long_mp;
extern GENIE_PROCEDURE genie_null_char;
extern GENIE_PROCEDURE genie_odd_int;
extern GENIE_PROCEDURE genie_odd_long_mp;
extern GENIE_PROCEDURE genie_or_bits;
extern GENIE_PROCEDURE genie_or_bool;
extern GENIE_PROCEDURE genie_or_long_mp;
extern GENIE_PROCEDURE genie_over_int;
extern GENIE_PROCEDURE genie_over_long_mp;
extern GENIE_PROCEDURE genie_overab_int;
extern GENIE_PROCEDURE genie_overab_long_mp;
extern GENIE_PROCEDURE genie_divab_real;
extern GENIE_PROCEDURE genie_pi;
extern GENIE_PROCEDURE genie_pi_long_mp;
extern GENIE_PROCEDURE genie_plusab_bytes;
extern GENIE_PROCEDURE genie_plusab_complex;
extern GENIE_PROCEDURE genie_plusab_int;
extern GENIE_PROCEDURE genie_plusab_long_bytes;
extern GENIE_PROCEDURE genie_plusab_long_complex;
extern GENIE_PROCEDURE genie_plusab_long_int;
extern GENIE_PROCEDURE genie_plusab_long_mp;
extern GENIE_PROCEDURE genie_plusab_real;
extern GENIE_PROCEDURE genie_plusab_string;
extern GENIE_PROCEDURE genie_plusto_bytes;
extern GENIE_PROCEDURE genie_plusto_long_bytes;
extern GENIE_PROCEDURE genie_plusto_string;
extern GENIE_PROCEDURE genie_pow_complex_int;
extern GENIE_PROCEDURE genie_pow_int;
extern GENIE_PROCEDURE genie_pow_long_complex_int;
extern GENIE_PROCEDURE genie_pow_long_mp;
extern GENIE_PROCEDURE genie_pow_long_mp_int;
extern GENIE_PROCEDURE genie_pow_long_mp_int_int;
extern GENIE_PROCEDURE genie_pow_real;
extern GENIE_PROCEDURE genie_pow_real_int;
extern GENIE_PROCEDURE genie_preemptive_sweep_heap;
extern GENIE_PROCEDURE genie_program_idf;
extern GENIE_PROCEDURE genie_re_complex;
extern GENIE_PROCEDURE genie_re_long_complex;
extern GENIE_PROCEDURE genie_real_lengths;
extern GENIE_PROCEDURE genie_real_shorths;
extern GENIE_PROCEDURE genie_real_width;
extern GENIE_PROCEDURE genie_repr_char;
extern GENIE_PROCEDURE genie_round_long_mp;
extern GENIE_PROCEDURE genie_round_real;
extern GENIE_PROCEDURE genie_seconds;
extern GENIE_PROCEDURE genie_shl_bits;
extern GENIE_PROCEDURE genie_shl_long_mp;
extern GENIE_PROCEDURE genie_shorten_bytes;
extern GENIE_PROCEDURE genie_shorten_long_complex_to_complex;
extern GENIE_PROCEDURE genie_shorten_long_mp_to_bits;
extern GENIE_PROCEDURE genie_shorten_long_mp_to_int;
extern GENIE_PROCEDURE genie_shorten_long_mp_to_real;
extern GENIE_PROCEDURE genie_shorten_longlong_complex_to_long_complex;
extern GENIE_PROCEDURE genie_shorten_longlong_mp_to_long_mp;
extern GENIE_PROCEDURE genie_shr_bits;
extern GENIE_PROCEDURE genie_shr_long_mp;
extern GENIE_PROCEDURE genie_sign_int;
extern GENIE_PROCEDURE genie_sign_long_mp;
extern GENIE_PROCEDURE genie_sign_real;
extern GENIE_PROCEDURE genie_sin_long_complex;
extern GENIE_PROCEDURE genie_sin_long_mp;
extern GENIE_PROCEDURE genie_sin_real;
extern GENIE_PROCEDURE genie_sinh_long_mp;
extern GENIE_PROCEDURE genie_small_real;
extern GENIE_PROCEDURE genie_sqrt_long_complex;
extern GENIE_PROCEDURE genie_sqrt_long_mp;
extern GENIE_PROCEDURE genie_sqrt_real;
extern GENIE_PROCEDURE genie_stack_pointer;
extern GENIE_PROCEDURE genie_stand_back;
extern GENIE_PROCEDURE genie_stand_back_channel;
extern GENIE_PROCEDURE genie_stand_draw_channel;
extern GENIE_PROCEDURE genie_stand_error;
extern GENIE_PROCEDURE genie_stand_error_channel;
extern GENIE_PROCEDURE genie_stand_in;
extern GENIE_PROCEDURE genie_stand_in_channel;
extern GENIE_PROCEDURE genie_stand_out;
extern GENIE_PROCEDURE genie_stand_out_channel;
extern GENIE_PROCEDURE genie_string_in_string;
extern GENIE_PROCEDURE genie_sub_complex;
extern GENIE_PROCEDURE genie_sub_int;
extern GENIE_PROCEDURE genie_sub_long_complex;
extern GENIE_PROCEDURE genie_sub_long_mp;
extern GENIE_PROCEDURE genie_sub_real;
extern GENIE_PROCEDURE genie_sweep_heap;
extern GENIE_PROCEDURE genie_system;
extern GENIE_PROCEDURE genie_system_stack_pointer;
extern GENIE_PROCEDURE genie_system_stack_size;
extern GENIE_PROCEDURE genie_tan_long_complex;
extern GENIE_PROCEDURE genie_tan_long_mp;
extern GENIE_PROCEDURE genie_tan_real;
extern GENIE_PROCEDURE genie_tanh_long_mp;
extern GENIE_PROCEDURE genie_term;
extern GENIE_PROCEDURE genie_times_char_int;
extern GENIE_PROCEDURE genie_times_int_char;
extern GENIE_PROCEDURE genie_times_int_string;
extern GENIE_PROCEDURE genie_times_string_int;
extern GENIE_PROCEDURE genie_timesab_complex;
extern GENIE_PROCEDURE genie_timesab_int;
extern GENIE_PROCEDURE genie_timesab_long_complex;
extern GENIE_PROCEDURE genie_timesab_long_int;
extern GENIE_PROCEDURE genie_timesab_long_mp;
extern GENIE_PROCEDURE genie_timesab_real;
extern GENIE_PROCEDURE genie_timesab_string;
extern GENIE_PROCEDURE genie_up_sema;
extern GENIE_PROCEDURE genie_vector_times_scalar;
extern GENIE_PROCEDURE genie_xor_bits;
extern GENIE_PROCEDURE genie_xor_bool;
extern GENIE_PROCEDURE genie_xor_long_mp;

extern PROPAGATOR_T genie_and_function (NODE_T *);
extern PROPAGATOR_T genie_assertion (NODE_T *);
extern PROPAGATOR_T genie_assignation (NODE_T *);
extern PROPAGATOR_T genie_assignation_quick (NODE_T *);
extern PROPAGATOR_T genie_call (NODE_T *);
extern PROPAGATOR_T genie_call_quick (NODE_T *);
extern PROPAGATOR_T genie_call_standenv_quick (NODE_T *);
extern PROPAGATOR_T genie_cast (NODE_T *);
extern PROPAGATOR_T genie_coercion (NODE_T *);
extern PROPAGATOR_T genie_collateral (NODE_T *);
extern PROPAGATOR_T genie_constant (NODE_T *);
extern PROPAGATOR_T genie_denoter (NODE_T *);
extern PROPAGATOR_T genie_deproceduring (NODE_T *);
extern PROPAGATOR_T genie_dereference_loc_identifier (NODE_T *);
extern PROPAGATOR_T genie_dereference_global_identifier (NODE_T *);
extern PROPAGATOR_T genie_dereference_slice_loc_name_quick (NODE_T *);
extern PROPAGATOR_T genie_dereference_slice_name_quick (NODE_T *);
extern PROPAGATOR_T genie_dereferencing (NODE_T *);
extern PROPAGATOR_T genie_dereferencing_quick (NODE_T *);
extern PROPAGATOR_T genie_enclosed (volatile NODE_T *);
extern PROPAGATOR_T genie_format_text (NODE_T *);
extern PROPAGATOR_T genie_formula (NODE_T *);
extern PROPAGATOR_T genie_formula_quick (NODE_T *);
extern PROPAGATOR_T genie_formula_standenv_lhs_triad_quick (NODE_T *);
extern PROPAGATOR_T genie_formula_standenv_quick (NODE_T *);
extern PROPAGATOR_T genie_formula_standenv_rhs_triad_quick (NODE_T *);
extern PROPAGATOR_T genie_generator (NODE_T *);
extern PROPAGATOR_T genie_identifier (NODE_T *);
extern PROPAGATOR_T genie_identifier_standenv (NODE_T *);
extern PROPAGATOR_T genie_identifier_standenv_proc (NODE_T *);
extern PROPAGATOR_T genie_identity_relation (NODE_T *);
extern PROPAGATOR_T genie_loc_assignation (NODE_T *);
extern PROPAGATOR_T genie_loc_constant_assignation (NODE_T *);
extern PROPAGATOR_T genie_loc_identifier (NODE_T *);
extern PROPAGATOR_T genie_global_identifier (NODE_T *);
extern PROPAGATOR_T genie_monadic (NODE_T *);
extern PROPAGATOR_T genie_nihil (NODE_T *);
extern PROPAGATOR_T genie_or_function (NODE_T *);
extern PROPAGATOR_T genie_parallel (NODE_T *);
extern PROPAGATOR_T genie_primary (NODE_T *);
extern PROPAGATOR_T genie_routine_text (NODE_T *);
extern PROPAGATOR_T genie_rowing (NODE_T *);
extern PROPAGATOR_T genie_rowing_ref_row_of_row (NODE_T *);
extern PROPAGATOR_T genie_rowing_ref_row_row (NODE_T *);
extern PROPAGATOR_T genie_rowing_row_of_row (NODE_T *);
extern PROPAGATOR_T genie_rowing_row_row (NODE_T *);
extern PROPAGATOR_T genie_secondary (NODE_T *);
extern PROPAGATOR_T genie_selection (NODE_T *);
extern PROPAGATOR_T genie_selection_name (NODE_T *);
extern PROPAGATOR_T genie_selection_value (NODE_T *);
extern PROPAGATOR_T genie_skip (NODE_T *);
extern PROPAGATOR_T genie_slice (NODE_T *);
extern PROPAGATOR_T genie_slice_loc_name_quick (NODE_T *);
extern PROPAGATOR_T genie_slice_name_quick (NODE_T *);
extern PROPAGATOR_T genie_tertiary (NODE_T *);
extern PROPAGATOR_T genie_unit (NODE_T *);
extern PROPAGATOR_T genie_uniting (NODE_T *);
extern PROPAGATOR_T genie_voiding (NODE_T *);
extern PROPAGATOR_T genie_voiding_assignation_quick (NODE_T *);
extern PROPAGATOR_T genie_voiding_loc_assignation (NODE_T *);
extern PROPAGATOR_T genie_voiding_loc_constant_assignation (NODE_T *);
extern PROPAGATOR_T genie_widening (NODE_T *);
extern PROPAGATOR_T genie_widening_int_to_real (NODE_T *);

extern A68_REF c_string_to_row_char (NODE_T *, char *, int);
extern A68_REF c_to_a_string (NODE_T *, char *);
extern A68_REF empty_row (NODE_T *, MOID_T *);
extern A68_REF empty_string (NODE_T *);
extern A68_REF genie_allocate_declarer (NODE_T *, ADDR_T *, A68_REF, BOOL_T);
extern A68_REF genie_assign_stowed (A68_REF, A68_REF *, NODE_T *, MOID_T *);
extern A68_REF genie_concatenate_rows (NODE_T *, MOID_T *, int, ADDR_T);
extern A68_REF genie_copy_stowed (A68_REF, NODE_T *, MOID_T *);
extern A68_REF genie_make_ref_row_of_row (NODE_T *, MOID_T *, MOID_T *, ADDR_T);
extern A68_REF genie_make_ref_row_row (NODE_T *, MOID_T *, MOID_T *, ADDR_T);
extern A68_REF genie_make_row (NODE_T *, MOID_T *, int, ADDR_T);
extern BOOL_T breakpoint_expression (NODE_T *);
extern BOOL_T close_device (NODE_T *, A68_FILE *);
extern BOOL_T genie_int_case_unit (NODE_T *, int, int *);
extern BOOL_T genie_no_user_symbols (SYMBOL_TABLE_T *);
extern BOOL_T genie_string_to_value_internal (NODE_T *, MOID_T *, char *, BYTE_T *);
extern BOOL_T genie_united_case_unit (NODE_T *, MOID_T *);
extern char *a_to_c_string (NODE_T *, char *, A68_REF);
extern char *genie_standard_format (NODE_T *, MOID_T *, void *);
extern double dabs (double);
extern double dmod (double, double);
extern double dsign (double);
extern double rng_53_bit (void);
extern int a68_string_size (NODE_T *, A68_REF);
extern int iabs (int);
extern int isign (int);
extern NODE_T *last_unit;
extern void dump_frame (int);
extern void dump_stack (int);
extern void exit_genie (NODE_T *, int);
extern void genie (MODULE_T *);
extern void genie_call_operator (NODE_T *, ADDR_T);
extern void genie_call_procedure (NODE_T *, MOID_T *, MOID_T *, MOID_T *, A68_PROCEDURE *, ADDR_T, ADDR_T);
extern void genie_check_initialisation (NODE_T *, BYTE_T *, MOID_T *);
extern void genie_declaration (NODE_T *);
extern void genie_dump_frames ();
extern void genie_enquiry_clause (NODE_T *);
extern void genie_f_and_becomes (NODE_T *, MOID_T *, GENIE_PROCEDURE *);
extern void genie_generator_bounds (NODE_T *);
extern void genie_generator_internal (NODE_T *, MOID_T *, TAG_T *, LEAP_T, ADDR_T);
extern void genie_init_lib (NODE_T *);
extern void genie_prepare_declarer (NODE_T *);
extern void genie_push_undefined (NODE_T *, MOID_T *);
extern void genie_revise_lower_bound (NODE_T *, A68_REF, A68_REF);
extern void genie_serial_clause (NODE_T *, jmp_buf *);
extern void genie_serial_units (NODE_T *, NODE_T **, jmp_buf *, int);
extern void genie_subscript (NODE_T *, ADDR_T *, int *, NODE_T **);
extern void genie_subscript_linear (NODE_T *, ADDR_T *, int *);
extern void genie_unit_trace (NODE_T *);
extern void init_rng (unsigned long);
extern void initialise_frame (NODE_T *);
extern void install_signal_handlers (void);
extern void show_data_item (FILE_T, NODE_T *, MOID_T *, BYTE_T *);
extern void show_frame (NODE_T *, FILE_T);
extern void show_stack_frame (FILE_T, NODE_T *, ADDR_T);
extern void single_step (NODE_T *, BOOL_T, BOOL_T);
extern void stack_dump (FILE_T, ADDR_T, int);
extern void stack_dump_light (ADDR_T);
extern void un_init_frame (NODE_T *);
extern void where (FILE_T, NODE_T *);

extern void genie_argc (NODE_T *);
extern void genie_argv (NODE_T *);
extern void genie_create_pipe (NODE_T *);
extern void genie_utctime (NODE_T *);
extern void genie_localtime (NODE_T *);
extern void genie_errno (NODE_T *);
extern void genie_execve (NODE_T *);
extern void genie_execve_child (NODE_T *);
extern void genie_execve_child_pipe (NODE_T *);
extern void genie_execve_output (NODE_T *);
extern void genie_fork (NODE_T *);
extern void genie_getenv (NODE_T *);
extern void genie_reset_errno (NODE_T *);
extern void genie_strerror (NODE_T *);
extern void genie_waitpid (NODE_T *);

#if defined HAVE_HTTP
extern void genie_http_content (NODE_T *);
extern void genie_tcp_request (NODE_T *);
#endif

#if defined HAVE_REGEX
extern void genie_grep_in_string (NODE_T *);
extern void genie_sub_in_string (NODE_T *);
#endif

#if defined HAVE_CURSES
extern BOOL_T curses_active;
extern void genie_curses_clear (NODE_T *);
extern void genie_curses_columns (NODE_T *);
extern void genie_curses_end (NODE_T *);
extern void genie_curses_getchar (NODE_T *);
extern void genie_curses_lines (NODE_T *);
extern void genie_curses_move (NODE_T *);
extern void genie_curses_putchar (NODE_T *);
extern void genie_curses_refresh (NODE_T *);
extern void genie_curses_start (NODE_T *);
#endif

#if defined HAVE_POSTGRESQL
extern void genie_pq_backendpid (NODE_T *);
extern void genie_pq_cmdstatus (NODE_T *);
extern void genie_pq_cmdtuples (NODE_T *);
extern void genie_pq_connectdb (NODE_T *);
extern void genie_pq_db (NODE_T *);
extern void genie_pq_errormessage (NODE_T *);
extern void genie_pq_exec (NODE_T *);
extern void genie_pq_fformat (NODE_T *);
extern void genie_pq_finish (NODE_T *);
extern void genie_pq_fname (NODE_T *);
extern void genie_pq_fnumber (NODE_T *);
extern void genie_pq_getisnull (NODE_T *);
extern void genie_pq_getvalue (NODE_T *);
extern void genie_pq_host (NODE_T *);
extern void genie_pq_nfields (NODE_T *);
extern void genie_pq_ntuples (NODE_T *);
extern void genie_pq_options (NODE_T *);
extern void genie_pq_parameterstatus (NODE_T *);
extern void genie_pq_pass (NODE_T *);
extern void genie_pq_port (NODE_T *);
extern void genie_pq_protocolversion (NODE_T *);
extern void genie_pq_reset (NODE_T *);
extern void genie_pq_resulterrormessage (NODE_T *);
extern void genie_pq_serverversion (NODE_T *);
extern void genie_pq_socket (NODE_T *);
extern void genie_pq_tty (NODE_T *);
extern void genie_pq_user (NODE_T *);
#endif

#endif
