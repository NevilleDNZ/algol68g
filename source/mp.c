/*!
\file mp.c
\brief multiprecision arithmetic library
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
A MULTIPRECISION ARITHMETIC LIBRARY FOR ALGOL68G

The question that is often raised is what applications justify multiprecision
calculations. A number of applications, some of them practical, have surfaced
over the years.

Most common application of multiprecision calculations are numerically unstable
calculations, that require many significant digits to arrive at a reliable
result.

Multiprecision calculations are used in "experimental mathematics". An
increasingly important application of computers is in doing experiments on
mathematical systems, when such system is not easily, or not at all, tractable
by analysis.

One important area of applications is in pure mathematics. Although numerical
calculations cannot substitute a formal proof, calculations can be used to
explore conjectures and reject those that are not sound, before a lengthy
attempt at such proof is undertaken.

Multiprecision calculations are especially useful in the study of mathematical
constants. One of the oldest applications of multiprecision computation is to
explore whether the expansions of classical constants such as "pi", "e" or
"sqrt(2)" are random in some sense. For example, digits of "pi" have not shown
statistical anomalies even now that billions of digits have been calculated.

A practical application of multiprecision computation is the emerging
field of public-key cryptography, that has spawned much research into advanced
algorithms for factoring large integers.

An indirect application of multiprecision computation is integrity testing.
A unique feature of multiprecision calculations is that they are unforgiving
of hardware, program or compiler error. Even a single computational error will
almost certainly result in a completely incorrect outcome after a possibly
correct start.

The routines in this library follow algorithms as described in the
literature, notably

D.M. Smith, "Efficient Multiple-Precision Evaluation of Elementary Functions"
Mathematics of Computation 52 (1989) 131-134

D.M. Smith, "A Multiple-Precision Division Algorithm"
Mathematics of Computation 66 (1996) 157-163

There are multiprecision libraries (freely) available, but this one is
particularly designed to work with Algol68G. It implements following modes:

   LONG INT, LONG REAL, LONG COMPLEX, LONG BITS
   LONG LONG INT, LONG LONG REAL, LONG LONG COMPLEX, LONG LONG BITS

Currently, LONG modes have a fixed precision, and LONG LONG modes have
user-definable precision. Precisions span about 30 decimal digits for
LONG modes up to (default) about 60 decimal digits for LONG LONG modes, a
range that is said to be adequate for most multiprecision applications.

Although the maximum length of a mp number is unbound, this implementation
is not particularly designed for more than about a thousand digits. It will
work at higher precisions, but with a performance penalty with respect to
state of the art implementations that may for instance use convolution for
multiplication.

This library takes a sloppy approach towards LONG INT and LONG BITS which are
implemented as LONG REAL and truncated where appropriate. This keeps the code
short at the penalty of some performance loss.

As is common practice, mp numbers are represented by a row of digits
in a large base. Layout of a mp number "z" is like this:

   MP_DIGIT_T *z;
   MP_STATUS (z)        Status word
   MP_EXPONENT (z)      Exponent with base MP_RADIX
   MP_DIGIT (z, 1 .. N) Digits 1 .. N

Note that this library assumes IEEE 754 compatible implementation of
type "double". It also assumes 32 (or 64) bit type "int".

Most "vintage" multiple precision libraries stored numbers as [] int.
However, since division and multiplication are O (N ** 2) operations, it is
advantageous to keep the base as high as possible. Modern computers handle
doubles at similar or better speed as integers, therefore this library
opts for storing numbers as [] double, trading space for speed. This may
change when 64 bit integers become commonplace.

Set a base such that "base ** 2" can be exactly represented by "double".
To facilitate transput, we require a base that is a power of 10.

If we choose the base right then in multiplication and division we do not need
to normalise intermediate results at each step since a number of additions
can be made before overflow occurs. That is why we specify "MAX_REPR_INT".

Mind that the precision of a mp number is at worst just
(LONG_MP_DIGITS - 1) * LOG_MP_BASE + 1, since the most significant mp digit
is also in range [0 .. MP_RADIX>. Do not specify less than 2 digits.

Since this software is distributed without any warranty, it is your
responsibility to validate the behaviour of the routines and their accuracy
using the source code provided. See the GNU General Public License for details.
*/

#include "algol68g.h"
#include "genie.h"
#include "gsl.h"
#include "mp.h"

/* If DOUBLE_PRECISION is defined functions are evaluated in double precision. */

#undef DOUBLE_PRECISION

/* Internal mp constants. */

static MP_DIGIT_T *ref_mp_pi = NULL;
static int mp_pi_size = -1;

static MP_DIGIT_T *ref_mp_ln_scale = NULL;
static int mp_ln_scale_size = -1;

static MP_DIGIT_T *ref_mp_ln_10 = NULL;
static int mp_ln_10_size = -1;

int varying_mp_digits = 9;

static int _j1_, _j2_;
#define MINIMUM(x, y) (_j1_ = (x), _j2_ = (y), _j1_ < _j2_ ? _j1_ : _j2_)

/*
MP_GUARDS: number of guard digits.

In calculations using intermediate results we will use guard digits.
We follow D.M. Smith in his recommendations for precisions greater than LONG.
*/

#ifdef DOUBLE_PRECISION
#define MP_GUARDS(digits) (digits)
#else
#define MP_GUARDS(digits) (((digits) == LONG_MP_DIGITS) ? 2 : (LOG_MP_BASE <= 5) ? 3 : 2)
#endif

/*!
\brief length in bytes of a long mp number
\return length in bytes of a long mp number
**/

size_t size_long_mp ()
{
  return (SIZE_MP (LONG_MP_DIGITS));
}

/*!
\brief length in digits of a long mp number
\return length in digits of a long mp number
**/

int long_mp_digits ()
{
  return (LONG_MP_DIGITS);
}

/*!
\brief length in bytes of a long long mp number
\return length in bytes of a long long mp number
**/

size_t size_longlong_mp ()
{
  return (SIZE_MP (varying_mp_digits));
}

/*!
\brief length in digits of a long long mp number
\return length in digits of a long long mp number
**/

int longlong_mp_digits ()
{
  return (varying_mp_digits);
}

/*!
\brief length in digits of mode
\param m
\return length in digits of mode m
**/

int get_mp_digits (MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_REAL) || m == MODE (LONG_COMPLEX) || m == MODE (LONG_BITS)) {
    return (long_mp_digits ());
  } else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_REAL) || m == MODE (LONGLONG_COMPLEX) || m == MODE (LONGLONG_BITS)) {
    return (longlong_mp_digits ());
  }
  return (0);
}

/*!
\brief length in bytes of mode
\param m
\return length in bytes of mode m
**/

int get_mp_size (MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_REAL) || m == MODE (LONG_COMPLEX) || m == MODE (LONG_BITS)) {
    return (size_long_mp ());
  } else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_REAL) || m == MODE (LONGLONG_COMPLEX) || m == MODE (LONGLONG_BITS)) {
    return (size_longlong_mp ());
  }
  return (0);
}

/*!
\brief length in bits of mode
\param m
\return length in bits of mode m
**/

int get_mp_bits_width (MOID_T * m)
{
  if (m == MODE (LONG_BITS)) {
    return (MP_BITS_WIDTH (LONG_MP_DIGITS));
  } else if (m == MODE (LONGLONG_BITS)) {
    return (MP_BITS_WIDTH (varying_mp_digits));
  }
  return (0);
}

/*!
\brief length in words of mode
\param m
\return length in words of mode m
**/

int get_mp_bits_words (MOID_T * m)
{
  if (m == MODE (LONG_BITS)) {
    return (MP_BITS_WORDS (LONG_MP_DIGITS));
  } else if (m == MODE (LONGLONG_BITS)) {
    return (MP_BITS_WORDS (varying_mp_digits));
  }
  return (0);
}

/*!
\brief whether z is a valid LONG INT
\param z
\return
**/

BOOL_T check_long_int (MP_DIGIT_T * z)
{
  return (MP_EXPONENT (z) >= 0 && MP_EXPONENT (z) < LONG_MP_DIGITS);
}

/*!
\brief whether z is a valid LONG LONG INT
\param z
\return
**/

BOOL_T check_longlong_int (MP_DIGIT_T * z)
{
  return (MP_EXPONENT (z) >= 0 && MP_EXPONENT (z) < varying_mp_digits);
}

/*!
\brief whether z is a valid representation for its mode
\param z
\param m
\return
**/

BOOL_T check_mp_int (MP_DIGIT_T * z, MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_BITS)) {
    return (check_long_int (z));
  } else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_BITS)) {
    return (check_longlong_int (z));
  }
  return (A_FALSE);
}

/*!
\brief convert precision to digits for long long number
\param n
\return
**/

int int_to_mp_digits (int n)
{
  return (2 + (int) ceil ((double) n / (double) LOG_MP_BASE));
}

/*!
\brief set number of digits for long long numbers
\param n
**/

void set_longlong_mp_digits (int n)
{
  varying_mp_digits = n;
}

/*!
\brief set z to short value x * MP_RADIX ** x_expo
\param z
\param x
\param x_expo
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *set_mp_short (MP_DIGIT_T * z, MP_DIGIT_T x, int x_expo, int digits)
{
  MP_DIGIT_T *d = &MP_DIGIT ((z), 2);
  MP_STATUS (z) = INITIALISED_MASK;
  MP_EXPONENT (z) = x_expo;
  MP_DIGIT (z, 1) = x;
  while (--digits) {
    *d++ = 0.0;
  }
  return (z);
}

/*!
\brief test whether x = y
\param p position in syntax tree, should not be NULL
\param x
\param y
\param digits precision as number of MP_DIGITs
\return
**/

static BOOL_T same_mp (NODE_T * p, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  int k;
  (void) p;
  if (MP_EXPONENT (x) == MP_EXPONENT (y)) {
    for (k = digits; k >= 1; k--) {
      if (MP_DIGIT (x, k) != MP_DIGIT (y, k)) {
	return (A_FALSE);
      }
    }
    return (A_TRUE);
  } else {
    return (A_FALSE);
  }
}

/*!
\brief unformatted write of z to stdout
\param str
\param z
\param digits precision as number of MP_DIGITs
\return
**/

void raw_write_mp (char *str, MP_DIGIT_T * z, int digits)
{
  int i;
  printf ("\n%s", str);
  for (i = 1; i <= digits; i++) {
    printf (" %07d", (int) MP_DIGIT (z, i));
  }
  printf (" ^ %d", (int) MP_EXPONENT (z));
  printf (" status=%d", (int) MP_STATUS (z));
  fflush (stdout);
}

/*!
\brief align 10-base z in a MP_RADIX mantissa
\param z
\param expo
\param digits precision as number of MP_DIGITs
\return result z
**/

static MP_DIGIT_T *align_mp (MP_DIGIT_T * z, int *expo, int digits)
{
  int i, shift;
  if (*expo >= 0) {
    shift = LOG_MP_BASE - *expo % LOG_MP_BASE - 1;
    *expo /= LOG_MP_BASE;
  } else {
    shift = (-*expo - 1) % LOG_MP_BASE;
    *expo = (*expo + 1) / LOG_MP_BASE;
    (*expo)--;
  }
/* Now normalise "z" */
  for (i = 1; i <= shift; i++) {
    int j, carry = 0;
    for (j = 1; j <= digits; j++) {
      int k = (int) MP_DIGIT (z, j) % 10;
      MP_DIGIT (z, j) = (int) (MP_DIGIT (z, j) / 10) + carry * (MP_RADIX / 10);
      carry = k;
    }
  }
  return (z);
}

/*!
\brief transform string into multi-precision number
\param p position in syntax tree, should not be NULL
\param z
\param s
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *string_to_mp (NODE_T * p, MP_DIGIT_T * z, char *s, int digits)
{
  int i, j, sign, weight, sum, comma, power;
  int expo;
  BOOL_T ok = A_TRUE;
  RESET_ERRNO;
  SET_MP_ZERO (z, digits);
  while (IS_SPACE (s[0])) {
    s++;
  }
/* Get the sign	*/
  sign = (s[0] == '-' ? -1 : 1);
  if (s[0] == '+' || s[0] == '-') {
    s++;
  }
/* Scan mantissa digits and put them into "z". */
  while (s[0] == '0') {
    s++;
  }
  i = 0;
  j = 1;
  sum = 0;
  comma = -1;
  power = 0;
  weight = MP_RADIX / 10;
  while (s[i] != '\0' && j <= digits && (IS_DIGIT (s[i]) || s[i] == '.')) {
    if (s[i] == '.') {
      comma = i;
    } else {
      int value = (int) s[i] - (int) '0';
      sum += weight * value;
      weight /= 10;
      power++;
      if (weight < 1) {
	MP_DIGIT (z, j++) = sum;
	sum = 0;
	weight = MP_RADIX / 10;
      }
    }
    i++;
  }
