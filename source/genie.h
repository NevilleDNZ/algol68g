/*-------1---------2---------3---------4---------5---------6---------7---------+
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2004 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful,but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      */

#ifndef A68G_GENIE_H
#define A68G_GENIE_H

/*-------1---------2---------3---------4---------5---------6---------7---------+
Macros.                                                                       */

#define IS_PROCEDURE_PARM A_TRUE
#define IS_NOT_PROCEDURE_PARM A_FALSE
#define BITS_WIDTH ((int) (1 + ceil (log(MAX_INT) / log(2))))
#define INT_WIDTH ((int) (1 + floor (log (MAX_INT) / log (10))))

#define CHAR_WIDTH (1 + (int) log10 ((double) SCHAR_MAX))
#define REAL_WIDTH (DBL_DIG)
#define EXP_WIDTH ((int) (1 + log10 ((double) DBL_MAX_10_EXP)))

#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MOID_SIZE(p) ((unsigned) ((p)->size))

#define HEAP_ADDRESS(n) ((BYTE_T *) & (heap_segment[n]))
#define IS_NIL(p) ((p).offset == 0 && (p).handle == NULL)

#define ADDRESS(z)\
  ((BYTE_T *) ((z)->segment == heap_segment ?\
   HEAP_ADDRESS ((z)->handle->offset + (z)->offset):\
   (z)->segment != NULL ?\
   & (((z)->segment)[(z)->offset]):\
   NULL))

#define TEST_NIL(p, z, m)\
  if (IS_NIL (z))\
    {\
      diagnostic (A_RUNTIME_ERROR, (p), ACCESSING_NIL_ERROR, (m));\
      exit_genie ((p), A_RUNTIME_ERROR);\
    }

#define TEST_INIT(p, z, m) {\
    if (! (z).status & INITIALISED_MASK) {\
     diagnostic (A_RUNTIME_ERROR, (p), EMPTY_VALUE_ERROR, (m));\
     exit_genie ((p), A_RUNTIME_ERROR);\
    }\
  }

#define PROTECT_FROM_SWEEP(p) if ((p)->protect_sweep != NULL) {\
  *(A68_REF *) &frame_segment[frame_pointer + FRAME_INFO_SIZE + (p)->protect_sweep->offset] =\
  *(A68_REF *) (STACK_OFFSET (- SIZE_OF (A68_REF)));\
  }

/*-------1---------2---------3---------4---------5---------6---------7---------+
Activation records in the frame stack.                                        */

typedef struct ACTIVATION ACTIVATION;

struct ACTIVATION
{
  ADDR_T static_link, dynamic_link;
  NODE_T *node;
  jmp_buf *jump_stat;
};

#define CLOSE_FRAME { frame_pointer = FRAME_DYNAMIC_LINK (frame_pointer); }

#define DESCENT(l) ((l) == global_level ? global_pointer : descent ((l)))

#define FRAME_DYNAMIC_LINK(n) (((ACTIVATION *) FRAME_ADDRESS(n))->dynamic_link)
#define FRAME_INCREMENT(n) (SYMBOL_TABLE (FRAME_TREE(n))->ap_increment)
#define FRAME_JUMP_STAT(n) (((ACTIVATION *) FRAME_ADDRESS(n))->jump_stat)
#define FRAME_LEXICAL_LEVEL(n) (SYMBOL_TABLE (FRAME_TREE(n))->level)
#define FRAME_STATIC_LINK(n) (((ACTIVATION *) FRAME_ADDRESS(n))->static_link)
#define FRAME_TREE(n) (NODE ((ACTIVATION *) FRAME_ADDRESS(n)))
#define FRAME_INFO_SIZE (ALIGN (SIZE_OF (ACTIVATION)))
#define FRAME_SHORTCUT(p) (& ((p)->genie.offset[DESCENT ((p)->genie.level)]))
#define FRAME_ADDRESS(n) (&frame_segment[n])
#define FRAME_LOCAL(n, m) (&frame_segment[(n) + FRAME_INFO_SIZE + (m)])
#define FRAME_OFFSET(n) (&frame_segment[frame_pointer + (n)])
#define FRAME_TOP (&frame_segment[frame_pointer])

/*-------1---------2---------3---------4---------5---------6---------7---------+
Macros for row-handling.                                                      */

