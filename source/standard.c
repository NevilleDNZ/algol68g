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

/*-------1---------2---------3---------4---------5---------6---------7---------+
This file contains Algol68G's standard environ. Transput routines are not here.
Some of the LONG operations are generic for LONG and LONG LONG.               */

#include "algol68g.h"
#include "genie.h"
#include "mp.h"

double cputime_0;

#define A68_MONAD(n, p, MODE, OP) void n (NODE_T * p) {\
  MODE *i;\
  POP_OPERAND_ADDRESS (p, i, MODE);\
  i->value = OP i->value;\
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Environment enquiries.                                                        */

A68_ENV_INT (genie_int_lengths, 3);
A68_ENV_INT (genie_int_shorts, 1);
A68_ENV_INT (genie_real_lengths, 3);
A68_ENV_INT (genie_real_shorts, 1);
A68_ENV_INT (genie_complex_lengths, 3);
A68_ENV_INT (genie_complex_shorts, 1);
A68_ENV_INT (genie_bits_lengths, 3);
A68_ENV_INT (genie_bits_shorts, 1);
A68_ENV_INT (genie_bytes_lengths, 2);
A68_ENV_INT (genie_bytes_shorts, 1);
A68_ENV_INT (genie_int_width, INT_WIDTH);
A68_ENV_INT (genie_long_int_width, LONG_INT_WIDTH);
A68_ENV_INT (genie_longlong_int_width, LONGLONG_INT_WIDTH);
A68_ENV_INT (genie_real_width, REAL_WIDTH);
A68_ENV_INT (genie_long_real_width, LONG_REAL_WIDTH);
A68_ENV_INT (genie_longlong_real_width, LONGLONG_REAL_WIDTH);
A68_ENV_INT (genie_exp_width, EXP_WIDTH);
A68_ENV_INT (genie_long_exp_width, LONG_EXP_WIDTH);
A68_ENV_INT (genie_longlong_exp_width, LONGLONG_EXP_WIDTH);
A68_ENV_INT (genie_bits_width, BITS_WIDTH);
A68_ENV_INT (genie_long_bits_width, get_mp_bits_width (MODE (LONG_BITS)));
A68_ENV_INT (genie_longlong_bits_width, get_mp_bits_width (MODE (LONGLONG_BITS)));
A68_ENV_INT (genie_bytes_width, BYTES_WIDTH);
A68_ENV_INT (genie_long_bytes_width, LONG_BYTES_WIDTH);
A68_ENV_INT (genie_max_abs_char, UCHAR_MAX);
A68_ENV_INT (genie_max_int, MAX_INT);
A68_ENV_REAL (genie_max_real, DBL_MAX);
A68_ENV_REAL (genie_small_real, DBL_EPSILON);
A68_ENV_REAL (genie_pi, A68G_PI);
A68_ENV_REAL (genie_seconds, seconds ());
A68_ENV_REAL (genie_cputime, seconds () - cputime_0);
A68_ENV_INT (genie_stack_pointer, stack_pointer);
A68_ENV_INT (genie_system_stack_size, stack_size);

/*-------1---------2---------3---------4---------5---------6---------7---------+
INT system stack pointer # stack pointer #.                                   */