/* Store the last digits. */
  if (j <= digits) {
    MP_DIGIT (z, j++) = sum;
  }
/* See if there is an exponent. */
  expo = 0;
  if (s[i] != '\0' && TO_UPPER (s[i]) == EXPONENT_CHAR) {
    char *end;
    expo = strtol (&(s[++i]), &end, 10);
    ok = (end[0] == '\0');
  } else {
    ok = (s[i] == '\0');
  }
/* Calculate effective exponent. */
  expo += (comma >= 0 ? comma - 1 : power - 1);
  align_mp (z, &expo, digits);
  MP_EXPONENT (z) = (MP_DIGIT (z, 1) == 0 ? 0 : (double) expo);
  MP_DIGIT (z, 1) *= sign;
  CHECK_MP_EXPONENT (p, z, "conversion");
  if (errno == 0 && ok) {
    return (z);
  } else {
    return (NULL);
  }
}

/*!
\brief convert integer to multi-precison number
\param p position in syntax tree, should not be NULL
\param z
\param k
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *int_to_mp (NODE_T * p, MP_DIGIT_T * z, int k, int digits)
{
  int n = 0, j, sign_k = (k >= 0 ? 1 : -1);
  int k2 = k;
  if (k < 0) {
    k = -k;
  }
  while ((k2 /= MP_RADIX) != 0) {
    n++;
  }
  SET_MP_ZERO (z, digits);
  MP_EXPONENT (z) = n;
  for (j = 1 + n; j >= 1; j--) {
    MP_DIGIT (z, j) = k % MP_RADIX;
    k /= MP_RADIX;
  }
  MP_DIGIT (z, 1) = sign_k * MP_DIGIT (z, 1);
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*!
\brief convert unsigned to multi-precison number
\param p position in syntax tree, should not be NULL
\param z
\param k
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *unsigned_to_mp (NODE_T * p, MP_DIGIT_T * z, unsigned k, int digits)
{
  int n = 0, j;
  unsigned k2 = k;
  while ((k2 /= MP_RADIX) != 0) {
    n++;
  }
  SET_MP_ZERO (z, digits);
  MP_EXPONENT (z) = n;
  for (j = 1 + n; j >= 1; j--) {
    MP_DIGIT (z, j) = k % MP_RADIX;
    k /= MP_RADIX;
  }
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*!
\brief convert multi-precision number to integer
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

int mp_to_int (NODE_T * p, MP_DIGIT_T * z, int digits)
{
/*
This routines looks a lot like "strtol". We do not use "mp_to_real" since int
could be wider than 2 ** 52.
*/
  int j, expo = (int) MP_EXPONENT (z);
  int sum = 0, weight = 1;
  BOOL_T negative;
  if (expo >= digits) {
    diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MOID (p));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  negative = MP_DIGIT (z, 1) < 0;
  if (negative) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
  for (j = 1 + expo; j >= 1; j--) {
    int term;
    if ((int) MP_DIGIT (z, j) > MAX_INT / weight) {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
    term = (int) MP_DIGIT (z, j) * weight;
    if (sum > MAX_INT - term) {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (INT));
      exit_genie (p, A_RUNTIME_ERROR);
    }
    sum += term;
    weight *= MP_RADIX;
  }
  return (negative ? -sum : sum);
}

/*!
\brief convert multi-precision number to unsigned
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

unsigned mp_to_unsigned (NODE_T * p, MP_DIGIT_T * z, int digits)
{
/*
This routines looks a lot like "strtol". We do not use "mp_to_real" since int
could be wider than 2 ** 52.
*/
  int j, expo = (int) MP_EXPONENT (z);
  unsigned sum = 0, weight = 1;
  if (expo >= digits) {
    diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MOID (p));
    exit_genie (p, A_RUNTIME_ERROR);
  }
  for (j = 1 + expo; j >= 1; j--) {
    unsigned term;
    if ((unsigned) MP_DIGIT (z, j) > MAX_UNT / weight) {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BITS));
      exit_genie (p, A_RUNTIME_ERROR);
    }
    term = (unsigned) MP_DIGIT (z, j) * weight;
    if (sum > MAX_UNT - term) {
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, MODE (BITS));
      exit_genie (p, A_RUNTIME_ERROR);
    }
    sum += term;
    weight *= MP_RADIX;
  }
  return (sum);
}

/*!
\brief convert double to multi-precison number
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *real_to_mp (NODE_T * p, MP_DIGIT_T * z, double x, int digits)
{
  int j, k, sign_x, sum, weight;
  int expo;
  double a;
  MP_DIGIT_T *u;
  SET_MP_ZERO (z, digits);
  if (x == 0.0) {
    return (z);
  }
/* Small integers can be done better by int_to_mp. */
  if (ABS (x) < MP_RADIX && (double) (int) x == x) {
    return (int_to_mp (p, z, (int) x, digits));
  }
  sign_x = (x > 0 ? 1 : -1);
/* Scale to [0, 0.1>. */
  a = x = ABS (x);
  expo = (int) log10 (a);
  a /= ten_to_the_power (expo);
  expo--;
  if (a >= 1) {
    a /= 10;
    expo++;
  }
/* Transport digits of x to the mantissa of z. */
  k = 0;
  j = 1;
  sum = 0;
  weight = (MP_RADIX / 10);
  u = &MP_DIGIT (z, 1);
  while (j <= digits && k < DBL_DIG) {
    double y = floor (a * 10);
    int value = (int) y;
    a = a * 10 - y;
    sum += weight * value;
    weight /= 10;
    if (weight < 1) {
      (u++)[0] = sum;
      sum = 0;
      weight = (MP_RADIX / 10);
    }
    k++;
  }
/* Store the last digits. */
  if (j <= digits) {
    u[0] = sum;
  }
  align_mp (z, &expo, digits);
  MP_EXPONENT (z) = expo;
  MP_DIGIT (z, 1) *= sign_x;
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*!
\brief convert multi-precision number to double
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

double mp_to_real (NODE_T * p, MP_DIGIT_T * z, int digits)
{
/* This routine looks a lot like "strtod". */
  (void) p;
  if (MP_EXPONENT (z) * LOG_MP_BASE <= DBL_MIN_10_EXP) {
    return (0);
  } else {
    int j;
    double sum = 0, weight;
    weight = ten_to_the_power ((int) (MP_EXPONENT (z) * LOG_MP_BASE));
    for (j = 1; j <= digits && (j - 2) * LOG_MP_BASE <= DBL_DIG; j++) {
      sum += ABS (MP_DIGIT (z, j)) * weight;
      weight /= MP_RADIX;
    }
    TEST_REAL_REPRESENTATION (p, sum);
    return (MP_DIGIT (z, 1) >= 0 ? sum : -sum);
  }
}

/*!
\brief convert z to a row of unsigned in the stack
\param p position in syntax tree, should not be NULL
\param z
\param m
\return result z
**/

unsigned *stack_mp_bits (NODE_T * p, MP_DIGIT_T * z, MOID_T * m)
{
  int digits = get_mp_digits (m), words = get_mp_bits_words (m), k, lim;
  unsigned *row, mask;
  MP_DIGIT_T *u, *v, *w;
  row = (unsigned *) STACK_ADDRESS (stack_pointer);
  INCREMENT_STACK_POINTER (p, words * SIZE_OF (unsigned));
  STACK_MP (u, p, digits);
  STACK_MP (v, p, digits);
  STACK_MP (w, p, digits);
  MOVE_MP (u, z, digits);
/* Argument check. */
  if (MP_DIGIT (u, 1) < 0.0) {
    errno = EDOM;
    diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, (m == MODE (LONG_BITS) ? MODE (LONG_INT) : MODE (LONGLONG_INT)));
    exit_genie (p, A_RUNTIME_ERROR);
  }
/* Convert radix MP_BITS_RADIX number. */
  for (k = words - 1; k >= 0; k--) {
    MOVE_MP (w, u, digits);
    over_mp_digit (p, u, u, MP_BITS_RADIX, digits);
    mul_mp_digit (p, v, u, MP_BITS_RADIX, digits);
    sub_mp (p, v, w, v, digits);
    row[k] = (int) MP_DIGIT (v, 1);
  }
/* Test on overflow: too many bits or not reduced to 0. */
  mask = 0x1;
  lim = get_mp_bits_width (m) % MP_BITS_BITS;
  for (k = 1; k < lim; k++) {
    mask <<= 1;
    mask |= 0x1;
  }
  if ((row[0] & ~mask) != 0x0 || MP_DIGIT (u, 1) != 0.0) {
    errno = ERANGE;
    diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, m);
    exit_genie (p, A_RUNTIME_ERROR);
  }
/* Exit. */
  return (row);
}

/*!
\brief whether LONG BITS value is in range
\param p position in syntax tree, should not be NULL
\param u
\param m
**/

void check_long_bits_value (NODE_T * p, MP_DIGIT_T * u, MOID_T * m)
{
  if (MP_EXPONENT (u) >= get_mp_digits (m) - 1) {
    ADDR_T pop_sp = stack_pointer;
    stack_mp_bits (p, u, m);
    stack_pointer = pop_sp;
  }
}

/*!
\brief convert row of unsigned to LONG BITS
\param p position in syntax tree, should not be NULL
\param u
\param row
\param m
\return result z
**/

MP_DIGIT_T *pack_mp_bits (NODE_T * p, MP_DIGIT_T * u, unsigned *row, MOID_T * m)
{
  int digits = get_mp_digits (m), words = get_mp_bits_words (m), k, lim;
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *v, *w;
/* Discard excess bits. */
  unsigned mask = 0x1, musk = 0x0;
  STACK_MP (v, p, digits);
  STACK_MP (w, p, digits);
  lim = get_mp_bits_width (m) % MP_BITS_BITS;
  for (k = 1; k < lim; k++) {
    mask <<= 1;
    mask |= 0x1;
  }
  row[0] &= mask;
  for (k = 1; k < (BITS_WIDTH - MP_BITS_BITS); k++) {
    musk <<= 1;
  }
  for (k = 0; k < MP_BITS_BITS; k++) {
    musk <<= 1;
    musk |= 0x1;
  }
/* Convert. */
  SET_MP_ZERO (u, digits);
  set_mp_short (v, (MP_DIGIT_T) 1, 0, digits);
  for (k = words - 1; k >= 0; k--) {
    mul_mp_digit (p, w, v, (MP_DIGIT_T) (musk & row[k]), digits);
    add_mp (p, u, u, w, digits);
    if (k != 0) {
      mul_mp_digit (p, v, v, MP_BITS_RADIX, digits);
    }
  }
  stack_pointer = pop_sp;
  return (u);
}

/*!
\brief normalise positive intermediate
\param w argument
\param k
\param digits precision as number of MP_DIGITs
**/

static void norm_mp_light (MP_DIGIT_T * w, int k, int digits)
{
/* Bring every digit back to [0 .. MP_RADIX>. */
  int j;
  MP_DIGIT_T *z;
  for (j = digits, z = &MP_DIGIT (w, digits); j >= k; j--, z--) {
    if (z[0] >= MP_RADIX) {
      z[0] -= (MP_DIGIT_T) MP_RADIX;
      z[-1] += 1;
    } else if (z[0] < 0) {
      z[0] += (MP_DIGIT_T) MP_RADIX;
      z[-1] -= 1;
    }
  }
}

/*!
\brief normalise positive intermediate
\param w argument
\param k
\param digits precision as number of MP_DIGITs
**/

static void norm_mp (MP_DIGIT_T * w, int k, int digits)
{
/* Bring every digit back to [0 .. MP_RADIX>. */
  int j;
  MP_DIGIT_T *z;
  for (j = digits, z = &MP_DIGIT (w, digits); j >= k; j--, z--) {
    if (z[0] >= MP_RADIX) {
      MP_DIGIT_T carry = (int) (z[0] / (MP_DIGIT_T) MP_RADIX);
      z[0] -= carry * (MP_DIGIT_T) MP_RADIX;
      z[-1] += carry;
    } else if (z[0] < 0) {
      MP_DIGIT_T carry = 1 + (int) ((-z[0] - 1) / (MP_DIGIT_T) MP_RADIX);
      z[0] += carry * (MP_DIGIT_T) MP_RADIX;
      z[-1] -= carry;
    }
  }
}