#define GET_DESCRIPTOR(a, t, p)\
  a = (A68_ARRAY *) ADDRESS (p);\
  t = (A68_TUPLE *) & (((BYTE_T *) (a)) [SIZE_OF (A68_ARRAY)]);

#define PUT_DESCRIPTOR(a, t, p)\
  *(A68_ARRAY *) ADDRESS (p) = (a);\
  *(A68_TUPLE *) &(((BYTE_T *) (ADDRESS(p))) [SIZE_OF (A68_ARRAY)]) = (t);

#define ROW_SIZE(t) (((t)->upper_bound >= (t)->lower_bound) ? ((t)->upper_bound - (t)->lower_bound + 1) : 0)

#define ROW_ELEMENT(a, k) (((ADDR_T) k + (a)->slice_offset) * (a)->elem_size + (a)->field_offset)

#define INDEX_1_DIM(a, t, k) ROW_ELEMENT (a, ((t)->span * ((int) k - (t)->shift)))

/*-------1---------2---------3---------4---------5---------6---------7---------+
Macros for execution.                                                         */

#define EXECUTE_UNIT(p)\
  ((p)->genie.propagator.unit ((p)->genie.propagator.source))

#define EXECUTE_UNIT_TRACE(p)\
  if ((MASK (p) & TRACE_MASK) || sys_request_flag || (p->info->module->options.time_limit > 0)) {\
    genie_unit_trace (p);\
  } else {\
    EXECUTE_UNIT (p);\
  }

/*-------1---------2---------3---------4---------5---------6---------7---------+
Stack manipulation.                                                           */

#define STACK_ADDRESS(n) ((BYTE_T *) &(stack_segment[n]))
#define STACK_OFFSET(n) ((BYTE_T *) &(stack_segment[stack_pointer + (n)]))
#define STACK_TOP ((BYTE_T *) &(stack_segment[stack_pointer]))

#define INCREMENT_STACK_POINTER(err, i) {\
    if ((stack_pointer += (i)) >= expr_stack_size) {\
      diagnostic(A_RUNTIME_ERROR, (err), "expression stack exhausted");\
      exit_genie((err), A_RUNTIME_ERROR);\
    }\
  }

#define DECREMENT_STACK_POINTER(err, i) {\
    stack_pointer -= (i);\
    (void) (err);\
  }

#define INCREMENT_FRAME_POINTER(err, i, size) {\
    if ((frame_pointer += (i)) >= (frame_stack_size - (size))) {\
      diagnostic(A_RUNTIME_ERROR, (err), "frame stack space exhausted");\
      exit_genie((err), A_RUNTIME_ERROR);\
    }\
  }

#define PUSH(p, addr, size) {\
    BYTE_T *sp = STACK_TOP;\
    INCREMENT_STACK_POINTER ((p), (size));\
    memcpy (sp, (BYTE_T *) (addr), (unsigned) (size));\
  }

#define POP(p, addr, size) {\
    DECREMENT_STACK_POINTER((p), (size));\
    memcpy ((BYTE_T *) (addr), STACK_TOP, (unsigned) (size));\
  }

#define POP_ADDRESS(p, addr, type) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (type));\
    (addr) = (type *) STACK_TOP;\
  }

#define PUSH_INT(p, k) {\
    A68_INT *_z_ = (A68_INT *) STACK_TOP;\
    _z_->status = INITIALISED_MASK;\
    _z_->value = (k);\
    INCREMENT_STACK_POINTER((p), SIZE_OF (A68_INT));\
  }

#define POP_INT(p, k) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_INT));\
    (*(k)) = *((A68_INT *) STACK_TOP);\
  }

#define PUSH_REAL(p, x) {\
    A68_REAL *_z_ = (A68_REAL *) STACK_TOP;\
    _z_->status = INITIALISED_MASK;\
    _z_->value = (x);\
    INCREMENT_STACK_POINTER((p), SIZE_OF (A68_REAL));\
  }

#define POP_REAL(p, x) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_REAL));\
    (*(x)) = *((A68_REAL *) STACK_TOP);\
  }

#define PUSH_BOOL(p, k) {\
    A68_BOOL *_z_ = (A68_BOOL *) STACK_TOP;\
    _z_->status = INITIALISED_MASK;\
    _z_->value = (k);\
    INCREMENT_STACK_POINTER((p), SIZE_OF (A68_BOOL));\
  }

#define POP_BOOL(p, k) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_BOOL));\
    (*(k)) = *((A68_BOOL *) STACK_TOP);\
  }

