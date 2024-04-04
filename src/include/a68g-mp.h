//! @file a68g-mp.h
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
//! Definitions for multiple precision modes.

#if !defined (__A68G_MP_H__)
#define __A68G_MP_H__

// A68G's multiprecision algorithms are not suited for more than a few hundred
// digits. This is however sufficient for most practical MP applications.

#define MP_MAX_DECIMALS 250
#define MP_MAX_DIGITS (1 + MP_MAX_DECIMALS / LOG_MP_RADIX)

#define MP_STATUS(z) ((z)[0])
#define MP_EXPONENT(z) ((z)[1])
#define MP_DIGIT(z, n) ((z)[(n) + 1])
#define MP_SIGN(z) (SIGN (MP_DIGIT (z, 1)))
#define LEN_MP(digs) (2 + digs)
#define SIZE_MP(digs) A68_ALIGN (LEN_MP (digs) * sizeof (MP_T))
#define IS_ZERO_MP(z) (MP_DIGIT (z, 1) == (MP_T) 0)

#define PLUS_INF_MP(u) ((UNSIGNED_T) MP_STATUS (u) & PLUS_INF_MASK)
#define MINUS_INF_MP(u) ((UNSIGNED_T) MP_STATUS (u) & MINUS_INF_MASK)
#define INF_MP(u) (PLUS_INF_MP (u) || MINUS_INF_MP (u))
#define CHECK_LONG_REAL(p, u, moid) PRELUDE_ERROR (INF_MP (u), p, ERROR_INFINITE, moid)

static inline MP_T *set_mp (MP_T * z, MP_T x, INT_T expo, int digs)
{
  memset (z, 0, SIZE_MP (digs));
  MP_STATUS (z) = (MP_T) INIT_MASK;
  MP_DIGIT (z, 1) = x;
  MP_EXPONENT (z) = (MP_T) expo;
  return z;
}

static inline MP_T *move_mp (MP_T *z, MP_T *x, int N) 
{
  MP_T *y = z;
  N += 2;
  while (N--) {
    *z++ = *x++;
  }
  return y;
}

static inline MP_T *move_mp_part (MP_T *z, MP_T *x, int N) 
{
  MP_T *y = z;
  while (N--) {
    *z++ = *x++;
  }
  return y;
}

static inline void check_mp_exp (NODE_T *p, MP_T *z) 
{
  MP_T expo = (MP_EXPONENT (z) < 0 ? -MP_EXPONENT (z) : MP_EXPONENT (z));
  if (expo > MAX_MP_EXPONENT || (expo == MAX_MP_EXPONENT && ABS (MP_DIGIT (z, 1)) > (MP_T) 1)) {
    errno = EDOM;
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_MP_OUT_OF_BOUNDS, NULL);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
}

static inline MP_T *mp_one (int digs)
{
  if (digs > A68_MP (mp_one_size)) {
    if (A68_MP (mp_one) != (MP_T *) NULL) {
      a68_free (A68_MP (mp_one));
    }
    A68_MP (mp_one) = (MP_T *) get_heap_space (SIZE_MP (digs));
    set_mp (A68_MP (mp_one), 1, 0, digs);
  }
  return A68_MP (mp_one);
}