/*!
\brief round multi-precision number
\param z result
\param w argument, must be positive
\param digits precision as number of MP_DIGITs
**/

static void round_mp (MP_DIGIT_T * z, MP_DIGIT_T * w, int digits)
{
/* Assume that w has precision of at least 2 + digits. */
  int last = (MP_DIGIT (w, 1) == 0 ? 2 + digits : 1 + digits);
  if (MP_DIGIT (w, last) >= MP_RADIX / 2) {
    MP_DIGIT (w, last - 1) += 1;
  }
  if (MP_DIGIT (w, last - 1) >= MP_RADIX) {
    norm_mp (w, 2, last);
  }
  if (MP_DIGIT (w, 1) == 0) {
    MOVE_DIGITS (&MP_DIGIT (z, 1), &MP_DIGIT (w, 2), digits);
    MP_EXPONENT (z) = MP_EXPONENT (w) - 1;
  } else {
/* Normally z != w, so no test on this. */
    MOVE_DIGITS (&MP_EXPONENT (z), &MP_EXPONENT (w), (1 + digits));
  }
/* Zero is zero is zero. */
  if (MP_DIGIT (z, 1) == 0) {
    MP_EXPONENT (z) = 0;
  }
}

/*!
\brief truncate at decimal point
\param p position in syntax tree, should not be NULL
\param z result
\param x argument
\param digits precision as number of MP_DIGITs
**/

void trunc_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  if (MP_EXPONENT (x) < 0) {
    SET_MP_ZERO (z, digits);
  } else if (MP_EXPONENT (x) >= digits) {
    errno = EDOM;
    diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, (WHETHER (MOID (p), PROC_SYMBOL) ? SUB (MOID (p)) : MOID (p)));
    exit_genie (p, A_RUNTIME_ERROR);
  } else {
    int k;
    MOVE_MP (z, x, digits);
    for (k = (int) (MP_EXPONENT (x) + 2); k <= digits; k++) {
      MP_DIGIT (z, k) = 0;
    }
  }
}

/*!
\brief shorten and round
\param p position in syntax tree, should not be NULL
\param z result
\param digits precision as number of MP_DIGITs
\param x
\param digits precision as number of MP_DIGITs_x
\return result z
**/

MP_DIGIT_T *shorten_mp (NODE_T * p, MP_DIGIT_T * z, int digits, MP_DIGIT_T * x, int digits_x)
{
  if (digits >= digits_x) {
    errno = EDOM;
    return (NULL);
  } else {
/* Reserve extra digits for proper rounding. */
    int pop_sp = stack_pointer, digits_h = digits + 2;
    MP_DIGIT_T *w;
    BOOL_T negative = MP_DIGIT (x, 1) < 0;
    STACK_MP (w, p, digits_h);
    if (negative) {
      MP_DIGIT (x, 1) = -MP_DIGIT (x, 1);
    }
    MP_STATUS (w) = 0;
    MP_EXPONENT (w) = MP_EXPONENT (x) + 1;
    MP_DIGIT (w, 1) = 0;
    MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits + 1);
    round_mp (z, w, digits);
    if (negative) {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
    stack_pointer = pop_sp;
    return (z);
  }
}

/*!
\brief lengthen x and assign to z
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs_z
\param x
\param digits precision as number of MP_DIGITs_x
\return result z
**/

MP_DIGIT_T *lengthen_mp (NODE_T * p, MP_DIGIT_T * z, int digits_z, MP_DIGIT_T * x, int digits_x)
{
  int j;
  (void) p;
  if (digits_z > digits_x) {
    if (z != x) {
      MOVE_DIGITS (&MP_DIGIT (z, 1), &MP_DIGIT (x, 1), digits_x);
      MP_EXPONENT (z) = MP_EXPONENT (x);
      MP_STATUS (z) = MP_STATUS (x);
    }
    for (j = 1 + digits_x; j <= digits_z; j++) {
      MP_DIGIT (z, j) = 0;
    }
  }
  return (z);
}

/*!
\brief set z to the sum of positive x and positive y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *add_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
/* Trivial cases. */
  if (MP_DIGIT (x, 1) == 0) {
    MOVE_MP (z, y, digits);
    return (z);
  } else if (MP_DIGIT (y, 1) == 0) {
    MOVE_MP (z, x, digits);
    return (z);
  }
/* We want positive arguments. */
  MP_DIGIT (x, 1) = ABS (x1);
  MP_DIGIT (y, 1) = ABS (y1);
  if (x1 >= 0 && y1 < 0) {
    sub_mp (p, z, x, y, digits);
  } else if (x1 < 0 && y1 >= 0) {
    sub_mp (p, z, y, x, digits);
  } else if (x1 < 0 && y1 < 0) {
    add_mp (p, z, x, y, digits);
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  } else {
/* Add. */
    int j, digits_h = 2 + digits;
    STACK_MP (w, p, digits_h);
    MP_DIGIT (w, 1) = 0;
    if (MP_EXPONENT (x) == MP_EXPONENT (y)) {
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (x);
      for (j = 1; j <= digits; j++) {
	MP_DIGIT (w, j + 1) = MP_DIGIT (x, j) + MP_DIGIT (y, j);
      }
      MP_DIGIT (w, digits_h) = 0;
    } else if (MP_EXPONENT (x) > MP_EXPONENT (y)) {
      int shl_y = (int) MP_EXPONENT (x) - (int) MP_EXPONENT (y);
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (x);
      for (j = 1; j < digits_h; j++) {
	int i_y = j - (int) shl_y;
	MP_DIGIT_T x_j = (j > digits ? 0 : MP_DIGIT (x, j));
	MP_DIGIT_T y_j = (i_y <= 0 || i_y > digits ? 0 : MP_DIGIT (y, i_y));
	MP_DIGIT (w, j + 1) = x_j + y_j;
      }
    } else {
      int shl_x = (int) MP_EXPONENT (y) - (int) MP_EXPONENT (x);
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (y);
      for (j = 1; j < digits_h; j++) {
	int i_x = j - (int) shl_x;
	MP_DIGIT_T x_j = (i_x <= 0 || i_x > digits ? 0 : MP_DIGIT (x, i_x));
	MP_DIGIT_T y_j = (j > digits ? 0 : MP_DIGIT (y, j));
	MP_DIGIT (w, j + 1) = x_j + y_j;
      }
    }
    norm_mp_light (w, 2, digits_h);
    round_mp (z, w, digits);
    CHECK_MP_EXPONENT (p, z, "addition");
  }
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1;		/* In case z IS x OR z IS y. */
  return (z);
}

/*!
\brief set z to the difference of positive x and positive y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *sub_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int fnz, k;
  MP_DIGIT_T *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  BOOL_T negative = A_FALSE;
/* Trivial cases. */
  if (MP_DIGIT (x, 1) == 0) {
    MOVE_MP (z, y, digits);
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    return (z);
  } else if (MP_DIGIT (y, 1) == 0) {
    MOVE_MP (z, x, digits);
    return (z);
  }
  MP_DIGIT (x, 1) = ABS (x1);
  MP_DIGIT (y, 1) = ABS (y1);
/* We want positive arguments. */
  if (x1 >= 0 && y1 < 0) {
    add_mp (p, z, x, y, digits);
  } else if (x1 < 0 && y1 >= 0) {
    add_mp (p, z, y, x, digits);
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  } else if (x1 < 0 && y1 < 0) {
    sub_mp (p, z, y, x, digits);
  } else {
/* Subtract. */
    int j, digits_h = 2 + digits;
    STACK_MP (w, p, digits_h);
    MP_DIGIT (w, 1) = 0;
    if (MP_EXPONENT (x) == MP_EXPONENT (y)) {
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (x);
      for (j = 1; j <= digits; j++) {
	MP_DIGIT (w, j + 1) = MP_DIGIT (x, j) - MP_DIGIT (y, j);
      }
      MP_DIGIT (w, digits_h) = 0;
    } else if (MP_EXPONENT (x) > MP_EXPONENT (y)) {
      int shl_y = (int) MP_EXPONENT (x) - (int) MP_EXPONENT (y);
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (x);
      for (j = 1; j < digits_h; j++) {
	int i_y = j - (int) shl_y;
	MP_DIGIT_T x_j = (j > digits ? 0 : MP_DIGIT (x, j));
	MP_DIGIT_T y_j = (i_y <= 0 || i_y > digits ? 0 : MP_DIGIT (y, i_y));
	MP_DIGIT (w, j + 1) = x_j - y_j;
      }
    } else {
      int shl_x = (int) MP_EXPONENT (y) - (int) MP_EXPONENT (x);
      MP_EXPONENT (w) = 1 + (int) MP_EXPONENT (y);
      for (j = 1; j < digits_h; j++) {
	int i_x = j - (int) shl_x;
	MP_DIGIT_T x_j = (i_x <= 0 || i_x > digits ? 0 : MP_DIGIT (x, i_x));
	MP_DIGIT_T y_j = (j > digits ? 0 : MP_DIGIT (y, j));
	MP_DIGIT (w, j + 1) = x_j - y_j;
      }
    }
/* Correct if we subtract large from small. */
    if (MP_DIGIT (w, 2) <= 0) {
      fnz = -1;
      for (j = 2; j <= digits_h && fnz < 0; j++) {
	if (MP_DIGIT (w, j) != 0) {
	  fnz = j;
	}
      }
      negative = (MP_DIGIT (w, fnz) < 0);
      if (negative) {
	for (j = fnz; j <= digits_h; j++) {
	  MP_DIGIT (w, j) = -MP_DIGIT (w, j);
	}
      }
    }
/* Normalise. */
    norm_mp_light (w, 2, digits_h);
    fnz = -1;
    for (j = 1; j <= digits_h && fnz < 0; j++) {
      if (MP_DIGIT (w, j) != 0) {
	fnz = j;
      }
    }
    if (fnz > 1) {
      int j = fnz - 1;
      for (k = 1; k <= digits_h - j; k++) {
	MP_DIGIT (w, k) = MP_DIGIT (w, k + j);
	MP_DIGIT (w, k + j) = 0;
      }
      MP_EXPONENT (w) -= j;
    }
/* Round. */
    round_mp (z, w, digits);
    if (negative) {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
    CHECK_MP_EXPONENT (p, z, "subtraction");
  }
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1;		/* In case z IS x OR z IS y. */
  return (z);
}