#define PUSH_CHAR(p, k) {\
    A68_CHAR *_z_ = (A68_CHAR *) STACK_TOP;\
    _z_->status = INITIALISED_MASK;\
    _z_->value = (k);\
    INCREMENT_STACK_POINTER((p), SIZE_OF (A68_CHAR));\
  }

#define POP_CHAR(p, k) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_CHAR));\
    (*(k)) = *((A68_CHAR *) STACK_TOP);\
  }

#define PUSH_BITS(p, k) {\
    A68_BITS *_z_ = (A68_BITS *) STACK_TOP;\
    _z_->status = INITIALISED_MASK;\
    _z_->value = (k);\
    INCREMENT_STACK_POINTER((p), SIZE_OF (A68_BITS));\
  }

#define POP_BITS(p, k) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_BITS));\
    (*(k)) = *((A68_BITS *) STACK_TOP);\
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

#define PUSH_COMPLEX(p, re, im) {\
    PUSH_REAL (p, re);\
    PUSH_REAL (p, im);\
  }

#define POP_COMPLEX(p, re, im) {\
    POP_REAL (p, im);\
    POP_REAL (p, re);\
  }

#define PUSH_REF(p, z) {\
    A68_REF _z_ = (z);\
    PUSH (p, &_z_, SIZE_OF (A68_REF));\
  }

#define PUSH_REF_FILE(p, z) {\
    PUSH (p, &(z), SIZE_OF (A68_REF));\
  }

#define PUSH_CHANNEL(p, z) {\
    PUSH (p, &(z), SIZE_OF (A68_CHANNEL));\
  }

#define PUSH_POINTER(p, z) {\
    A68_POINTER _w_;\
    _w_.status = INITIALISED_MASK;\
    _w_.value = z;\
    PUSH (p, &_w_, SIZE_OF (A68_POINTER));\
  }

#define POP_REF(p, z) {\
    DECREMENT_STACK_POINTER((p), SIZE_OF (A68_REF));\
    (*(z)) = *((A68_REF *) STACK_TOP);\
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

/*-------1---------2---------3---------4---------5---------6---------7---------+
Macros for checking representation of values.                                 */

#ifdef HAVE_IEEE_754
#define TEST_REAL_REPRESENTATION(p, u)\
  if (isnan (u) || isinf (u)) {\
      errno = ERANGE;\
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (REAL), NULL);\
      exit_genie (p, A_RUNTIME_ERROR);\
  }
#else
#define TEST_REAL_REPRESENTATION(p, u) {;}
#endif

#ifdef HAVE_IEEE_754
#define TEST_COMPLEX_REPRESENTATION(p, u, v)\
  if (isnan (u) || isinf (u) || isnan (v) || isinf (v)) {\
      errno = ERANGE;\
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (COMPLEX), NULL);\
      exit_genie (p, A_RUNTIME_ERROR);\
  }
#else
#define TEST_COMPLEX_REPRESENTATION(p, u, v) {;}
#endif

#define TEST_TIMES_OVERFLOW_INT(p, u, v)\
  if (v != 0) {\
    int au = (u >= 0 ? u : -u);\
    int av = (v >= 0 ? v : -v);\
    if (au > MAX_INT / av) {\
      errno = ERANGE;\
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT), NULL);\
      exit_genie (p, A_RUNTIME_ERROR);\
      }\
  }

#ifdef HAVE_IEEE_754
#define TEST_TIMES_OVERFLOW_REAL(p, u, v) {;}
#else
#define TEST_TIMES_OVERFLOW_REAL(p, u, v)\
  if (v != 0.0) {\
    double au = (u >= 0 ? u : -u);\
    double av = (v >= 0 ? v : -v);\
    if (au > DBL_MAX / av) {\
      errno = ERANGE;\
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (REAL), NULL);\
      exit_genie (p, A_RUNTIME_ERROR);\
      }\
  }
#endif

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

#ifndef DO_SYSTEM_STACK_CHECK
#define LOW_STACK_ALERT {;}
#elif defined HAVE_UNIX
#define LOW_STACK_ALERT {\
  BYTE_T stack_offset;\
  double limit = (stack_size > (128 * KILOBYTE) ? 0.9 * stack_size : 0.5 * stack_size);\
  abend (stack_size> 0 && fabs ((double) (int) system_stack_offset - (double) (int) &stack_offset) > limit, "program too complex", "imminent stack overflow");\
  }
