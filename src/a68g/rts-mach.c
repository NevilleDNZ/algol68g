//! @file rts-mach.c
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2024 J. Marcel van der Veer [algol68g@xs4all.nl].

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
//! Machine parameters.

// These routines are variants/extensions of SLATEC routines.
// SLATEC Common Mathematical Library is a FORTRAN 77 library of general purpose
// mathematical and statistical routines, developed at US Government research 
// laboratories and therefore public domain software. 
// Repository: http://www.netlib.org/slatec/

#include "a68g.h"
#include "a68g-prelude.h"
#include "a68g-genie.h"
#include "a68g-numbers.h"
#include "a68g-double.h"

int a68g_i32mach (int i)
{
// Based SLATEC routine I1MACH.
  switch (i) {
// i32mach(1) = the standard input unit. 
    case 1: {
	return STDIN_FILENO;
      }
// i32mach(2) = the standard output unit. 
    case 2: {
	return STDOUT_FILENO;
      }
// i32mach(3) = the standard punch unit. 
    case 3: {
	return STDOUT_FILENO;
      }
// i32mach(4) = the standard error message unit. 
    case 4: {
	return STDERR_FILENO;
      }
// i32mach(5) = the number of bits per int storage unit. 
    case 5: {
	return CHAR_BIT * sizeof (int);
      }
// i32mach(6) = the number of characters per int storage unit. 
    case 6: {
	return sizeof (int);
      }
// i32mach(7) = a, the base. 
    case 7: {
	return 2;
      }
// i32mach(8) = s, the number of base-a digits. 
    case 8: {
	return CHAR_BIT * sizeof (int) - 1;
      }
// i32mach(9) = a**s - 1, the largest magnitude. 
    case 9: {
	return INT_MAX;
      }
// i32mach(10) = b, the base. 
    case 10: {
	return FLT_RADIX;
      }
// i32mach(11) = t, the number of base-b digits. 
    case 11: {
	return FLT_MANT_DIG;
      }
// i32mach(12) = emin, the smallest exponent e. 
    case 12: {
	return FLT_MIN_EXP;
      }
// i32mach(13) = emax, the largest exponent e. 
    case 13: {
	return FLT_MAX_EXP;
      }
// i32mach(14) = t, the number of base-b digits. 
    case 14: {
	return DBL_MANT_DIG;
      }
// i32mach(15) = emin, the smallest exponent e. 
    case 15: {
	return DBL_MIN_EXP;
      }
// i32mach(16) = emax, the largest exponent e. 
    case 16: {
	return DBL_MAX_EXP;
      }
    default: {
	return 0;
      }
    }
}

//! @brief PROC i32mach (INT) INT

void genie_i32mach (NODE_T *p)
{
  A68_INT i;
  POP_OBJECT (p, &i, A68_INT);
  PUSH_VALUE (p, a68g_i32mach (VALUE (&i)), A68_INT);
}

REAL_T a68g_r64mach (int i)
{
// Based SLATEC routine R1MACH.
  switch (i) {
// r64mach(1) = b**(emin-1), the smallest positive magnitude. 
    case 1: {
	return DBL_MIN;
      }
// r64mach(2) = b**emax*(1 - b**(-t)), the largest magnitude. 
    case 2: {
	return DBL_MAX;
      }
// r64mach(3) = b**(-t), the smallest relative spacing. 
    case 3: {
	return 0.5 * DBL_EPSILON;
      }
// r64mach(4) = b**(1-t), the largest relative spacing. 
    case 4: {
	return DBL_EPSILON;
      }
// r64mach(5) = log10(b) 
    case 5: {
	return CONST_M_LOG10_2;
      }
// r64mach(6), the minimum exponent in base 10.
    case 6: {
        return DBL_MIN_10_EXP;
      }
// r64mach(7), the maximum exponent in base 10.
    case 7: {
        return DBL_MAX_10_EXP;
      }
// r64mach(8), the number of significant digits in base 10.
    case 8: {
        return DBL_DIG;
      }
// r64mach(9), the number of mantissa bits.
    case 9: {
        return DBL_MANT_DIG;
      }
    default: {
	return 0.0;
      }
    }
}

//! @brief PROC r64mach (INT) REAL

void genie_r64mach (NODE_T *p)
{
  A68_INT i;
  POP_OBJECT (p, &i, A68_INT);
  PUSH_VALUE (p, a68g_r64mach (VALUE (&i)), A68_REAL);
}

#if (A68_LEVEL >= 3)

INT_T a68g_i64mach (int i)
{
// Based SLATEC routine I1MACH.
  switch (i) {
// i64mach(6) = the number of characters per int storage unit. 
    case 6: {
	return sizeof (INT_T);
      }
// i64mach(9) = a**s - 1, the largest magnitude. 
    case 9: {
	return LLONG_MAX;
      }
    default: {
	return a68g_i32mach (i);
      }
    }
}

//! @brief PROC i64mach (INT) INT

void genie_i64mach (NODE_T *p)
{
  A68_INT i;
  POP_OBJECT (p, &i, A68_INT);
  PUSH_VALUE (p, a68g_i64mach (VALUE (&i)), A68_INT);
}

DOUBLE_T a68g_r128mach (int i)
{
// Based SLATEC routine D1MACH.
  switch (i) {
// r128mach(1) = b**(emin-1), the smallest positive magnitude. 
    case 1: {
	return FLT128_MIN;
      }
// r128mach(2) = b**emax*(1 - b**(-t)), the largest magnitude. 
    case 2: {
	return FLT128_MAX;
      }
// r128mach(3) = b**(-t), the smallest relative spacing. 
    case 3: {
	return 0.5 * FLT128_EPSILON;
      }
// r128mach(4) = b**(1-t), the largest relative spacing. 
    case 4: {
	return FLT128_EPSILON;
      }
// r128mach(5) = log10(b) 
    case 5: {
	return CONST_M_LOG10_2_Q;
      }
// r128mach(6), the minimum exponent in base 10.
    case 6: {
        return FLT128_MIN_10_EXP;
      }
// r128mach(7), the maximum exponent in base 10.
    case 7: {
        return FLT128_MAX_10_EXP;
      }
// r128mach(8), the number of significant digits in base 10.
    case 8: {
        return FLT128_DIG;
      }
// r128mach(9), the number of mantissa bits.
    case 9: {
        return FLT128_MANT_DIG;
      }
    default: {
	return 0.0;
      }
    }
}

//! @brief PROC r128mach (INT) REAL

void genie_r128mach (NODE_T *p)
{
  A68_INT i;
  POP_OBJECT (p, &i, A68_INT);
  PUSH_VALUE (p, dble (a68g_r128mach (VALUE (&i))), A68_LONG_REAL);
}

#endif