/*!
\brief set z to the product of x and y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mul_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  MP_DIGIT_T *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  int i, oflow, digits_h = 2 + digits;
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT (x, 1) = ABS (x1);
  MP_DIGIT (y, 1) = ABS (y1);
  STACK_MP (w, p, digits_h);
  SET_MP_ZERO (w, digits_h);
  MP_EXPONENT (w) = MP_EXPONENT (x) + MP_EXPONENT (y) + 1;
/* Calculate z = x * y. */
  oflow = (int) (floor) ((double) MAX_REPR_INT / (2 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
  ABNORMAL_END (oflow <= 1, "inadequate MP_RADIX", NULL);
  if (digits < oflow) {
    for (i = digits; i >= 1; i--) {
      MP_DIGIT_T yi = MP_DIGIT (y, i);
      if (yi != 0) {
	int k = digits_h - i;
	int j = (k > digits ? digits : k);
	MP_DIGIT_T *u = &MP_DIGIT (w, i + j);
	MP_DIGIT_T *v = &MP_DIGIT (x, j);
	while (j-- >= 1) {
	  (u--)[0] += yi * (v--)[0];
	}
      }
    }
  } else {
    for (i = digits; i >= 1; i--) {
      MP_DIGIT_T yi = MP_DIGIT (y, i);
      if (yi != 0) {
	int k = digits_h - i;
	int j = (k > digits ? digits : k);
	MP_DIGIT_T *u = &MP_DIGIT (w, i + j);
	MP_DIGIT_T *v = &MP_DIGIT (x, j);
	if ((digits - i + 1) % oflow == 0) {
	  norm_mp (w, 2, digits_h);
	}
	while (j-- >= 1) {
	  (u--)[0] += yi * (v--)[0];
	}
      }
    }
  }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = ((x1 * y1) >= 0 ? z1 : -z1);
  CHECK_MP_EXPONENT (p, z, "multiplication");
  return (z);
}

/*!
\brief set z to the quotient of x and y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *div_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
/*
This routine is an implementation of

   D. M. Smith, "A Multiple-Precision Division Algorithm"
   Mathematics of Computation 66 (1996) 157-163.

This algorithm is O(N ** 2) but runs faster than straightforward methods by
skipping most of the intermediate normalisation and recovering from wrong
guesses without separate correction steps.
*/
  double xd;
  MP_DIGIT_T *t, *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  int k, oflow, digits_w = 4 + digits;
  ADDR_T pop_sp = stack_pointer;
  if (y1 == 0) {
    errno = ERANGE;
    return (NULL);
  }
/* Determine normalisation interval assuming that q < 2b in each step. */
  oflow = (int) (floor) ((double) MAX_REPR_INT / (3 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
  ABNORMAL_END (oflow <= 1, "inadequate MP_RADIX", NULL);
  MP_DIGIT (x, 1) = ABS (x1);
  MP_DIGIT (y, 1) = ABS (y1);
/* `w' will be the working nominator in which the quotient develops. */
  STACK_MP (w, p, digits_w);
  MP_EXPONENT (w) = MP_EXPONENT (x) - MP_EXPONENT (y);
  MP_DIGIT (w, 1) = 0;
  MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits);
  MP_DIGIT (w, digits + 2) = 0.0;
  MP_DIGIT (w, digits + 3) = 0.0;
  MP_DIGIT (w, digits + 4) = 0.0;
/* Estimate the denominator. Take four terms to also suit small MP_RADIX. */
  xd = (MP_DIGIT (y, 1) * MP_RADIX + MP_DIGIT (y, 2)) * MP_RADIX + MP_DIGIT (y, 3) + MP_DIGIT (y, 4) / MP_RADIX;
  t = &MP_DIGIT (w, 2);
  if (digits + 2 < oflow) {
    for (k = 1; k <= digits + 2; k++, t++) {
      double xn, q;
      int first = k + 2;
/* Estimate quotient digit. */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      if (q != 0) {
/* Correct the nominator. */
	int j, len = k + digits + 1;
	int lim = (len < digits_w ? len : digits_w);
	MP_DIGIT_T *u = t, *v = &MP_DIGIT (y, 1);
	for (j = first; j <= lim; j++) {
	  (u++)[0] -= q * (v++)[0];
	}
      }
      t[0] += (t[-1] * MP_RADIX);
      t[-1] = q;
    }
  } else {
    for (k = 1; k <= digits + 2; k++, t++) {
      double xn, q;
      int first = k + 2;
      if (k % oflow == 0) {
	norm_mp (w, first, digits_w);
      }
/* Estimate quotient digit. */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      if (q != 0) {
/* Correct the nominator. */
	int j, len = k + digits + 1;
	int lim = (len < digits_w ? len : digits_w);
	MP_DIGIT_T *u = t, *v = &MP_DIGIT (y, 1);
	for (j = first; j <= lim; j++) {
	  (u++)[0] -= q * (v++)[0];
	}
      }
      t[0] += (t[-1] * MP_RADIX);
      t[-1] = q;
    }
  }
  norm_mp (w, 2, digits_w);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = ((x1 * y1) >= 0 ? z1 : -z1);
  CHECK_MP_EXPONENT (p, z, "division");
  return (z);
}

/*!
\brief set z to the integer quotient of x and y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *over_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *y_g, *z_g;
  ADDR_T pop_sp = stack_pointer;
  if (MP_DIGIT (y, 1) == 0) {
    errno = ERANGE;
    return (NULL);
  }
  STACK_MP (x_g, p, digits_g);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  lengthen_mp (p, y_g, digits_g, y, digits);
  div_mp (p, z_g, x_g, y_g, digits_g);
  trunc_mp (p, z_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Restore and exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to x mod y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mod_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  int digits_g = digits + MP_GUARDS (digits);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *x_g, *y_g, *z_g;
  if (MP_DIGIT (y, 1) == 0) {
    errno = EDOM;
    return (NULL);
  }
  STACK_MP (x_g, p, digits_g);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  lengthen_mp (p, y_g, digits_g, y, digits);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* x mod y = x - y * trunc (x / y). */
  over_mp (p, z_g, x_g, y_g, digits_g);
  mul_mp (p, z_g, y_g, z_g, digits_g);
  sub_mp (p, z_g, x_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Restore and exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to the product of x and digit y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mul_mp_digit (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T y, int digits)
{
/* This is an O(N) routine for multiplication by a short value. */
  MP_DIGIT_T *w, z1, x1 = MP_DIGIT (x, 1), y1 = y, *u, *v;
  int j, digits_h = 2 + digits;
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT (x, 1) = ABS (x1);
  y = ABS (y1);
  STACK_MP (w, p, digits_h);
  SET_MP_ZERO (w, digits_h);
  MP_EXPONENT (w) = MP_EXPONENT (x) + 1;
  j = digits;
  u = &MP_DIGIT (w, 1 + digits);
  v = &MP_DIGIT (x, digits);
  while (j-- >= 1) {
    (u--)[0] += y * (v--)[0];
  }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = ((x1 * y1) >= 0 ? z1 : -z1);
  CHECK_MP_EXPONENT (p, z, "multiplication");
  return (z);
}

/*!
\brief set z to x/2
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *half_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  MP_DIGIT_T *w, z1, x1 = MP_DIGIT (x, 1), *u, *v;
  int j, digits_h = 2 + digits;
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT (x, 1) = ABS (x1);
  STACK_MP (w, p, digits_h);
  SET_MP_ZERO (w, digits_h);
/* Calculate x * 0.5. */
  MP_EXPONENT (w) = MP_EXPONENT (x);
  j = digits;
  u = &MP_DIGIT (w, 1 + digits);
  v = &MP_DIGIT (x, digits);
  while (j-- >= 1) {
    (u--)[0] += (MP_RADIX / 2) * (v--)[0];
  }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = (x1 >= 0 ? z1 : -z1);
  CHECK_MP_EXPONENT (p, z, "halving");
  return (z);
}

/*!
\brief set z to the quotient of x and digit y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *div_mp_digit (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T y, int digits)
{
  double xd;
  MP_DIGIT_T *t, *w, z1, x1 = MP_DIGIT (x, 1), y1 = y;
  int k, oflow, digits_w = 4 + digits;
  ADDR_T pop_sp = stack_pointer;
  if (y == 0) {
    errno = ERANGE;
    return (NULL);
  }
/* Determine normalisation interval assuming that q < 2b in each step. */
  oflow = (int) (floor) ((double) MAX_REPR_INT / (3 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
  ABNORMAL_END (oflow <= 1, "inadequate MP_RADIX", NULL);
/* Work with positive operands. */
  MP_DIGIT (x, 1) = ABS (x1);
  y = ABS (y1);
  STACK_MP (w, p, digits_w);
  MP_EXPONENT (w) = MP_EXPONENT (x);
  MP_DIGIT (w, 1) = 0;
  MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits);
  MP_DIGIT (w, digits + 2) = 0.0;
  MP_DIGIT (w, digits + 3) = 0.0;
  MP_DIGIT (w, digits + 4) = 0.0;
/* Estimate the denominator. */
  xd = (double) y *MP_RADIX * MP_RADIX;
  t = &MP_DIGIT (w, 2);
  if (digits + 2 < oflow) {
    for (k = 1; k <= digits + 2; k++, t++) {
      double xn, q;
      int first = k + 2;
/* Estimate quotient digit and correct. */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      t[0] += (t[-1] * MP_RADIX - q * y);
      t[-1] = q;
    }
  } else {
    for (k = 1; k <= digits + 2; k++, t++) {
      double xn, q;
      int first = k + 2;
      if (k % oflow == 0) {
	norm_mp (w, first, digits_w);
      }
/* Estimate quotient digit and correct. */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      t[0] += (t[-1] * MP_RADIX - q * y);
      t[-1] = q;
    }
  }
  norm_mp (w, 2, digits_w);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = ((x1 * y1) >= 0 ? z1 : -z1);
  CHECK_MP_EXPONENT (p, z, "division");
  return (z);
}

/*!
\brief set z to the integer quotient of x and y
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *over_mp_digit (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T y, int digits)
{
  int digits_g = digits + MP_GUARDS (digits);
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *x_g, *z_g;
  if (y == 0) {
    errno = ERANGE;
    return (NULL);
  }
  STACK_MP (x_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  div_mp_digit (p, z_g, x_g, y, digits_g);
  trunc_mp (p, z_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Restore and exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to the reciprocal of x
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *rec_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *one;
  if (MP_DIGIT (x, 1) == 0) {
    errno = ERANGE;
    return (NULL);
  }
  STACK_MP (one, p, digits);
  set_mp_short (one, (MP_DIGIT_T) 1, 0, digits);
  div_mp (p, z, one, x, digits);
/* Restore and exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to x ** n
\param p position in syntax tree, should not be NULL
\param z
\param x
\param n
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *pow_mp_int (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int n, int digits)
{
  int pop_sp = stack_pointer, bit, digits_g = digits + MP_GUARDS (digits);
  BOOL_T negative;
  MP_DIGIT_T *z_g, *x_g;
  STACK_MP (z_g, p, digits_g);
  STACK_MP (x_g, p, digits_g);
  set_mp_short (z_g, (MP_DIGIT_T) 1, 0, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  negative = (n < 0);
  if (negative) {
    n = -n;
  }
  bit = 1;
  while ((unsigned) bit <= (unsigned) n) {
    if (n & bit) {
      mul_mp (p, z_g, z_g, x_g, digits_g);
    }
    mul_mp (p, x_g, x_g, x_g, digits_g);
    bit *= 2;
  }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  if (negative) {
    rec_mp (p, z, z, digits);
  }
  CHECK_MP_EXPONENT (p, z, "power");
  return (z);
}

/*!
\brief test on |z| > 0.001 for argument reduction in "sin" and "exp"
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

static BOOL_T eps_mp (MP_DIGIT_T * z, int digits)
{
  if (MP_DIGIT (z, 1) == 0) {
    return (A_FALSE);
  } else if (MP_EXPONENT (z) > -1) {
    return (A_TRUE);
  } else if (MP_EXPONENT (z) < -1) {
    return (A_FALSE);
  } else {
#if (MP_RADIX == DEFAULT_MP_RADIX)
/* More or less optimised for LONG and default LONG LONG precisions. */
    return (digits <= 10 ? ABS (MP_DIGIT (z, 1)) > 100000 : ABS (MP_DIGIT (z, 1)) > 10000);
#else
    switch (LOG_MP_BASE) {
    case 3:
      {
	return (ABS (MP_DIGIT (z, 1)) > 1);
      }
    case 4:
      {
	return (ABS (MP_DIGIT (z, 1)) > 10);
      }
    case 5:
      {
	return (ABS (MP_DIGIT (z, 1)) > 100);
      }
    case 6:
      {
	return (ABS (MP_DIGIT (z, 1)) > 1000);
      }
    default:
      {
	ABNORMAL_END (A_TRUE, "unexpected mp base", "");
	return (A_FALSE);
      }
    }
#endif
  }
}