#elif defined PRE_MACOS_X
#define LOW_STACK_ALERT {\
  double limit = (stack_size > (128 * KILOBYTE) ? 0.1 * stack_size : 0.5 * stack_size);\
  abend (stack_size > 0 && StackSpace () < limit, "program too complex", "imminent stack overflow");\
  }
#else
#define LOW_STACK_ALERT {;}	/* No stack check since segment size is unknown. */
#endif

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

#define A68_ENV_INT(n, k) void n (NODE_T *p) {PUSH_INT (p, (k));}
#define A68_ENV_REAL(n, z) void n (NODE_T *p) {PUSH_REAL (p, (z));}

/*-------1---------2---------3---------4---------5---------6---------7---------+
External symbols.                                                             */

extern ADDR_T frame_pointer, stack_pointer, heap_pointer, handle_pointer, global_pointer;
extern A68_FORMAT nil_format;
extern A68_HANDLE nil_handle, *free_handles, *unfree_handles;
extern A68_POINTER nil_pointer;
extern A68_REF nil_ref;
extern A68_REF stand_in, stand_out;
extern BOOL_T in_monitor;
extern BYTE_T *frame_segment, *stack_segment, *heap_segment, *handle_segment;
extern MOID_T *top_expr_moid;
extern double cputime_0;
extern int global_level, max_lex_lvl, ret_code;
extern jmp_buf genie_exit_label;

extern int free_handle_count, max_handle_count;
extern void sweep_heap (NODE_T *, ADDR_T);

extern GENIE_PROCEDURE genie_abs_bool;
extern GENIE_PROCEDURE genie_abs_char;
extern GENIE_PROCEDURE genie_abs_complex;
extern GENIE_PROCEDURE genie_abs_int;
extern GENIE_PROCEDURE genie_abs_long_complex;
extern GENIE_PROCEDURE genie_abs_long_mp;
extern GENIE_PROCEDURE genie_abs_real;
extern GENIE_PROCEDURE genie_acos_long_mp;
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
extern GENIE_PROCEDURE genie_arcsin_real;
extern GENIE_PROCEDURE genie_arctan_real;
extern GENIE_PROCEDURE genie_arg_complex;
extern GENIE_PROCEDURE genie_arg_long_complex;
extern GENIE_PROCEDURE genie_asin_long_mp;
extern GENIE_PROCEDURE genie_atan2_real;
extern GENIE_PROCEDURE genie_atan_long_mp;
extern GENIE_PROCEDURE genie_bin_int;
extern GENIE_PROCEDURE genie_bin_long_mp;
extern GENIE_PROCEDURE genie_bin_possible;
extern GENIE_PROCEDURE genie_bits_lengths;
extern GENIE_PROCEDURE genie_bits_pack;
extern GENIE_PROCEDURE genie_bits_shorts;
extern GENIE_PROCEDURE genie_bits_width;
extern GENIE_PROCEDURE genie_break;
extern GENIE_PROCEDURE genie_bytes_lengths;
extern GENIE_PROCEDURE genie_bytes_shorts;
extern GENIE_PROCEDURE genie_bytes_width;
extern GENIE_PROCEDURE genie_bytespack;
extern GENIE_PROCEDURE genie_char_in_string;
extern GENIE_PROCEDURE genie_complex_lengths;
extern GENIE_PROCEDURE genie_complex_shorts;
extern GENIE_PROCEDURE genie_conj_complex;
extern GENIE_PROCEDURE genie_conj_long_complex;
extern GENIE_PROCEDURE genie_cos_long_mp;
extern GENIE_PROCEDURE genie_cos_real;
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
extern GENIE_PROCEDURE genie_get_possible;
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
extern GENIE_PROCEDURE genie_int_shorts;
extern GENIE_PROCEDURE genie_int_width;
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
extern GENIE_PROCEDURE genie_overab_real;
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
extern GENIE_PROCEDURE genie_program_idf;
extern GENIE_PROCEDURE genie_put_possible;
extern GENIE_PROCEDURE genie_re_complex;
extern GENIE_PROCEDURE genie_re_long_complex;
extern GENIE_PROCEDURE genie_real_lengths;
extern GENIE_PROCEDURE genie_real_shorts;
extern GENIE_PROCEDURE genie_real_width;
extern GENIE_PROCEDURE genie_repr_char;
extern GENIE_PROCEDURE genie_reset_possible;
extern GENIE_PROCEDURE genie_round_long_mp;
extern GENIE_PROCEDURE genie_round_real;
extern GENIE_PROCEDURE genie_seconds;
extern GENIE_PROCEDURE genie_set_possible;
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
extern GENIE_PROCEDURE genie_sin_long_mp;
extern GENIE_PROCEDURE genie_sin_real;
extern GENIE_PROCEDURE genie_small_real;
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
extern GENIE_PROCEDURE genie_sub_complex;
extern GENIE_PROCEDURE genie_sub_int;
extern GENIE_PROCEDURE genie_sub_long_complex;
extern GENIE_PROCEDURE genie_sub_long_mp;
extern GENIE_PROCEDURE genie_sub_real;
extern GENIE_PROCEDURE genie_system_stack_pointer;
extern GENIE_PROCEDURE genie_system_stack_size;
extern GENIE_PROCEDURE genie_sweep_heap;
extern GENIE_PROCEDURE genie_system;
extern GENIE_PROCEDURE genie_tan_long_mp;
extern GENIE_PROCEDURE genie_tan_real;
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
extern GENIE_PROCEDURE genie_vector_add;
extern GENIE_PROCEDURE genie_vector_div;
extern GENIE_PROCEDURE genie_vector_inner_product;
extern GENIE_PROCEDURE genie_vector_move;
extern GENIE_PROCEDURE genie_vector_mul;
extern GENIE_PROCEDURE genie_vector_set;
extern GENIE_PROCEDURE genie_vector_sub;
extern GENIE_PROCEDURE genie_vector_times_scalar;
extern GENIE_PROCEDURE genie_xor_bits;
extern GENIE_PROCEDURE genie_xor_bool;
extern GENIE_PROCEDURE genie_xor_long_mp;