static inline MP_T *lit_mp (NODE_T *p, MP_T u, INT_T expo, int digs)
{
  ADDR_T pop_sp = A68_SP;
  if ((A68_SP += SIZE_MP (digs)) > A68 (expr_stack_limit)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  MP_T *z = (MP_T *) STACK_ADDRESS (pop_sp);
  (void) set_mp (z, u, expo, digs);
  return z;
}

static inline MP_T *nil_mp (NODE_T *p, int digs)
{
  ADDR_T pop_sp = A68_SP;
  if ((A68_SP += SIZE_MP (digs)) > A68 (expr_stack_limit)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  MP_T *z = (MP_T *) STACK_ADDRESS (pop_sp);
  (void) set_mp (z, 0, 0, digs);
  return z;
}

static inline MP_T *empty_mp (NODE_T *p, int digs)
{
  ADDR_T pop_sp = A68_SP;
  if ((A68_SP += SIZE_MP (digs)) > A68 (expr_stack_limit)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  return (MP_T *) STACK_ADDRESS (pop_sp);
}

extern MP_T *lengthen_mp (NODE_T *, MP_T *, int, MP_T *, int);

static inline MP_T *len_mp (NODE_T *p, MP_T *u, int digs, int gdigs)
{
  ADDR_T pop_sp = A68_SP;
  if ((A68_SP += SIZE_MP (gdigs)) > A68 (expr_stack_limit)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  MP_T *z = (MP_T *) STACK_ADDRESS (pop_sp);
  for (int k = 1; k <= digs; k++) {
    MP_DIGIT (z, k) = MP_DIGIT (u, k);
  }
  for (int k = digs + 1; k <= gdigs; k++) {
    MP_DIGIT (z, k) = (MP_T) 0;
  }
  MP_EXPONENT (z) = MP_EXPONENT (u);
  MP_STATUS (z) = MP_STATUS (u);
  return z;
}

static inline MP_T *cut_mp (NODE_T *p, MP_T *u, int digs, int gdigs)
{
  ADDR_T pop_sp = A68_SP;
  ASSERT (digs > gdigs);
  BOOL_T neg = MP_DIGIT (u, 1) < 0;
  if ((A68_SP += SIZE_MP (gdigs)) > A68 (expr_stack_limit)) {
    diagnostic (A68_RUNTIME_ERROR, p, ERROR_STACK_OVERFLOW);
    extern void exit_genie (NODE_T *, int);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  MP_T *z = (MP_T *) STACK_ADDRESS (pop_sp);
  for (int k = 1; k <= gdigs; k++) {
    MP_DIGIT (z, k) = MP_DIGIT (u, k);
  }
  if (neg) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
  if (MP_DIGIT (u, gdigs + 1) >= MP_RADIX / 2) {
    MP_DIGIT (z, gdigs) += 1;
    for (int k = digs; k >= 2 && MP_DIGIT (z, k) == MP_RADIX; k --) {
      MP_DIGIT (z, k) = 0;
      MP_DIGIT (z, k - 1) ++;
    }
  }
  if (neg) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
  MP_EXPONENT (z) = MP_EXPONENT (u);
  MP_STATUS (z) = MP_STATUS (u);
  return z;
}

//! @brief Length in bytes of a long mp number.

static inline size_t size_mp (void)
{
  return (size_t) SIZE_MP (LONG_MP_DIGITS);
}

//! @brief Length in digits of a long mp number.

static inline int mp_digits (void)
{
  return LONG_MP_DIGITS;
}

//! @brief Length in bytes of a long long mp number.

static inline size_t size_long_mp (void)
{
  return (size_t) (SIZE_MP (A68_MP (varying_mp_digits)));
}

//! @brief digits in a long mp number.

static inline int long_mp_digits (void)
{
  return A68_MP (varying_mp_digits);
}

#define SET_MP_ZERO(z, digits)\
  (void) set_mp ((z), 0, 0, digits);

#define SET_MP_ONE(z, digits)\
  (void) set_mp ((z), (MP_T) 1, 0, digits);

#define SET_MP_MINUS_ONE(z, digits)\
  (void) set_mp ((z), (MP_T) -1, 0, digits);

#define SET_MP_HALF(z, digits)\
  (void) set_mp ((z), (MP_T) (MP_RADIX / 2), -1, digits);

#define SET_MP_MINUS_HALF(z, digits)\
  (void) set_mp ((z), (MP_T) -(MP_RADIX / 2), -1, digits);

#define SET_MP_QUART(z, digits)\
  (void) set_mp ((z), (MP_T) (MP_RADIX / 4), -1, digits);

enum {MP_SQRT_PI, MP_PI, MP_LN_PI, MP_SQRT_TWO_PI, MP_TWO_PI, MP_HALF_PI, MP_180_OVER_PI, MP_PI_OVER_180};

// If MP_DOUBLE_PRECISION is defined functions are evaluated in double precision.

#undef MP_DOUBLE_PRECISION

#define MINIMUM(x, y) ((x) < (y) ? (x) : (y))

// GUARD_DIGITS: number of guard digits.

#if defined (MP_DOUBLE_PRECISION)
#define GUARD_DIGITS(digits) (digits)
#else
#define GUARD_DIGITS(digits) (2)
#endif

#define FUN_DIGITS(n) ((n) + GUARD_DIGITS (n))

BOOL_T check_mp_int (MP_T *, MOID_T *);
BOOL_T same_mp (NODE_T *, MP_T *, MP_T *, int);
int long_mp_digits (void);
INT_T mp_to_int (NODE_T *, MP_T *, int);
int width_to_mp_digits (int);
MP_T *abs_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acosdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acosh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acos_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acotdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acot_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acsc_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *acscdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *add_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *align_mp (MP_T *, INT_T *, int);
MP_T *asec_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *asecdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *asindg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *asinh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *asin_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *atan2dg_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *atan2_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *atandg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *atanh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *atan_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *beta_inc_mp (NODE_T *, MP_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *beta_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *cacosh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cacos_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *casinh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *casin_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *catanh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *catan_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *ccosh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *ccos_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cdiv_mp (NODE_T *, MP_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *cexp_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cln_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cmul_mp (NODE_T *, MP_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *cosdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cosh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cas_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cos_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cospi_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cotdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cot_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cotpi_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *csc_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *cscdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *csinh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *csin_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *csqrt_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *ctanh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *ctan_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *curt_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *div_mp_digit (NODE_T *, MP_T *, MP_T *, MP_T, int);
MP_T *div_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *entier_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *erfc_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *erf_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *expm1_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *exp_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *floor_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *gamma_inc_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *gamma_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *half_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *hyp_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *hypot_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *int_to_mp (NODE_T *, MP_T *, INT_T, int);
MP_T *inverfc_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *inverf_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *lnbeta_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *lngamma_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *ln_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *log_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *minus_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *minus_one_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *mod_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *mp_ln_10 (NODE_T *, MP_T *, int);
MP_T *mp_ln_scale (NODE_T *, MP_T *, int);
MP_T *mp_pi (NODE_T *, MP_T *, int, int);
MP_T *ten_up_mp (NODE_T *, MP_T *, int, int);
MP_T *mul_mp_digit (NODE_T *, MP_T *, MP_T *, MP_T, int);
MP_T *mul_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *one_minus_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *over_mp_digit (NODE_T *, MP_T *, MP_T *, MP_T, int);
MP_T *over_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *plus_one_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *pow_mp_int (NODE_T *, MP_T *, MP_T *, INT_T, int);
MP_T *pow_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *real_to_mp (NODE_T *, MP_T *, REAL_T, int);
MP_T *rec_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *round_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *sec_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *secdg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *set_mp (MP_T *, MP_T, INT_T, int);
MP_T *shorten_mp (NODE_T *, MP_T *, int, MP_T *, int);
MP_T *sindg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *sinh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *sin_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *sinpi_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *sqrt_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *strtomp (NODE_T *, MP_T *, char *, int);
MP_T *sub_mp (NODE_T *, MP_T *, MP_T *, MP_T *, int);
MP_T *tandg_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *tanh_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *tan_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *tanpi_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *tenth_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *trunc_mp (NODE_T *, MP_T *, MP_T *, int);
MP_T *unt_to_mp (NODE_T *, MP_T *, UNSIGNED_T, int);
REAL_T mp_to_real (NODE_T *, MP_T *, int);
void eq_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void ge_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void genie_pi_mp (NODE_T *);
void gt_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void le_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void lt_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void ne_mp (NODE_T *, A68_BOOL *, MP_T *, MP_T *, int);
void raw_write_mp (char *, MP_T *, int);
void set_long_mp_digits (int);
void test_mp_int_range (NODE_T *, MP_T *, MOID_T *);

GPROC genie_infinity_mp;
GPROC genie_minus_infinity_mp;
GPROC genie_beta_inc_mp;
GPROC genie_gamma_inc_mp;
GPROC genie_gamma_inc_f_mp;
GPROC genie_gamma_inc_g_mp;
GPROC genie_gamma_inc_h_mp;
GPROC genie_gamma_inc_gf_mp;
GPROC genie_abs_mp;
GPROC genie_abs_mp_complex;
GPROC genie_acosdg_mp;
GPROC genie_acosdg_mp;
GPROC genie_acosh_mp;
GPROC genie_acosh_mp_complex;
GPROC genie_acos_mp;
GPROC genie_acos_mp_complex;
GPROC genie_acotdg_mp;
GPROC genie_acot_mp;
GPROC genie_asec_mp;
GPROC genie_asecdg_mp;
GPROC genie_acsc_mp;
GPROC genie_acscdg_mp;
GPROC genie_add_mp;
GPROC genie_add_mp_complex;
GPROC genie_and_mp;
GPROC genie_arg_mp_complex;
GPROC genie_asindg_mp;
GPROC genie_asindg_mp;
GPROC genie_asinh_mp;
GPROC genie_asinh_mp_complex;
GPROC genie_asin_mp;
GPROC genie_asin_mp_complex;
GPROC genie_atan2_mp;
GPROC genie_atandg_mp;
GPROC genie_atan2dg_mp;
GPROC genie_atanh_mp;
GPROC genie_atanh_mp_complex;
GPROC genie_atan_mp;
GPROC genie_atan_mp_complex;
GPROC genie_bin_mp;
GPROC genie_clear_long_mp_bits;
GPROC genie_conj_mp_complex;
GPROC genie_cosdg_mp;
GPROC genie_cosh_mp;
GPROC genie_cosh_mp_complex;
GPROC genie_cas_mp;
GPROC genie_cos_mp;
GPROC genie_cos_mp_complex;
GPROC genie_cospi_mp;
GPROC genie_cotdg_mp;
GPROC genie_cot_mp;
GPROC genie_sec_mp;
GPROC genie_secdg_mp;
GPROC genie_csc_mp;
GPROC genie_cscdg_mp;
GPROC genie_cotpi_mp;
GPROC genie_curt_mp;
GPROC genie_divab_mp;
GPROC genie_divab_mp_complex;
GPROC genie_div_mp;
GPROC genie_div_mp_complex;
GPROC genie_elem_long_mp_bits;
GPROC genie_elem_long_mp_bits;
GPROC genie_entier_mp;
GPROC genie_eq_mp;
GPROC genie_eq_mp_complex;
GPROC genie_erfc_mp;
GPROC genie_erf_mp;
GPROC genie_exp_mp;
GPROC genie_exp_mp_complex;
GPROC genie_gamma_mp;
GPROC genie_lngamma_mp;
GPROC genie_beta_mp;
GPROC genie_lnbeta_mp;
GPROC genie_ge_mp;
GPROC genie_get_long_mp_bits;
GPROC genie_get_long_mp_complex;
GPROC genie_get_long_mp_int;
GPROC genie_get_long_mp_real;
GPROC genie_get_mp_complex;
GPROC genie_gt_mp;
GPROC genie_im_mp_complex;
GPROC genie_inverfc_mp;
GPROC genie_inverf_mp;
GPROC genie_le_mp;
GPROC genie_lengthen_complex_to_mp_complex;
GPROC genie_lengthen_int_to_mp;
GPROC genie_lengthen_mp_complex_to_long_mp_complex;
GPROC genie_lengthen_mp_to_long_mp;
GPROC genie_lengthen_real_to_mp;
GPROC genie_lengthen_unt_to_mp;
GPROC genie_ln_mp;
GPROC genie_ln_mp_complex;
GPROC genie_log_mp;
GPROC genie_long_mp_bits_width;
GPROC genie_long_mp_exp_width;
GPROC genie_long_mp_int_width;
GPROC genie_long_mp_max_bits;
GPROC genie_long_mp_max_int;
GPROC genie_long_mp_max_real;
GPROC genie_long_mp_min_real;
GPROC genie_long_mp_real_width;
GPROC genie_long_mp_small_real;
GPROC genie_lt_mp;
GPROC genie_minusab_mp;
GPROC genie_minusab_mp_complex;
GPROC genie_minus_mp;
GPROC genie_minus_mp_complex;
GPROC genie_modab_mp;
GPROC genie_mod_mp;
GPROC genie_mul_mp;
GPROC genie_mul_mp_complex;
GPROC genie_ne_mp;
GPROC genie_ne_mp_complex;
GPROC genie_not_mp;
GPROC genie_odd_mp;
GPROC genie_or_mp;
GPROC genie_overab_mp;
GPROC genie_over_mp;
GPROC genie_pi_mp;
GPROC genie_plusab_mp;
GPROC genie_plusab_mp_complex;
GPROC genie_pow_mp;
GPROC genie_pow_mp_complex_int;
GPROC genie_pow_mp_int;
GPROC genie_pow_mp_int_int;
GPROC genie_print_long_mp_bits;
GPROC genie_print_long_mp_complex;
GPROC genie_print_long_mp_int;
GPROC genie_print_long_mp_real;
GPROC genie_print_mp_complex;
GPROC genie_put_long_mp_bits;
GPROC genie_put_long_mp_complex;
GPROC genie_put_long_mp_int;
GPROC genie_put_long_mp_real;
GPROC genie_put_mp_complex;
GPROC genie_read_long_mp_bits;
GPROC genie_read_long_mp_complex;
GPROC genie_read_long_mp_int;
GPROC genie_read_long_mp_real;
GPROC genie_read_mp_complex;
GPROC genie_re_mp_complex;
GPROC genie_round_mp;
GPROC genie_set_long_mp_bits;
GPROC genie_shl_mp;
GPROC genie_shorten_long_mp_complex_to_mp_complex;
GPROC genie_shorten_long_mp_to_mp;
GPROC genie_shorten_mp_complex_to_complex;
GPROC genie_shorten_mp_to_bits;
GPROC genie_shorten_mp_to_int;
GPROC genie_shorten_mp_to_real;
GPROC genie_shr_mp;
GPROC genie_sign_mp;
GPROC genie_sindg_mp;
GPROC genie_sinh_mp;
GPROC genie_sinh_mp_complex;
GPROC genie_sin_mp;
GPROC genie_sin_mp_complex;
GPROC genie_sinpi_mp;
GPROC genie_sqrt_mp;
GPROC genie_sqrt_mp_complex;
GPROC genie_sub_mp;
GPROC genie_sub_mp_complex;
GPROC genie_tandg_mp;
GPROC genie_tanh_mp;
GPROC genie_tanh_mp_complex;
GPROC genie_tan_mp;
GPROC genie_tan_mp_complex;
GPROC genie_tanpi_mp;
GPROC genie_timesab_mp;
GPROC genie_timesab_mp_complex;
GPROC genie_xor_mp;

#if defined (HAVE_GNU_MPFR)
GPROC genie_beta_inc_mpfr;
GPROC genie_ln_beta_mpfr;
GPROC genie_beta_mpfr;
GPROC genie_gamma_inc_mpfr;
GPROC genie_gamma_inc_real_mpfr;
GPROC genie_gamma_inc_double_mpfr;
GPROC genie_gamma_mpfr;
GPROC genie_lngamma_mpfr;
GPROC genie_mpfr_erfc_mp;
GPROC genie_mpfr_erf_mp;
GPROC genie_mpfr_inverfc_mp;
GPROC genie_mpfr_inverf_mp;
GPROC genie_mpfr_mp;
size_t mpfr_digits (void);
#endif

#if (A68_LEVEL >= 3)
GPROC genie_quad_mp;
#endif

#if (A68_LEVEL <= 2)
int get_mp_bits_width (MOID_T *);
int get_mp_bits_words (MOID_T *);
MP_BITS_T *stack_mp_bits (NODE_T *, MP_T *, MOID_T *);
#endif

#endif
