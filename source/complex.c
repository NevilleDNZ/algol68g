/*!
\file complex.c
\brief standard environ routines for complex numbers
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

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


/*
This file contains Algol68G's standard environ for complex numbers.
Some of the LONG operations are generic for LONG and LONG LONG.

Some routines are based on
* GNU Scientific Library
* Abramowitz and Stegun.
*/

#include "algol68g.h"
#include "genie.h"
#include "mp.h"


/*!
\brief OP +* = (REAL, REAL) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_icomplex (NODE_T * p)
{
  (void) p;
}

/*!
\brief OP +* = (INT, INT) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_iint_complex (NODE_T * p)
{
  A68_INT jre, jim;
  POP_INT (p, &jim);
  POP_INT (p, &jre);
  PUSH_REAL (p, (double) jre.value);
  PUSH_REAL (p, (double) jim.value);
}

/*!
\brief OP RE = (COMPLEX) REAL
\param p position in syntax tree, should not be NULL
**/

void genie_re_complex (NODE_T * p)
{
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_REAL));
}

/*!
\brief OP IM = (COMPLEX) REAL
\param p position in syntax tree, should not be NULL
**/

void genie_im_complex (NODE_T * p)
{
  A68_REAL im;
  POP_REAL (p, &im);
  *(A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL))) = im;
}

/*!
\brief OP - = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_minus_complex (NODE_T * p)
{
  A68_REAL *rex, *imx;
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value = -imx->value;
  rex->value = -rex->value;
  (void) p;
}

/*!
\brief ABS = (COMPLEX) REAL
\param p position in syntax tree, should not be NULL
**/

void genie_abs_complex (NODE_T * p)
{
  A68_REAL rex, imx;
  POP_COMPLEX (p, &rex, &imx);
  PUSH_REAL (p, a68g_hypot (rex.value, imx.value));
}

/*!
\brief OP ARG = (COMPLEX) REAL
\param p position in syntax tree, should not be NULL
**/

void genie_arg_complex (NODE_T * p)
{
  A68_REAL rex, imx;
  POP_COMPLEX (p, &rex, &imx);
  if (rex.value != 0.0 || imx.value != 0.0) {
    PUSH_REAL (p, atan2 (imx.value, rex.value));
  } else {
    diagnostic (A_RUNTIME_ERROR, p, INVALID_ARGUMENT_ERROR, MODE (COMPLEX), NULL);
    exit_genie (p, A_RUNTIME_ERROR);
  }
}

/*!
\brief OP CONJ = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_conj_complex (NODE_T * p)
{
  A68_REAL *im;
  POP_OPERAND_ADDRESS (p, im, A68_REAL);
  im->value = -im->value;
}

/*!
\brief OP + = (COMPLEX, COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_add_complex (NODE_T * p)
{
  A68_REAL *rex, *imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value += imy.value;
  rex->value += rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*!
\brief OP - = (COMPLEX, COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sub_complex (NODE_T * p)
{
  A68_REAL *rex, *imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  imx = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  rex = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  imx->value -= imy.value;
  rex->value -= rey.value;
  TEST_COMPLEX_REPRESENTATION (p, rex->value, imx->value);
}

/*!
\brief OP * = (COMPLEX, COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_mul_complex (NODE_T * p)
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

/*!
\brief OP / = (COMPLEX, COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_div_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  double re = 0.0, im = 0.0;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
#ifndef HAVE_IEEE_754
  if (rey.value == 0.0 && imy.value == 0.0) {
    diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (COMPLEX));
    exit_genie (p, A_RUNTIME_ERROR);
  }
#endif
  if (ABS (rey.value) >= ABS (imy.value)) {
    double r = imy.value / rey.value, den = rey.value + r * imy.value;
    re = (rex.value + r * imx.value) / den;
    im = (imx.value - r * rex.value) / den;
  } else {
    double r = rey.value / imy.value, den = imy.value + r * rey.value;
    re = (rex.value * r + imx.value) / den;
    im = (imx.value * r - rex.value) / den;
  }
  TEST_COMPLEX_REPRESENTATION (p, re, im);
  PUSH_COMPLEX (p, re, im);
}

/*!
\brief OP ** = (COMPLEX, INT) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_pow_complex_int (NODE_T * p)
{
  A68_REAL rex, imx;
  double rey, imy, rez, imz, rea;
  A68_INT j;
  int expo;
  BOOL_T negative;
  POP_INT (p, &j);
  POP_COMPLEX (p, &rex, &imx);
  rez = 1.0;
  imz = 0.0;
  rey = rex.value;
  imy = imx.value;
  expo = 1;
  negative = (j.value < 0.0);
  if (negative) {
    j.value = -j.value;
  }
  while ((unsigned) expo <= (unsigned) (j.value)) {
    if (expo & j.value) {
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
  if (negative) {
    PUSH_REAL (p, 1.0);
    PUSH_REAL (p, 0.0);
    PUSH_REAL (p, rez);
    PUSH_REAL (p, imz);
    genie_div_complex (p);
  } else {
    PUSH_REAL (p, rez);
    PUSH_REAL (p, imz);
  }
}

/*!
\brief OP = = (COMPLEX, COMPLEX) BOOL
\param p position in syntax tree, should not be NULL
**/

