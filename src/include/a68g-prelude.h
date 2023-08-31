//! @file a68g-prelude.h
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
//! RTS related definitions.

#if !defined (__A68G_PRELUDE_H__)
#define __A68G_PRELUDE_H__

#define A68_STD A68_TRUE
#define A68_EXT A68_FALSE

// ALGOL68C type procs.

#define A68C_DEFIO(name, pname, mode) {\
  m = a68_proc (MODE (mode), M_REF_FILE, NO_MOID);\
  a68_idf (A68_EXT, "get" #name, m, genie_get_##pname);\
  m = a68_proc (M_VOID, M_REF_FILE, MODE (mode), NO_MOID);\
  a68_idf (A68_EXT, "put" #name, m, genie_put_##pname);\
  m = a68_proc (MODE (mode), NO_MOID);\
  a68_idf (A68_EXT, "read" #name, m, genie_read_##pname);\
  m = a68_proc (M_VOID, MODE (mode), NO_MOID);\
  a68_idf (A68_EXT, "print" #name, m, genie_print_##pname);\
}

#define IS_NL_FF(ch) ((ch) == NEWLINE_CHAR || (ch) == FORMFEED_CHAR)

#define A68_MONAD(n, MODE, OP)\
void n (NODE_T * p) {\
  MODE *i;\
  POP_OPERAND_ADDRESS (p, i, MODE);\
  VALUE (i) = OP (VALUE (i));\
}

#if (A68_LEVEL >= 3)
GPROC genie_lt_bits;
GPROC genie_gt_bits;
DOUBLE_T mp_to_double (NODE_T *, MP_T *, int);
MP_T *double_to_mp (NODE_T *, MP_T *, DOUBLE_T, int);
#endif

MOID_T *a68_proc (MOID_T *, ...);
void a68_idf (BOOL_T, char *, MOID_T *, GPROC *);
void a68_prio (char *, int);
void a68_op (BOOL_T, char *, MOID_T *, GPROC *);
void a68_mode (int, char *, MOID_T **);
A68_REF c_string_to_row_char (NODE_T *, char *, int);
A68_REF c_to_a_string (NODE_T *, char *, int);
A68_REF empty_row (NODE_T *, MOID_T *);
A68_REF empty_string (NODE_T *);
A68_REF genie_make_row (NODE_T *, MOID_T *, int, ADDR_T);
A68_REF genie_store (NODE_T *, MOID_T *, A68_REF *, A68_REF *);
A68_REF heap_generator_2 (NODE_T *, MOID_T *, int, int);
A68_REF heap_generator_3 (NODE_T *, MOID_T *, int, int, int);
A68_REF heap_generator (NODE_T *, MOID_T *, int);
A68_REF tmp_to_a68_string (NODE_T *, char *);
ADDR_T calculate_internal_index (A68_TUPLE *, int);
BOOL_T close_device (NODE_T *, A68_FILE *);
BOOL_T genie_int_case_unit (NODE_T *, int, int *);
BOOL_T increment_internal_index (A68_TUPLE *, int);
char *a_to_c_string (NODE_T *, char *, A68_REF);
char *propagator_name (PROP_PROC * p);
FILE *a68_fopen (char *, char *, char *);
GPROC get_global_level;
GPROC initialise_frame;
int a68_finite_real (REAL_T);
int a68_isinf_real (REAL_T);
int a68_isnan_real (REAL_T);
int a68_string_size (NODE_T *, A68_REF);
int char_value (int);
int grep_in_string (char *, char *, int *, int *);
INT_T a68_round (REAL_T);
PROP_T genie_generator (NODE_T *);
REAL_T seconds (void);
REAL_T ten_up (int);
ssize_t io_read_conv (FILE_T, void *, size_t);
ssize_t io_read (FILE_T, void *, size_t);
ssize_t io_write_conv (FILE_T, const void *, size_t);
ssize_t io_write (FILE_T, const void *, size_t);
unt heap_available (void);
void a68_div_complex (A68_REAL *, A68_REAL *, A68_REAL *);
void a68_exit (int);
void a68_exp_real_complex (A68_REAL *, A68_REAL *);
void change_breakpoints (NODE_T *, unt, int, BOOL_T *, char *);
void change_masks (NODE_T *, unt, BOOL_T);
void colour_object (BYTE_T *, MOID_T *);
void deltagammainc (REAL_T *, REAL_T *, REAL_T, REAL_T, REAL_T, REAL_T);
void exit_genie (NODE_T *, int);
void gc_heap (NODE_T *, ADDR_T);
void genie_call_event_routine (NODE_T *, MOID_T *, A68_PROCEDURE *, ADDR_T, ADDR_T);
void genie_call_operator (NODE_T *, ADDR_T);
void genie_call_procedure (NODE_T *, MOID_T *, MOID_T *, MOID_T *, A68_PROCEDURE *, ADDR_T, ADDR_T);
void genie_check_initialisation (NODE_T *, BYTE_T *, MOID_T *);
void genie_f_and_becomes (NODE_T *, MOID_T *, GPROC *);
void genie_find_proc_op (NODE_T *, int *);
void genie_free (NODE_T *);
void genie_generator_internal (NODE_T *, MOID_T *, TAG_T *, LEAP_T, ADDR_T);
void genie_generator_stowed (NODE_T *, BYTE_T *, NODE_T **, ADDR_T *);
void genie_init_rng (void);
void genie_preprocess (NODE_T *, int *, void *);
void genie_push_undefined (NODE_T *, MOID_T *);
void genie_read_standard_format (NODE_T *, MOID_T *, BYTE_T *, A68_REF, int *);
void genie_read_standard (NODE_T *, MOID_T *, BYTE_T *, A68_REF);
void genie_serial_clause (NODE_T *, jmp_buf *);
void genie_serial_units (NODE_T *, NODE_T **, jmp_buf *, ADDR_T);
void genie_string_to_value (NODE_T *, MOID_T *, BYTE_T *, A68_REF);
void genie_subscript (NODE_T *, A68_TUPLE **, INT_T *, NODE_T **);
void genie_value_to_string (NODE_T *, MOID_T *, BYTE_T *, int);
void genie_variable_dec (NODE_T *, NODE_T **, ADDR_T);
void genie_write_standard_format (NODE_T *, MOID_T *, BYTE_T *, A68_REF, int *);
void genie_write_standard (NODE_T *, MOID_T *, BYTE_T *, A68_REF);
void initialise_internal_index (A68_TUPLE *, int);
void io_close_tty_line (void);
void io_write_string (FILE_T, const char *);
void monitor_error (char *, char *);
void mp_strtou (NODE_T *, MP_T *, char *, MOID_T *);
void open_format_frame (NODE_T *, A68_REF, A68_FORMAT *, BOOL_T, BOOL_T);
void print_internal_index (FILE_T, A68_TUPLE *, int);
void print_item (NODE_T *, FILE_T, BYTE_T *, MOID_T *);
void purge_format_read (NODE_T *, A68_REF);
void purge_format_write (NODE_T *, A68_REF);
void single_step (NODE_T *, unt);
void skip_nl_ff (NODE_T *, int *, A68_REF);
void stack_dump (FILE_T, ADDR_T, int, int *);
void value_sign_error (NODE_T *, MOID_T *, A68_REF);
void where_in_source (FILE_T, NODE_T *);

// Standard prelude RTS

GPROC genie_a68g_argc;
GPROC genie_a68g_argv;
GPROC genie_abend;
GPROC genie_abs_bits;
GPROC genie_abs_bool;
GPROC genie_abs_char;
GPROC genie_abs_complex;
GPROC genie_abs_int;
GPROC genie_abs_real;
GPROC genie_acos_complex;
GPROC genie_acosdg_real;
GPROC genie_acosh_complex;
GPROC genie_acosh_real;
GPROC genie_acos_real;
GPROC genie_acotdg_real;
GPROC genie_acot_real;
GPROC genie_acsc_real;
GPROC genie_acscdg_real;
GPROC genie_add_bits;
GPROC genie_add_bytes;
GPROC genie_add_char;
GPROC genie_add_complex;
GPROC genie_add_int;
GPROC genie_add_long_bytes;
GPROC genie_add_mp_int;
GPROC genie_add_real;
GPROC genie_add_string;
GPROC genie_and_bits;
GPROC genie_and_bool;
GPROC genie_argc;
GPROC genie_arg_complex;
GPROC genie_argv;
GPROC genie_asec_real;
GPROC genie_asecdg_real;
GPROC genie_asin_complex;
GPROC genie_asindg_real;
GPROC genie_asinh_complex;
GPROC genie_asinh_real;
GPROC genie_asin_real;
GPROC genie_associate;
GPROC genie_atan2dg_real;
GPROC genie_atan2_real;
GPROC genie_atan_complex;
GPROC genie_atandg_real;
GPROC genie_atanh_complex;
GPROC genie_atanh_real;
GPROC genie_atan_real;
GPROC genie_backspace;
GPROC genie_backtrace;
GPROC genie_beta_inc_cf_real;
GPROC genie_beta_real;
GPROC genie_bin_int;
GPROC genie_bin_possible;
GPROC genie_bits;
GPROC genie_bits_lengths;
GPROC genie_bits_pack;
GPROC genie_bits_shorths;
GPROC genie_bits_width;
GPROC genie_blank_char;
GPROC genie_block;
GPROC genie_break;
GPROC genie_bytes_lengths;
GPROC genie_bytespack;
GPROC genie_bytes_shorths;
GPROC genie_bytes_width;
GPROC genie_cd;
GPROC genie_char_in_string;
GPROC genie_choose_real;
GPROC genie_clear_bits;
GPROC genie_clear_long_bits;
GPROC genie_close;
GPROC genie_columns;
GPROC genie_complex_lengths;
GPROC genie_complex_shorths;
GPROC genie_compressible;
GPROC genie_conj_complex;
GPROC genie_cas_real;
GPROC genie_cos_complex;
GPROC genie_cosdg_real;
GPROC genie_cosh_complex;
GPROC genie_cosh_real;
GPROC genie_cospi_real;
GPROC genie_cos_real;
GPROC genie_cotdg_real;
GPROC genie_cotpi_real;
GPROC genie_cot_real;
GPROC genie_cputime;
GPROC genie_create;
GPROC genie_create_pipe;
GPROC genie_csc_real;
GPROC genie_cscdg_real;
GPROC genie_curt_real;
GPROC genie_r128mach;
GPROC genie_debug;
GPROC genie_declaration;
GPROC genie_directory;
GPROC genie_divab_complex;
GPROC genie_divab_real;
GPROC genie_div_complex;
GPROC genie_div_int;
GPROC genie_div_real;
GPROC genie_draw_possible;
GPROC genie_dyad_elems;
GPROC genie_dyad_lwb;
GPROC genie_dyad_upb;
GPROC genie_elem_bits;
GPROC genie_elem_bytes;
GPROC genie_elem_long_bits;
GPROC genie_elem_long_bits;
GPROC genie_elem_long_bytes;
GPROC genie_elem_string;
GPROC genie_enquiry_clause;
GPROC genie_entier_real;
GPROC genie_eof;
GPROC genie_eoln;
GPROC genie_eq_bits;
GPROC genie_eq_bool;
GPROC genie_eq_bytes;
GPROC genie_eq_char;
GPROC genie_eq_complex;
GPROC genie_eq_int;
GPROC genie_eq_long_bytes;
GPROC genie_eq_real;
GPROC genie_eq_string;
GPROC genie_erase;
GPROC genie_erfc_real;
GPROC genie_erf_real;
GPROC genie_errno;
GPROC genie_error_char;
GPROC genie_establish;
GPROC genie_evaluate;
GPROC genie_exec;
GPROC genie_exec_sub;
GPROC genie_exec_sub_output;
GPROC genie_exec_sub_pipeline;
GPROC genie_exp_char;
GPROC genie_exp_complex;
GPROC genie_exp_real;
GPROC genie_exp_width;
GPROC genie_fact_real;
GPROC genie_file_is_block_device;
GPROC genie_file_is_char_device;
GPROC genie_file_is_directory;
GPROC genie_file_is_regular;
GPROC genie_file_mode;
GPROC genie_first_random;
GPROC genie_flip_char;
GPROC genie_flop_char;
GPROC genie_fork;
GPROC genie_formfeed_char;
GPROC genie_gamma_inc_f_real;
GPROC genie_gamma_inc_gf_real;
GPROC genie_gamma_inc_g_real;
GPROC genie_gamma_inc_h_real;
GPROC genie_gamma_real;
GPROC genie_garbage_collections;
GPROC genie_garbage_freed;
GPROC genie_garbage_refused;
GPROC genie_garbage_seconds;
GPROC genie_gc_heap;
GPROC genie_ge_bits;
GPROC genie_ge_bytes;
GPROC genie_ge_char;
GPROC genie_ge_int;
GPROC genie_ge_long_bits;
GPROC genie_ge_long_bytes;
GPROC genie_generator_bounds;
GPROC genie_ge_real;
GPROC genie_ge_string;
GPROC genie_get_bits;
GPROC genie_get_bool;
GPROC genie_get_char;
GPROC genie_get_complex;
GPROC genie_getenv;
GPROC genie_get_int;
GPROC genie_get_long_bits;
GPROC genie_get_long_int;
GPROC genie_get_long_real;
GPROC genie_get_possible;
GPROC genie_get_real;
GPROC genie_get_sound;
GPROC genie_get_string;
GPROC genie_get_text;
GPROC genie_getf_text;
GPROC genie_grep_in_string;
GPROC genie_grep_in_substring;
GPROC genie_gt_bytes;
GPROC genie_gt_char;
GPROC genie_gt_int;
GPROC genie_gt_long_bytes;
GPROC genie_gt_real;
GPROC genie_gt_string;
GPROC genie_i32mach;
GPROC genie_i_complex;
GPROC genie_identity_dec;
GPROC genie_idf;
GPROC genie_idle;
GPROC genie_i_int_complex;
GPROC genie_im_complex;
GPROC genie_infinity_real;
GPROC genie_init_heap;
GPROC genie_init_transput;
GPROC genie_int_lengths;
GPROC genie_int_shorths;
GPROC genie_int_width;
GPROC genie_inverfc_real;
GPROC genie_inverf_real;
GPROC genie_is_alnum;
GPROC genie_is_alpha;
GPROC genie_is_cntrl;
GPROC genie_is_digit;
GPROC genie_is_graph;
GPROC genie_is_lower;
GPROC genie_is_print;
GPROC genie_is_punct;
GPROC genie_is_space;
GPROC genie_is_upper;
GPROC genie_is_xdigit;
GPROC genie_last_char_in_string;
GPROC genie_le_bits;
GPROC genie_le_bytes;
GPROC genie_le_char;
GPROC genie_le_int;
GPROC genie_le_long_bits;
GPROC genie_le_long_bytes;
GPROC genie_leng_bytes;
GPROC genie_lengthen_long_bits_to_row_bool;
GPROC genie_le_real;
GPROC genie_le_string;
GPROC genie_lj_e_12_6;
GPROC genie_lj_f_12_6;
GPROC genie_ln1p_real;
GPROC genie_ln_beta_real;
GPROC genie_ln_choose_real;
GPROC genie_ln_complex;
GPROC genie_ln_fact_real;
GPROC genie_ln_gamma_real;
GPROC genie_ln_real;
GPROC genie_localtime;
GPROC genie_lock;
GPROC genie_log_real;
GPROC genie_long_bits_pack;
GPROC genie_long_bits_width;
GPROC genie_long_bytespack;
GPROC genie_long_bytes_width;
GPROC genie_long_exp_width;
GPROC genie_long_int_width;
GPROC genie_long_max_bits;
GPROC genie_long_max_int;
GPROC genie_long_max_real;
GPROC genie_long_min_real;
GPROC genie_long_next_random;
GPROC genie_long_real_width;
GPROC genie_long_small_real;
GPROC genie_lt_bytes;
GPROC genie_lt_char;
GPROC genie_lt_int;
GPROC genie_lt_long_bytes;
GPROC genie_lt_real;
GPROC genie_lt_string;
GPROC genie_make_term;
GPROC genie_max_abs_char;
GPROC genie_max_bits;
GPROC genie_max_int;
GPROC genie_max_real;
GPROC genie_min_real;
GPROC genie_minusab_complex;
GPROC genie_minusab_int;
GPROC genie_minusab_mp_int;
GPROC genie_minusab_real;
GPROC genie_minus_complex;
GPROC genie_minus_infinity_real;
GPROC genie_minus_int;
GPROC genie_minus_real;
GPROC genie_modab_int;
GPROC genie_mod_bits;
GPROC genie_mod_int;
GPROC genie_monad_elems;
GPROC genie_monad_lwb;
GPROC genie_monad_upb;
GPROC genie_mp_radix;
GPROC genie_mul_complex;
GPROC genie_mul_int;
GPROC genie_mul_mp_int;
GPROC genie_mul_real;
GPROC genie_ne_bits;
GPROC genie_ne_bool;
GPROC genie_ne_bytes;
GPROC genie_ne_char;
GPROC genie_ne_complex;
GPROC genie_ne_int;
GPROC genie_ne_long_bytes;
GPROC genie_ne_real;
GPROC genie_ne_string;
GPROC genie_new_line;
GPROC genie_newline_char;
GPROC genie_new_page;
GPROC genie_new_sound;
GPROC genie_next_random;
GPROC genie_next_rnd;
GPROC genie_not_bits;
GPROC genie_not_bool;
GPROC genie_null_char;
GPROC genie_odd_int;
GPROC genie_on_file_end;
GPROC genie_on_format_end;
GPROC genie_on_format_error;
GPROC genie_on_gc_event;
GPROC genie_on_line_end;
GPROC genie_on_open_error;
GPROC genie_on_page_end;
GPROC genie_on_transput_error;
GPROC genie_on_value_error;
GPROC genie_open;
GPROC genie_operator_dec;
GPROC genie_or_bits;
GPROC genie_or_bool;
GPROC genie_overab_int;
GPROC genie_over_bits;
GPROC genie_over_int;
GPROC genie_pi;
GPROC genie_plusab_bytes;
GPROC genie_plusab_complex;
GPROC genie_plusab_int;
GPROC genie_plusab_long_bytes;
GPROC genie_plusab_mp_int;
GPROC genie_plusab_real;
GPROC genie_plusab_string;
GPROC genie_plusto_bytes;
GPROC genie_plusto_long_bytes;
GPROC genie_plusto_string;
GPROC genie_pow_complex_int;
GPROC genie_pow_int;
GPROC genie_pow_real;
GPROC genie_pow_real_int;
GPROC genie_preemptive_gc_heap;
GPROC genie_print_bits;
GPROC genie_print_bool;
GPROC genie_print_char;
GPROC genie_print_complex;
GPROC genie_print_int;
GPROC genie_print_long_bits;
GPROC genie_print_long_int;
GPROC genie_print_long_real;
GPROC genie_print_real;
GPROC genie_print_string;
GPROC genie_print_string;
GPROC genie_proc_variable_dec;
GPROC genie_program_idf;
GPROC genie_put_bits;
GPROC genie_put_bool;
GPROC genie_put_char;
GPROC genie_put_complex;
GPROC genie_put_int;
GPROC genie_put_long_bits;
GPROC genie_put_long_int;
GPROC genie_put_long_real;
GPROC genie_put_possible;
GPROC genie_put_real;
GPROC genie_put_string;
GPROC genie_put_text;
GPROC genie_putf_text;
GPROC genie_pwd;
GPROC genie_read;
GPROC genie_read_bin;
GPROC genie_read_bin_file;
GPROC genie_read_bits;
GPROC genie_read_bool;
GPROC genie_read_char;
GPROC genie_read_complex;
GPROC genie_read_file;
GPROC genie_read_file_format;
GPROC genie_read_format;
GPROC genie_read_int;
GPROC genie_read_line;
GPROC genie_read_long_bits;
GPROC genie_read_long_int;
GPROC genie_read_long_real;
GPROC genie_read_real;
GPROC genie_read_string;
GPROC genie_real_lengths;
GPROC genie_realpath;
GPROC genie_real_shorths;
GPROC genie_real_width;
GPROC genie_re_complex;
GPROC genie_reidf_possible;
GPROC genie_repr_char;
GPROC genie_reset;
GPROC genie_reset_errno;
GPROC genie_reset_possible;
GPROC genie_rol_bits;
GPROC genie_ror_bits;
GPROC genie_round_real;
GPROC genie_rows;
GPROC genie_sec_real;
GPROC genie_secdg_real;
GPROC genie_set;
GPROC genie_set_bits;
GPROC genie_set_long_bits;
GPROC genie_set_possible;
GPROC genie_set_return_code;
GPROC genie_set_sound;
GPROC genie_shl_bits;
GPROC genie_shorten_bytes;
GPROC genie_shr_bits;
GPROC genie_sign_int;
GPROC genie_sign_real;
GPROC genie_sin_complex;
GPROC genie_sindg_real;
GPROC genie_sinh_complex;
GPROC genie_sinh_real;
GPROC genie_sinpi_real;
GPROC genie_sin_real;
GPROC genie_sleep;
GPROC genie_small_real;
GPROC genie_sort_row_string;
GPROC genie_sound_channels;
GPROC genie_sound_rate;
GPROC genie_sound_resolution;
GPROC genie_sound_samples;
GPROC genie_space;
GPROC genie_sqrt_complex;
GPROC genie_sqrt_real;
GPROC genie_stack_pointer;
GPROC genie_stand_back;
GPROC genie_stand_back_channel;
GPROC genie_stand_draw_channel;
GPROC genie_stand_error;
GPROC genie_stand_error_channel;
GPROC genie_stand_in;
GPROC genie_stand_in_channel;
GPROC genie_stand_out;
GPROC genie_stand_out_channel;
GPROC genie_strerror;
GPROC genie_string;
GPROC genie_stringf;
GPROC genie_string_in_string;
GPROC genie_sub_bits;
GPROC genie_sub_complex;
GPROC genie_sub_in_string;
GPROC genie_sub_int;
GPROC genie_sub_mp_int;
GPROC genie_sub_real;
GPROC genie_system;
GPROC genie_system_heap_pointer;
GPROC genie_system_stack_pointer;
GPROC genie_system_stack_size;
GPROC genie_tab_char;
GPROC genie_tan_complex;
GPROC genie_tandg_real;
GPROC genie_tanh_complex;
GPROC genie_tanh_real;
GPROC genie_tanpi_real;
GPROC genie_tan_real;
GPROC genie_term;
GPROC genie_timesab_complex;
GPROC genie_timesab_int;
GPROC genie_timesab_mp_int;
GPROC genie_timesab_real;
GPROC genie_timesab_string;
GPROC genie_times_bits;
GPROC genie_times_char_int;
GPROC genie_times_int_char;
GPROC genie_times_int_string;
GPROC genie_times_string_int;
GPROC genie_to_lower;
GPROC genie_to_upper;
GPROC genie_unimplemented;
GPROC genie_utctime;
GPROC genie_waitpid;
GPROC genie_whole;
GPROC genie_write;
GPROC genie_write_bin;
GPROC genie_write_bin_file;
GPROC genie_write_file;
GPROC genie_write_file_format;
GPROC genie_write_format;
GPROC genie_xor_bits;
GPROC genie_xor_bool;

#if defined (S_ISFIFO)
GPROC genie_file_is_fifo;
#endif

#if defined (S_ISLNK)
GPROC genie_file_is_link;
#endif

#if defined (BUILD_PARALLEL_CLAUSE)
GPROC genie_down_sema;
GPROC genie_level_int_sema;
GPROC genie_level_sema_int;
GPROC genie_up_sema;
#endif

#if defined (BUILD_WWW)
GPROC genie_http_content;
GPROC genie_tcp_request;
#if defined (HAVE_CURL)
GPROC genie_https_content;
#endif
#endif

#if defined (HAVE_CURSES)
GPROC genie_curses_clear;
GPROC genie_curses_del_char;
GPROC genie_curses_green;
GPROC genie_curses_cyan;
GPROC genie_curses_white;
GPROC genie_curses_red;
GPROC genie_curses_yellow;
GPROC genie_curses_magenta;
GPROC genie_curses_blue;
GPROC genie_curses_green_inverse;
GPROC genie_curses_cyan_inverse;
GPROC genie_curses_white_inverse;
GPROC genie_curses_red_inverse;
GPROC genie_curses_yellow_inverse;
GPROC genie_curses_magenta_inverse;
GPROC genie_curses_blue_inverse;
GPROC genie_curses_columns;
GPROC genie_curses_end;
GPROC genie_curses_getchar;
GPROC genie_curses_lines;
GPROC genie_curses_move;
GPROC genie_curses_putchar;
GPROC genie_curses_refresh;
GPROC genie_curses_start;
#endif

#if defined (HAVE_POSTGRESQL)
GPROC genie_pq_backendpid;
GPROC genie_pq_cmdstatus;
GPROC genie_pq_cmdtuples;
GPROC genie_pq_connectdb;
GPROC genie_pq_db;
GPROC genie_pq_errormessage;
GPROC genie_pq_exec;
GPROC genie_pq_fformat;
GPROC genie_pq_finish;
GPROC genie_pq_fname;
GPROC genie_pq_fnumber;
GPROC genie_pq_getisnull;
GPROC genie_pq_getvalue;
GPROC genie_pq_host;
GPROC genie_pq_nfields;
GPROC genie_pq_ntuples;
GPROC genie_pq_options;
GPROC genie_pq_parameterstatus;
GPROC genie_pq_pass;
GPROC genie_pq_port;
GPROC genie_pq_protocolversion;
GPROC genie_pq_reset;
GPROC genie_pq_resulterrormessage;
GPROC genie_pq_serverversion;
GPROC genie_pq_socket;
GPROC genie_pq_tty;
GPROC genie_pq_user;
#endif

#if defined (HAVE_GNU_PLOTUTILS)
GPROC genie_draw_aspect;
GPROC genie_draw_atom;
GPROC genie_draw_background_colour;
GPROC genie_draw_background_colour_name;
GPROC genie_draw_circle;
GPROC genie_draw_clear;
GPROC genie_draw_colour;
GPROC genie_draw_colour_name;
GPROC genie_draw_fillstyle;
GPROC genie_draw_fontname;
GPROC genie_draw_fontsize;
GPROC genie_draw_get_colour_name;
GPROC genie_draw_line;
GPROC genie_draw_linestyle;
GPROC genie_draw_linewidth;
GPROC genie_draw_move;
GPROC genie_draw_point;
GPROC genie_draw_rect;
GPROC genie_draw_show;
GPROC genie_draw_star;
GPROC genie_draw_text;
GPROC genie_draw_textangle;
GPROC genie_make_device;
#endif

#if defined (BUILD_PARALLEL_CLAUSE)
PROP_T genie_parallel (NODE_T *);
BOOL_T is_main_thread (void);
void genie_abend_all_threads (NODE_T *, jmp_buf *, NODE_T *);
void genie_set_exit_from_threads (int);
#define SAME_THREAD(p, q) (pthread_equal((p), (q)) != 0)
#define OTHER_THREAD(p, q) (pthread_equal((p), (q)) == 0)
#endif

#if defined (BUILD_LINUX)
GPROC genie_sigsegv;
#endif

#endif