/*!
\brief set z to sqrt (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *sqrt_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int pop_sp = stack_pointer, digits_g = 2 * digits + MP_GUARDS (digits), digits_h;
  MP_DIGIT_T *tmp, *x_g, *z_g;
  BOOL_T reciprocal = A_FALSE;
  if (MP_DIGIT (x, 1) == 0) {
    stack_pointer = pop_sp;
    SET_MP_ZERO (z, digits);
    return (z);
  }
  if (MP_DIGIT (x, 1) < 0) {
    stack_pointer = pop_sp;
    errno = EDOM;
    return (NULL);
  }
  STACK_MP (z_g, p, digits_g);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (tmp, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* Scaling for small x; sqrt (x) = 1 / sqrt (1 / x) */
  if ((reciprocal = MP_EXPONENT (x_g) < 0) == A_TRUE) {
    rec_mp (p, x_g, x_g, digits_g);
  }
  if (ABS (MP_EXPONENT (x_g)) >= 2) {
/* For extreme arguments we want accurate results as well. */
    int expo = (int) MP_EXPONENT (x_g);
    MP_EXPONENT (x_g) = expo % 2;
    sqrt_mp (p, z_g, x_g, digits_g);
    MP_EXPONENT (z_g) += expo / 2;
  } else {
/* Argument is in range. Estimate the root as double. */
    int decimals;
    double x_d = mp_to_real (p, x_g, digits_g);
    real_to_mp (p, z_g, sqrt (x_d), digits_g);
/* Newton's method: x<n+1> = (x<n> + a / x<n>) / 2 */
    decimals = DOUBLE_ACCURACY;
    do {
      decimals <<= 1;
      digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
      div_mp (p, tmp, x_g, z_g, digits_h);
      add_mp (p, tmp, z_g, tmp, digits_h);
      half_mp (p, z_g, tmp, digits_h);
    }
    while (decimals < 2 * digits_g * LOG_MP_BASE);
  }
  if (reciprocal) {
    rec_mp (p, z_g, z_g, digits);
  }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to curt (x), the cube root
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *curt_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits), digits_h;
  MP_DIGIT_T *tmp, *x_g, *z_g;
  BOOL_T reciprocal = A_FALSE, change_sign = A_FALSE;
  if (MP_DIGIT (x, 1) == 0) {
    stack_pointer = pop_sp;
    SET_MP_ZERO (z, digits);
    return (z);
  }
  if (MP_DIGIT (x, 1) < 0) {
    change_sign = A_TRUE;
    MP_DIGIT (x, 1) = -MP_DIGIT (x, 1);
  }
  STACK_MP (z_g, p, digits_g);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (tmp, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* Scaling for small x; curt (x) = 1 / curt (1 / x) */
  if ((reciprocal = MP_EXPONENT (x_g) < 0) == A_TRUE) {
    rec_mp (p, x_g, x_g, digits_g);
  }
  if (ABS (MP_EXPONENT (x_g)) >= 3) {
/* For extreme arguments we want accurate results as well. */
    int expo = (int) MP_EXPONENT (x_g);
    MP_EXPONENT (x_g) = expo % 3;
    curt_mp (p, z_g, x_g, digits_g);
    MP_EXPONENT (z_g) += expo / 3;
  } else {
/* Argument is in range. Estimate the root as double. */
    int decimals;
    real_to_mp (p, z_g, curt (mp_to_real (p, x_g, digits_g)), digits_g);
/* Newton's method: x<n+1> = (2 x<n> + a / x<n> ^ 2) / 3 */
    decimals = DOUBLE_ACCURACY;
    do {
      decimals <<= 1;
      digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
      mul_mp (p, tmp, z_g, z_g, digits_h);
      div_mp (p, tmp, x_g, tmp, digits_h);
      add_mp (p, tmp, z_g, tmp, digits_h);
      add_mp (p, tmp, z_g, tmp, digits_h);
      div_mp_digit (p, z_g, tmp, (MP_DIGIT_T) 3, digits_h);
    }
    while (decimals < digits_g * LOG_MP_BASE);
  }
  if (reciprocal) {
    rec_mp (p, z_g, z_g, digits);
  }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit. */
  stack_pointer = pop_sp;
  if (change_sign) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
  return (z);
}