extern PROPAGATOR_T genie_and_function (NODE_T *);
extern PROPAGATOR_T genie_assertion (NODE_T *);
extern PROPAGATOR_T genie_assignation (NODE_T *);
extern PROPAGATOR_T genie_call (NODE_T *);
extern PROPAGATOR_T genie_call_quick (NODE_T *);
extern PROPAGATOR_T genie_call_standenv_quick (NODE_T *);
extern PROPAGATOR_T genie_cast (NODE_T *);
extern PROPAGATOR_T genie_closed (NODE_T *);
extern PROPAGATOR_T genie_coercion (NODE_T *);
extern PROPAGATOR_T genie_collateral (NODE_T *);
extern PROPAGATOR_T genie_constant (NODE_T *);
extern PROPAGATOR_T genie_denoter (NODE_T *);
extern PROPAGATOR_T genie_deproceduring (NODE_T *);
extern PROPAGATOR_T genie_dereference_identifier (NODE_T *);
extern PROPAGATOR_T genie_dereference_loc_identifier (NODE_T *);
extern PROPAGATOR_T genie_dereferencing (NODE_T *);
extern PROPAGATOR_T genie_dereferencing_quick (NODE_T *);
extern PROPAGATOR_T genie_enclosed (NODE_T *);
extern PROPAGATOR_T genie_format_text (NODE_T *);
extern PROPAGATOR_T genie_formula (NODE_T *);
extern PROPAGATOR_T genie_formula_quick (NODE_T *);
extern PROPAGATOR_T genie_formula_quick (NODE_T *);
extern PROPAGATOR_T genie_formula_standenv_quick (NODE_T *);
extern PROPAGATOR_T genie_generator (NODE_T *);
extern PROPAGATOR_T genie_identifier (NODE_T *);
extern PROPAGATOR_T genie_identifier_standenv (NODE_T *);
extern PROPAGATOR_T genie_identifier_standenv_proc (NODE_T *);
extern PROPAGATOR_T genie_identity_relation (NODE_T *);
extern PROPAGATOR_T genie_loc_assignation (NODE_T *);
extern PROPAGATOR_T genie_loc_constant_assignation (NODE_T *);
extern PROPAGATOR_T genie_loc_identifier (NODE_T *);
extern PROPAGATOR_T genie_loop (NODE_T *);
extern PROPAGATOR_T genie_monadic (NODE_T *);
extern PROPAGATOR_T genie_nihil (NODE_T *);
extern PROPAGATOR_T genie_or_function (NODE_T *);
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
extern PROPAGATOR_T genie_slice_name_quick (NODE_T *);
extern PROPAGATOR_T genie_tertiary (NODE_T *);
extern PROPAGATOR_T genie_unit (NODE_T *);
extern PROPAGATOR_T genie_uniting (NODE_T *);
extern PROPAGATOR_T genie_voiding (NODE_T *);
extern PROPAGATOR_T genie_voiding_loc_assignation (NODE_T *);
extern PROPAGATOR_T genie_voiding_loc_constant_assignation (NODE_T *);
extern PROPAGATOR_T genie_widening (NODE_T *);
extern PROPAGATOR_T genie_widening_int_to_real (NODE_T *);