void genie_eq_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
  PUSH_BOOL (p, (rex.value == rey.value) && (imx.value == imy.value));
}

/*!
\brief OP /= = (COMPLEX, COMPLEX) BOOL
\param p position in syntax tree, should not be NULL
\return
**/

void genie_ne_complex (NODE_T * p)
{
  A68_REAL rex, imx, rey, imy;
  POP_COMPLEX (p, &rey, &imy);
  POP_COMPLEX (p, &rex, &imx);
  PUSH_BOOL (p, !((rex.value == rey.value) && (imx.value == imy.value)));
}

/*!
\brief OP +:= = (REF COMPLEX, COMPLEX) REF COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_plusab_complex (NODE_T * p)
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

/*!
\brief OP -:= = (REF COMPLEX, COMPLEX) REF COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_minusab_complex (NODE_T * p)
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

/*!
\brief OP *:= = (REF COMPLEX, COMPLEX) REF COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_timesab_complex (NODE_T * p)
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

/*!
\brief OP /:= = (REF COMPLEX, COMPLEX) REF COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_divab_complex (NODE_T * p)
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
  if (rey.value == 0.0 && imy.value == 0.0) {
    diagnostic (A_RUNTIME_ERROR, p, DIVISION_BY_ZERO_ERROR, MODE (COMPLEX));
    exit_genie (p, A_RUNTIME_ERROR);
  }
#endif
  if (ABS (rey.value) >= ABS (imy.value)) {
    double r = imy.value / rey.value, den = rey.value + r * imy.value;
    rez = (rex->value + r * imx->value) / den;
    imz = (imx->value - r * rex->value) / den;
  } else {
    double r = rey.value / imy.value, den = imy.value + r * rey.value;
    rez = (rex->value * r + imx->value) / den;
    imz = (imx->value * r - rex->value) / den;
  }
  TEST_COMPLEX_REPRESENTATION (p, rez, imz);
  imx->value = imz;
  rex->value = rez;
}

/*!
\brief OP LENG = (COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_lengthen_complex_to_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL));
  MP_DIGIT_T *z;
  A68_REAL a, b;
  POP_REAL (p, &b);
  POP_REAL (p, &a);
  STACK_MP (z, p, digits);
  real_to_mp (p, z, a.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
  STACK_MP (z, p, digits);
  real_to_mp (p, z, b.value, digits);
  MP_STATUS (z) = INITIALISED_MASK;
}

/*!
\brief OP SHORTEN = (LONG COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_shorten_long_complex_to_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_REAL (p, mp_to_real (p, a, digits));
  PUSH_REAL (p, mp_to_real (p, b, digits));
}

/*!
\brief OP LENG = (LONG COMPLEX) LONG LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_lengthen_long_complex_to_longlong_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  int digs_long = get_mp_digits (MODE (LONGLONG_REAL)), size_long = get_mp_size (MODE (LONGLONG_REAL));
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *a, *b, *c, *d;
  b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  STACK_MP (c, p, digs_long);
  STACK_MP (d, p, digs_long);
  lengthen_mp (p, c, digs_long, a, digits);
  lengthen_mp (p, d, digs_long, b, digits);
  MOVE_MP (a, c, digs_long);
  MOVE_MP (&a[2 + digs_long], d, digs_long);
  stack_pointer = pop_sp;
  MP_STATUS (a) = INITIALISED_MASK;
  (&a[2 + digs_long])[0] = INITIALISED_MASK;
  INCREMENT_STACK_POINTER (p, 2 * (size_long - size));
}

/*!
\brief OP SHORTEN = (LONG LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_shorten_longlong_complex_to_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MODE (LONG_REAL)), size = get_mp_size (MODE (LONG_REAL));
  int digs_long = get_mp_digits (MODE (LONGLONG_REAL)), size_long = get_mp_size (MODE (LONGLONG_REAL));
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *a, *b;
  b = (MP_DIGIT_T *) STACK_OFFSET (-size_long);
  a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size_long);
  shorten_mp (p, a, digits, a, digs_long);
  shorten_mp (p, &a[2 + digits], digits, b, digs_long);
  stack_pointer = pop_sp;
  MP_STATUS (a) = INITIALISED_MASK;
  (&a[2 + digits])[0] = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, 2 * (size_long - size));
}

/*!
\brief OP RE = (LONG COMPLEX) LONG REAL
\param p position in syntax tree, should not be NULL
**/