/*!
\brief set z to sqrt (x^2 + y^2)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *hypot_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *t, *u, *v;
  STACK_MP (t, p, digits);
  STACK_MP (u, p, digits);
  STACK_MP (v, p, digits);
  MOVE_MP (u, x, digits);
  MOVE_MP (v, y, digits);
  MP_DIGIT (u, 1) = ABS (MP_DIGIT (u, 1));
  MP_DIGIT (v, 1) = ABS (MP_DIGIT (v, 1));
  if (IS_ZERO_MP (u)) {
    MOVE_MP (z, v, digits);
  } else if (IS_ZERO_MP (v)) {
    MOVE_MP (z, u, digits);
  } else {
    set_mp_short (t, (MP_DIGIT_T) 1, 0, digits);
    sub_mp (p, z, u, v, digits);
    if (MP_DIGIT (z, 1) > 0) {
      div_mp (p, z, v, u, digits);
      mul_mp (p, z, z, z, digits);
      add_mp (p, z, t, z, digits);
      sqrt_mp (p, z, z, digits);
      mul_mp (p, z, u, z, digits);
    } else {
      div_mp (p, z, u, v, digits);
      mul_mp (p, z, z, z, digits);
      add_mp (p, z, t, z, digits);
      sqrt_mp (p, z, z, digits);
      mul_mp (p, z, v, z, digits);
    }
  }
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to exp (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *exp_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/* Argument is reduced by using exp (z / (2 ** n)) ** (2 ** n) = exp(z). */
  int m, n, pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *sum, *pow, *fac, *tmp;
  BOOL_T iterate;
  if (MP_DIGIT (x, 1) == 0) {
    set_mp_short (z, (MP_DIGIT_T) 1, 0, digits);
    return (z);
  }
  STACK_MP (x_g, p, digits_g);
  STACK_MP (sum, p, digits_g);
  STACK_MP (pow, p, digits_g);
  STACK_MP (fac, p, digits_g);
  STACK_MP (tmp, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  m = 0;
/* Scale x down. */
  while (eps_mp (x_g, digits_g)) {
    m++;
    half_mp (p, x_g, x_g, digits_g);
  }
/* Calculate Taylor sum
   exp (z) = 1 + z / 1 ! + z ** 2 / 2 ! + ... */
  set_mp_short (sum, (MP_DIGIT_T) 1, 0, digits_g);
  add_mp (p, sum, sum, x_g, digits_g);
  mul_mp (p, pow, x_g, x_g, digits_g);
#if (MP_RADIX == DEFAULT_MP_RADIX)
  half_mp (p, tmp, pow, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 6, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 24, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 120, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 720, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 5040, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 40320, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 362880, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  set_mp_short (fac, (MP_DIGIT_T) 3628800, 0, digits_g);
  n = 10;
#else
  set_mp_short (fac, (MP_DIGIT_T) 2, 0, digits_g);
  n = 2;
#endif
  iterate = MP_DIGIT (pow, 1) != 0;
  while (iterate) {
    div_mp (p, tmp, pow, fac, digits_g);
    if (MP_EXPONENT (tmp) <= (MP_EXPONENT (sum) - digits_g)) {
      iterate = A_FALSE;
    } else {
      add_mp (p, sum, sum, tmp, digits_g);
      mul_mp (p, pow, pow, x_g, digits_g);
      n++;
      mul_mp_digit (p, fac, fac, (MP_DIGIT_T) n, digits_g);
    }
  }
/* Square exp (x) up. */
  while (m--) {
    mul_mp (p, sum, sum, sum, digits_g);
  }
  shorten_mp (p, z, digits, sum, digits_g);
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to exp (x) - 1, assuming x to be close to 0
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *expm1_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int n, pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *sum, *pow, *fac, *tmp;
  BOOL_T iterate;
  if (MP_DIGIT (x, 1) == 0) {
    set_mp_short (z, (MP_DIGIT_T) 1, 0, digits);
    return (z);
  }
  STACK_MP (x_g, p, digits_g);
  STACK_MP (sum, p, digits_g);
  STACK_MP (pow, p, digits_g);
  STACK_MP (fac, p, digits_g);
  STACK_MP (tmp, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* Calculate Taylor sum expm1 (z) = z / 1 ! + z ** 2 / 2 ! + ... */
  SET_MP_ZERO (sum, digits_g);
  add_mp (p, sum, sum, x_g, digits_g);
  mul_mp (p, pow, x_g, x_g, digits_g);
#if (MP_RADIX == DEFAULT_MP_RADIX)
  half_mp (p, tmp, pow, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 6, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 24, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 120, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 720, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 5040, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 40320, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  div_mp_digit (p, tmp, pow, 362880, digits_g);
  add_mp (p, sum, sum, tmp, digits_g);
  mul_mp (p, pow, pow, x_g, digits_g);
  set_mp_short (fac, (MP_DIGIT_T) 3628800, 0, digits_g);
  n = 10;
#else
  set_mp_short (fac, (MP_DIGIT_T) 2, 0, digits_g);
  n = 2;
#endif
  iterate = MP_DIGIT (pow, 1) != 0;
  while (iterate) {
    div_mp (p, tmp, pow, fac, digits_g);
    if (MP_EXPONENT (tmp) <= (MP_EXPONENT (sum) - digits_g)) {
      iterate = A_FALSE;
    } else {
      add_mp (p, sum, sum, tmp, digits_g);
      mul_mp (p, pow, pow, x_g, digits_g);
      n++;
      mul_mp_digit (p, fac, fac, (MP_DIGIT_T) n, digits_g);
    }
  }
  shorten_mp (p, z, digits, sum, digits_g);
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief ln scale with digits precision
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mp_ln_scale (NODE_T * p, MP_DIGIT_T * z, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *z_g;
  STACK_MP (z_g, p, digits_g);
/* First see if we can restore a previous calculation. */
  if (digits_g <= mp_ln_scale_size) {
    MOVE_MP (z_g, ref_mp_ln_scale, digits_g);
  } else {
/* No luck with the kept value, we generate a longer one. */
    set_mp_short (z_g, (MP_DIGIT_T) 1, 1, digits_g);
    ln_mp (p, z_g, z_g, digits_g);
    ref_mp_ln_scale = (MP_DIGIT_T *) get_heap_space ((unsigned) SIZE_MP (digits_g));
    MOVE_MP (ref_mp_ln_scale, z_g, digits_g);
    mp_ln_scale_size = digits_g;
  }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief ln 10 with digits precision
\param p position in syntax tree, should not be NULL
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mp_ln_10 (NODE_T * p, MP_DIGIT_T * z, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *z_g;
  STACK_MP (z_g, p, digits_g);
/* First see if we can restore a previous calculation. */
  if (digits_g <= mp_ln_10_size) {
    MOVE_MP (z_g, ref_mp_ln_10, digits_g);
  } else {
/* No luck with the kept value, we generate a longer one. */
    set_mp_short (z_g, (MP_DIGIT_T) 10, 0, digits_g);
    ln_mp (p, z_g, z_g, digits_g);
    ref_mp_ln_10 = (MP_DIGIT_T *) get_heap_space ((unsigned) SIZE_MP (digits_g));
    MOVE_MP (ref_mp_ln_10, z_g, digits_g);
    mp_ln_10_size = digits_g;
  }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to ln (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *ln_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/* Depending on the argument we choose either Taylor or Newton. */
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  BOOL_T negative, scale;
  MP_DIGIT_T *x_g, *z_g, expo = 0;
  if (MP_DIGIT (x, 1) <= 0) {
    errno = EDOM;
    return (NULL);
  }
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (z_g, p, digits_g);
/* We use ln (1 / x) = - ln (x) */
  negative = MP_EXPONENT (x_g) < 0;
  if (negative) {
    rec_mp (p, x_g, x_g, digits);
  }
/* We want correct results for extreme arguments. We scale when "x_g" exceeds
   "MP_RADIX ** +- 2", using ln (x * MP_RADIX ** n) = ln (x) + n * ln (MP_RADIX).*/
  scale = (ABS (MP_EXPONENT (x_g)) >= 2);
  if (scale) {
    expo = MP_EXPONENT (x_g);
    MP_EXPONENT (x_g) = 0;
  }
  if (MP_EXPONENT (x_g) == 0 && MP_DIGIT (x_g, 1) == 1 && MP_DIGIT (x_g, 2) == 0) {
/* Taylor sum for x close to unity.
   ln (x) = (x - 1) - (x - 1) ** 2 / 2 + (x - 1) ** 3 / 3 - ...
   This is faster for small x and avoids cancellation. */
    MP_DIGIT_T *one, *tmp, *pow;
    int n = 2;
    BOOL_T iterate;
    STACK_MP (one, p, digits_g);
    STACK_MP (tmp, p, digits_g);
    STACK_MP (pow, p, digits_g);
    set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
    sub_mp (p, x_g, x_g, one, digits_g);
    mul_mp (p, pow, x_g, x_g, digits_g);
    MOVE_MP (z_g, x_g, digits_g);
    iterate = MP_DIGIT (pow, 1) != 0;
    while (iterate) {
      div_mp_digit (p, tmp, pow, (MP_DIGIT_T) n, digits_g);
      if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g)) {
	iterate = A_FALSE;
      } else {
	MP_DIGIT (tmp, 1) = (n % 2 == 0 ? -MP_DIGIT (tmp, 1) : MP_DIGIT (tmp, 1));
	add_mp (p, z_g, z_g, tmp, digits_g);
	mul_mp (p, pow, pow, x_g, digits_g);
	n++;
      }
    }
  } else {
/* Newton's method: x<n+1> = x<n> - 1 + a / exp (x<n>). */
    MP_DIGIT_T *tmp, *z_0, *one;
    int decimals;
    STACK_MP (tmp, p, digits_g);
    STACK_MP (one, p, digits_g);
    STACK_MP (z_0, p, digits_g);
    set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
    SET_MP_ZERO (z_0, digits_g);
/* Construct an estimate. */
    real_to_mp (p, z_g, log (mp_to_real (p, x_g, digits_g)), digits_g);
    decimals = DOUBLE_ACCURACY;
    do {
      int digits_h;
      decimals <<= 1;
      digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
      exp_mp (p, tmp, z_g, digits_h);
      div_mp (p, tmp, x_g, tmp, digits_h);
      sub_mp (p, z_g, z_g, one, digits_h);
      add_mp (p, z_g, z_g, tmp, digits_h);
    }
    while (decimals < digits_g * LOG_MP_BASE);
  }
/* Inverse scaling. */
  if (scale) {
/* ln (x * MP_RADIX ** n) = ln (x) + n * ln (MP_RADIX) */
    MP_DIGIT_T *ln_base;
    STACK_MP (ln_base, p, digits_g);
    (void) mp_ln_scale (p, ln_base, digits_g);
    mul_mp_digit (p, ln_base, ln_base, (MP_DIGIT_T) expo, digits_g);
    add_mp (p, z_g, z_g, ln_base, digits_g);
  }
  if (negative) {
    MP_DIGIT (z_g, 1) = -MP_DIGIT (z_g, 1);
  }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to log (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *log_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int pop_sp = stack_pointer;
  MP_DIGIT_T *ln_10;
  STACK_MP (ln_10, p, digits);
  if (ln_mp (p, z, x, digits) == NULL) {
    errno = EDOM;
    return (NULL);
  }
  (void) mp_ln_10 (p, ln_10, digits);
  div_mp (p, z, z, ln_10, digits);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set sh and ch to sinh (z) and cosh (z) respectively
\param p position in syntax tree, should not be NULL
\param sh
\param ch
\param z
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *hyp_mp (NODE_T * p, MP_DIGIT_T * sh, MP_DIGIT_T * ch, MP_DIGIT_T * z, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *x_g, *y_g, *z_g;
  STACK_MP (x_g, p, digits);
  STACK_MP (y_g, p, digits);
  STACK_MP (z_g, p, digits);
  MOVE_MP (z_g, z, digits);
  exp_mp (p, x_g, z_g, digits);
  rec_mp (p, y_g, x_g, digits);
  add_mp (p, ch, x_g, y_g, digits);
/* Avoid cancellation for sinh. */
  if ((MP_DIGIT (x_g, 1) == 1 && MP_DIGIT (x_g, 2) == 0) || (MP_DIGIT (y_g, 1) == 1 && MP_DIGIT (y_g, 2) == 0)) {
    expm1_mp (p, x_g, z_g, digits);
    MP_DIGIT (z_g, 1) = -MP_DIGIT (z_g, 1);
    expm1_mp (p, y_g, z_g, digits);
  }
  sub_mp (p, sh, x_g, y_g, digits);
  half_mp (p, sh, sh, digits);
  half_mp (p, ch, ch, digits);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to sinh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *sinh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *y_g, *z_g;
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  hyp_mp (p, z_g, y_g, x_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to asinh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *asinh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  if (IS_ZERO_MP (x)) {
    SET_MP_ZERO (z, digits);
    return (z);
  } else {
    ADDR_T pop_sp = stack_pointer;
    int digits_g;
    MP_DIGIT_T *x_g, *y_g, *z_g;
    if (MP_EXPONENT (x) >= -1) {
      digits_g = digits + MP_GUARDS (digits);
    } else {
/* Extra precision when x^2+1 gets close to 1 */
      digits_g = 2 * digits + MP_GUARDS (digits);
    }
    STACK_MP (x_g, p, digits_g);
    lengthen_mp (p, x_g, digits_g, x, digits);
    STACK_MP (y_g, p, digits_g);
    STACK_MP (z_g, p, digits_g);
    mul_mp (p, z_g, x_g, x_g, digits_g);
    set_mp_short (y_g, 1, 0, digits_g);
    add_mp (p, y_g, z_g, y_g, digits_g);
    sqrt_mp (p, y_g, y_g, digits_g);
    add_mp (p, y_g, y_g, x_g, digits_g);
    ln_mp (p, z_g, y_g, digits_g);
    if (IS_ZERO_MP (z_g)) {
      MOVE_MP (z, x, digits);
    } else {
      shorten_mp (p, z, digits, z_g, digits_g);
    }
    stack_pointer = pop_sp;
    return (z);
  }
}

/*!
\brief set z to cosh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *cosh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *y_g, *z_g;
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  hyp_mp (p, y_g, z_g, x_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to acosh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *acosh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g;
  MP_DIGIT_T *x_g, *y_g, *z_g;
  if (MP_DIGIT (x, 1) == 1 && MP_DIGIT (x, 2) == 0) {
/* Extra precision when x^2-1 gets close to 0 */
    digits_g = 2 * digits + MP_GUARDS (digits);
  } else {
    digits_g = digits + MP_GUARDS (digits);
  }
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  mul_mp (p, z_g, x_g, x_g, digits_g);
  set_mp_short (y_g, 1, 0, digits_g);
  sub_mp (p, y_g, z_g, y_g, digits_g);
  sqrt_mp (p, y_g, y_g, digits_g);
  add_mp (p, y_g, y_g, x_g, digits_g);
  ln_mp (p, z_g, y_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to tanh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *tanh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *y_g, *z_g;
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  hyp_mp (p, y_g, z_g, x_g, digits_g);
  div_mp (p, z_g, y_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to atanh (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *atanh_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *y_g, *z_g;
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  set_mp_short (y_g, 1, 0, digits_g);
  add_mp (p, z_g, y_g, x_g, digits_g);
  sub_mp (p, y_g, y_g, x_g, digits_g);
  div_mp (p, y_g, z_g, y_g, digits_g);
  ln_mp (p, z_g, y_g, digits_g);
  half_mp (p, z_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief return pi with digits precision, using Borwein & Borwein AGM
\param p position in syntax tree, should not be NULL
\param api
\param mult
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *mp_pi (NODE_T * p, MP_DIGIT_T * api, int mult, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  BOOL_T iterate;
  MP_DIGIT_T *pi_g, *one, *two, *x_g, *y_g, *u_g, *v_g;
  STACK_MP (pi_g, p, digits_g);
/* First see if we can restore a previous calculation. */
  if (digits_g <= mp_pi_size) {
    MOVE_MP (pi_g, ref_mp_pi, digits_g);
  } else {
/* No luck with the kept value, hence we generate a longer "pi".
   Calculate "pi" using the Borwein & Borwein AGM algorithm.
   This AGM doubles the numbers of digits at every pass. */
    STACK_MP (one, p, digits_g);
    STACK_MP (two, p, digits_g);
    STACK_MP (x_g, p, digits_g);
    STACK_MP (y_g, p, digits_g);
    STACK_MP (u_g, p, digits_g);
    STACK_MP (v_g, p, digits_g);
    set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
    set_mp_short (two, (MP_DIGIT_T) 2, 0, digits_g);
    set_mp_short (x_g, (MP_DIGIT_T) 2, 0, digits_g);
    sqrt_mp (p, x_g, x_g, digits_g);
    add_mp (p, pi_g, x_g, two, digits_g);
    sqrt_mp (p, y_g, x_g, digits_g);
    iterate = A_TRUE;
    while (iterate) {
/* New x. */
      sqrt_mp (p, u_g, x_g, digits_g);
      div_mp (p, v_g, one, u_g, digits_g);
      add_mp (p, u_g, u_g, v_g, digits_g);
      half_mp (p, x_g, u_g, digits_g);
/* New pi. */
      add_mp (p, u_g, x_g, one, digits_g);
      add_mp (p, v_g, y_g, one, digits_g);
      div_mp (p, u_g, u_g, v_g, digits_g);
      mul_mp (p, v_g, pi_g, u_g, digits_g);
/* Done yet? */
      if (same_mp (p, v_g, pi_g, digits_g)) {
	iterate = A_FALSE;
      } else {
	MOVE_MP (pi_g, v_g, digits_g);
/* New y. */
	sqrt_mp (p, u_g, x_g, digits_g);
	div_mp (p, v_g, one, u_g, digits_g);
	mul_mp (p, u_g, y_g, u_g, digits_g);
	add_mp (p, u_g, u_g, v_g, digits_g);
	add_mp (p, v_g, y_g, one, digits_g);
	div_mp (p, y_g, u_g, v_g, digits_g);
      }
    }
/* Keep the result for future restore. */
    ref_mp_pi = (MP_DIGIT_T *) get_heap_space ((unsigned) SIZE_MP (digits_g));
    MOVE_MP (ref_mp_pi, pi_g, digits_g);
    mp_pi_size = digits_g;
  }
  switch (mult) {
  case MP_PI:
    {
      break;
    }
  case MP_TWO_PI:
    {
      mul_mp_digit (p, pi_g, pi_g, (MP_DIGIT_T) 2, digits_g);
      break;
    }
  case MP_HALF_PI:
    {
      half_mp (p, pi_g, pi_g, digits_g);
      break;
    }
  }
  shorten_mp (p, api, digits, pi_g, digits_g);
  stack_pointer = pop_sp;
  return (api);
}

/*!
\brief set z to sin (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *sin_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/* Use triple-angle relation to reduce argument. */
  int pop_sp = stack_pointer, m, n, digits_g = digits + MP_GUARDS (digits);
  BOOL_T flip, negative, iterate, even;
  MP_DIGIT_T *x_g, *pi, *tpi, *hpi, *sqr, *tmp, *pow, *fac, *z_g;
/* We will use "pi" */
  STACK_MP (pi, p, digits_g);
  STACK_MP (tpi, p, digits_g);
  STACK_MP (hpi, p, digits_g);
  mp_pi (p, pi, MP_PI, digits_g);
  mp_pi (p, tpi, MP_TWO_PI, digits_g);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
/* Argument reduction (1): sin (x) = sin (x mod 2 pi) */
  STACK_MP (x_g, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mod_mp (p, x_g, x_g, tpi, digits_g);
/* Argument reduction (2): sin (-x) = sin (x)
                           sin (x) = - sin (x - pi); pi < x <= 2 pi
                           sin (x) = sin (pi - x);   pi / 2 < x <= pi. */
  negative = MP_DIGIT (x_g, 1) < 0;
  if (negative) {
    MP_DIGIT (x_g, 1) = -MP_DIGIT (x_g, 1);
  }
  STACK_MP (tmp, p, digits_g);
  sub_mp (p, tmp, x_g, pi, digits_g);
  flip = MP_DIGIT (tmp, 1) > 0;
  if (flip) {			/* x > pi. */
    sub_mp (p, x_g, x_g, pi, digits_g);
  }
  sub_mp (p, tmp, x_g, hpi, digits_g);
  if (MP_DIGIT (tmp, 1) > 0) {	/* x > pi / 2 */
    sub_mp (p, x_g, pi, x_g, digits_g);
  }
/* Argument reduction (3): (follows from De Moivre's theorem)
   sin (3x) = sin (x) * (3 - 4 sin ^ 2 (x)) */
  m = 0;
  while (eps_mp (x_g, digits_g)) {
    m++;
    div_mp_digit (p, x_g, x_g, (MP_DIGIT_T) 3, digits_g);
  }
/* Taylor sum. */
  STACK_MP (sqr, p, digits_g);
  STACK_MP (pow, p, digits_g);
  STACK_MP (fac, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  mul_mp (p, sqr, x_g, x_g, digits_g);	/* sqr = x ** 2 */
  mul_mp (p, pow, sqr, x_g, digits_g);	/* pow = x ** 3 */
  MOVE_MP (z_g, x_g, digits_g);
#if (MP_RADIX == DEFAULT_MP_RADIX)
  div_mp_digit (p, tmp, pow, 6, digits_g);
  sub_mp (p, z_g, z_g, tmp, digits_g);
  mul_mp (p, pow, pow, sqr, digits_g);
  div_mp_digit (p, tmp, pow, 120, digits_g);
  add_mp (p, z_g, z_g, tmp, digits_g);
  mul_mp (p, pow, pow, sqr, digits_g);
  div_mp_digit (p, tmp, pow, 5040, digits_g);
  sub_mp (p, z_g, z_g, tmp, digits_g);
  mul_mp (p, pow, pow, sqr, digits_g);
  set_mp_short (fac, (MP_DIGIT_T) 362880, 0, digits_g);
  n = 9;
  even = A_TRUE;
#else
  set_mp_short (fac, (MP_DIGIT_T) 6, 0, digits_g);
  n = 3;
  even = A_FALSE;
#endif
  iterate = MP_DIGIT (pow, 1) != 0;
  while (iterate) {
    div_mp (p, tmp, pow, fac, digits_g);
    if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g)) {
      iterate = A_FALSE;
    } else {
      if (even) {
	add_mp (p, z_g, z_g, tmp, digits_g);
	even = A_FALSE;
      } else {
	sub_mp (p, z_g, z_g, tmp, digits_g);
	even = A_TRUE;
      }
      mul_mp (p, pow, pow, sqr, digits_g);
      mul_mp_digit (p, fac, fac, (MP_DIGIT_T) (++n), digits_g);
      mul_mp_digit (p, fac, fac, (MP_DIGIT_T) (++n), digits_g);
    }
  }
/* Inverse scaling using sin (3x) = sin (x) * (3 - 4 sin ** 2 (x))
   Use existing mp's for intermediates. */
  set_mp_short (fac, (MP_DIGIT_T) 3, 0, digits_g);
  while (m--) {
    mul_mp (p, pow, z_g, z_g, digits_g);
    mul_mp_digit (p, pow, pow, (MP_DIGIT_T) 4, digits_g);
    sub_mp (p, pow, fac, pow, digits_g);
    mul_mp (p, z_g, pow, z_g, digits_g);
  }
  shorten_mp (p, z, digits, z_g, digits_g);
  if (negative ^ flip) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to cos (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *cos_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/*
Use cos (x) = sin (pi / 2 - x).
Compute x mod 2 pi before subtracting to avoid cancellation.
*/
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *hpi, *tpi, *x_g, *y;
  STACK_MP (hpi, p, digits_g);
  STACK_MP (tpi, p, digits_g);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (y, p, digits);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
  mp_pi (p, tpi, MP_TWO_PI, digits_g);
  mod_mp (p, x_g, x_g, tpi, digits_g);
  sub_mp (p, x_g, hpi, x_g, digits_g);
  shorten_mp (p, y, digits, x_g, digits_g);
  sin_mp (p, z, y, digits);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to tan (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *tan_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/* Use tan (x) = sin (x) / sqrt (1 - sin ^ 2 (x)). */
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *sns, *cns, *one, *pi, *hpi, *x_g, *y_g;
  BOOL_T negate;
  STACK_MP (one, p, digits);
  STACK_MP (pi, p, digits_g);
  STACK_MP (hpi, p, digits_g);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (y_g, p, digits_g);
  STACK_MP (sns, p, digits);
  STACK_MP (cns, p, digits);
/* Argument mod pi. */
  mp_pi (p, pi, MP_PI, digits_g);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mod_mp (p, x_g, x_g, pi, digits_g);
  if (MP_DIGIT (x_g, 1) >= 0) {
    sub_mp (p, y_g, x_g, hpi, digits_g);
    negate = MP_DIGIT (y_g, 1) > 0;
  } else {
    add_mp (p, y_g, x_g, hpi, digits_g);
    negate = MP_DIGIT (y_g, 1) < 0;
  }
  shorten_mp (p, x, digits, x_g, digits_g);
/* tan(x) = sin(x) / sqrt (1 - sin ** 2 (x)) */
  sin_mp (p, sns, x, digits);
  set_mp_short (one, (MP_DIGIT_T) 1, 0, digits);
  mul_mp (p, cns, sns, sns, digits);
  sub_mp (p, cns, one, cns, digits);
  sqrt_mp (p, cns, cns, digits);
  if (div_mp (p, z, sns, cns, digits) == NULL) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  }
  stack_pointer = pop_sp;
  if (negate) {
    MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
  }
  return (z);
}

/*!
\brief set z to arcsin (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *asin_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *one, *x_g, *y, *z_g;
  STACK_MP (y, p, digits);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  STACK_MP (one, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
  mul_mp (p, z_g, x_g, x_g, digits_g);
  sub_mp (p, z_g, one, z_g, digits_g);
  if (sqrt_mp (p, z_g, z_g, digits) == NULL) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  }
  if (MP_DIGIT (z_g, 1) == 0) {
    mp_pi (p, z, MP_HALF_PI, digits);
    MP_DIGIT (z, 1) = (MP_DIGIT (x_g, 1) >= 0 ? MP_DIGIT (z, 1) : -MP_DIGIT (z, 1));
    stack_pointer = pop_sp;
    return (z);
  }
  if (div_mp (p, x_g, x_g, z_g, digits_g) == NULL) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  }
  shorten_mp (p, y, digits, x_g, digits_g);
  atan_mp (p, z, y, digits);
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to arccos (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *acos_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *one, *x_g, *y, *z_g;
  BOOL_T negative = MP_DIGIT (x, 1) < 0;
  if (MP_DIGIT (x, 1) == 0) {
    mp_pi (p, z, MP_HALF_PI, digits);
    stack_pointer = pop_sp;
    return (z);
  }
  STACK_MP (y, p, digits);
  STACK_MP (x_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  STACK_MP (one, p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
  mul_mp (p, z_g, x_g, x_g, digits_g);
  sub_mp (p, z_g, one, z_g, digits_g);
  if (sqrt_mp (p, z_g, z_g, digits) == NULL) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  }
  if (div_mp (p, x_g, z_g, x_g, digits_g) == NULL) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  }
  shorten_mp (p, y, digits, x_g, digits_g);
  atan_mp (p, z, y, digits);
  if (negative) {
    mp_pi (p, y, MP_PI, digits);
    add_mp (p, z, z, y, digits);
  }
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to arctan (x)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *atan_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, int digits)
{
/* Depending on the argument we choose either Taylor or Newton. */
  int pop_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *x_g, *z_g;
  BOOL_T flip, negative;
  STACK_MP (x_g, p, digits_g);
  STACK_MP (z_g, p, digits_g);
  if (MP_DIGIT (x, 1) == 0) {
    stack_pointer = pop_sp;
    SET_MP_ZERO (z, digits);
    return (z);
  }
  lengthen_mp (p, x_g, digits_g, x, digits);
  negative = (MP_DIGIT (x_g, 1) < 0);
  if (negative) {
    MP_DIGIT (x_g, 1) = -MP_DIGIT (x_g, 1);
  }
/* For larger arguments we use atan(x) = pi/2 - atan(1/x) */
  flip = ((MP_EXPONENT (x_g) > 0) || (MP_EXPONENT (x_g) == 0 && MP_DIGIT (x_g, 1) > 1)) && (MP_DIGIT (x_g, 1) != 0);
  if (flip) {
    rec_mp (p, x_g, x_g, digits_g);
  }
  if (MP_EXPONENT (x_g) < -1 || (MP_EXPONENT (x_g) == -1 && MP_DIGIT (x_g, 1) < MP_RADIX / 100)) {
/* Taylor sum for x close to zero.
   arctan (x) = x - x ** 3 / 3 + x ** 5 / 5 - x ** 7 / 7 + ..
   This is faster for small x and avoids cancellation. */
    MP_DIGIT_T *tmp, *pow, *sqr;
    int n = 3;
    BOOL_T iterate, even;
    STACK_MP (tmp, p, digits_g);
    STACK_MP (pow, p, digits_g);
    STACK_MP (sqr, p, digits_g);
    mul_mp (p, sqr, x_g, x_g, digits_g);
    mul_mp (p, pow, sqr, x_g, digits_g);
    MOVE_MP (z_g, x_g, digits_g);
    even = A_FALSE;
    iterate = MP_DIGIT (pow, 1) != 0;
    while (iterate) {
      div_mp_digit (p, tmp, pow, (MP_DIGIT_T) n, digits_g);
      if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g)) {
	iterate = A_FALSE;
      } else {
	if (even) {
	  add_mp (p, z_g, z_g, tmp, digits_g);
	  even = A_FALSE;
	} else {
	  sub_mp (p, z_g, z_g, tmp, digits_g);
	  even = A_TRUE;
	}
	mul_mp (p, pow, pow, sqr, digits_g);
	n += 2;
      }
    }
  } else {
/* Newton's method: x<n+1> = x<n> - cos (x<n>) * (sin (x<n>) - a cos (x<n>)) */
    MP_DIGIT_T *tmp, *z_0, *sns, *cns, *one;
    int decimals, digits_h;
    STACK_MP (tmp, p, digits_g);
    STACK_MP (z_0, p, digits_g);
    STACK_MP (sns, p, digits_g);
    STACK_MP (cns, p, digits_g);
    STACK_MP (one, p, digits_g);
    SET_MP_ZERO (z_0, digits_g);
    set_mp_short (one, (MP_DIGIT_T) 1, 0, digits_g);
/* Construct an estimate. */
    real_to_mp (p, z_g, atan (mp_to_real (p, x_g, digits_g)), digits_g);
    decimals = DOUBLE_ACCURACY;
    do {
      decimals <<= 1;
      digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
      sin_mp (p, sns, z_g, digits_h);
      mul_mp (p, tmp, sns, sns, digits_h);
      sub_mp (p, tmp, one, tmp, digits_h);
      sqrt_mp (p, cns, tmp, digits_h);
      mul_mp (p, tmp, x_g, cns, digits_h);
      sub_mp (p, tmp, sns, tmp, digits_h);
      mul_mp (p, tmp, tmp, cns, digits_h);
      sub_mp (p, z_g, z_g, tmp, digits_h);
    }
    while (decimals < digits_g * LOG_MP_BASE);
  }
  if (flip) {
    MP_DIGIT_T *hpi;
    STACK_MP (hpi, p, digits_g);
    sub_mp (p, z_g, mp_pi (p, hpi, MP_HALF_PI, digits_g), z_g, digits_g);
  }
  shorten_mp (p, z, digits, z_g, digits_g);
  MP_DIGIT (z, 1) = (negative ? -MP_DIGIT (z, 1) : MP_DIGIT (z, 1));
/* Exit. */
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set z to atan2 (x, y)
\param p position in syntax tree, should not be NULL
\param z
\param x
\param y
\param digits precision as number of MP_DIGITs
\return result z
**/

MP_DIGIT_T *atan2_mp (NODE_T * p, MP_DIGIT_T * z, MP_DIGIT_T * x, MP_DIGIT_T * y, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *t;
  STACK_MP (t, p, digits);
  if (MP_DIGIT (x, 1) == 0 && MP_DIGIT (y, 1) == 0) {
    errno = EDOM;
    stack_pointer = pop_sp;
    return (NULL);
  } else {
    BOOL_T flip = MP_DIGIT (y, 1) < 0;
    MP_DIGIT (y, 1) = ABS (MP_DIGIT (y, 1));
    if (IS_ZERO_MP (x)) {
      mp_pi (p, z, MP_HALF_PI, digits);
    } else {
      BOOL_T flop = MP_DIGIT (x, 1) <= 0;
      MP_DIGIT (x, 1) = ABS (MP_DIGIT (x, 1));
      div_mp (p, z, y, x, digits);
      atan_mp (p, z, z, digits);
      if (flop) {
	mp_pi (p, t, MP_PI, digits);
	sub_mp (p, z, t, z, digits);
      }
    }
    if (flip) {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  }
  stack_pointer = pop_sp;
  return (z);
}

/*!
\brief set a I b to a I b * c I d
\param p position in syntax tree, should not be NULL
\param a
\param b
\param c
\param d
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *cmul_mp (NODE_T * p, MP_DIGIT_T * a, MP_DIGIT_T * b, MP_DIGIT_T * c, MP_DIGIT_T * d, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  int digits_g = digits + MP_GUARDS (digits);
  MP_DIGIT_T *la, *lb, *lc, *ld, *ac, *bd, *ad, *bc;
  STACK_MP (la, p, digits_g);
  STACK_MP (lb, p, digits_g);
  STACK_MP (lc, p, digits_g);
  STACK_MP (ld, p, digits_g);
  lengthen_mp (p, la, digits_g, a, digits);
  lengthen_mp (p, lb, digits_g, b, digits);
  lengthen_mp (p, lc, digits_g, c, digits);
  lengthen_mp (p, ld, digits_g, d, digits);
  STACK_MP (ac, p, digits_g);
  STACK_MP (bd, p, digits_g);
  STACK_MP (ad, p, digits_g);
  STACK_MP (bc, p, digits_g);
  mul_mp (p, ac, la, lc, digits_g);
  mul_mp (p, bd, lb, ld, digits_g);
  mul_mp (p, ad, la, ld, digits_g);
  mul_mp (p, bc, lb, lc, digits_g);
  sub_mp (p, la, ac, bd, digits_g);
  add_mp (p, lb, ad, bc, digits_g);
  shorten_mp (p, a, digits, la, digits_g);
  shorten_mp (p, b, digits, lb, digits_g);
  stack_pointer = pop_sp;
  return (a);
}

/*!
\brief set a I b to a I b / c I d
\param p position in syntax tree, should not be NULL
\param a
\param b
\param c
\param d
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *cdiv_mp (NODE_T * p, MP_DIGIT_T * a, MP_DIGIT_T * b, MP_DIGIT_T * c, MP_DIGIT_T * d, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *q, *r;
  STACK_MP (q, p, digits);
  STACK_MP (r, p, digits);
  MOVE_MP (q, c, digits);
  MOVE_MP (r, d, digits);
  MP_DIGIT (q, 1) = ABS (MP_DIGIT (q, 1));
  MP_DIGIT (r, 1) = ABS (MP_DIGIT (r, 1));
  sub_mp (p, q, q, r, digits);
  if (MP_DIGIT (q, 1) >= 0) {
    if (div_mp (p, q, d, c, digits) == NULL) {
      errno = ERANGE;
      return (NULL);
    }
    mul_mp (p, r, d, q, digits);
    add_mp (p, r, r, c, digits);
    mul_mp (p, c, b, q, digits);
    add_mp (p, c, c, a, digits);
    div_mp (p, c, c, r, digits);
    mul_mp (p, d, a, q, digits);
    sub_mp (p, d, b, d, digits);
    div_mp (p, d, d, r, digits);
  } else {
    if (div_mp (p, q, c, d, digits) == NULL) {
      errno = ERANGE;
      return (NULL);
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
  stack_pointer = pop_sp;
  return (a);
}

/*!
\brief set r I i to sqrt (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *csqrt_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  if (IS_ZERO_MP (re) && IS_ZERO_MP (im)) {
    SET_MP_ZERO (re, digits_g);
    SET_MP_ZERO (im, digits_g);
  } else {
    MP_DIGIT_T *c1, *t, *x, *y, *u, *v, *w;
    STACK_MP (c1, p, digits_g);
    STACK_MP (t, p, digits_g);
    STACK_MP (x, p, digits_g);
    STACK_MP (y, p, digits_g);
    STACK_MP (u, p, digits_g);
    STACK_MP (v, p, digits_g);
    STACK_MP (w, p, digits_g);
    set_mp_short (c1, 1, 0, digits_g);
    MOVE_MP (x, re, digits_g);
    MOVE_MP (y, im, digits_g);
    MP_DIGIT (x, 1) = ABS (MP_DIGIT (x, 1));
    MP_DIGIT (y, 1) = ABS (MP_DIGIT (y, 1));
    sub_mp (p, w, x, y, digits_g);
    if (MP_DIGIT (w, 1) >= 0) {
      div_mp (p, t, y, x, digits_g);
      mul_mp (p, v, t, t, digits_g);
      add_mp (p, u, c1, v, digits_g);
      sqrt_mp (p, v, u, digits_g);
      add_mp (p, u, c1, v, digits_g);
      half_mp (p, v, u, digits_g);
      sqrt_mp (p, u, v, digits_g);
      sqrt_mp (p, v, x, digits_g);
      mul_mp (p, w, u, v, digits_g);
    } else {
      div_mp (p, t, x, y, digits_g);
      mul_mp (p, v, t, t, digits_g);
      add_mp (p, u, c1, v, digits_g);
      sqrt_mp (p, v, u, digits_g);
      add_mp (p, u, t, v, digits_g);
      half_mp (p, v, u, digits_g);
      sqrt_mp (p, u, v, digits_g);
      sqrt_mp (p, v, y, digits_g);
      mul_mp (p, w, u, v, digits_g);
    }
    if (MP_DIGIT (re, 1) >= 0) {
      MOVE_MP (re, w, digits_g);
      add_mp (p, u, w, w, digits_g);
      div_mp (p, im, im, u, digits_g);
    } else {
      if (MP_DIGIT (im, 1) < 0) {
	MP_DIGIT (w, 1) = -MP_DIGIT (w, 1);
      }
      add_mp (p, v, w, w, digits_g);
      div_mp (p, re, im, v, digits_g);
      MOVE_MP (im, w, digits_g);
    }
  }
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to exp(r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *cexp_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im, *u;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  STACK_MP (u, p, digits_g);
  exp_mp (p, u, re, digits_g);
  cos_mp (p, re, im, digits_g);
  sin_mp (p, im, im, digits_g);
  mul_mp (p, re, re, u, digits_g);
  mul_mp (p, im, im, u, digits_g);
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to ln (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *cln_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im, *u, *v, *s, *t;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  STACK_MP (s, p, digits_g);
  STACK_MP (t, p, digits_g);
  STACK_MP (u, p, digits_g);
  STACK_MP (v, p, digits_g);
  MOVE_MP (u, re, digits_g);
  MOVE_MP (v, im, digits_g);
  hypot_mp (p, s, u, v, digits_g);
  MOVE_MP (u, re, digits_g);
  MOVE_MP (v, im, digits_g);
  atan2_mp (p, t, u, v, digits_g);
  ln_mp (p, re, s, digits_g);
  MOVE_MP (im, t, digits_g);
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to sin (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *csin_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im, *s, *c, *sh, *ch;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  STACK_MP (s, p, digits_g);
  STACK_MP (c, p, digits_g);
  STACK_MP (sh, p, digits_g);
  STACK_MP (ch, p, digits_g);
  if (IS_ZERO_MP (im)) {
    sin_mp (p, re, re, digits_g);
    SET_MP_ZERO (im, digits_g);
  } else {
    sin_mp (p, s, re, digits_g);
    cos_mp (p, c, re, digits_g);
    hyp_mp (p, sh, ch, im, digits_g);
    mul_mp (p, re, s, ch, digits_g);
    mul_mp (p, im, c, sh, digits_g);
  }
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to cos (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *ccos_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im, *s, *c, *sh, *ch;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  STACK_MP (s, p, digits_g);
  STACK_MP (c, p, digits_g);
  STACK_MP (sh, p, digits_g);
  STACK_MP (ch, p, digits_g);
  if (IS_ZERO_MP (im)) {
    cos_mp (p, re, re, digits_g);
    SET_MP_ZERO (im, digits_g);
  } else {
    sin_mp (p, s, re, digits_g);
    cos_mp (p, c, re, digits_g);
    hyp_mp (p, sh, ch, im, digits_g);
    MP_DIGIT (sh, 1) = -MP_DIGIT (sh, 1);
    mul_mp (p, re, c, ch, digits_g);
    mul_mp (p, im, s, sh, digits_g);
  }
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to tan (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *ctan_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *s, *t, *u, *v;
  RESET_ERRNO;
  STACK_MP (s, p, digits);
  STACK_MP (t, p, digits);
  STACK_MP (u, p, digits);
  STACK_MP (v, p, digits);
  MOVE_MP (u, r, digits);
  MOVE_MP (v, i, digits);
  csin_mp (p, u, v, digits);
  MOVE_MP (s, u, digits);
  MOVE_MP (t, v, digits);
  MOVE_MP (u, r, digits);
  MOVE_MP (v, i, digits);
  ccos_mp (p, u, v, digits);
  cdiv_mp (p, s, t, u, v, digits);
  MOVE_MP (r, s, digits);
  MOVE_MP (i, t, digits);
  stack_pointer = pop_sp;
  return (r);
}

/*!
\brief set r I i to asin (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *casin_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  if (IS_ZERO_MP (im)) {
    asin_mp (p, re, re, digits_g);
  } else {
    MP_DIGIT_T *c1, *u, *v, *a, *b;
    STACK_MP (c1, p, digits_g);
    set_mp_short (c1, 1, 0, digits_g);
    STACK_MP (u, p, digits_g);
    STACK_MP (v, p, digits_g);
    STACK_MP (a, p, digits_g);
    STACK_MP (b, p, digits_g);
/* u=sqrt((r+1)^2+i^2), v=sqrt((r-1)^2+i^2) */
    add_mp (p, a, re, c1, digits_g);
    sub_mp (p, b, re, c1, digits_g);
    hypot_mp (p, u, a, im, digits_g);
    hypot_mp (p, v, b, im, digits_g);
/* a=(u+v)/2, b=(u-v)/2 */
    add_mp (p, a, u, v, digits_g);
    half_mp (p, a, a, digits_g);
    sub_mp (p, b, u, v, digits_g);
    half_mp (p, b, b, digits_g);
/* r=asin(b), i=ln(a+sqrt(a^2-1)) */
    mul_mp (p, u, a, a, digits_g);
    sub_mp (p, u, u, c1, digits_g);
    sqrt_mp (p, u, u, digits_g);
    add_mp (p, u, a, u, digits_g);
    ln_mp (p, im, u, digits_g);
    asin_mp (p, re, b, digits_g);
  }
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (re);
}

/*!
\brief set r I i to acos (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *cacos_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  if (IS_ZERO_MP (im)) {
    acos_mp (p, re, re, digits_g);
  } else {
    MP_DIGIT_T *c1, *u, *v, *a, *b;
    STACK_MP (c1, p, digits_g);
    set_mp_short (c1, 1, 0, digits_g);
    STACK_MP (u, p, digits_g);
    STACK_MP (v, p, digits_g);
    STACK_MP (a, p, digits_g);
    STACK_MP (b, p, digits_g);
/* u=sqrt((r+1)^2+i^2), v=sqrt((r-1)^2+i^2) */
    add_mp (p, a, re, c1, digits_g);
    sub_mp (p, b, re, c1, digits_g);
    hypot_mp (p, u, a, im, digits_g);
    hypot_mp (p, v, b, im, digits_g);
/* a=(u+v)/2, b=(u-v)/2 */
    add_mp (p, a, u, v, digits_g);
    half_mp (p, a, a, digits_g);
    sub_mp (p, b, u, v, digits_g);
    half_mp (p, b, b, digits_g);
/* r=acos(b), i=-ln(a+sqrt(a^2-1)) */
    mul_mp (p, u, a, a, digits_g);
    sub_mp (p, u, u, c1, digits_g);
    sqrt_mp (p, u, u, digits_g);
    add_mp (p, u, a, u, digits_g);
    ln_mp (p, im, u, digits_g);
    MP_DIGIT (im, 1) = -MP_DIGIT (im, 1);
    acos_mp (p, re, b, digits_g);
  }
  shorten_mp (p, r, digits, re, digits_g);
  shorten_mp (p, i, digits, im, digits_g);
  stack_pointer = pop_sp;
  return (re);
}

/*!
\brief set r I i to atan (r I i)
\param p position in syntax tree, should not be NULL
\param r
\param i
\param digits precision as number of MP_DIGITs
\return real part of result
**/

MP_DIGIT_T *catan_mp (NODE_T * p, MP_DIGIT_T * r, MP_DIGIT_T * i, int digits)
{
  ADDR_T pop_sp = stack_pointer;
  MP_DIGIT_T *re, *im, *u, *v;
  int digits_g = digits + MP_GUARDS (digits);
  STACK_MP (re, p, digits_g);
  STACK_MP (im, p, digits_g);
  lengthen_mp (p, re, digits_g, r, digits);
  lengthen_mp (p, im, digits_g, i, digits);
  STACK_MP (u, p, digits_g);
  STACK_MP (v, p, digits_g);
  if (IS_ZERO_MP (im)) {
    atan_mp (p, u, re, digits_g);
    SET_MP_ZERO (v, digits_g);
  } else {
    MP_DIGIT_T *c1, *a, *b;
    STACK_MP (c1, p, digits_g);
    set_mp_short (c1, 1, 0, digits_g);
    STACK_MP (a, p, digits_g);
    STACK_MP (b, p, digits_g);
/* a=sqrt(r^2+(i+1)^2), b=sqrt(r^2+(i-1)^2) */
    add_mp (p, a, im, c1, digits_g);
    sub_mp (p, b, im, c1, digits_g);
    hypot_mp (p, u, re, a, digits_g);
    hypot_mp (p, v, re, b, digits_g);
/* im=ln(a/b)/4 */
    div_mp (p, u, u, v, digits_g);
    ln_mp (p, u, u, digits_g);
    half_mp (p, v, u, digits_g);
/* re=atan(2r/(1-r^2-i^2)) */
    mul_mp (p, a, re, re, digits_g);
    mul_mp (p, b, im, im, digits_g);
    sub_mp (p, u, c1, a, digits_g);
    sub_mp (p, b, u, b, digits_g);
    add_mp (p, a, re, re, digits_g);
    div_mp (p, a, a, b, digits_g);
    atan_mp (p, u, a, digits_g);
    half_mp (p, u, u, digits_g);
  }
  shorten_mp (p, r, digits, u, digits_g);
  shorten_mp (p, i, digits, v, digits_g);
  stack_pointer = pop_sp;
  return (re);
}