extern ADDR_T descent (int);
extern ADDR_T static_link_for_frame (int);
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
extern BOOL_T close_device (NODE_T *, A68_FILE *);
extern BOOL_T genie_check_initialisation (NODE_T *, BYTE_T *, MOID_T *, BOOL_T *);
extern BOOL_T genie_int_case_unit (NODE_T *, int, int *);
extern BOOL_T genie_string_to_value_internal (NODE_T *, MOID_T *, char *, BYTE_T *);
extern BOOL_T genie_united_case_unit (NODE_T *, MOID_T *);
extern char *a_to_c_string (NODE_T *, char *, A68_REF);
extern char *genie_function_name (PROPAGATOR_PROCEDURE *);
extern char *genie_standard_format (NODE_T *, MOID_T *, void *);
extern char *string_plusab_char (char *, char);
extern double dabs (double);
extern double dmod (double, double);
extern double dsign (double);
extern double rng_53_bit (void);
extern int a68_string_size (NODE_T *, A68_REF);
extern int iabs (int);
extern int isign (int);
extern void close_frame ();
extern void down_garbage_sema (void);
extern void dump_frame (int);
extern void dump_stack (int);
extern void exit_genie (NODE_T *, int);
extern void genie (MODULE_T *);
extern void genie_call_operator (NODE_T *, ADDR_T);
extern void genie_conditional (NODE_T *, MOID_T *);
extern void genie_declaration (NODE_T *);
extern void genie_dump_frames ();
extern void genie_enquiry_clause (NODE_T *);
extern void genie_init_lib (NODE_T *);
extern void genie_int_case (NODE_T *, MOID_T *);
extern void genie_prepare_declarer (NODE_T *);
extern void genie_push_undefined (NODE_T *, MOID_T *);
extern void genie_revise_lower_bound (NODE_T *, A68_REF, A68_REF);
extern void genie_serial_clause (NODE_T *, jmp_buf *);
extern void genie_serial_units (NODE_T *, NODE_T **, jmp_buf *, int);
extern void genie_subscript (NODE_T *, ADDR_T *, int *, NODE_T **);
extern void genie_subscript_linear (NODE_T *, ADDR_T *, int *);
extern void genie_unit_trace (NODE_T *);
extern void genie_united_case (NODE_T *, MOID_T *);
extern void init_rng (unsigned long);
extern void initialise_frame (NODE_T *);
extern void install_signal_handlers (void);
extern void open_frame (NODE_T *, int, ADDR_T);
extern void genie_scope_check (NODE_T *, MOID_T *);
extern void show_data_item (FILE_T, NODE_T *, MOID_T *, BYTE_T *);
extern void show_frame (NODE_T *, FILE_T);
extern void show_stack_frame (FILE_T, NODE_T *, ADDR_T);
extern void single_step (NODE_T *);
extern void stack_dump (FILE_T, ADDR_T, int);
extern void stack_dump_light (ADDR_T);
extern void un_init_frame (NODE_T *);
extern void up_garbage_sema (void);
extern void where (FILE_T, NODE_T *);

#ifdef HAVE_UNIX
extern void genie_argc (NODE_T *);
extern void genie_argv (NODE_T *);
extern void genie_create_pipe (NODE_T *);
extern void genie_errno (NODE_T *);
extern void genie_execve (NODE_T *);
extern void genie_execve_child (NODE_T *);
extern void genie_execve_child_pipe (NODE_T *);
extern void genie_fork (NODE_T *);
extern void genie_getenv (NODE_T *);
extern void genie_reset_errno (NODE_T *);
extern void genie_strerror (NODE_T *);
extern void genie_waitpid (NODE_T *);
#endif

#ifdef HAVE_CURSES
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

#endif

#ifdef HAVE_IEEE_754
#define NAN_STRING "NOT_A_NUMBER"
#define INF_STRING "INFINITY"
#endif