void genie_re_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_STATUS (a) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, (int) size_long_mp ());
}

/*!
\brief OP IM = (LONG COMPLEX) LONG REAL
\param p position in syntax tree, should not be NULL
**/

void genie_im_long_complex (NODE_T * p)
{
  int digits = get_mp_digits (MOID (PACK (MOID (p)))), size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MOVE_MP (a, b, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  DECREMENT_STACK_POINTER (p, (int) size_long_mp ());
}

/*!
\brief OP - = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_minus_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT (a, 1) = -MP_DIGIT (a, 1);
  MP_DIGIT (b, 1) = -MP_DIGIT (b, 1);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
}

/*!
\brief OP CONJ = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
\return
**/

void genie_conj_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT (b, 1) = -MP_DIGIT (b, 1);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
}

/*!
\brief OP ABS = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_abs_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *z;
  STACK_MP (z, p, digits);
  hypot_mp (p, z, a, b, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, size);
  MOVE_MP (a, z, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief OP ARG = (LONG COMPLEX) LONG REAL
\param p position in syntax tree, should not be NULL
**/

void genie_arg_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *z;
  STACK_MP (z, p, digits);
  atan2_mp (p, z, a, b, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, size);
  MOVE_MP (a, z, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief OP + = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_add_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *d = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *c = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  add_mp (p, b, b, d, digits);
  add_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP - = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sub_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *d = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *c = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  sub_mp (p, b, b, d, digits);
  sub_mp (p, a, a, c, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP * = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_mul_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *d = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *c = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  cmul_mp (p, a, b, c, d, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP / = (LONG COMPLEX, LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_div_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *d = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *c = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  cdiv_mp (p, a, b, c, d, digits);
  MP_STATUS (a) = INITIALISED_MASK;
  MP_STATUS (b) = INITIALISED_MASK;
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP ** = (LONG COMPLEX, INT) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_pow_long_complex_int (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp;
  MP_DIGIT_T *rex, *imx, *rey, *imy, *rez, *imz, *rea, *acc;
  A68_INT j;
  int expo;
  BOOL_T negative;
  POP_INT (p, &j);
  pop_sp = stack_pointer;
  imx = (MP_DIGIT_T *) STACK_OFFSET (-size);
  rex = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  STACK_MP (rez, p, digits);
  set_mp_short (rez, (MP_DIGIT_T) 1, 0, digits);
  STACK_MP (imz, p, digits);
  SET_MP_ZERO (imz, digits);
  STACK_MP (rey, p, digits);
  STACK_MP (imy, p, digits);
  MOVE_MP (rey, rex, digits);
  MOVE_MP (imy, imx, digits);
  STACK_MP (rea, p, digits);
  STACK_MP (acc, p, digits);
  expo = 1;
  negative = (j.value < 0.0);
  if (negative) {
    j.value = -j.value;
  }
  while ((unsigned) expo <= (unsigned) (j.value)) {
    if (expo & j.value) {
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
  stack_pointer = pop_sp;
  if (negative) {
    set_mp_short (rex, (MP_DIGIT_T) 1, 0, digits);
    SET_MP_ZERO (imx, digits);
    INCREMENT_STACK_POINTER (p, 2 * size);
    genie_div_long_complex (p);
  } else {
    MOVE_MP (rex, rez, digits);
    MOVE_MP (imx, imz, digits);
  }
  MP_STATUS (rex) = INITIALISED_MASK;
  MP_STATUS (imx) = INITIALISED_MASK;
}

/*!
\brief OP = = (LONG COMPLEX, LONG COMPLEX) BOOL
\param p position in syntax tree, should not be NULL
**/

void genie_eq_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  genie_sub_long_complex (p);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_BOOL (p, MP_DIGIT (a, 1) == 0 && MP_DIGIT (b, 1) == 0);
}

/*!
\brief OP /= = (LONG COMPLEX, LONG COMPLEX) BOOL
\param p position in syntax tree, should not be NULL
**/

void genie_ne_long_complex (NODE_T * p)
{
  int size = get_mp_size (MOID (PACK (MOID (p))));
  MP_DIGIT_T *b = (MP_DIGIT_T *) STACK_OFFSET (-3 * size);
  MP_DIGIT_T *a = (MP_DIGIT_T *) STACK_OFFSET (-4 * size);
  genie_sub_long_complex (p);
  DECREMENT_STACK_POINTER (p, 2 * size);
  PUSH_BOOL (p, MP_DIGIT (a, 1) != 0 || MP_DIGIT (b, 1) != 0);
}

/*!
\brief OP +:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_plusab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  A68_REF *z;
  MP_DIGIT_T *a, *b, *c, *d, *e, *f, *g, *h;
  d = (MP_DIGIT_T *) (STACK_OFFSET (-size));
  c = (MP_DIGIT_T *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (MP_DIGIT_T *) ADDRESS (z);
  b = (MP_DIGIT_T *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  STACK_MP (e, p, digits);
  STACK_MP (f, p, digits);
  STACK_MP (g, p, digits);
  STACK_MP (h, p, digits);
  MOVE_MP (e, a, digits);
  MOVE_MP (f, b, digits);
  MOVE_MP (g, c, digits);
  MOVE_MP (h, d, digits);
  genie_add_long_complex (p);
  MOVE_MP (a, e, digits);
  MOVE_MP (b, f, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP -:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_minusab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  A68_REF *z;
  MP_DIGIT_T *a, *b, *c, *d, *e, *f, *g, *h;
  d = (MP_DIGIT_T *) (STACK_OFFSET (-size));
  c = (MP_DIGIT_T *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (MP_DIGIT_T *) ADDRESS (z);
  b = (MP_DIGIT_T *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  STACK_MP (e, p, digits);
  STACK_MP (f, p, digits);
  STACK_MP (g, p, digits);
  STACK_MP (h, p, digits);
  MOVE_MP (e, a, digits);
  MOVE_MP (f, b, digits);
  MOVE_MP (g, c, digits);
  MOVE_MP (h, d, digits);
  genie_sub_long_complex (p);
  MOVE_MP (a, e, digits);
  MOVE_MP (b, f, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP *:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_timesab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  A68_REF *z;
  MP_DIGIT_T *a, *b, *c, *d, *e, *f, *g, *h;
  d = (MP_DIGIT_T *) (STACK_OFFSET (-size));
  c = (MP_DIGIT_T *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (MP_DIGIT_T *) ADDRESS (z);
  b = (MP_DIGIT_T *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  STACK_MP (e, p, digits);
  STACK_MP (f, p, digits);
  STACK_MP (g, p, digits);
  STACK_MP (h, p, digits);
  MOVE_MP (e, a, digits);
  MOVE_MP (f, b, digits);
  MOVE_MP (g, c, digits);
  MOVE_MP (h, d, digits);
  genie_mul_long_complex (p);
  MOVE_MP (a, e, digits);
  MOVE_MP (b, f, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief OP /:= = (REF LONG COMPLEX, LONG COMPLEX) REF LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_divab_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (NEXT (PACK (MOID (p))));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  A68_REF *z;
  MP_DIGIT_T *a, *b, *c, *d, *e, *f, *g, *h;
  d = (MP_DIGIT_T *) (STACK_OFFSET (-size));
  c = (MP_DIGIT_T *) (STACK_OFFSET (-2 * size));
  z = (A68_REF *) (STACK_OFFSET (-2 * size - SIZE_OF (A68_REF)));
  TEST_NIL (p, *z, MOID (PREVIOUS (p)));
  a = (MP_DIGIT_T *) ADDRESS (z);
  b = (MP_DIGIT_T *) & (((BYTE_T *) a)[size]);
  TEST_MP_INIT (p, a, MOID (NEXT (p)));
  TEST_MP_INIT (p, b, MOID (NEXT (p)));
  STACK_MP (e, p, digits);
  STACK_MP (f, p, digits);
  STACK_MP (g, p, digits);
  STACK_MP (h, p, digits);
  MOVE_MP (e, a, digits);
  MOVE_MP (f, b, digits);
  MOVE_MP (g, c, digits);
  MOVE_MP (h, d, digits);
  genie_div_long_complex (p);
  MOVE_MP (a, e, digits);
  MOVE_MP (b, f, digits);
  stack_pointer = pop_sp;
  DECREMENT_STACK_POINTER (p, 2 * size);
}

/*!
\brief PROC csqrt = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sqrt_complex (NODE_T * p)
{
  A68_REAL *re, *im;
  im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (re->value == 0.0 && im->value == 0.0) {
    re->value = 0.0;
    im->value = 0.0;
  } else {
    double x = ABS (re->value), y = ABS (im->value), w;
    if (x >= y) {
      double t = y / x;
      w = sqrt (x) * sqrt (0.5 * (1.0 + sqrt (1.0 + t * t)));
    } else {
      double t = x / y;
      w = sqrt (y) * sqrt (0.5 * (t + sqrt (1.0 + t * t)));
    }
    if (re->value >= 0.0) {
      re->value = w;
      im->value = im->value / (2.0 * w);
    } else {
      double ai = im->value;
      double vi = (ai >= 0.0 ? w : -w);
      re->value = ai / (2.0 * vi);
      im->value = vi;
    }
  }
  math_rte (p, errno != 0, MODE (COMPLEX), NULL);
}

/*!
\brief PROC long csqrt = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sqrt_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  RESET_ERRNO;
  csqrt_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC cexp = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_exp_complex (NODE_T * p)
{
  A68_REAL *re, *im;
  double r;
  im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  r = exp (re->value);
  re->value = r * cos (im->value);
  im->value = r * sin (im->value);
  math_rte (p, errno != 0, MODE (COMPLEX), NULL);
}

/*!
\brief PROC long cexp = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_exp_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  cexp_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC cln = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_ln_complex (NODE_T * p)
{
  A68_REAL *re, *im, r, th;
  im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  PUSH_COMPLEX (p, re->value, im->value);
  genie_abs_complex (p);
  POP_REAL (p, &r);
  PUSH_COMPLEX (p, re->value, im->value);
  genie_arg_complex (p);
  POP_REAL (p, &th);
  re->value = log (r.value);
  im->value = th.value;
  math_rte (p, errno != 0, MODE (COMPLEX), NULL);
}

/*!
\brief PROC long cln = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_ln_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  cln_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC csin = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sin_complex (NODE_T * p)
{
  A68_REAL *re, *im;
  im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (im->value == 0.0) {
    re->value = sin (re->value);
    im->value = 0.0;
  } else {
    double r = re->value, i = im->value;
    re->value = sin (r) * cosh (i);
    im->value = cos (r) * sinh (i);
  }
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long csin = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_sin_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  csin_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC ccos = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_cos_complex (NODE_T * p)
{
  A68_REAL *re, *im;
  im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (im->value == 0.0) {
    re->value = cos (re->value);
    im->value = 0.0;
  } else {
    double r = re->value, i = im->value;
    re->value = cos (r) * cosh (i);
    im->value = sin (r) * sinh (-i);
  }
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long ccos = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_cos_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  ccos_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC ctan = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_tan_complex (NODE_T * p)
{
  A68_REAL *im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  A68_REAL *re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  double r, i;
  A68_REAL u, v;
  RESET_ERRNO;
  r = re->value;
  i = im->value;
  PUSH_REAL (p, r);
  PUSH_REAL (p, i);
  genie_sin_complex (p);
  POP_REAL (p, &v);
  POP_REAL (p, &u);
  PUSH_REAL (p, r);
  PUSH_REAL (p, i);
  genie_cos_complex (p);
  re->value = u.value;
  im->value = v.value;
  genie_div_complex (p);
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long ctan = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_tan_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  ctan_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC carcsin= (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
\return
**/

void genie_arcsin_complex (NODE_T * p)
{
  A68_REAL *re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  A68_REAL *im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (im == 0) {
    re->value = asin (re->value);
  } else {
    double r = re->value, i = im->value;
    double u = a68g_hypot (r + 1, i), v = a68g_hypot (r - 1, i);
    double a = 0.5 * (u + v), b = 0.5 * (u - v);
    re->value = asin (b);
    im->value = log (a + sqrt (a * a - 1));
  }
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long arcsin = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_asin_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  RESET_ERRNO;
  casin_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC carccos = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_arccos_complex (NODE_T * p)
{
  A68_REAL *re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  A68_REAL *im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (im == 0) {
    re->value = acos (re->value);
  } else {
    double r = re->value, i = im->value;
    double u = a68g_hypot (r + 1, i), v = a68g_hypot (r - 1, i);
    double a = 0.5 * (u + v), b = 0.5 * (u - v);
    re->value = acos (b);
    im->value = -log (a + sqrt (a * a - 1));
  }
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long carccos = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_acos_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  RESET_ERRNO;
  cacos_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}

/*!
\brief PROC carctan = (COMPLEX) COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_arctan_complex (NODE_T * p)
{
  A68_REAL *re = (A68_REAL *) (STACK_OFFSET (-2 * SIZE_OF (A68_REAL)));
  A68_REAL *im = (A68_REAL *) (STACK_OFFSET (-SIZE_OF (A68_REAL)));
  RESET_ERRNO;
  if (im == 0) {
    re->value = atan (re->value);
  } else {
    double r = re->value, i = im->value;
    double a = a68g_hypot (r, i + 1), b = a68g_hypot (r, i - 1);
    re->value = 0.5 * atan (2 * r / (1 - r * r - i * i));
    im->value = 0.5 * log (a / b);
  }
  math_rte (p, errno != 0, MODE (REAL), NULL);
}

/*!
\brief PROC long catan = (LONG COMPLEX) LONG COMPLEX
\param p position in syntax tree, should not be NULL
**/

void genie_atan_long_complex (NODE_T * p)
{
  MOID_T *mode = MOID (PACK (MOID (p)));
  int digits = get_mp_digits (mode), size = get_mp_size (mode);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re = (MP_DIGIT_T *) STACK_OFFSET (-2 * size);
  MP_DIGIT_T *im = (MP_DIGIT_T *) STACK_OFFSET (-size);
  RESET_ERRNO;
  catan_mp (p, re, im, digits);
  stack_pointer = pop_sp;
  MP_STATUS (re) = INITIALISED_MASK;
  MP_STATUS (im) = INITIALISED_MASK;
  math_rte (p, errno != 0, mode, NULL);
}
