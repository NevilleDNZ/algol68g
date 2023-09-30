//! @file a68g-lib.h
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
//! Internal math routines.

#if !defined (__A68G_LIB_H__)
#define __A68G_LIB_H__

#define A68_INVALID(c)\
  if (c) {\
    errno = EDOM;\
    return 0;\
  }

#define A68_OVERFLOW(c)\
  if (c) {\
    errno = ERANGE;\
    return 0;\
  }

#define A68_MAX_FAC 170

INT_T a68_add_int (INT_T, INT_T);
INT_T a68_mod_int (INT_T, INT_T);
INT_T a68_mul_int (INT_T, INT_T);
INT_T a68_m_up_n (INT_T, INT_T);
INT_T a68_over_int (INT_T, INT_T);
INT_T a68_round (REAL_T);
INT_T a68_sub_int (INT_T, INT_T);
REAL_T a68_abs_real (REAL_T);
REAL_T a68_acosdg_real (REAL_T);
REAL_T a68_acosh_real (REAL_T);
REAL_T a68_acotdg_real (REAL_T);
REAL_T a68_acot_real (REAL_T);
REAL_T a68_acsc_real (REAL_T);
REAL_T a68_acscdg_real (REAL_T);
REAL_T a68_asec_real (REAL_T);
REAL_T a68_asecdg_real (REAL_T);
REAL_T a68_asindg_real (REAL_T);
REAL_T a68_asinh_real (REAL_T);
REAL_T a68_atan2_real (REAL_T, REAL_T);
REAL_T a68_atandg_real (REAL_T);
REAL_T a68_atanh_real (REAL_T);
REAL_T a68_beta_real (REAL_T, REAL_T);
REAL_T a68_choose_real (INT_T, INT_T);
REAL_T a68_cas_real (REAL_T);
REAL_T a68_cosdg_real (REAL_T);
REAL_T a68_cospi_real (REAL_T);
REAL_T a68_cot_realdg_real (REAL_T);
REAL_T a68_cot_realpi (REAL_T);
REAL_T a68_cot_real (REAL_T);
REAL_T a68_csc_real (REAL_T);
REAL_T a68_cscdg_real (REAL_T);
REAL_T a68_sec_real (REAL_T);
REAL_T a68_secdg_real (REAL_T);
REAL_T a68_div_int (INT_T, INT_T);
REAL_T a68_exp_real (REAL_T);
REAL_T a68_fact_real (INT_T);
REAL_T a68_fdiv_real (REAL_T, REAL_T);
REAL_T a68_hypot_real (REAL_T, REAL_T);
REAL_T a68_int_real (REAL_T);
REAL_T a68_inverfc_real (REAL_T);
REAL_T a68_inverf_real (REAL_T);
REAL_T a68_ln1p_real (REAL_T);
REAL_T a68_ln1p_real (REAL_T);
REAL_T a68_ln_beta_real (REAL_T, REAL_T);
REAL_T a68_ln_choose_real (INT_T, INT_T);
REAL_T a68_ln_fact_real (INT_T);
REAL_T a68_ln_real (REAL_T);
REAL_T a68_max_real (REAL_T, REAL_T);
REAL_T a68_min_real (REAL_T, REAL_T);
REAL_T a68_nan_real (void);
REAL_T a68_neginf_double_real (void);
REAL_T a68_posinf_real (void);
REAL_T a68_psi_real (REAL_T);
REAL_T a68_sign_real (REAL_T);
REAL_T a68_sindg_real (REAL_T);
REAL_T a68_sinpi_real (REAL_T);
REAL_T a68_tandg_real (REAL_T);
REAL_T a68_tanpi_real (REAL_T);
REAL_T a68_x_up_n_real (REAL_T, INT_T);
REAL_T a68_x_up_y (REAL_T, REAL_T);
REAL_T a68_beta_inc_real (REAL_T, REAL_T, REAL_T);
DOUBLE_T a68_beta_inc_double (DOUBLE_T, DOUBLE_T, DOUBLE_T);
DOUBLE_T a68_cot_double (DOUBLE_T);
DOUBLE_T a68_csc_double (DOUBLE_T);
DOUBLE_T a68_cscdg_double (DOUBLE_T);
DOUBLE_T a68_sec_double (DOUBLE_T);
DOUBLE_T a68_secdg_double (DOUBLE_T);
DOUBLE_T a68_acot_double (DOUBLE_T);
DOUBLE_T a68_acsc_double (DOUBLE_T);
DOUBLE_T a68_acscdg_double (DOUBLE_T);
DOUBLE_T a68_asec_double (DOUBLE_T);
DOUBLE_T a68_asecdg_double (DOUBLE_T);
DOUBLE_T a68_sindg_double (DOUBLE_T);
DOUBLE_T a68_cas_double (DOUBLE_T);
DOUBLE_T a68_cosdg_double (DOUBLE_T);
DOUBLE_T a68_tandg_double (DOUBLE_T);
DOUBLE_T a68_asindg_double (DOUBLE_T);
DOUBLE_T a68_acosdg_double (DOUBLE_T);
DOUBLE_T a68_atandg_double (DOUBLE_T);
DOUBLE_T a68_cotdg_double (DOUBLE_T);
DOUBLE_T a68_acotdg_double (DOUBLE_T);
DOUBLE_T a68_sinpi_double (DOUBLE_T); 
DOUBLE_T a68_cospi_double (DOUBLE_T); 
DOUBLE_T a68_tanpi_double (DOUBLE_T);
DOUBLE_T a68_cotpi_double (DOUBLE_T);

#endif
