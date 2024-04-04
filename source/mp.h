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

#ifndef A68G_MP_H
#define A68G_MP_H

/*-------1---------2---------3---------4---------5---------6---------7---------+
Definitions for the multi-precision library.                                  */

typedef double mp_digit;

#define DEFAULT_MP_RADIX 	10000000
#define DEFAULT_DOUBLE_DIGITS	5

#define MP_RADIX      		DEFAULT_MP_RADIX
#define LOG_MP_BASE    		7
#define MP_BITS_RADIX		8388608
#define MP_BITS_BITS		23

/* 28-35 decimal digits for LONG REAL */
#define LONG_MP_DIGITS 		DEFAULT_DOUBLE_DIGITS

#define MAX_REPR_INT   		9e15	/* About 2^53, max int fitting a double */

#define MAX_MP_EXPONENT 142857	/* Arbitrary. Let M = MAX_REPR_INT then the largest range
				   should be M / Log M / LOG_MP_BASE, but this isn't tested */

#define MAX_MP_PRECISION 5000	/* Can be larger, but provokes a warning */

extern int varying_mp_digits;

#define LONG_EXP_WIDTH       (EXP_WIDTH)
#define LONGLONG_EXP_WIDTH   (EXP_WIDTH)

#define LONG_WIDTH           (LONG_MP_DIGITS * LOG_MP_BASE)
#define LONGLONG_WIDTH       (varying_mp_digits * LOG_MP_BASE)

#define LONG_INT_WIDTH       (1 + LONG_WIDTH)
#define LONGLONG_INT_WIDTH   (1 + LONGLONG_WIDTH)

/* When changing L REAL_WIDTH mind that a mp number may not have more than
   1 + (MP_DIGITS - 1) * LOG_MP_BASE digits. Next definition is in accordance 
   with REAL_WIDTH.                                                           */

#define LONG_REAL_WIDTH      ((LONG_MP_DIGITS - 1) * LOG_MP_BASE)
#define LONGLONG_REAL_WIDTH  ((varying_mp_digits - 1) * LOG_MP_BASE)

#define LOG2_10			3.321928094887362347870319430
#define MP_BITS_WIDTH(k)	((int) ceil((k) * LOG_MP_BASE * LOG2_10) - 1)
#define MP_BITS_WORDS(k)	((int) ceil ((double) MP_BITS_WIDTH (k) / (double) MP_BITS_BITS))

enum
{ MP_PI, MP_TWO_PI, MP_HALF_PI };

/*-------1---------2---------3---------4---------5---------6---------7---------+
MP Macros                                                                     */

#define MP_STATUS(z) ((z)[0])
#define MP_EXPONENT(z) ((z)[1])
#define MP_DIGIT(z, n) ((z)[(n) + 1])

#define SIZE_MP(digits) ((2 + digits) * SIZE_OF (mp_digit))

#define MOVE_MP(z, x, digits) {memmove ((z), (x), (unsigned) SIZE_MP(digits));}

#define TEST_MP_INIT(p, z, m) {\
	if (! ((int) z[0] & INITIALISED_MASK)) {\
		diagnostic (A_RUNTIME_ERROR, (p), EMPTY_VALUE_ERROR, (m));\
		exit_genie ((p), 1);\
		}\
	}

#define CHECK_MP_EXPONENT(p, z, str) {\
  mp_digit expo = fabs (MP_EXPONENT (z));\
  if (expo > MAX_MP_EXPONENT || (expo == MAX_MP_EXPONENT && fabs (MP_DIGIT (z, 1)) > 1.0))\
    {\
      errno = ERANGE;\
      diagnostic (A_RUNTIME_ERROR, p, "multiprecision value out of bounds", NULL);\
      exit_genie (p, 1);\
    }\
}

#define SET_MP_ZERO(z, digits)\
  {memset (&(z)[1], 0, (unsigned) (((digits) + 1) * SIZE_OF (mp_digit)));}

/*-------1---------2---------3---------4---------5---------6---------7---------+
External procedures.                                                          */

extern BOOL_T check_long_int (mp_digit *);
extern BOOL_T check_longlong_int (mp_digit *);
extern BOOL_T check_mp_int (mp_digit *, MOID_T *);
extern char *long_sub_fixed (NODE_T *, mp_digit *, int, int, int);
extern char *long_sub_whole (NODE_T *, mp_digit *, int, int);
extern double mp_to_real (NODE_T *, mp_digit *, int);
extern int get_mp_bits_width (MOID_T *);
extern int get_mp_bits_words (MOID_T *);
extern int get_mp_digits (MOID_T *);
extern int get_mp_size (MOID_T *);
extern int int_to_mp_digits (int);
extern int long_mp_digits (void);
extern int longlong_mp_digits (void);
extern int mp_to_int (NODE_T *, mp_digit *, int);
extern mp_digit *acos_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *add_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *asin_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *atan_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *cos_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *curt_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *div_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *div_mp_digit (NODE_T *, mp_digit *, mp_digit *, mp_digit, int);
extern mp_digit *exp_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *int_to_mp (NODE_T *, mp_digit *, int, int);
extern mp_digit *lengthen_mp (NODE_T *, mp_digit *, int, mp_digit *, int);
extern mp_digit *ln_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *log_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *mod_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *mp_pi (NODE_T *, mp_digit *, int, int);
extern mp_digit *mul_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *mul_mp_digit (NODE_T *, mp_digit *, mp_digit *, mp_digit, int);
extern mp_digit *over_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *over_mp_digit (NODE_T *, mp_digit *, mp_digit *, mp_digit, int);
extern mp_digit *pack_mp_bits (NODE_T *, mp_digit *, unsigned *, MOID_T *);
extern mp_digit *pow_mp_int (NODE_T *, mp_digit *, mp_digit *, int, int);
extern mp_digit *real_to_mp (NODE_T *, mp_digit *, double, int);
extern mp_digit *set_mp_short (mp_digit *, mp_digit, int, int);
extern mp_digit *shorten_mp (NODE_T *, mp_digit *, int, mp_digit *, int);
extern mp_digit *sin_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *sqrt_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *stack_mp (NODE_T *, int);
extern mp_digit *string_to_mp (NODE_T *, mp_digit *, char *, int);
extern mp_digit *sub_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
extern mp_digit *tan_mp (NODE_T *, mp_digit *, mp_digit *, int);
extern mp_digit *unsigned_to_mp (NODE_T *, mp_digit *, unsigned, int);
extern size_t size_long_mp (void);
extern size_t size_longlong_mp (void);
extern unsigned *stack_mp_bits (NODE_T *, mp_digit *, MOID_T *);
extern unsigned mp_to_unsigned (NODE_T *, mp_digit *, int);
extern void check_long_bits_value (NODE_T *, mp_digit *, MOID_T *);
extern void long_standardise (NODE_T *, mp_digit *, int, int, int, int *);
extern void raw_write_mp (char *, mp_digit *, int);
extern void set_longlong_mp_digits (int);
extern void trunc_mp (mp_digit *, mp_digit *, int);

#endif