void
genie_system_stack_pointer (NODE_T * p)
{
  BYTE_T stack_offset;
  PUSH_INT (p, (int) system_stack_offset - (int) &stack_offset);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG INT max long int                                                         */

void
genie_long_max_int (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_INT));
  mp_digit *z = stack_mp (p, digits);
  int k, j = 1 + digits;
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = digits - 1;
  for (k = 2; k <= j; k++)
    {
      z[k] = MP_RADIX - 1;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG LONG INT max long long int                                               */

void
genie_longlong_max_int (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONGLONG_INT));
  mp_digit *z = stack_mp (p, digits);
  int k, j = 1 + digits;
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = digits - 1;
  for (k = 2; k <= j; k++)
    {
      z[k] = MP_RADIX - 1;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG REAL max long real                                                       */

void
genie_long_max_real (NODE_T * p)
{
  int j, digits = get_mp_digits (MODE (LONG_REAL));
  mp_digit *z = stack_mp (p, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = MAX_MP_EXPONENT - 1;
  for (j = 2; j <= 1 + digits; j++)
    {
      z[j] = MP_RADIX - 1;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG LONG REAL max long long real                                             */

void
genie_longlong_max_real (NODE_T * p)
{
  int j, digits = get_mp_digits (MODE (LONGLONG_REAL));
  mp_digit *z = stack_mp (p, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = MAX_MP_EXPONENT - 1;
  for (j = 2; j <= 1 + digits; j++)
    {
      z[j] = MP_RADIX - 1;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG REAL small long real                                                     */

void
genie_long_small_real (NODE_T * p)
{
  int j, digits = get_mp_digits (MODE (LONG_REAL));
  mp_digit *z = stack_mp (p, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = -(digits - 1);
  MP_DIGIT (z, 1) = 1;
  for (j = 3; j <= 1 + digits; j++)
    {
      z[j] = 0;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG LONG REAL small long long real                                           */

void
genie_longlong_small_real (NODE_T * p)
{
  int j, digits = get_mp_digits (MODE (LONGLONG_REAL));
  mp_digit *z = stack_mp (p, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = -(digits - 1);
  MP_DIGIT (z, 1) = 1;
  for (j = 3; j <= 1 + digits; j++)
    {
      z[j] = 0;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
BITS max bits                                                                 */

void
genie_max_bits (NODE_T * p)
{
  PUSH_BITS (p, MAX_BITS);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG BITS long max bits                                                       */

void
genie_long_max_bits (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_BITS));
  int width = get_mp_bits_width (MODE (LONG_BITS));
  ADDR_T save_sp;
  mp_digit *z = stack_mp (p, digits), *one;
  save_sp = stack_pointer;
  one = stack_mp (p, digits);
  set_mp_short (z, (mp_digit) 2, 0, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits);
  pow_mp_int (p, z, z, width, digits);
  sub_mp (p, z, z, one, digits);
  stack_pointer = save_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG LONG BITS long long max bits                                             */

void
genie_longlong_max_bits (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONGLONG_BITS));
  int width = get_mp_bits_width (MODE (LONGLONG_BITS));
  ADDR_T save_sp;
  mp_digit *z = stack_mp (p, digits), *one;
  save_sp = stack_pointer;
  one = stack_mp (p, digits);
  set_mp_short (z, (mp_digit) 2, 0, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits);
  pow_mp_int (p, z, z, width, digits);
  sub_mp (p, z, z, one, digits);
  stack_pointer = save_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
LONG REAL long pi                                                             */

void
genie_pi_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p));
  mp_digit *z = stack_mp (p, digits);
  mp_pi (p, z, MP_PI, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
BOOL operations.                                                              */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP NOT = (BOOL) BOOL                                                          */

A68_MONAD (genie_not_bool, p, A68_BOOL, !)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (BOOL) INT                                                           */
     void genie_abs_bool (NODE_T * p)
{
  A68_BOOL j;
  POP_BOOL (p, &j);
  PUSH_INT (p, j.value ? 1 : 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (BOOL, BOOL) BOOL                                                          */

#define A68_BOOL_DYAD(n, p, OP) void n (NODE_T * p) {\
  A68_BOOL *i, *j;\
  POP_OPERAND_ADDRESSES (p, i, j, A68_BOOL);\
  i->value = i->value OP j->value;\
}

A68_BOOL_DYAD (genie_and_bool, p, &)A68_BOOL_DYAD (genie_or_bool, p, |)A68_BOOL_DYAD (genie_xor_bool, p, ^)A68_BOOL_DYAD (genie_eq_bool, p, ==) A68_BOOL_DYAD (genie_ne_bool, p, !=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
INT operations.                                                               */
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (INT) INT                                                              */
  A68_MONAD (genie_minus_int, p, A68_INT, -)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (INT) INT                                                            */
     void genie_abs_int (NODE_T * p)
{
  A68_INT *j;
  POP_OPERAND_ADDRESS (p, j, A68_INT);
  j->value = (j->value >= 0 ? j->value : -j->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SIGN = (INT) INT                                                           */

void
genie_sign_int (NODE_T * p)
{
  A68_INT *j;
  POP_OPERAND_ADDRESS (p, j, A68_INT);
  j->value = j->value == 0 ? 0 : (j->value > 0 ? 1 : -1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ODD = (INT) INT                                                            */

void
genie_odd_int (NODE_T * p)
{
  A68_INT j;
  POP_INT (p, &j);
  PUSH_BOOL (p, (j.value >= 0 ? j.value : -j.value) % 2 == 1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
test_int_addition: whether addition does not yield INT overflow.              */

void
test_int_addition (NODE_T * p, int i, int j)
{
  if ((i >= 0 && j >= 0) || (i < 0 && j < 0))
    {
      int ai = (i >= 0 ? i : -i), aj = (j >= 0 ? j : -j);
      if (ai > INT_MAX - aj)
	{
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (INT, INT) INT                                                         */

void
genie_add_int (NODE_T * p)
{
  A68_INT *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_INT);
  test_int_addition (p, i->value, j->value);
  i->value += j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (INT, INT) INT                                                         */

void
genie_sub_int (NODE_T * p)
{
  A68_INT *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_INT);
  test_int_addition (p, i->value, -j->value);
  i->value -= j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (INT, INT) INT                                                         */

void
genie_mul_int (NODE_T * p)
{
  A68_INT *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_INT);
  TEST_TIMES_OVERFLOW_INT (p, i->value, j->value);
  i->value *= j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP OVER = (INT, INT) INT                                                      */

void
genie_over_int (NODE_T * p)
{
  A68_INT *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_INT);
  if (j->value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  i->value /= j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP MOD = (INT, INT) INT                                                       */

void
genie_mod_int (NODE_T * p)
{
  A68_INT *i, *j;
  int k;
  POP_OPERAND_ADDRESSES (p, i, j, A68_INT);
  if (j->value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  k = i->value % j->value;
  if (k < 0)
    {
      k += (j->value >= 0 ? j->value : -j->value);
    }
  i->value = k;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP / = (INT, INT) REAL                                                        */

void
genie_div_int (NODE_T * p)
{
  A68_INT i, j;
  POP_INT (p, &j);
  POP_INT (p, &i);
  if (j.value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_REAL (p, (double) (i.value) / (double) (j.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (INT, INT) INT                                                        */

void
genie_pow_int (NODE_T * p)
{
  A68_INT i, j;
  int expo, mult, prod;
  POP_INT (p, &j);
  if (j.value < 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, "invalid M exponent", MODE (INT), j.value);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  POP_INT (p, &i);
  prod = 1;
  mult = i.value;
  expo = 1;
  while ((unsigned) expo <= (unsigned) (j.value))
    {
      if (j.value & expo)
	{
	  TEST_TIMES_OVERFLOW_INT (p, prod, mult);
	  prod *= mult;
	}
      expo <<= 1;
      if (expo <= j.value)
	{
	  TEST_TIMES_OVERFLOW_INT (p, mult, mult);
	  mult *= mult;
	}
    }
  PUSH_INT (p, prod);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (INT, INT) BOOL                                                            */

#define A68_CMP_INT(n, p, OP) void n (NODE_T * p) {\
  A68_INT i, j;\
  POP_INT (p, &j);\
  POP_INT (p, &i);\
  PUSH_BOOL (p, i.value OP j.value);\
  }

A68_CMP_INT (genie_eq_int, p, ==) A68_CMP_INT (genie_ne_int, p, !=)A68_CMP_INT (genie_lt_int, p, <)A68_CMP_INT (genie_gt_int, p, >)A68_CMP_INT (genie_le_int, p, <=)A68_CMP_INT (genie_ge_int, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF INT, INT) REF INT                                               */
     void
     genie_plusab_int (NODE_T * p)
{
  A68_INT *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_INT);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_INT));
  address = (A68_INT *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (INT));
  test_int_addition (p, address->value, i->value);
  address->value += i->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF INT, INT) REF INT                                               */

void
genie_minusab_int (NODE_T * p)
{
  A68_INT *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_INT);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_INT));
  address = (A68_INT *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (INT));
  test_int_addition (p, address->value, -i->value);
  address->value -= i->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF INT, INT) REF INT                                               */

void
genie_timesab_int (NODE_T * p)
{
  A68_INT *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_INT);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_INT));
  address = (A68_INT *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (INT));
  TEST_TIMES_OVERFLOW_INT (p, address->value, i->value);
  address->value *= i->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP %:= = (REF INT, INT) REF INT                                               */

void
genie_overab_int (NODE_T * p)
{
  A68_INT *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_INT);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_INT));
  address = (A68_INT *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (INT));
  if (i->value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  address->value /= i->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP %*:= = (REF INT, INT) REF INT                                              */

void
genie_modab_int (NODE_T * p)
{
  A68_INT *i, *address;
  A68_REF *z;
  int k;
  POP_ADDRESS (p, i, A68_INT);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_INT));
  address = (A68_INT *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (INT));
  if (i->value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  k = address->value % i->value;
  if (k < 0)
    {
      k += (i->value >= 0 ? i->value : -i->value);
    }
  address->value = k;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (INT) LONG INT                                                      */

void
genie_lengthen_int_to_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_INT));
  mp_digit *z;
  A68_INT k;
  POP_INT (p, &k);
  z = stack_mp (p, digits);
  int_to_mp (p, z, k.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (BITS) LONG BITS                                                    */

void
genie_lengthen_unsigned_to_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_INT));
  mp_digit *z;
  A68_BITS k;
  POP_BITS (p, &k);
  z = stack_mp (p, digits);
  unsigned_to_mp (p, z, (unsigned) k.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG INT) INT                                                   */

void
genie_shorten_long_mp_to_int (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  DECREMENT_STACK_POINTER (p, size);
  MP_STATUS (z) = INITIALISED_MASK;
  PUSH_INT (p, mp_to_int (p, z, digits));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ODD = (LONG INT) BOOL                                                      */

void
genie_odd_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  DECREMENT_STACK_POINTER (p, size);
  if (MP_EXPONENT (z) <= digits - 1)
    {
      PUSH_BOOL (p, (int) (z[(int) (2 + MP_EXPONENT (z))]) % 2 != 0);
    }
  else
    {
      PUSH_BOOL (p, A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
test_long_int_range: whether z is a LONG INT                                  */

void
test_long_int_range (NODE_T * p, mp_digit * z, MOID_T * m)
{
  if (!check_mp_int (z, m))
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, m);
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (LONG INT, LONG INT) LONG INT                                          */

void
genie_add_long_int (NODE_T * p)
{
  MOID_T *m = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  add_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (LONG INT, LONG INT) LONG INT                                          */

void
genie_minus_long_int (NODE_T * p)
{
  MOID_T *m = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  sub_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (LONG INT, LONG INT) LONG INT                                          */

void
genie_mul_long_int (NODE_T * p)
{
  MOID_T *m = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  mul_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (LONG MODE, INT) LONG INT                                             */

void
genie_pow_long_mp_int_int (NODE_T * p)
{
  MOID_T *m = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  A68_INT k;
  mp_digit *x;
  POP_INT (p, &k);
  x = (mp_digit *) STACK_OFFSET (-size);
  pow_mp_int (p, x, x, k.value, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF LONG INT, LONG INT) REF LONG INT                                */

void
genie_plusab_long_int (NODE_T * p)
{
  MOID_T *m = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  add_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF LONG INT, LONG INT) REF LONG INT                                */

void
genie_minusab_long_int (NODE_T * p)
{
  MOID_T *m = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  sub_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF LONG INT, LONG INT) REF LONG INT                                */

void
genie_timesab_long_int (NODE_T * p)
{
  MOID_T *m = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (m), size = get_mp_size (m);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  mul_mp (p, x, x, y, digits);
  test_long_int_range (p, x, m);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
REAL operations. REAL math is in gsl.c                                        */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (REAL) REAL                                                            */

A68_MONAD (genie_minus_real, p, A68_REAL, -)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (REAL) REAL                                                          */
     void genie_abs_real (NODE_T * p)
{
  A68_REAL *x;
  POP_OPERAND_ADDRESS (p, x, A68_REAL);
  x->value = (x->value >= 0 ? x->value : -x->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP TRUNC = (REAL) REAL                                                        */

A68_MONAD (genie_nint_real, p, A68_REAL, (int))
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ROUND = (REAL) REAL                                                        */
     void genie_round_real (NODE_T * p)
{
  A68_REAL x;
  int j;
  POP_REAL (p, &x);
  if (x.value > 0)
    {
      j = (int) (x.value + 0.5);
    }
  else
    {
      j = (int) (x.value - 0.5);
    }
  PUSH_INT (p, j);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ENTIER = (REAL) REAL                                                       */

void
genie_entier_real (NODE_T * p)
{
  A68_REAL x;
  POP_REAL (p, &x);
  if (x.value < -MAX_INT || x.value > MAX_INT)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_INT (p, (int) floor (x.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SIGN = (REAL) INT                                                          */

void
genie_sign_real (NODE_T * p)
{
  A68_REAL x;
  POP_REAL (p, &x);
  PUSH_INT (p, x.value > 0 ? 1 : x.value < 0 ? -1 : 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (REAL, REAL) REAL                                                      */

void
genie_add_real (NODE_T * p)
{
  A68_REAL *x, *y;
  POP_OPERAND_ADDRESSES (p, x, y, A68_REAL);
  x->value += y->value;
  TEST_REAL_REPRESENTATION (p, x->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (REAL, REAL) REAL                                                      */

void
genie_sub_real (NODE_T * p)
{
  A68_REAL *x, *y;
  POP_OPERAND_ADDRESSES (p, x, y, A68_REAL);
  x->value -= y->value;
  TEST_REAL_REPRESENTATION (p, x->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (REAL, REAL) REAL                                                      */

void
genie_mul_real (NODE_T * p)
{
  A68_REAL *x, *y;
  POP_OPERAND_ADDRESSES (p, x, y, A68_REAL);
  TEST_TIMES_OVERFLOW_REAL (p, x->value, y->value);
  x->value *= y->value;
  TEST_REAL_REPRESENTATION (p, x->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP / = (REAL, REAL) REAL                                                      */

void
genie_div_real (NODE_T * p)
{
  A68_REAL *x, *y;
  POP_OPERAND_ADDRESSES (p, x, y, A68_REAL);
#ifndef HAVE_IEEE_754
  if (y->value == 0.0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (REAL));
      exit_genie (p, A_RUNTIME_ERROR);
    }
#endif
  x->value /= y->value;
  TEST_REAL_REPRESENTATION (p, x->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (REAL, INT) REAL                                                      */

void
genie_pow_real_int (NODE_T * p)
{
  A68_INT j;
  A68_REAL x;
  int expo;
  double mult, prod;
  BOOL_T negative;
  POP_INT (p, &j);
  negative = j.value < 0;
  j.value = (j.value >= 0 ? j.value : -j.value);
  POP_REAL (p, &x);
  prod = 1;
  mult = x.value;
  expo = 1;
  while ((unsigned) expo <= (unsigned) (j.value))
    {
      if (j.value & expo)
	{
	  TEST_TIMES_OVERFLOW_REAL (p, prod, mult);
	  prod *= mult;
	}
      expo <<= 1;
      if (expo <= j.value)
	{
	  TEST_TIMES_OVERFLOW_REAL (p, mult, mult);
	  mult *= mult;
	}
    }
  TEST_REAL_REPRESENTATION (p, prod);
  if (negative)
    {
      prod = 1.0 / prod;
    }
  PUSH_REAL (p, prod);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (REAL, REAL) REAL                                                     */

void
genie_pow_real (NODE_T * p)
{
  A68_REAL x, y;
  double z;
  int m = 0;
  POP_REAL (p, &y);
  if (y.value < 0)
    {
      y.value = -y.value;
      m = 1;
    }
  POP_REAL (p, &x);
  if (x.value <= 0.0)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (REAL), &x);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  errno = 0;
  z = exp (y.value * log (x.value));
  if (errno != 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, "arithmetic exception");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_REAL (p, z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (REAL, REAL) BOOL                                                          */

#define A68_CMP_REAL(n, p, OP) void n (NODE_T * p) {\
  A68_REAL i, j;\
  POP_REAL (p, &j);\
  POP_REAL (p, &i);\
  PUSH_BOOL (p, i.value OP j.value);\
  }

A68_CMP_REAL (genie_eq_real, p, ==) A68_CMP_REAL (genie_ne_real, p, !=)A68_CMP_REAL (genie_lt_real, p, <)A68_CMP_REAL (genie_gt_real, p, >)A68_CMP_REAL (genie_le_real, p, <=)A68_CMP_REAL (genie_ge_real, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF REAL, REAL) REF REAL                                            */
     void
     genie_plusab_real (NODE_T * p)
{
  A68_REAL *a, *address;
  A68_REF *z;
  POP_ADDRESS (p, a, A68_REAL);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_REAL));
  address = (A68_REAL *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (REAL));
  address->value += a->value;
  TEST_REAL_REPRESENTATION (p, address->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF REAL, REAL) REF REAL                                            */

void
genie_minusab_real (NODE_T * p)
{
  A68_REAL *a, *address;
  A68_REF *z;
  POP_ADDRESS (p, a, A68_REAL);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_REAL));
  address = (A68_REAL *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (REAL));
  address->value -= a->value;
  TEST_REAL_REPRESENTATION (p, address->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF REAL, REAL) REF REAL                                            */

void
genie_timesab_real (NODE_T * p)
{
  A68_REAL *a, *address;
  A68_REF *z;
  POP_ADDRESS (p, a, A68_REAL);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_REAL));
  address = (A68_REAL *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (REAL));
  TEST_TIMES_OVERFLOW_REAL (p, address->value, a->value);
  address->value *= a->value;
  TEST_REAL_REPRESENTATION (p, address->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /:= = (REF REAL, REAL) REF REAL                                            */

void
genie_overab_real (NODE_T * p)
{
  A68_REAL *a, *address;
  A68_REF *z;
  POP_ADDRESS (p, a, A68_REAL);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_REAL));
  address = (A68_REAL *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (REAL));
#ifndef HAVE_IEEE_754
  if (a->value == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (REAL));
      exit_genie (p, A_RUNTIME_ERROR);
    }
#endif
  address->value /= a->value;
  TEST_REAL_REPRESENTATION (p, address->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (REAL) LONG REAL                                                    */

void
genie_lengthen_real_to_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL));
  mp_digit *z;
  A68_REAL x;
  POP_REAL (p, &x);
  z = stack_mp (p, digits);
  real_to_mp (p, z, x.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG REAL) REAL                                                 */

void
genie_shorten_long_mp_to_real (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  DECREMENT_STACK_POINTER (p, size);
  PUSH_REAL (p, mp_to_real (p, z, digits));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ROUND = (LONG REAL) LONG INT                                               */

void
genie_round_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *y = stack_mp (p, digits);
  set_mp_short (y, (mp_digit) (MP_RADIX / 2), -1, digits);
  if (MP_DIGIT (z, 1) >= 0)
    {
      add_mp (p, z, z, y, digits);
      trunc_mp (z, z, digits);
    }
  else
    {
      sub_mp (p, z, z, y, digits);
      trunc_mp (z, z, digits);
    }
  MP_STATUS (z) = INITIALISED_MASK;
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ENTIER = (LONG REAL) LONG INT                                              */

void
genie_entier_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (PACK (MOID (p)))), size = get_mp_size (MOID (PACK (MOID (p))));
  ADDR_T old_sp = stack_pointer;
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  if (MP_DIGIT (z, 1) >= 0)
    {
      trunc_mp (z, z, digits);
    }
  else
    {
      mp_digit *y = stack_mp (p, digits);
      set_mp_short (y, (mp_digit) 1, 0, digits);
      trunc_mp (z, z, digits);
      sub_mp (p, z, z, y, digits);
    }
  MP_STATUS (z) = INITIALISED_MASK;
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long sqrt = (LONG REAL) LONG REAL                                        */

void
genie_sqrt_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (sqrt_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longsqrt");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long curt = (LONG REAL) LONG REAL # cube root #                          */

void
genie_curt_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (curt_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longcurt");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long exp = (LONG REAL) LONG REAL                                         */

void
genie_exp_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  ADDR_T old_sp = stack_pointer;
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  exp_mp (p, x, x, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long ln = (LONG REAL) LONG REAL                                          */

void
genie_ln_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  ADDR_T old_sp = stack_pointer;
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (ln_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longln");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long log = (LONG REAL) LONG REAL                                         */

void
genie_log_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  ADDR_T old_sp = stack_pointer;
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (log_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longlog");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long sin = (LONG REAL) LONG REAL                                         */

void
genie_sin_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  sin_mp (p, x, x, digits);
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long cos = (LONG REAL) LONG REAL                                         */

void
genie_cos_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  cos_mp (p, x, x, digits);
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long tan = (LONG REAL) LONG REAL                                         */

void
genie_tan_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (tan_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longtan");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long arcsin = (LONG REAL) LONG REAL                                      */

void
genie_asin_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (asin_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longarcsin");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long arccos = (LONG REAL) LONG REAL                                      */

void
genie_acos_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  if (acos_mp (p, x, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, "longarcsin");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long arctan = (LONG REAL) LONG REAL                                      */

void
genie_atan_long_mp (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p)), size = get_mp_size (MOID (p));
  mp_digit *x = (mp_digit *) STACK_OFFSET (-size);
  atan_mp (p, x, x, digits);
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Arithmetic operations.                                                        */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (LONG MODE) LONG LONG MODE                                          */

void
genie_lengthen_long_mp_to_longlong_mp (NODE_T * p)
{
  mp_digit *z;
  DECREMENT_STACK_POINTER (p, (int) size_long_mp ());
  z = stack_mp (p, longlong_mp_digits ());
  lengthen_mp (p, z, longlong_mp_digits (), z, long_mp_digits ());
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG LONG MODE) LONG MODE                                       */

void
genie_shorten_longlong_mp_to_long_mp (NODE_T * p)
{
  mp_digit *z;
  MOID_T *m = SUB (MOID (p));
  DECREMENT_STACK_POINTER (p, (int) size_longlong_mp ());
  z = stack_mp (p, long_mp_digits ());
  if (m == MODE (LONG_INT))
    {
      if (MP_EXPONENT (z) > LONG_MP_DIGITS - 1)
	{
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, m, NULL);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
  shorten_mp (p, z, long_mp_digits (), z, longlong_mp_digits ());
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (LONG MODE) LONG MODE                                                  */

void
genie_minus_long_mp (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (LONG MODE) LONG MODE                                                */

void
genie_abs_long_mp (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_DIGIT (z, 1) = fabs (MP_DIGIT (z, 1));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SIGN = (LONG MODE) INT                                                     */

void
genie_sign_long_mp (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  DECREMENT_STACK_POINTER (p, size);
  if (MP_DIGIT (z, 1) == 0)
    {
      PUSH_INT (p, 0);
    }
  else if (MP_DIGIT (z, 1) > 0)
    {
      PUSH_INT (p, 1);
    }
  else
    {
      PUSH_INT (p, -1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (LONG MODE, LONG MODE) LONG MODE                                       */

void
genie_add_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  add_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (LONG MODE, LONG MODE) LONG MODE                                       */

void
genie_sub_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  sub_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (LONG MODE, LONG MODE) LONG MODE                                       */

void
genie_mul_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  mul_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP / = (LONG MODE, LONG MODE) LONG MODE                                       */

void
genie_div_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  if (div_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_REAL));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP % = (LONG MODE, LONG MODE) LONG MODE                                       */

void
genie_over_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  if (over_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP %* = (LONG MODE, LONG MODE) LONG MODE                                      */

void
genie_mod_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  if (mod_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (MP_DIGIT (x, 1) < 0)
    {
      MP_DIGIT (y, 1) = fabs (MP_DIGIT (y, 1));
      add_mp (p, x, x, y, digits);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF LONG MODE, LONG MODE) REF LONG MODE                             */

void
genie_plusab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  add_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF LONG MODE, LONG MODE) REF LONG MODE                             */

void
genie_minusab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  sub_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF LONG MODE, LONG MODE) REF LONG MODE                             */

void
genie_timesab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  mul_mp (p, x, x, y, digits);
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /:= = (REF LONG MODE, LONG MODE) REF LONG MODE                             */

void
genie_divab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  if (div_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MOID (NEXT (p)));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP %:= = (REF LONG MODE, LONG MODE) REF LONG MODE                             */

void
genie_overab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  if (over_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MOID (NEXT (p)));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP %*:= = (REF LONG MODE, LONG MODE) REF LONG MODE                            */

void
genie_modab_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *x, *y;
  A68_REF *z;
  y = (mp_digit *) (STACK_OFFSET (-size));
  z = (A68_REF *) (STACK_OFFSET (-size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  x = (mp_digit *) ADDRESS (z);
  TEST_MP_INIT (p, x, MOID (NEXT (p)));
  if (mod_mp (p, x, x, y, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MOID (NEXT (p)));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (MP_DIGIT (x, 1) < 0)
    {
      MP_DIGIT (y, 1) = fabs (MP_DIGIT (y, 1));
      add_mp (p, x, x, y, digits);
    }
  MP_STATUS (x) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (LONG MODE, LONG MODE) BOOL                                                */

#define A68_CMP_LONG(n, p, OP) void n (NODE_T * p) {\
  MOID_T *mode = MOID (PACK (MOID (p)));\
  int digits = get_mp_digits (mode), size = get_mp_size (mode);\
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);\
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);\
  sub_mp (p, x, x, y, digits);\
  DECREMENT_STACK_POINTER (p, 2 * size);\
  PUSH_BOOL (p, MP_DIGIT (x, 1) OP 0);\
}

A68_CMP_LONG (genie_eq_long_mp, p, ==) A68_CMP_LONG (genie_ne_long_mp, p, !=)A68_CMP_LONG (genie_lt_long_mp, p, <)A68_CMP_LONG (genie_gt_long_mp, p, >)A68_CMP_LONG (genie_le_long_mp, p, <=)A68_CMP_LONG (genie_ge_long_mp, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (LONG MODE, INT) LONG MODE                                            */
     void
     genie_pow_long_mp_int (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  A68_INT k;
  mp_digit *x;
  POP_INT (p, &k);
  x = (mp_digit *) STACK_OFFSET (-size);
  pow_mp_int (p, x, x, k.value, digits);
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (LONG MODE, LONG MODE) LONG MODE                                      */

void
genie_pow_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *x = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *y = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *z = stack_mp (p, digits);
  if (ln_mp (p, z, x, digits) == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MOID (p), x, SYMBOL (p));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  mul_mp (p, z, y, z, digits);
  exp_mp (p, x, z, digits);
  stack_pointer = old_sp - size;
  MP_STATUS (x) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
COMPLEX operations.                                                           */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +* = (REAL, REAL) COMPLEX                                                  */

void
genie_icomplex (NODE_T * p)
{
  (void) p;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +* = (INT, INT) COMPLEX                                                    */

void
genie_iint_complex (NODE_T * p)
{
  A68_INT jre, jim;
  POP_INT (p, &jim);
  POP_INT (p, &jre);
  PUSH_REAL (p, (double) jre.value);
  PUSH_REAL (p, (double) jim.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP RE = (COMPLEX) REAL                                                        */

void
genie_re_complex (NODE_T * p)
{
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_REAL));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP IM = (COMPLEX) REAL                                                        */

void
genie_im_complex (NODE_T * p)
{
  A68_REAL im;
  POP_REAL (p, &im);
  *(A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL))) = im;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (COMPLEX) COMPLEX                                                      */

void
genie_minus_complex (NODE_T * p)
{
  A68_REAL *rex, *imx;
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value = -imx->value;
  rex->value = -rex->value;
  (void) p;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (REF COMPLEX) REAL                                                   */

void
genie_abs_complex (NODE_T * p)
{
  A68_REAL rex, imx;
  double z;
  POP_COMPLEX (p, &rex, &imx);
  rex.value = fabs (rex.value);
  imx.value = fabs (imx.value);
  if (rex.value == 0.0)
    {
      z = imx.value;
    }
  else if (imx.value == 0.0)
    {
      z = rex.value;
    }
  else if (rex.value > imx.value)
    {
      double t = imx.value / rex.value;
      z = rex.value * sqrt (1.0 + t * t);
    }
  else
    {
      double t = rex.value / imx.value;
      z = imx.value * sqrt (1.0 + t * t);
    }
  PUSH_REAL (p, z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ARG = (REF COMPLEX) REAL                                                   */

void
genie_arg_complex (NODE_T * p)
{
  A68_REAL rex, imx;
  POP_COMPLEX (p, &rex, &imx);
  if (rex.value != 0.0 || imx.value != 0.0)
    {
      PUSH_REAL (p, atan2 (imx.value, rex.value));
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (COMPLEX), NULL);
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP CONJ = (COMPLEX) COMPLEX                                                   */

void
genie_conj_complex (NODE_T * p)
{
  A68_REAL *im;
  POP_OPERAND_ADDRESS (p, im, A68_REAL);
  im->value = -im->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (COMPLEX, COMPLEX) COMPLEX                                             */

void
genie_add_complex (NODE_T * p)
{
  A68_REAL *rex, *imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value += imy.value;
  rex->value += rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (COMPLEX, COMPLEX) COMPLEX                                             */

void
genie_sub_complex (NODE_T * p)
{
  A68_REAL *rex, *imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value -= imy.value;
  rex->value -= rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (COMPLEX, COMPLEX) COMPLEX                                             */

void
genie_mul_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  double re, im;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
  re = rex.value * rey.value - imx.value * imy.value;
  im = imx.value * rey.value + rex.value * imy.value;
  TEST_COMPLEX_REPRESENTATION (p, re, im);
  PUSH_COMPLEX (p, re, im);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP / = (COMPLEX, COMPLEX) COMPLEX                                             */

void
genie_div_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  double re = 0, im = 0;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
#ifndef HAVE_IEEE_754
  if (rey.value == 0.0 && imy.value == 0.0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (COMPLEX));
      exit_genie (p, A_RUNTIME_ERROR);
    }
#endif
  if (fabs (rey.value) >= fabs (imy.value))
    {
      double r = imy.value / rey.value, den = rey.value + r * imy.value;
      re = (rex.value + r * imx.value) / den;
      im = (imx.value - r * rex.value) / den;
    }
  else
    {
      double r = rey.value / imy.value, den = imy.value + r * rey.value;
      re = (rex.value * r + imx.value) / den;
      im = (imx.value * r - rex.value) / den;
    }
  TEST_COMPLEX_REPRESENTATION (p, re, im);
  PUSH_COMPLEX (p, re, im);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (COMPLEX, INT) COMPLEX                                                */

void
genie_pow_complex_int (NODE_T * p)
{
  A68_REAL rex, imx;
  double rey, imy, rez, imz, rea;
  A68_INT j;
  int expo;
  BOOL_T negative;
  POP_INT (p, &j);
  POP_COMPLEX (p, &rex, &imx);
  rez = 1;
  imz = 0;
  rey = rex.value;
  imy = imx.value;
  expo = 1;
  negative = (j.value < 0);
  if (negative)
    {
      j.value = -j.value;
    }
  while ((unsigned) expo <= (unsigned) (j.value))
    {
      if (expo & j.value)
	{
	  rea = rez * rey - imz * imy;
	  imz = rez * imy + imz * rey;
	  rez = rea;
	}
      rea = rey * rey - imy * imy;
      imy = imy * rey + rey * imy;
      rey = rea;
      expo <<= 1;
    }
  TEST_COMPLEX_REPRESENTATION (p, rez, imz);
  if (negative)
    {
      PUSH_REAL (p, 1);
      PUSH_REAL (p, 0);
      PUSH_REAL (p, rez);
      PUSH_REAL (p, imz);
      genie_div_complex (p);
    }
  else
    {
      PUSH_REAL (p, rez);
      PUSH_REAL (p, imz);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP = = (COMPLEX, COMPLEX) BOOL                                                */

void
genie_eq_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
  PUSH_BOOL (p, (rex.value == rey.value) && (imx.value == imy.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /= = (COMPLEX, COMPLEX) BOOL                                               */

void
genie_ne_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
  PUSH_BOOL (p, !((rex.value == rey.value) && (imx.value == imy.value)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF COMPLEX, COMPLEX) REF COMPLEX                                   */

void
genie_plusab_complex (NODE_T * p)
{
  A68_REF *z;
  A68_REAL rey, imy, *rex, *imx, *address;
  POP_COMPLEX (p, &rey, &imy);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_COMPLEX));
  address = (A68_REAL *) ADDRESS (z);
  imx = &address[1];
  TEST_INIT (p, *imx, MODE (COMPLEX));
  rex = &address[0];
  TEST_INIT (p, *rex, MODE (COMPLEX));
  imx->value += imy.value;
  rex->value += rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF COMPLEX, COMPLEX) REF COMPLEX                                   */

void
genie_minusab_complex (NODE_T * p)
{
  A68_REF *z;
  A68_REAL rey, imy, *rex, *imx, *address;
  POP_COMPLEX (p, &rey, &imy);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_COMPLEX));
  address = (A68_REAL *) ADDRESS (z);
  imx = &address[1];
  TEST_INIT (p, *imx, MODE (COMPLEX));
  rex = &address[0];
  TEST_INIT (p, *rex, MODE (COMPLEX));
  imx->value -= imy.value;
  rex->value -= rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF COMPLEX, COMPLEX) REF COMPLEX                                   */

void
genie_timesab_complex (NODE_T * p)
{
  A68_REF *z;
  A68_REAL rey, imy, *rex, *imx, *address;
  double rez, imz;
  POP_COMPLEX (p, &rey, &imy);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_COMPLEX));
  address = (A68_REAL *) ADDRESS (z);
  imx = &address[1];
  TEST_INIT (p, *imx, MODE (COMPLEX));
  rex = &address[0];
  TEST_INIT (p, *rex, MODE (COMPLEX));
  rez = rex->value * rey.value - imx->value * imy.value;
  imz = imx->value * rey.value + rex->value * imy.value;
  TEST_COMPLEX_REPRESENTATION (p, rez, imz);
  imx->value = imz;
  rex->value = rez;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /:= = (REF COMPLEX, COMPLEX) REF COMPLEX                                   */

void
genie_divab_complex (NODE_T * p)
{
  A68_REF *z;
  A68_REAL rey, imy, *rex, *imx, *address;
  double rez, imz;
  POP_COMPLEX (p, &rey, &imy);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_COMPLEX));
  address = (A68_REAL *) ADDRESS (z);
  imx = &address[1];
  TEST_INIT (p, *imx, MODE (COMPLEX));
  rex = &address[0];
  TEST_INIT (p, *rex, MODE (COMPLEX));
#ifndef HAVE_IEEE_754
  if (rey.value == 0.0 && imy.value == 0.0)
    {
      diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (COMPLEX));
      exit_genie (p, A_RUNTIME_ERROR);
    }
#endif
  if (fabs (rey.value) >= fabs (imy.value))
    {
      double r = imy.value / rey.value, den = rey.value + r * imy.value;
      rez = (rex->value + r * imx->value) / den;
      imz = (imx->value - r * rex->value) / den;
    }
  else
    {
      double r = rey.value / imy.value, den = imy.value + r * rey.value;
      rez = (rex->value * r + imx->value) / den;
      imz = (imx->value * r - rex->value) / den;
    }
  TEST_COMPLEX_REPRESENTATION (p, rez, imz);
  imx->value = imz;
  rex->value = rez;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (COMPLEX) LONG COMPLEX                                              */

void
genie_lengthen_complex_to_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL));
  mp_digit *z;
  A68_REAL a, b;
  POP_REAL (p, &b);
  POP_REAL (p, &a);
  z = stack_mp (p, digits);
  real_to_mp (p, z, a.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  z = stack_mp (p, digits);
  real_to_mp (p, z, b.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG COMPLEX) COMPLEX                                           */

void
genie_shorten_long_complex_to_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_REAL (p, mp_to_real (p, a, digits));
  PUSH_REAL (p, mp_to_real (p, b, digits));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (LONG COMPLEX) LONG LONG COMPLEX                                    */

void
genie_lengthen_long_complex_to_longlong_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  int digits_long = get_mp_digits (MODE (LONGLONG_REAL)), size_long = get_mp_size (MODE (LONGLONG_REAL));
  ADDR_T old_sp = stack_pointer;
  mp_digit *a, *b, *c, *d;
  b = (mp_digit *) STACK_OFFSET (-size);
  a = (mp_digit *) STACK_OFFSET (-2 * size);
  c = (mp_digit *) stack_mp (p, digits_long);
  d = (mp_digit *) stack_mp (p, digits_long);
  lengthen_mp (p, c, digits_long, a, digits);
  lengthen_mp (p, d, digits_long, b, digits);
  MOVE_MP (a, c, digits_long);
  MOVE_MP (&a[2 + digits_long], d, digits_long);
  stack_pointer = old_sp;
  MP_STATUS (a) = INITIALISED_MASK;
  (&a[2 + digits_long])[0] = INITIALISED_MASK;
  INCREMENT_STACK_POINTER (p, 2 * (size_long - size));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG LONG COMPLEX) LONG COMPLEX                                 */

void
genie_shorten_longlong_complex_to_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  int digits_long = get_mp_digits (MODE (LONGLONG_REAL)), size_long = get_mp_size (MODE (LONGLONG_REAL));
  ADDR_T old_sp = stack_pointer;
  mp_digit *a, *b;
  b = (mp_digit *) STACK_OFFSET (-size_long);
  a = (mp_digit *) STACK_OFFSET (-2 * size_long);
  shorten_mp (p, a, digits, a, digits_long);
  shorten_mp (p, &a[2 + digits], digits, b, digits_long);
  stack_pointer = old_sp;
  MP_STATUS (a) = INITIALISED_MASK;
  (&a[2 + digits])[0] = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, 2 * (size_long - size));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP RE = (LONG COMPLEX) LONG REAL                                              */

void
genie_re_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  MP_STATUS (a) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, (int) size_long_mp ());
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP IM = (LONG COMPLEX) LONG REAL                                              */

void
genie_im_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MOID (PACK (MOID (p)))), size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  MOVE_MP (a, b, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, (int) size_long_mp ());
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (LONG COMPLEX) LONG COMPLEX                                            */

void
genie_minus_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  MP_DIGIT (a, 1) = -MP_DIGIT (a, 1);
  MP_DIGIT (b, 1) = -MP_DIGIT (b, 1);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP CONJ = (LONG COMPLEX) LONG COMPLEX                                         */

void
genie_conj_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  MP_DIGIT (b, 1) = -MP_DIGIT (b, 1);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (LONG COMPLEX) LONG COMPLEX                                          */

void
genie_abs_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *t = stack_mp (p, digits);
  mp_digit *z = stack_mp (p, digits);
/* Prevent overflow of intermediate results */
  MP_DIGIT (a, 1) = fabs (MP_DIGIT (a, 1));
  MP_DIGIT (b, 1) = fabs (MP_DIGIT (b, 1));
  if (MP_DIGIT (a, 1) == 0)
    {
      MOVE_MP (z, b, digits);
    }
  else if (MP_DIGIT (b, 1) == 0)
    {
      MOVE_MP (z, a, digits);
    }
  else
    {
      set_mp_short (t, (mp_digit) 1, 0, digits);
      sub_mp (p, z, a, b, digits);
      if (MP_DIGIT (z, 1) > 0)
	{
	  div_mp (p, z, b, a, digits);
	  mul_mp (p, z, z, z, digits);
	  add_mp (p, z, t, z, digits);
	  sqrt_mp (p, z, z, digits);
	  mul_mp (p, z, a, z, digits);
	}
      else
	{
	  div_mp (p, z, a, b, digits);
	  mul_mp (p, z, z, z, digits);
	  add_mp (p, z, t, z, digits);
	  sqrt_mp (p, z, z, digits);
	  mul_mp (p, z, b, z, digits);
	}
    }
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, size);
  MOVE_MP (a, z, digits);
  MP_STATUS (a) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ARG = (LONG COMPLEX) LONG REAL

This is an implementation of FORTRAN ATAN2, actually.                         */

void
genie_arg_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *b = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *t = stack_mp (p, digits);
  mp_digit *z = stack_mp (p, digits);
  if (MP_DIGIT (a, 1) == 0 && MP_DIGIT (b, 1) == 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (LONG_COMPLEX), NULL);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else
    {
      BOOL_T flip = MP_DIGIT (b, 1) < 0;
      MP_DIGIT (b, 1) = fabs (MP_DIGIT (b, 1));
      if (MP_DIGIT (a, 1) == 0)
	{
	  mp_pi (p, z, MP_HALF_PI, digits);
	}
      else
	{
	  BOOL_T flop = MP_DIGIT (a, 1) <= 0;
	  MP_DIGIT (a, 1) = fabs (MP_DIGIT (a, 1));
	  div_mp (p, z, b, a, digits);
	  atan_mp (p, z, z, digits);
	  if (flop)
	    {
	      mp_pi (p, t, MP_PI, digits);
	      sub_mp (p, z, t, z, digits);
	    }
	}
      if (flip)
	{
	  MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
	}
    }
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, size);
  MOVE_MP (a, z, digits);
  MP_STATUS (a) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX                              */

void
genie_add_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *d = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *c = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
  add_mp (p, b, b, d, digits);
  add_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP - = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX                              */

void
genie_sub_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *d = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *c = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
  sub_mp (p, b, b, d, digits);
  sub_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX                              */

void
genie_mul_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *d = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *c = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
  mp_digit *ac = stack_mp (p, digits);
  mp_digit *bd = stack_mp (p, digits);
  mp_digit *ad = stack_mp (p, digits);
  mp_digit *bc = stack_mp (p, digits);
  mul_mp (p, ac, a, c, digits);
  mul_mp (p, bd, b, d, digits);
  mul_mp (p, ad, a, d, digits);
  mul_mp (p, bc, b, c, digits);
/* Possible cancellation here */
  sub_mp (p, a, ac, bd, digits);
  add_mp (p, b, ad, bc, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP / = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX                              */

void
genie_div_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *d = (mp_digit *) STACK_OFFSET (-size);
  mp_digit *c = (mp_digit *) STACK_OFFSET (-2 * size);
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
/* Prevent overflow of intermediate results */
  mp_digit *q = stack_mp (p, digits);
  mp_digit *r = stack_mp (p, digits);
  MOVE_MP (q, c, digits);
  MOVE_MP (r, d, digits);
  MP_DIGIT (q, 1) = fabs (MP_DIGIT (q, 1));
  MP_DIGIT (r, 1) = fabs (MP_DIGIT (r, 1));
  sub_mp (p, q, q, r, digits);
  if (MP_DIGIT (q, 1) >= 0)
    {
      if (div_mp (p, q, d, c, digits) == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_COMPLEX));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      mul_mp (p, r, d, q, digits);
      add_mp (p, r, r, c, digits);
      mul_mp (p, c, b, q, digits);
      add_mp (p, c, c, a, digits);
      div_mp (p, c, c, r, digits);
      mul_mp (p, d, a, q, digits);
      sub_mp (p, d, b, d, digits);
      div_mp (p, d, d, r, digits);
    }
  else
    {
      if (div_mp (p, q, c, d, digits) == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_COMPLEX));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      mul_mp (p, r, c, q, digits);
      add_mp (p, r, r, d, digits);
      mul_mp (p, c, a, q, digits);
      add_mp (p, c, c, b, digits);
      div_mp (p, c, c, r, digits);
      mul_mp (p, d, b, q, digits);
      sub_mp (p, d, d, a, digits);
      div_mp (p, d, d, r, digits);
    }
  MOVE_MP (a, c, digits);
  MOVE_MP (b, d, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ** = (LONG COMPLEX, INT) LONG COMPLEX                                      */

void
genie_pow_long_complex_int (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp;
  mp_digit *rex, *imx, *rey, *imy, *rez, *imz, *rea, *acc;
  A68_INT j;
  int expo;
  BOOL_T negative;
  POP_INT (p, &j);
  old_sp = stack_pointer;
  imx = (mp_digit *) STACK_OFFSET (-size);
  rex = (mp_digit *) STACK_OFFSET (-2 * size);
  rez = stack_mp (p, digits);
  set_mp_short (rez, (mp_digit) 1, 0, digits);
  imz = stack_mp (p, digits);
  SET_MP_ZERO (imz, digits);
  rey = stack_mp (p, digits);
  imy = stack_mp (p, digits);
  MOVE_MP (rey, rex, digits);
  MOVE_MP (imy, imx, digits);
  rea = stack_mp (p, digits);
  acc = stack_mp (p, digits);
  expo = 1;
  negative = (j.value < 0);
  if (negative)
    {
      j.value = -j.value;
    }
  while ((unsigned) expo <= (unsigned) (j.value))
    {
      if (expo & j.value)
	{
	  mul_mp (p, acc, imz, imy, digits);
	  mul_mp (p, rea, rez, rey, digits);
	  sub_mp (p, rea, rea, acc, digits);
	  mul_mp (p, acc, imz, rey, digits);
	  mul_mp (p, imz, rez, imy, digits);
	  add_mp (p, imz, imz, acc, digits);
	  MOVE_MP (rez, rea, digits);
	}
      mul_mp (p, acc, imy, imy, digits);
      mul_mp (p, rea, rey, rey, digits);
      sub_mp (p, rea, rea, acc, digits);
      mul_mp (p, acc, imy, rey, digits);
      mul_mp (p, imy, rey, imy, digits);
      add_mp (p, imy, imy, acc, digits);
      MOVE_MP (rey, rea, digits);
      expo <<= 1;
    }
  stack_pointer = old_sp;
  if (negative)
    {
      set_mp_short (rex, (mp_digit) 1, 0, digits);
      SET_MP_ZERO (imx, digits);
      INCREMENT_STACK_POINTER (p, 2 * size);
      genie_div_long_complex (p);
    }
  else
    {
      MOVE_MP (rex, rez, digits);
      MOVE_MP (imx, imz, digits);
    }
  MP_STATUS (rex) = INITIALISED_MASK;
  MP_STATUS (imx) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP = = (LONG COMPLEX, LONG COMPLEX) BOOL                                      */

void
genie_eq_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
  genie_sub_long_complex (p);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_BOOL (p, MP_DIGIT (a, 1) == 0 && MP_DIGIT (b, 1) == 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /= = (LONG COMPLEX, LONG COMPLEX) BOOL                                     */

void
genie_ne_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  mp_digit *b = (mp_digit *) STACK_OFFSET (-3 * size);
  mp_digit *a = (mp_digit *) STACK_OFFSET (-4 * size);
  genie_sub_long_complex (p);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_BOOL (p, MP_DIGIT (a, 1) != 0 || MP_DIGIT (b, 1) != 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX                    */

void
genie_plusab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  A68_REF *z;
  mp_digit *a, *b, *c, *d;
  d = (mp_digit *) (STACK_OFFSET (-size));
  c = (mp_digit *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (mp_digit *) ADDRESS (z);
  b = (mp_digit *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  add_mp (p, b, b, d, digits);
  add_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP -:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX                    */

void
genie_minusab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  A68_REF *z;
  mp_digit *a, *b, *c, *d;
  d = (mp_digit *) (STACK_OFFSET (-size));
  c = (mp_digit *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (mp_digit *) ADDRESS (z);
  b = (mp_digit *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  sub_mp (p, b, b, d, digits);
  sub_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX                    */

void
genie_timesab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  A68_REF *z;
  mp_digit *a, *b, *c, *d, *ac, *bd, *ad, *bc;
  d = (mp_digit *) (STACK_OFFSET (-size));
  c = (mp_digit *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (mp_digit *) ADDRESS (z);
  b = (mp_digit *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  ac = stack_mp (p, digits);
  bd = stack_mp (p, digits);
  ad = stack_mp (p, digits);
  bc = stack_mp (p, digits);
  mul_mp (p, ac, a, c, digits);
  mul_mp (p, bd, b, d, digits);
  mul_mp (p, ad, a, d, digits);
  mul_mp (p, bc, b, c, digits);
/* Possible cancellation here */
  sub_mp (p, a, ac, bd, digits);
  add_mp (p, b, ad, bc, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP /:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX                    */

void
genie_divab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  A68_REF *z;
  mp_digit *a, *b, *c, *d, *q, *r;
  d = (mp_digit *) (STACK_OFFSET (-size));
  c = (mp_digit *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (mp_digit *) ADDRESS (z);
  b = (mp_digit *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
/* Prevent overflow of intermediate results */
  q = stack_mp (p, digits);
  r = stack_mp (p, digits);
  MOVE_MP (q, c, digits);
  MOVE_MP (r, d, digits);
  MP_DIGIT (q, 1) = fabs (MP_DIGIT (q, 1));
  MP_DIGIT (r, 1) = fabs (MP_DIGIT (r, 1));
  sub_mp (p, q, q, r, digits);
  if (MP_DIGIT (q, 1) >= 0)
    {
      if (div_mp (p, q, d, c, digits) == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_COMPLEX));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      mul_mp (p, r, d, q, digits);
      add_mp (p, r, r, c, digits);
      mul_mp (p, c, b, q, digits);
      add_mp (p, c, c, a, digits);
      div_mp (p, c, c, r, digits);
      mul_mp (p, d, a, q, digits);
      sub_mp (p, d, b, d, digits);
      div_mp (p, d, d, r, digits);
    }
  else
    {
      if (div_mp (p, q, c, d, digits) == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (LONG_COMPLEX));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      mul_mp (p, r, c, q, digits);
      add_mp (p, r, r, d, digits);
      mul_mp (p, c, a, q, digits);
      add_mp (p, c, c, b, digits);
      div_mp (p, c, c, r, digits);
      mul_mp (p, d, b, q, digits);
      sub_mp (p, d, d, a, digits);
      div_mp (p, d, d, r, digits);
    }
  MOVE_MP (a, c, digits);
  MOVE_MP (b, d, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Character operations.                                                         */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (CHAR, CHAR) BOOL                                                          */

#define A68_CMP_CHAR(n, p, OP) void n (NODE_T * p) {\
  A68_CHAR i, j;\
  POP_CHAR (p, &j);\
  POP_CHAR (p, &i);\
  PUSH_BOOL (p, i.value OP j.value);\
  }

A68_CMP_CHAR (genie_eq_char, p, ==) A68_CMP_CHAR (genie_ne_char, p, !=)A68_CMP_CHAR (genie_lt_char, p, <)A68_CMP_CHAR (genie_gt_char, p, >)A68_CMP_CHAR (genie_le_char, p, <=)A68_CMP_CHAR (genie_ge_char, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ABS = (CHAR) INT                                                           */
     void
     genie_abs_char (NODE_T * p)
{
  A68_CHAR i;
  POP_CHAR (p, &i);
  PUSH_INT (p, TO_UCHAR (i.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP REPR = (INT) CHAR                                                          */

void
genie_repr_char (NODE_T * p)
{
  A68_INT k;
  POP_INT (p, &k);
  if (k.value < 0 || k.value > UCHAR_MAX)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (CHAR));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_CHAR (p, (char) (k.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (CHAR, CHAR) STRING                                                    */

void
genie_add_char (NODE_T * p)
{
  A68_CHAR a, b;
  A68_REF c, d;
  A68_ARRAY *a_3;
  A68_TUPLE *t_3;
  BYTE_T *b_3;
/* right part */
  POP_CHAR (p, &b);
  TEST_INIT (p, b, MODE (CHAR));
/* left part */
  POP_CHAR (p, &a);
  TEST_INIT (p, a, MODE (CHAR));
/* sum */
  c = heap_generator (p, MODE (STRING), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  protect_sweep_handle (&c);
  d = heap_generator (p, MODE (STRING), 2 * SIZE_OF (A68_CHAR));
  protect_sweep_handle (&d);
  GET_DESCRIPTOR (a_3, t_3, &c);
  a_3->dimensions = 1;
  MOID (a_3) = MODE (CHAR);
  a_3->elem_size = SIZE_OF (A68_CHAR);
  a_3->slice_offset = 0;
  a_3->field_offset = 0;
  a_3->array = d;
  t_3->lower_bound = 1;
  t_3->upper_bound = 2;
  t_3->shift = t_3->lower_bound;
  t_3->span = 1;
/* add chars */
  b_3 = ADDRESS (&a_3->array);
  MOVE ((BYTE_T *) & b_3[0], (BYTE_T *) & a, SIZE_OF (A68_CHAR));
  MOVE ((BYTE_T *) & b_3[SIZE_OF (A68_CHAR)], (BYTE_T *) & b, SIZE_OF (A68_CHAR));
  PUSH (p, &c, SIZE_OF (A68_REF));
  unprotect_sweep_handle (&c);
  unprotect_sweep_handle (&d);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, STRING) CHAR # ALGOL68C #                                     */

void
genie_elem_string (NODE_T * p)
{
  A68_REF z;
  A68_ARRAY *a;
  A68_TUPLE *t;
  A68_INT k;
  BYTE_T *base;
  A68_CHAR *ch;
  POP_REF (p, &z);
  TEST_INIT (p, z, MODE (STRING));
  TEST_NIL (p, z, MODE (STRING));
  POP_INT (p, &k);
  GET_DESCRIPTOR (a, t, &z);
  if (k.value < t->lower_bound)
    {
      diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else if (k.value > t->upper_bound)
    {
      diagnostic (A_RUNTIME_ERROR, p, INDEX_OUT_OF_BOUNDS);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  base = ADDRESS (&(a->array));
  ch = (A68_CHAR *) & (base[INDEX_1_DIM (a, t, k.value)]);
  PUSH_CHAR (p, ch->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (STRING, STRING) STRING                                                */

void
genie_add_string (NODE_T * p)
{
  A68_REF a, b, c, d;
  A68_ARRAY *a_1, *a_2, *a_3;
  A68_TUPLE *t_1, *t_2, *t_3;
  int l_1, l_2, k, m;
  BYTE_T *b_1, *b_2, *b_3;
/* right part */
  POP_REF (p, &b);
  TEST_INIT (p, b, MODE (STRING));
  GET_DESCRIPTOR (a_2, t_2, &b);
  l_2 = ROW_SIZE (t_2);
/* left part */
  POP_REF (p, &a);
  TEST_INIT (p, a, MODE (STRING));
  GET_DESCRIPTOR (a_1, t_1, &a);
  l_1 = ROW_SIZE (t_1);
/* sum */
  c = heap_generator (p, MODE (STRING), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  protect_sweep_handle (&c);
  d = heap_generator (p, MODE (STRING), (l_1 + l_2) * SIZE_OF (A68_CHAR));
  protect_sweep_handle (&d);
/* Calculate again since heap sweeper might have moved data */
  GET_DESCRIPTOR (a_1, t_1, &a);
  GET_DESCRIPTOR (a_2, t_2, &b);
  GET_DESCRIPTOR (a_3, t_3, &c);
  a_3->dimensions = 1;
  MOID (a_3) = MODE (CHAR);
  a_3->elem_size = SIZE_OF (A68_CHAR);
  a_3->slice_offset = 0;
  a_3->field_offset = 0;
  a_3->array = d;
  t_3->lower_bound = 1;
  t_3->upper_bound = l_1 + l_2;
  t_3->shift = t_3->lower_bound;
  t_3->span = 1;
/* add strings */
  b_1 = ADDRESS (&a_1->array);
  b_2 = ADDRESS (&a_2->array);
  b_3 = ADDRESS (&a_3->array);
  m = 0;
  for (k = t_1->lower_bound; k <= t_1->upper_bound; k++)
    {
      MOVE ((BYTE_T *) & b_3[m], (BYTE_T *) & b_1[INDEX_1_DIM (a_1, t_1, k)], SIZE_OF (A68_CHAR));
      m += SIZE_OF (A68_CHAR);
    }
  for (k = t_2->lower_bound; k <= t_2->upper_bound; k++)
    {
      MOVE ((BYTE_T *) & b_3[m], (BYTE_T *) & b_2[INDEX_1_DIM (a_2, t_2, k)], SIZE_OF (A68_CHAR));
      m += SIZE_OF (A68_CHAR);
    }
  PUSH (p, &c, SIZE_OF (A68_REF));
  unprotect_sweep_handle (&c);
  unprotect_sweep_handle (&d);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF STRING, STRING) REF STRING                                      */

void
genie_plusab_string (NODE_T * p)
{
  A68_REF refa, a, b;
  POP_REF (p, &b);
  POP_REF (p, &refa);
  TEST_NIL (p, refa, MODE (REF_STRING));
  a = *(A68_REF *) ADDRESS (&refa);
  TEST_INIT (p, a, MODE (STRING));
  PUSH_REF (p, a);
  PUSH_REF (p, b);
  genie_add_string (p);
  POP (p, (A68_REF *) ADDRESS (&refa), SIZE_OF (A68_REF));
  PUSH_REF (p, refa);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +=: = (STRING, REF STRING) REF STRING                                      */

void
genie_plusto_string (NODE_T * p)
{
  A68_REF refa, a, b;
  POP_REF (p, &refa);
  TEST_NIL (p, refa, MODE (REF_STRING));
  a = *(A68_REF *) ADDRESS (&refa);
  TEST_INIT (p, a, MODE (STRING));
  POP_REF (p, &b);
  PUSH_REF (p, b);
  PUSH_REF (p, a);
  genie_add_string (p);
  POP (p, (A68_REF *) ADDRESS (&refa), SIZE_OF (A68_REF));
  PUSH_REF (p, refa);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP *:= = (REF STRING, INT) REF STRING                                         */

void
genie_timesab_string (NODE_T * p)
{
  A68_INT k;
  A68_REF refa, a;
  int i;
/* INT */
  POP_INT (p, &k);
  if (k.value < 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (INT), k);
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* REF STRING */
  POP_REF (p, &refa);
  TEST_NIL (p, refa, MODE (REF_STRING));
  a = *(A68_REF *) ADDRESS (&refa);
  TEST_INIT (p, a, MODE (STRING));
/* Multiplication as repeated addition */
  PUSH_REF (p, empty_string (p));
  for (i = 1; i <= k.value; i++)
    {
      PUSH_REF (p, a);
      genie_add_string (p);
    }
/* The stack contains a STRING, promote to REF STRING */
  POP_REF (p, (A68_REF *) ADDRESS (&refa));
  PUSH_REF (p, refa);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (INT, STRING) STRING                                                   */

void
genie_times_int_string (NODE_T * p)
{
  A68_INT k;
  A68_REF a;
  POP_REF (p, &a);
  POP_INT (p, &k);
  if (k.value < 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (INT), k);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_REF (p, empty_string (p));
  while (k.value--)
    {
      PUSH_REF (p, a);
      genie_add_string (p);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (STRING, INT) STRING                                                   */

void
genie_times_string_int (NODE_T * p)
{
  A68_INT k;
  A68_REF a;
  POP_INT (p, &k);
  POP_REF (p, &a);
  PUSH (p, &k, SIZE_OF (A68_INT));
  PUSH (p, &a, SIZE_OF (A68_REF));
  genie_times_int_string (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (INT, CHAR) STRING                                                     */

void
genie_times_int_char (NODE_T * p)
{
  A68_INT str_size;
  A68_CHAR a;
  A68_REF z, row;
  A68_ARRAY arr;
  A68_TUPLE tup;
  BYTE_T *base;
  int k;
/* Pop operands */
  POP_CHAR (p, &a);
  POP_INT (p, &str_size);
  if (str_size.value < 0)
    {
      diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (INT), str_size);
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Make new_one string */
  z = heap_generator (p, MODE (ROW_CHAR), SIZE_OF (A68_ARRAY) + SIZE_OF (A68_TUPLE));
  protect_sweep_handle (&z);
  row = heap_generator (p, MODE (ROW_CHAR), (int) (str_size.value) * SIZE_OF (A68_CHAR));
  protect_sweep_handle (&row);
  arr.dimensions = 1;
  arr.type = MODE (CHAR);
  arr.elem_size = SIZE_OF (A68_CHAR);
  arr.slice_offset = 0;
  arr.field_offset = 0;
  arr.array = row;
  tup.lower_bound = 1;
  tup.upper_bound = str_size.value;
  tup.shift = tup.lower_bound;
  tup.span = 1;
  PUT_DESCRIPTOR (arr, tup, &z);
/* Copy */
  base = ADDRESS (&row);
  for (k = 0; k < str_size.value; k++)
    {
      A68_CHAR ch;
      ch.status = INITIALISED_MASK;
      ch.value = a.value;
      *(A68_CHAR *) & base[k * SIZE_OF (A68_CHAR)] = ch;
    }
  PUSH_REF (p, z);
  unprotect_sweep_handle (&z);
  unprotect_sweep_handle (&row);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP * = (CHAR, INT) STRING                                                     */

void
genie_times_char_int (NODE_T * p)
{
  A68_INT k;
  A68_CHAR a;
  POP_INT (p, &k);
  POP_CHAR (p, &a);
  PUSH (p, &k, SIZE_OF (A68_INT));
  PUSH (p, &a, SIZE_OF (A68_CHAR));
  genie_times_int_char (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
string_difference: return difference between two STRINGs in the stack.        */

static int
string_difference (NODE_T * p)
{
  A68_REF row1, row2;
  A68_ARRAY *a_1, *a_2;
  A68_TUPLE *t_1, *t_2;
  BYTE_T *b_1, *b_2;
  int size, s_1, s_2, k, diff;
/* Pop operands */
  POP_REF (p, &row2);
  TEST_INIT (p, row2, MODE (STRING));
  GET_DESCRIPTOR (a_2, t_2, &row2);
  s_2 = ROW_SIZE (t_2);
  POP_REF (p, &row1);
  TEST_INIT (p, row1, MODE (STRING));
  GET_DESCRIPTOR (a_1, t_1, &row1);
  s_1 = ROW_SIZE (t_1);
/* Get difference */
  size = s_1 > s_2 ? s_1 : s_2;
  diff = 0;
  b_1 = ADDRESS (&a_1->array);
  b_2 = ADDRESS (&a_2->array);
  for (k = 0; k < size && diff == 0; k++)
    {
      int a, b;
      if (s_1 > 0 && k < s_1)
	{
	  A68_CHAR *ch = (A68_CHAR *) & b_1[INDEX_1_DIM (a_1, t_1, t_1->lower_bound + k)];
	  a = (int) ch->value;
	}
      else
	{
	  a = 0;
	}
      if (s_2 > 0 && k < s_2)
	{
	  A68_CHAR *ch = (A68_CHAR *) & b_2[INDEX_1_DIM (a_2, t_2, t_2->lower_bound + k)];
	  b = (int) ch->value;
	}
      else
	{
	  b = 0;
	}
      diff += (TO_UCHAR (a) - TO_UCHAR (b));
    }
  return (diff);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (STRING, STRING) BOOL                                                      */

#define A68_CMP_STRING(n, p, OP) void n (NODE_T * p) {\
  int k = string_difference (p);\
  PUSH_BOOL (p, k OP 0);\
}

A68_CMP_STRING (genie_eq_string, p, ==) A68_CMP_STRING (genie_ne_string, p, !=)A68_CMP_STRING (genie_lt_string, p, <)A68_CMP_STRING (genie_gt_string, p, >)A68_CMP_STRING (genie_le_string, p, <=)A68_CMP_STRING (genie_ge_string, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
char_in_string: look up char "c" in string "row".                             */
     static BOOL_T
     char_in_string (char c, int *pos, A68_REF row)
{
  if (row.status & INITIALISED_MASK)
    {
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      int size, k, n = 1;
      BOOL_T found = A_FALSE;
      GET_DESCRIPTOR (arr, tup, &row);
      size = ROW_SIZE (tup);
      if (size > 0)
	{
	  BYTE_T *base_address = ADDRESS (&(arr->array));
	  for (k = tup->lower_bound; k <= tup->upper_bound && !found; k++)
	    {
	      int addr = (tup->span * (k - tup->shift) + arr->slice_offset) * arr->elem_size + arr->field_offset;
	      A68_CHAR *ch = (A68_CHAR *) & (base_address[addr]);
	      found = (c == ch->value);
	      if (found)
		{
		  *pos = n;
		}
	      else
		{
		  n++;
		}
	    }
	}
      return (found);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC char in string = (CHAR, REF INT, STRING) BOOL                            */

void
genie_char_in_string (NODE_T * p)
{
  A68_CHAR c;
  A68_REF ref_pos, ref_str;
  int k;
  POP_REF (p, &ref_str);
  POP_REF (p, &ref_pos);
  POP_CHAR (p, &c);
  if (char_in_string ((char) c.value, &k, ref_str))
    {
      A68_INT pos;
      TEST_NIL (p, ref_pos, MODE (REF_INT));
      pos.status = INITIALISED_MASK;
      pos.value = k;
      *(A68_INT *) ADDRESS (&ref_pos) = pos;
      PUSH_BOOL (p, A_TRUE);
    }
  else
    {
      PUSH_BOOL (p, A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Operators for ROWS.                                                           */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LWB = (ROWS) INT                                                           */

void
genie_monad_lwb (NODE_T * p)
{
  A68_REF z;
  A68_ARRAY *x;
  A68_TUPLE *t;
  POP_REF (p, &z);
/* Decrease pointer since a UNION is on the stack */
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
  TEST_INIT (p, z, MODE (ROWS));
  TEST_NIL (p, z, MODE (ROWS));
  GET_DESCRIPTOR (x, t, &z);
  PUSH_INT (p, t->lower_bound);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP UPB = (ROWS) INT                                                           */

void
genie_monad_upb (NODE_T * p)
{
  A68_REF z;
  A68_ARRAY *x;
  A68_TUPLE *t;
  POP_REF (p, &z);
/* Decrease pointer since a UNION is on the stack */
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
  TEST_INIT (p, z, MODE (ROWS));
  TEST_NIL (p, z, MODE (ROWS));
  GET_DESCRIPTOR (x, t, &z);
  PUSH_INT (p, t->upper_bound);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LWB = (INT, ROWS) INT                                                      */

void
genie_dyad_lwb (NODE_T * p)
{
  A68_REF z;
  A68_ARRAY *x;
  A68_TUPLE *t;
  A68_INT k;
  POP_REF (p, &z);
/* Decrease pointer since a UNION is on the stack */
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
  TEST_INIT (p, z, MODE (ROWS));
  TEST_NIL (p, z, MODE (ROWS));
  POP_INT (p, &k);
  GET_DESCRIPTOR (x, t, &z);
  if (k.value < 1 || k.value > x->dimensions)
    {
      diagnostic (A_RUNTIME_ERROR, p, "invalid dimension D", (int) k.value);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_INT (p, t[k.value - 1].lower_bound);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP UPB = (INT, ROWS) INT                                                      */

void
genie_dyad_upb (NODE_T * p)
{
  A68_REF z;
  A68_ARRAY *x;
  A68_TUPLE *t;
  A68_INT k;
  POP_REF (p, &z);
/* Decrease pointer since a UNION is on the stack */
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_POINTER));
  TEST_INIT (p, z, MODE (ROWS));
  TEST_NIL (p, z, MODE (ROWS));
  POP_INT (p, &k);
  GET_DESCRIPTOR (x, t, &z);
  if (k.value < 1 || k.value > x->dimensions)
    {
      diagnostic (A_RUNTIME_ERROR, p, "invalid dimension D", (int) k.value);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  PUSH_INT (p, t[k.value - 1].upper_bound);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
RNG functions are in gsl.c.                                                   */

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC first random = (INT) VOID                                                */

void
genie_first_random (NODE_T * p)
{
  A68_INT i;
  POP_INT (p, &i);
  init_rng ((unsigned long) i.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC next random = REAL                                                       */

void
genie_next_random (NODE_T * p)
{
  PUSH_REAL (p, rng_53_bit ());
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC next long random = LONG REAL                                             */

void
genie_long_next_random (NODE_T * p)
{
  int digits = get_mp_digits (MOID (p));
  mp_digit *z = stack_mp (p, digits);
  int k = 2 + digits;
  while (--k > 1)
    {
      z[k] = (int) (rng_53_bit () * MP_RADIX);
    }
  MP_EXPONENT (z) = -1;
  MP_STATUS (z) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
BYTES operations.                                                             */

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, BYTES) CHAR                                                   */

void
genie_elem_bytes (NODE_T * p)
{
  A68_BYTES j;
  A68_INT i;
  POP_BYTES (p, &j);
  POP_INT (p, &i);
  if (i.value < 1 || i.value > BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (i.value > (int) strlen (j.value))
    {
      genie_null_char (p);
    }
  else
    {
      PUSH_CHAR (p, j.value[i.value - 1]);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC bytes pack = (STRING) BYTES                                              */

void
genie_bytespack (NODE_T * p)
{
  A68_REF z;
  A68_BYTES b;
  POP_REF (p, &z);
  TEST_INIT (p, z, MODE (STRING));
  TEST_NIL (p, z, MODE (STRING));
  if (a68_string_size (p, z) > BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (STRING));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  b.status = INITIALISED_MASK;
  a_to_c_string (p, b.value, z);
  PUSH (p, &b, SIZE_OF (A68_BYTES));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (BYTES, BYTES) BYTES                                                   */

void
genie_add_bytes (NODE_T * p)
{
  A68_BYTES *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_BYTES);
  if (((int) strlen (i->value) + (int) strlen (j->value)) > BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcat (i->value, j->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF BYTES, BYTES) REF BYTES                                         */

void
genie_plusab_bytes (NODE_T * p)
{
  A68_BYTES *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_BYTES);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_BYTES));
  address = (A68_BYTES *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (BYTES));
  if (((int) strlen (address->value) + (int) strlen (i->value)) > BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcat (address->value, i->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +=: = (BYTES, REF BYTES) REF BYTES                                         */

void
genie_plusto_bytes (NODE_T * p)
{
  A68_BYTES i, *address, j;
  A68_REF z;
  POP_REF (p, &z);
  TEST_NIL (p, z, MODE (REF_BYTES));
  address = (A68_BYTES *) ADDRESS (&z);
  TEST_INIT (p, *address, MODE (BYTES));
  POP (p, &i, SIZE_OF (A68_BYTES));
  if (((int) strlen (address->value) + (int) strlen (i.value)) > BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcpy (j.value, i.value);
  strcat (j.value, address->value);
  strcpy (address->value, j.value);
  PUSH_REF (p, z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
compare_bytes: difference between BYTE strings                                */

static int
compare_bytes (NODE_T * p)
{
  A68_BYTES x, y;
  POP_BYTES (p, &y);
  POP_BYTES (p, &x);
  return (strcmp (x.value, y.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (BYTES, BYTES) BOOL                                                        */

#define A68_CMP_BYTES(n, p, OP) void n (NODE_T * p) {\
  int k = compare_bytes (p);\
  PUSH_BOOL (p, k OP 0);\
}

A68_CMP_BYTES (genie_eq_bytes, p, ==) A68_CMP_BYTES (genie_ne_bytes, p, !=)A68_CMP_BYTES (genie_lt_bytes, p, <)A68_CMP_BYTES (genie_gt_bytes, p, >)A68_CMP_BYTES (genie_le_bytes, p, <=)A68_CMP_BYTES (genie_ge_bytes, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP LENG = (BYTES) LONG BYTES                                                  */
     void
     genie_leng_bytes (NODE_T * p)
{
  A68_BYTES a;
  POP_BYTES (p, &a);
  PUSH_LONG_BYTES (p, a.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG BYTES) BYTES                                               */

void
genie_shorten_bytes (NODE_T * p)
{
  A68_LONG_BYTES a;
  POP_LONG_BYTES (p, &a);
  PUSH_BYTES (p, a.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, LONG BYTES) CHAR                                              */

void
genie_elem_long_bytes (NODE_T * p)
{
  A68_LONG_BYTES j;
  A68_INT i;
  POP_LONG_BYTES (p, &j);
  POP_INT (p, &i);
  if (i.value < 1 || i.value > LONG_BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (i.value > (int) strlen (j.value))
    {
      genie_null_char (p);
    }
  else
    {
      PUSH_CHAR (p, j.value[i.value - 1]);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long bytes pack = (STRING) LONG BYTES                                    */

void
genie_long_bytespack (NODE_T * p)
{
  A68_REF z;
  A68_LONG_BYTES b;
  POP_REF (p, &z);
  TEST_INIT (p, z, MODE (STRING));
  TEST_NIL (p, z, MODE (STRING));
  if (a68_string_size (p, z) > LONG_BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (STRING));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  b.status = INITIALISED_MASK;
  a_to_c_string (p, b.value, z);
  PUSH (p, &b, SIZE_OF (A68_LONG_BYTES));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP + = (LONG BYTES, LONG BYTES) LONG BYTES                                    */

void
genie_add_long_bytes (NODE_T * p)
{
  A68_LONG_BYTES *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_LONG_BYTES);
  if (((int) strlen (i->value) + (int) strlen (j->value)) > LONG_BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (LONG_BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcat (i->value, j->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +:= = (REF LONG BYTES, LONG BYTES) REF LONG BYTES                          */

void
genie_plusab_long_bytes (NODE_T * p)
{
  A68_LONG_BYTES *i, *address;
  A68_REF *z;
  POP_ADDRESS (p, i, A68_LONG_BYTES);
  POP_OPERAND_ADDRESS (p, z, A68_REF);
  TEST_NIL (p, *z, MODE (REF_LONG_BYTES));
  address = (A68_LONG_BYTES *) ADDRESS (z);
  TEST_INIT (p, *address, MODE (LONG_BYTES));
  if (((int) strlen (address->value) + (int) strlen (i->value)) > LONG_BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (LONG_BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcat (address->value, i->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP +=: = (LONG BYTES, REF LONG BYTES) REF LONG BYTES                          */

void
genie_plusto_long_bytes (NODE_T * p)
{
  A68_BYTES i, *address, j;
  A68_REF z;
  POP_REF (p, &z);
  TEST_NIL (p, z, MODE (REF_LONG_BYTES));
  address = (A68_BYTES *) ADDRESS (&z);
  TEST_INIT (p, *address, MODE (LONG_BYTES));
  POP (p, &i, SIZE_OF (A68_LONG_BYTES));
  if (((int) strlen (address->value) + (int) strlen (i.value)) > LONG_BYTES_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (LONG_BYTES));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  strcpy (j.value, i.value);
  strcat (j.value, address->value);
  strcpy (address->value, j.value);
  PUSH_REF (p, z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
compare_long_bytes: difference between LONG BYTE strings                      */

static int
compare_long_bytes (NODE_T * p)
{
  A68_LONG_BYTES x, y;
  POP_LONG_BYTES (p, &y);
  POP_LONG_BYTES (p, &x);
  return (strcmp (x.value, y.value));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP (LONG BYTES, LONG BYTES) BOOL                                              */

#define A68_CMP_LONG_BYTES(n, p, OP) void n (NODE_T * p) {\
  int k = compare_long_bytes (p);\
  PUSH_BOOL (p, k OP 0);\
}

A68_CMP_LONG_BYTES (genie_eq_long_bytes, p, ==) A68_CMP_LONG_BYTES (genie_ne_long_bytes, p, !=)A68_CMP_LONG_BYTES (genie_lt_long_bytes, p, <)A68_CMP_LONG_BYTES (genie_gt_long_bytes, p, >)A68_CMP_LONG_BYTES (genie_le_long_bytes, p, <=)A68_CMP_LONG_BYTES (genie_ge_long_bytes, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
BITS operations.                                                              */
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP NOT = (BITS) BITS                                                          */
  A68_MONAD (genie_not_bits, p, A68_BITS, ~)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP AND = (BITS, BITS) BITS                                                    */
     void genie_and_bits (NODE_T * p)
{
  A68_BITS *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_BITS);
  i->value = i->value & j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP OR = (BITS, BITS) BITS                                                     */

void
genie_or_bits (NODE_T * p)
{
  A68_BITS *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_BITS);
  i->value = i->value | j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP XOR = (BITS, BITS) BITS                                                    */

void
genie_xor_bits (NODE_T * p)
{
  A68_BITS *i, *j;
  POP_OPERAND_ADDRESSES (p, i, j, A68_BITS);
  i->value = i->value ^ j->value;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP = = (BITS, BITS) BOOL                                                      */

#define A68_CMP_BITS(n, p, OP) void n (NODE_T * p) {\
  A68_BITS i, j;\
  POP_BITS (p, &j);\
  POP_BITS (p, &i);\
  PUSH_BOOL (p, i.value OP j.value);\
  }

A68_CMP_BITS (genie_eq_bits, p, ==) A68_CMP_BITS (genie_ne_bits, p, !=)A68_CMP_BITS (genie_lt_bits, p, <)A68_CMP_BITS (genie_gt_bits, p, >)A68_CMP_BITS (genie_le_bits, p, <=)A68_CMP_BITS (genie_ge_bits, p, >=)
/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHL = (BITS, INT) BITS                                                     */
     void
     genie_shl_bits (NODE_T * p)
{
  A68_BITS i;
  A68_INT j;
  POP_INT (p, &j);
  POP_BITS (p, &i);
  if (j.value >= 0)
    {
      if (i.value > (MAX_BITS >> j.value))
	{
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BITS));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      PUSH_BITS (p, i.value << j.value);
    }
  else
    {
      PUSH_BITS (p, i.value >> -j.value);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHR = (BITS, INT) BITS                                                     */

void
genie_shr_bits (NODE_T * p)
{
  A68_INT *j;
  POP_OPERAND_ADDRESS (p, j, A68_INT);
  j->value = -j->value;
  genie_shl_bits (p);		/* Conform RR */
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, BITS) BOOL                                                    */

void
genie_elem_bits (NODE_T * p)
{
  A68_BITS j;
  A68_INT i;
  int n;
  POP_BITS (p, &j);
  POP_INT (p, &i);
  if (i.value < 1 || i.value > BITS_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  for (n = 0; n < (BITS_WIDTH - i.value); n++)
    {
      j.value = j.value >> 1;
    }
  PUSH_BOOL (p, j.value & 0x1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP BIN = (INT) BITS                                                           */

void
genie_bin_int (NODE_T * p)
{
  A68_INT i;
  POP_INT (p, &i);
/* RR does not convert negative numbers. Algol68G does. */
  PUSH_BITS (p, i.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP BIN = (LONG INT) LONG BITS                                                 */

void
genie_bin_long_mp (NODE_T * p)
{
  MOID_T *mode = SUB (MOID (p));
  int size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
/* We convert just for the operand check. */
  (void) stack_mp_bits (p, (mp_digit *) STACK_OFFSET (-size), mode);
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP NOT = (LONG BITS) LONG BITS                                                */

void
genie_not_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int size = get_mp_size (mode);
  ADDR_T old_sp = stack_pointer;
  int k, words = get_mp_bits_words (mode);
  mp_digit *u = (mp_digit *) STACK_OFFSET (-size);
  unsigned *row = stack_mp_bits (p, u, mode);
  for (k = 0; k < words; k++)
    {
      row[k] = ~row[k];
    }
  pack_mp_bits (p, u, row, mode);
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHORTEN = (LONG BITS) BITS                                                 */

void
genie_shorten_long_mp_to_bits (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  mp_digit *z = (mp_digit *) STACK_OFFSET (-size);
  DECREMENT_STACK_POINTER (p, size);
  MP_STATUS (z) = INITIALISED_MASK;
  PUSH_BITS (p, mp_to_unsigned (p, z, digits));
}
/*-------1---------2---------3---------4---------5---------6---------7---------+
elem_long_bits                                                                */

unsigned
elem_long_bits (NODE_T * p, int k, mp_digit * z, MOID_T * m)
{
  int n;
  ADDR_T save_sp = stack_pointer;
  unsigned *words = stack_mp_bits (p, z, m), w;
  k += (MP_BITS_BITS - get_mp_bits_width (m) % MP_BITS_BITS - 1);
  w = words[k / MP_BITS_BITS];
  for (n = 0; n < (MP_BITS_BITS - k % MP_BITS_BITS - 1); n++)
    {
      w = w >> 1;
    }
  stack_pointer = save_sp;
  return (w & 0x1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, LONG BITS) BOOL                                               */

void
genie_elem_long_bits (NODE_T * p)
{
  A68_INT *i;
  mp_digit *z;
  unsigned w;
  int bits = get_mp_bits_width (MODE (LONG_BITS)), size = get_mp_size (MODE (LONG_BITS));
  z = (mp_digit *) STACK_OFFSET (-size);
  i = (A68_INT *) STACK_OFFSET (-(size + SIZE_OF (A68_INT)));
  if (i->value < 1 || i->value > bits)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  w = elem_long_bits (p, i->value, z, MODE (LONG_BITS));
  DECREMENT_STACK_POINTER (p, size + SIZE_OF (A68_INT));
  PUSH_BOOL (p, w != 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP ELEM = (INT, LONG LONG BITS) BOOL                                          */

void
genie_elem_longlong_bits (NODE_T * p)
{
  A68_INT *i;
  mp_digit *z;
  unsigned w;
  int bits = get_mp_bits_width (MODE (LONGLONG_BITS)), size = get_mp_size (MODE (LONGLONG_BITS));
  z = (mp_digit *) STACK_OFFSET (-size);
  i = (A68_INT *) STACK_OFFSET (-(size + SIZE_OF (A68_INT)));
  if (i->value < 1 || i->value > bits)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  w = elem_long_bits (p, i->value, z, MODE (LONGLONG_BITS));
  DECREMENT_STACK_POINTER (p, size + SIZE_OF (A68_INT));
  PUSH_BOOL (p, w != 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC bits pack = ([] BOOL) BITS                                               */

void
genie_bits_pack (NODE_T * p)
{
  A68_REF z;
  A68_BITS b;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base;
  int size, k;
  unsigned bit;
  POP_REF (p, &z);
  TEST_INIT (p, z, MODE (ROW_BOOL));
  TEST_NIL (p, z, MODE (ROW_BOOL));
  GET_DESCRIPTOR (arr, tup, &z);
  size = ROW_SIZE (tup);
  if (size < 0 || size > BITS_WIDTH)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (ROW_BOOL));
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Convert so that LWB goes to MSB, so ELEM gives same order. */
  base = ADDRESS (&arr->array);
  b.value = 0;
/* Set bit mask. */
  bit = 0x1;
  for (k = 0; k < BITS_WIDTH - size; k++)
    {
      bit <<= 1;
    }
  for (k = tup->upper_bound; k >= tup->lower_bound; k--)
    {
      int addr = INDEX_1_DIM (arr, tup, k);
      A68_BOOL *boo = (A68_BOOL *) & (base[addr]);
      TEST_INIT (p, *boo, MODE (BOOL));
      if (boo->value)
	{
	  b.value |= bit;
	}
      bit <<= 1;
    }
  b.status = INITIALISED_MASK;
  PUSH (p, &b, SIZE_OF (A68_BITS));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC long bits pack = ([] BOOL) LONG BITS
PROC long long bits pack = ([] BOOL) LONG LONG BITS                           */

void
genie_long_bits_pack (NODE_T * p)
{
  MOID_T *mode = MOID (p);
  A68_REF z;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base;
  int size, k, bits, digits;
  ADDR_T old_sp;
  mp_digit *sum, *fact;
  POP_REF (p, &z);
  TEST_INIT (p, z, MODE (ROW_BOOL));
  TEST_NIL (p, z, MODE (ROW_BOOL));
  GET_DESCRIPTOR (arr, tup, &z);
  size = ROW_SIZE (tup);
  bits = get_mp_bits_width (mode);
  digits = get_mp_digits (mode);
  if (size < 0 || size > bits)
    {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (ROW_BOOL));
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Convert so that LWB goes to MSB, so ELEM gives same order as [] BOOL. */
  base = ADDRESS (&arr->array);
  sum = stack_mp (p, digits);
  SET_MP_ZERO (sum, digits);
  old_sp = stack_pointer;
/* Set bit mask. */
  fact = stack_mp (p, digits);
  set_mp_short (fact, (mp_digit) 1, 0, digits);
  for (k = 0; k < bits - size; k++)
    {
      mul_mp_digit (p, fact, fact, (mp_digit) 2, digits);
    }
  for (k = tup->upper_bound; k >= tup->lower_bound; k--)
    {
      int addr = INDEX_1_DIM (arr, tup, k);
      A68_BOOL *boo = (A68_BOOL *) & (base[addr]);
      TEST_INIT (p, *boo, MODE (BOOL));
      if (boo->value)
	{
	  add_mp (p, sum, sum, fact, digits);
	}
      mul_mp_digit (p, fact, fact, (mp_digit) 2, digits);
    }
  stack_pointer = old_sp;
  MP_STATUS (sum) = INITIALISED_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHL = (LONG BITS, INT) LONG BITS                                           */

void
genie_shl_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  MOID_T *int_m = (mode == MODE (LONG_BITS) ? MODE (LONG_INT) : MODE (LONGLONG_INT));
  int size = get_mp_size (mode), digits = get_mp_digits (mode);
  A68_INT j;
  ADDR_T save_sp;
  mp_digit *u, *two, *pow;
  BOOL_T multiply;
/* Pop number of bits. */
  POP_INT (p, &j);
  if (j.value >= 0)
    {
      multiply = A_TRUE;
    }
  else
    {
      multiply = A_FALSE;
      j.value = -j.value;
    }
  u = (mp_digit *) STACK_OFFSET (-size);
/* Determine multiplication factor, 2 ** j. */
  save_sp = stack_pointer;
  two = stack_mp (p, digits);
  set_mp_short (two, (mp_digit) 2, 0, digits);
  pow = stack_mp (p, digits);
  pow_mp_int (p, pow, two, j.value, digits);
  test_long_int_range (p, pow, int_m);
/*Implement shift. */
  if (multiply)
    {
      mul_mp (p, u, u, pow, digits);
      check_long_bits_value (p, u, mode);
    }
  else
    {
      over_mp (p, u, u, pow, digits);
    }
  stack_pointer = save_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP SHR = (LONG BITS, INT) LONG BITS                                           */

void
genie_shr_long_mp (NODE_T * p)
{
  A68_INT *j;
  POP_OPERAND_ADDRESS (p, j, A68_INT);
  j->value = -j->value;
  genie_shl_long_mp (p);	/* Conform RR */
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP AND = (LONG BITS, LONG BITS) LONG BITS                                     */

void
genie_and_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int k, size = get_mp_size (mode), words = get_mp_bits_words (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *u = (mp_digit *) STACK_OFFSET (-2 * size), *v = (mp_digit *) STACK_OFFSET (-size);
  unsigned *row_u = stack_mp_bits (p, u, mode), *row_v = stack_mp_bits (p, v, mode);
  for (k = 0; k < words; k++)
    {
      row_u[k] &= row_v[k];
    }
  pack_mp_bits (p, u, row_u, mode);
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP OR = (LONG BITS, LONG BITS) LONG BITS                                      */

void
genie_or_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int k, size = get_mp_size (mode), words = get_mp_bits_words (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *u = (mp_digit *) STACK_OFFSET (-2 * size), *v = (mp_digit *) STACK_OFFSET (-size);
  unsigned *row_u = stack_mp_bits (p, u, mode), *row_v = stack_mp_bits (p, v, mode);
  for (k = 0; k < words; k++)
    {
      row_u[k] |= row_v[k];
    }
  pack_mp_bits (p, u, row_u, mode);
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
OP XOR = (LONG BITS, LONG BITS) LONG BITS                                     */

void
genie_xor_long_mp (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int k, size = get_mp_size (mode), words = get_mp_bits_words (mode);
  ADDR_T old_sp = stack_pointer;
  mp_digit *u = (mp_digit *) STACK_OFFSET (-2 * size), *v = (mp_digit *) STACK_OFFSET (-size);
  unsigned *row_u = stack_mp_bits (p, u, mode), *row_v = stack_mp_bits (p, v, mode);
  for (k = 0; k < words; k++)
    {
      row_u[k] ^= row_v[k];
    }
  pack_mp_bits (p, u, row_u, mode);
  stack_pointer = old_sp;
  DECREMENT_STACK_POINTER (p, size);
}
