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
Since this software is distributed without any warranty, it is your 
responsibility to validate the behaviour of the routines and their accuracy 
using the source code provided. Consult the GNU General Public License for 
further details.                                                              */

/*-------1---------2---------3---------4---------5---------6---------7---------+
A MULTIPRECISION ARITHMETIC LIBRARY FOR ALGOL68G

The question that is often raised is what applications justify multiprecision 
calculations. A number of applications, some of them practical, have surfaced 
over the years.

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
correct start.                                                                */

/*-------1---------2---------3---------4---------5---------6---------7---------+
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

   mp_digit *z;
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
is also in range [0 .. MP_RADIX>. Do not specify less than 2 digits.           */

#include "algol68g.h"
#include "genie.h"
#include "gsl.h"
#include "mp.h"

/* Internal mp constants */

static mp_digit *ref_mp_pi = NULL;
static int mp_pi_size = -1;

static mp_digit *ref_mp_ln_scale = NULL;
static int mp_ln_scale_size = -1;

static mp_digit *ref_mp_ln_10 = NULL;
static int mp_ln_10_size = -1;

void raw_write_mp (char *, mp_digit *, int);
mp_digit *sub_mp (NODE_T *, mp_digit *, mp_digit *, mp_digit *, int);
mp_digit *rec_mp (NODE_T *, mp_digit *, mp_digit *, int);

int varying_mp_digits = 9;

#define DOUBLE_ACCURACY (DBL_DIG - 1)

/* Next switch is for debugging purposes */

#define MOVE_DIGITS(z, x, digits) {MOVE ((z), (x), (unsigned) ((digits) * SIZE_OF (mp_digit)));}

static int _j1_, _j2_;
#define MINIMUM(x, y) (_j1_ = (x), _j2_ = (y), _j1_ < _j2_ ? _j1_ : _j2_)

/*-------1---------2---------3---------4---------5---------6---------7---------+
stack_mp: allocate temporary space in the evaluation stack.                   */

mp_digit *
stack_mp (NODE_T * p, int digits)
{
  int stack_mp_sp = stack_pointer;
  INCREMENT_STACK_POINTER (p, SIZE_MP (digits));
  return ((mp_digit *) STACK_ADDRESS (stack_mp_sp));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
MP_GUARDS: number of guard digits.

In calculations using intermediate results we will use guard digits.
We follow D.M. Smith in his recommendations for precisions greater than LONG. */

#define MP_GUARDS(digits)\
  (((digits) == LONG_MP_DIGITS) ? 1 : (LOG_MP_BASE <= 5) ? 3 : 2)

/*-------1---------2---------3---------4---------5---------6---------7---------+
size_long_mp: the length in bytes of a long mp number.                        */

size_t size_long_mp ()
{
  return (SIZE_MP (LONG_MP_DIGITS));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_mp_digits: the length in digits of a long mp number.                     */

int
long_mp_digits ()
{
  return (LONG_MP_DIGITS);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
size_longlong_mp: the length in bytes of a long long mp number.               */

size_t size_longlong_mp ()
{
  return (SIZE_MP (varying_mp_digits));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
longlong_mp_digits: the length in digits of a long long mp number.            */

int
longlong_mp_digits ()
{
  return (varying_mp_digits);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mp_digits: return the length in digits of mode m.                         */

int
get_mp_digits (MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_REAL) || m == MODE (LONG_COMPLEX) || m == MODE (LONG_BITS))
    {
      return (long_mp_digits ());
    }
  else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_REAL) || m == MODE (LONGLONG_COMPLEX) || m == MODE (LONGLONG_BITS))
    {
      return (longlong_mp_digits ());
    }
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mp_size: return the length in bytes of mode m.	                      */

int
get_mp_size (MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_REAL) || m == MODE (LONG_COMPLEX) || m == MODE (LONG_BITS))
    {
      return (size_long_mp ());
    }
  else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_REAL) || m == MODE (LONGLONG_COMPLEX) || m == MODE (LONGLONG_BITS))
    {
      return (size_longlong_mp ());
    }
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mp_bits_width: return the length in bits of mode m.	                      */

int
get_mp_bits_width (MOID_T * m)
{
  if (m == MODE (LONG_BITS))
    {
      return (MP_BITS_WIDTH (LONG_MP_DIGITS));
    }
  else if (m == MODE (LONGLONG_BITS))
    {
      return (MP_BITS_WIDTH (varying_mp_digits));
    }
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mp_bits_words: return the length in words of mode m.                      */

int
get_mp_bits_words (MOID_T * m)
{
  if (m == MODE (LONG_BITS))
    {
      return (MP_BITS_WORDS (LONG_MP_DIGITS));
    }
  else if (m == MODE (LONGLONG_BITS))
    {
      return (MP_BITS_WORDS (varying_mp_digits));
    }
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_long_int: whether z is a LONG INT.                                      */

BOOL_T check_long_int (mp_digit * z)
{
  return (MP_EXPONENT (z) >= 0 && MP_EXPONENT (z) < LONG_MP_DIGITS);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_longlong_int: whether z is a LONG LONG INT.                             */

BOOL_T check_longlong_int (mp_digit * z)
{
  return (MP_EXPONENT (z) >= 0 && MP_EXPONENT (z) < varying_mp_digits);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_mp_int: whether z is a LONG INT.                                        */

BOOL_T check_mp_int (mp_digit * z, MOID_T * m)
{
  if (m == MODE (LONG_INT) || m == MODE (LONG_BITS))
    {
      return (check_long_int (z));
    }
  else if (m == MODE (LONGLONG_INT) || m == MODE (LONGLONG_BITS))
    {
      return (check_longlong_int (z));
    }
  return (A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
int_to_mp_digits: convert precision "n" to digits for long long mp.           */

int
int_to_mp_digits (int n)
{
  return (2 + (int) ceil ((double) n / (double) LOG_MP_BASE));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_longlong_mp_digits: set number of digits for long long mp's.              */

void
set_longlong_mp_digits (int n)
{
  varying_mp_digits = n;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_mp_short: set z to short value x * MP_RADIX ** x_expo.                     */

mp_digit *
set_mp_short (mp_digit * z, mp_digit x, int x_expo, int digits)
{
  MP_EXPONENT (z) = x_expo;
  MP_DIGIT (z, 1) = x;
  memset (&MP_DIGIT (z, 2), 0, (unsigned) ((digits - 1) * SIZE_OF (mp_digit)));
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
same_mp: test x = y, that is, x - y = 0.

Start checking at the end, that is where differences remain longest.          */

static BOOL_T
same_mp (NODE_T * p, mp_digit * x, mp_digit * y, int digits)
{
  int k;
  (void) p;
  if (MP_EXPONENT (x) == MP_EXPONENT (y))
    {
      for (k = digits; k >= 1; k--)
	{
	  if (MP_DIGIT (x, k) != MP_DIGIT (y, k))
	    {
	      return (A_FALSE);
	    }
	}
      return (A_TRUE);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
raw_write_mp: unformatted write of z to stdout.                               */

void
raw_write_mp (char *str, mp_digit * z, int digits)
{
  int i;
  printf ("\n%s", str);
  for (i = 1; i <= digits; i++)
    {
      printf (" %d", (int) MP_DIGIT (z, i));
    }
  printf (" ^ %d", (int) MP_EXPONENT (z));
  printf (" status=%d", (int) MP_STATUS (z));
  fflush (stdout);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
align_mp: align 10-base z in a MP_RADIX mantissa.                              */

static mp_digit *
align_mp (mp_digit * z, int *expo, int digits)
{
  int i, shift;
  if (*expo >= 0)
    {
      shift = LOG_MP_BASE - *expo % LOG_MP_BASE - 1;
      *expo /= LOG_MP_BASE;
    }
  else
    {
      shift = (-*expo - 1) % LOG_MP_BASE;
      *expo = (*expo + 1) / LOG_MP_BASE;
      (*expo)--;
    }
/* Now normalise "z" */
  for (i = 1; i <= shift; i++)
    {
      int j, carry = 0;
      for (j = 1; j <= digits; j++)
	{
	  int k = (int) MP_DIGIT (z, j) % 10;
	  MP_DIGIT (z, j) = (int) (MP_DIGIT (z, j) / 10) + carry * (MP_RADIX / 10);
	  carry = k;
	}
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
string_to_mp: mechanically transform s into mp number z.
C-name probably would be "strtomp".                                           */

mp_digit *
string_to_mp (NODE_T * p, mp_digit * z, char *s, int digits)
{
  int i, j, sign, weight, sum, comma, power;
  int expo;
  BOOL_T ok = A_TRUE;
  errno = 0;
  SET_MP_ZERO (z, digits);
  while (IS_SPACE (s[0]))
    {
      s++;
    }
/* Get the sign	*/
  sign = (s[0] == '-' ? -1 : 1);
  if (s[0] == '+' || s[0] == '-')
    {
      s++;
    }
/* Scan mantissa digits and put them into "z" */
  while (s[0] == '0')
    {
      s++;
    }
  i = 0;
  j = 1;
  sum = 0;
  comma = -1;
  power = 0;
  weight = MP_RADIX / 10;
  while (s[i] != '\0' && j <= digits && (IS_DIGIT (s[i]) || s[i] == '.'))
    {
      if (s[i] == '.')
	{
	  comma = i;
	}
      else
	{
	  int value = (int) s[i] - (int) '0';
	  sum += weight * value;
	  weight /= 10;
	  power++;
	  if (weight < 1)
	    {
	      MP_DIGIT (z, j++) = sum;
	      sum = 0;
	      weight = MP_RADIX / 10;
	    }
	}
      i++;
    }
/* Store the last digits */
  if (j <= digits)
    {
      MP_DIGIT (z, j++) = sum;
    }
/* See if there is an exponent */
  expo = 0;
  if (s[i] != '\0' && TO_UPPER (s[i]) == EXPONENT_CHAR)
    {
      char *end;
      expo = strtol (&(s[++i]), &end, 10);
      ok = (end[0] == '\0');
    }
  else
    {
      ok = (s[i] == '\0');
    }
/* Calculate effective exponent */
  expo += (comma >= 0 ? comma - 1 : power - 1);
  align_mp (z, &expo, digits);
/* Put the sign and exponent */
  MP_EXPONENT (z) = (MP_DIGIT (z, 1) == 0 ? 0 : (double) expo);
  MP_DIGIT (z, 1) *= sign;
  CHECK_MP_EXPONENT (p, z, "conversion");
  if (errno == 0 && ok)
    {
      return (z);
    }
  else
    {
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
int_to_mp: convert integer k to mp z.                                         */

mp_digit *
int_to_mp (NODE_T * p, mp_digit * z, int k, int digits)
{
  int n = 0, j, sign_k = (k >= 0 ? 1 : -1);
  int k2 = k;
  if (k < 0)
    {
      k = -k;
    }
  while ((k2 /= MP_RADIX) != 0)
    {
      n++;
    }
  SET_MP_ZERO (z, digits);
  MP_EXPONENT (z) = n;
  for (j = 1 + n; j >= 1; j--)
    {
      MP_DIGIT (z, j) = k % MP_RADIX;
      k /= MP_RADIX;
    }
  MP_DIGIT (z, 1) = sign_k * MP_DIGIT (z, 1);
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unsigned_to_mp: convert integer k to mp z.                                    */

mp_digit *
unsigned_to_mp (NODE_T * p, mp_digit * z, unsigned k, int digits)
{
  int n = 0, j;
  unsigned k2 = k;
  while ((k2 /= MP_RADIX) != 0)
    {
      n++;
    }
  SET_MP_ZERO (z, digits);
  MP_EXPONENT (z) = n;
  for (j = 1 + n; j >= 1; j--)
    {
      MP_DIGIT (z, j) = k % MP_RADIX;
      k /= MP_RADIX;
    }
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_to_int: convert mp to an int.
This routines looks a lot like "strtol". We do not use "mp_to_real" since int
could be wider than 2 ** 52.                                                  */

int
mp_to_int (NODE_T * p, mp_digit * z, int digits)
{
  int j, expo = (int) MP_EXPONENT (z);
  int sum = 0, weight = 1;
  BOOL_T negative;
  if (expo >= digits)
    {
      diagnostic (A_RUNTIME_ERROR, p, "M overflow", MOID (p));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  negative = MP_DIGIT (z, 1) < 0;
  if (negative)
    {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  for (j = 1 + expo; j >= 1; j--)
    {
      int term;
      if ((int) MP_DIGIT (z, j) > MAX_INT / weight)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "M overflow", MODE (INT));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      term = (int) MP_DIGIT (z, j) * weight;
      if (sum > MAX_INT - term)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "M overflow", MODE (INT));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      sum += term;
      weight *= MP_RADIX;
    }
  return (negative ? -sum : sum);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_to_unsigned: convert mp to an unsigned.
This routines looks a lot like "strtoul". We do not use "mp_to_real" since 
unsigned could be wider than 2 ** 52.                                         */

unsigned
mp_to_unsigned (NODE_T * p, mp_digit * z, int digits)
{
  int j, expo = (int) MP_EXPONENT (z);
  unsigned sum = 0, weight = 1;
  if (expo >= digits)
    {
      diagnostic (A_RUNTIME_ERROR, p, "M overflow", MOID (p));
      exit_genie (p, A_RUNTIME_ERROR);
    }
  for (j = 1 + expo; j >= 1; j--)
    {
      unsigned term;
      if ((unsigned) MP_DIGIT (z, j) > MAX_UNT / weight)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "M overflow", MODE (BITS));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      term = (unsigned) MP_DIGIT (z, j) * weight;
      if (sum > MAX_UNT - term)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "M overflow", MODE (BITS));
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      sum += term;
      weight *= MP_RADIX;
    }
  return (sum);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
real_to_mp: convert double x to mp z with at most digits precision.           */

mp_digit *
real_to_mp (NODE_T * p, mp_digit * z, double x, int digits)
{
  int j, k, sign_x, sum, weight;
  int expo;
  double a;
  mp_digit *u;
  SET_MP_ZERO (z, digits);
  if (x == 0.0)
    {
      return (z);
    }
/* Small integers can be done better by int_to_mp */
  if (fabs (x) < MP_RADIX)
    {
      if ((double) (int) x == x)
	{
	  return (int_to_mp (p, z, (int) x, digits));
	}
    }
  sign_x = (x > 0 ? 1 : -1);
/* Scale to [0, 0.1> */
  a = x = fabs (x);
  expo = (int) log10 (a);
  a /= ten_to_the_power (expo);
  expo--;
  if (a >= 1)
    {
      a /= 10;
      expo++;
    }
/* Transport digits of x to the mantissa of z */
  k = 0;
  j = 1;
  sum = 0;
  weight = (MP_RADIX / 10);
  u = &MP_DIGIT (z, 1);
  while (j <= digits && k < DBL_DIG)
    {
      double y = floor (a * 10);
      int value = (int) y;
      a = a * 10 - y;
      sum += weight * value;
      weight /= 10;
      if (weight < 1)
	{
	  (u++)[0] = sum;
	  sum = 0;
	  weight = (MP_RADIX / 10);
	}
      k++;
    }
/* Store the last digits */
  if (j <= digits)
    {
      u[0] = sum;
    }
/* Exit */
  align_mp (z, &expo, digits);
  MP_EXPONENT (z) = expo;
  MP_DIGIT (z, 1) *= sign_x;
  CHECK_MP_EXPONENT (p, z, "conversion");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_to_real: convert z to a double.
This routine looks a lot like "strtod".                                       */

double
mp_to_real (NODE_T * p, mp_digit * z, int digits)
{
  (void) p;
  if (MP_EXPONENT (z) * LOG_MP_BASE <= DBL_MIN_10_EXP)
    {
      return (0);
    }
  else
    {
      int j;
      double sum = 0, weight;
      weight = ten_to_the_power ((int) (MP_EXPONENT (z) * LOG_MP_BASE));
      for (j = 1; j <= digits && (j - 2) * LOG_MP_BASE <= DBL_DIG; j++)
	{
	  sum += fabs (MP_DIGIT (z, j)) * weight;
	  weight /= MP_RADIX;
	}
      TEST_REAL_REPRESENTATION (p, sum);
      return (MP_DIGIT (z, 1) >= 0 ? sum : -sum);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
norm_mp_light: normalise positive intermediate w.                                   
Bring every digit back to [0 .. MP_RADIX>.                                     */

static void
norm_mp_light (mp_digit * w, int k, int digits)
{
  int j;
  mp_digit *z;
  for (j = digits, z = &MP_DIGIT (w, digits); j >= k; j--, z--)
    {
      if (z[0] >= MP_RADIX)
	{
	  z[0] -= (mp_digit) MP_RADIX;
	  z[-1] += 1;
	}
      else if (z[0] < 0)
	{
	  z[0] += (mp_digit) MP_RADIX;
	  z[-1] -= 1;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
norm_mp: normalise positive intermediate w.                                   
Bring every digit back to [0 .. MP_RADIX>.                                     */

static void
norm_mp (mp_digit * w, int k, int digits)
{
  int j;
  mp_digit *z;
  for (j = digits, z = &MP_DIGIT (w, digits); j >= k; j--, z--)
    {
      if (z[0] >= MP_RADIX)
	{
	  mp_digit carry = (int) (z[0] / (mp_digit) MP_RADIX);
	  z[0] -= carry * (mp_digit) MP_RADIX;
	  z[-1] += carry;
	}
      else if (z[0] < 0)
	{
	  mp_digit carry = 1 + (int) ((-z[0] - 1) / (mp_digit) MP_RADIX);
	  z[0] += carry * (mp_digit) MP_RADIX;
	  z[-1] -= carry;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
round_mp: round positive w and put it in z.
Assume that w has precision of at least 2 + digits.                           */

static void
round_mp (mp_digit * z, mp_digit * w, int digits)
{
  int last = (MP_DIGIT (w, 1) == 0 ? 2 + digits : 1 + digits);
  if (MP_DIGIT (w, last) >= MP_RADIX / 2)
    {
      MP_DIGIT (w, last - 1) += 1;
    }
  if (MP_DIGIT (w, last - 1) >= MP_RADIX)
    {
      norm_mp (w, 2, last);
    }
  if (MP_DIGIT (w, 1) == 0)
    {
      MOVE_DIGITS (&MP_DIGIT (z, 1), &MP_DIGIT (w, 2), digits);
      MP_EXPONENT (z) = MP_EXPONENT (w) - 1;
    }
  else
    {
/* Normally z is not "w", so no test on this */
      MOVE_DIGITS (&MP_EXPONENT (z), &MP_EXPONENT (w), (1 + digits));
    }
/* Zero is zero is zero */
  if (MP_DIGIT (z, 1) == 0)
    {
      MP_EXPONENT (z) = 0;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
trunc_mp: truncate x at the decimal point and put it in z.                    */

void
trunc_mp (mp_digit * z, mp_digit * x, int digits)
{
  if (MP_EXPONENT (x) < 0)
    {
      SET_MP_ZERO (z, digits);
    }
  else
    {
      int k;
      MOVE_MP (z, x, digits);
      for (k = (int) (MP_EXPONENT (x) + 2); k <= digits; k++)
	{
	  MP_DIGIT (z, k) = 0;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
shorten_mp: shorten and round x and assign it to z.                           */

mp_digit *
shorten_mp (NODE_T * p, mp_digit * z, int digits, mp_digit * x, int digits_x)
{
  if (digits >= digits_x)
    {
      return (NULL);
    }
  else
    {
/* Reserve extra digits for proper rounding */
      int old_sp = stack_pointer, digits_h = digits + 2;
      mp_digit *w = stack_mp (p, digits_h);
      BOOL_T negative = MP_DIGIT (x, 1) < 0;
      if (negative)
	{
	  MP_DIGIT (x, 1) = -MP_DIGIT (x, 1);
	}
      MP_STATUS (w) = 0;
      MP_EXPONENT (w) = MP_EXPONENT (x) + 1;
      MP_DIGIT (w, 1) = 0;
      MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits + 1);
      round_mp (z, w, digits);
      if (negative)
	{
	  MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
	}
      stack_pointer = old_sp;
      return (z);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
lengthen_mp: lengthen x and assign it to z.                                   */

mp_digit *
lengthen_mp (NODE_T * p, mp_digit * z, int digits_z, mp_digit * x, int digits_x)
{
  int j;
  (void) p;
  if (digits_z > digits_x)
    {
      if (z != x)
	{
	  MOVE_DIGITS (&MP_DIGIT (z, 1), &MP_DIGIT (x, 1), digits_x);
	  MP_EXPONENT (z) = MP_EXPONENT (x);
	  MP_STATUS (z) = MP_STATUS (x);
	}
      for (j = 1 + digits_x; j <= digits_z; j++)
	{
	  MP_DIGIT (z, j) = 0;
	}
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_mp: set z to the sum of positive x and positive y.                        */

mp_digit *
add_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  int old_sp, sign_x, sign_y;
  mp_digit *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  old_sp = stack_pointer;
/* Trivial case: x + 0 = x */
  if (MP_DIGIT (x, 1) == 0)
    {
      MOVE_MP (z, y, digits);
      return (z);
    }
  else if (MP_DIGIT (y, 1) == 0)
    {
      MOVE_MP (z, x, digits);
      return (z);
    }
/* We want positive arguments */
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y1 >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  MP_DIGIT (y, 1) = fabs (y1);
  if (sign_x == 1 && sign_y == -1)
    {
      sub_mp (p, z, x, y, digits);
    }
  else if (sign_x == -1 && sign_y == 1)
    {
      sub_mp (p, z, y, x, digits);
    }
  else if (sign_x == -1 && sign_y == -1)
    {
      add_mp (p, z, x, y, digits);
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  else
    {
      int j, digits_h, shl_x, shl_y, exp_z;
/* Preparations, shift */
      if (MP_EXPONENT (x) >= MP_EXPONENT (y))
	{
	  exp_z = (int) MP_EXPONENT (x);
	  shl_x = 0;
	  shl_y = (int) MP_EXPONENT (x) - (int) MP_EXPONENT (y);
	}
      else
	{
	  exp_z = (int) MP_EXPONENT (y);
	  shl_x = (int) MP_EXPONENT (y) - (int) MP_EXPONENT (x);
	  shl_y = 0;
	}
      digits_h = 2 + digits;
      w = stack_mp (p, digits_h);
      MP_EXPONENT (w) = exp_z + 1;
      MP_DIGIT (w, 1) = 0;
/* Add */
      for (j = 1; j <= digits_h; j++)
	{
	  int i_x = j - (int) shl_x;
	  int i_y = j - (int) shl_y;
	  mp_digit x_j = (i_x < 1 || i_x > digits ? 0 : MP_DIGIT (x, i_x));
	  mp_digit y_j = (i_y < 1 || i_y > digits ? 0 : MP_DIGIT (y, i_y));
	  MP_DIGIT (w, j + 1) = x_j + y_j;
	}
      norm_mp_light (w, 2, digits_h);
      round_mp (z, w, digits);
      CHECK_MP_EXPONENT (p, z, "addition");
    }
/* Restore and exit. */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1;		/* In case z IS x OR z IS y */
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sub_mp: set z to the difference of positive x and positive y.                 */

mp_digit *
sub_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  int old_sp, sign_x, sign_y, first_non_zero, k;
  mp_digit *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  BOOL_T negative;
  old_sp = stack_pointer;
/* Trivial cases */
  if (MP_DIGIT (x, 1) == 0)
    {
      MOVE_MP (z, y, digits);
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
      return (z);
    }
  else if (MP_DIGIT (y, 1) == 0)
    {
      MOVE_MP (z, x, digits);
      return (z);
    }
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y1 >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  MP_DIGIT (y, 1) = fabs (y1);
/* We want positive arguments */
  if (sign_x == 1 && sign_y == -1)
    {
      add_mp (p, z, x, y, digits);
    }
  else if (sign_x == -1 && sign_y == 1)
    {
      add_mp (p, z, y, x, digits);
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  else if (sign_x == -1 && sign_y == -1)
    {
      sub_mp (p, z, y, x, digits);
    }
  else
    {
      int j, digits_h, shl_x, shl_y, exp_z;
/* Preparations, shift */
      if (MP_EXPONENT (x) >= MP_EXPONENT (y))
	{
	  exp_z = (int) MP_EXPONENT (x);
	  shl_x = 0;
	  shl_y = (int) MP_EXPONENT (x) - (int) MP_EXPONENT (y);
	}
      else
	{
	  exp_z = (int) MP_EXPONENT (y);
	  shl_x = (int) MP_EXPONENT (y) - (int) MP_EXPONENT (x);
	  shl_y = 0;
	}
      digits_h = 2 + digits;
      w = stack_mp (p, digits_h);
      MP_EXPONENT (w) = exp_z + 1;
      MP_DIGIT (w, 1) = 0;
/* Subtract */
      first_non_zero = -1;
      for (j = 1; j <= digits_h; j++)
	{
	  int k = j + 1;
	  int i_x = j - (int) shl_x;
	  int i_y = j - (int) shl_y;
	  mp_digit x_j = (i_x < 1 || i_x > digits ? 0 : MP_DIGIT (x, i_x));
	  mp_digit y_j = (i_y < 1 || i_y > digits ? 0 : MP_DIGIT (y, i_y));
	  MP_DIGIT (w, k) = x_j - y_j;
	  if (first_non_zero < 0 && MP_DIGIT (w, k) != 0)
	    {
	      first_non_zero = k;
	    }
	}
/* Correct if we subtract large from small */
      negative = MP_DIGIT (w, first_non_zero) < 0;
      if (negative)
	{
	  for (j = first_non_zero; j <= digits_h; j++)
	    {
	      MP_DIGIT (w, j) = -MP_DIGIT (w, j);
	    }
	}
/* Normalise. */
      norm_mp_light (w, 2, digits_h);
      first_non_zero = -1;
      for (j = 1; j <= digits_h && first_non_zero < 0; j++)
	{
	  if (MP_DIGIT (w, j) != 0)
	    {
	      first_non_zero = j;
	    }
	}
      if (first_non_zero > 1)
	{
	  int j = first_non_zero - 1;
	  for (k = 1; k <= digits_h - j; k++)
	    {
	      MP_DIGIT (w, k) = MP_DIGIT (w, k + j);
	      MP_DIGIT (w, k + j) = 0;
	    }
	  MP_EXPONENT (w) -= j;
	}
/* Round. */
      round_mp (z, w, digits);
      if (negative)
	{
	  MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
	}
      CHECK_MP_EXPONENT (p, z, "subtraction");
    }
/* Restore and exit. */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1;		/* In case z IS x OR z IS y */
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mul_mp: set z to the product of x and y.                                      */

mp_digit *
mul_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  mp_digit *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  int i, digits_h, old_sp, sign_x, sign_y, overflow;
  old_sp = stack_pointer;
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y1 >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  MP_DIGIT (y, 1) = fabs (y1);
/* Reserve result */
  digits_h = 2 + digits;
  w = stack_mp (p, digits_h);
  SET_MP_ZERO (w, digits_h);
  MP_EXPONENT (w) = MP_EXPONENT (x) + MP_EXPONENT (y) + 1;
/* Calculate "x * y" */
  overflow = (int) (floor) ((double) MAX_REPR_INT / (2 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
  for (i = digits; i >= 1; i--)
    {
      mp_digit yi = MP_DIGIT (y, i);
      if (yi != 0)
	{
	  int k = digits_h - i;
	  int j = (k > digits ? digits : k);
	  mp_digit *u = &MP_DIGIT (w, i + j);
	  mp_digit *v = &MP_DIGIT (x, j);
	  if (overflow <= 1 || (digits - i + 1) % overflow == 0)
	    {
	      norm_mp (w, 2, digits_h);
	    }
	  while (j-- >= 1)
	    {
	      (u--)[0] += yi * (v--)[0];
	    }
	}
    }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Restore and exit. */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1 * sign_x * sign_y;
  CHECK_MP_EXPONENT (p, z, "multiplication");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
div_mp: set z to the quotient of x and y.

This routine is an implementation of

   D. M. Smith, "A Multiple-Precision Division Algorithm" 
   Mathematics of Computation 66 (1996) 157-163.

This algorithm is O(N ** 2) but runs faster than straightforward methods by 
skipping most of the intermediate normalisation and recovering from wrong 
guesses without separate correction steps.                                    */

mp_digit *
div_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  double xd;
  mp_digit *t, *w, z1, x1 = MP_DIGIT (x, 1), y1 = MP_DIGIT (y, 1);
  int sign_x, sign_y, k, overflow, old_sp, digits_w;
/* Argument check */
  if (y1 == 0)
    {
      return (NULL);
    }
/* Initialisations */
  old_sp = stack_pointer;
/* Determine normalisation interval assuming that q < 2b in each step */
  overflow = (int) (floor) ((double) MAX_REPR_INT / (3 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
/* Work with positive operands. */
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y1 >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  MP_DIGIT (y, 1) = fabs (y1);
/* "w" will be our working nominator in which the quotient develops */
  digits_w = 4 + digits;
  w = stack_mp (p, digits_w);
  MP_EXPONENT (w) = MP_EXPONENT (x) - MP_EXPONENT (y);
  MP_DIGIT (w, 1) = 0;
  MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits);
  MP_DIGIT (w, digits + 2) = 0.0;
  MP_DIGIT (w, digits + 3) = 0.0;
  MP_DIGIT (w, digits + 4) = 0.0;
/* Estimate the denominator. Take four terms to also suit small MP_RADIX */
  xd = (MP_DIGIT (y, 1) * MP_RADIX + MP_DIGIT (y, 2)) * MP_RADIX + MP_DIGIT (y, 3) + MP_DIGIT (y, 4) / MP_RADIX;
/* Main loop */
  t = &MP_DIGIT (w, 2);
  for (k = 1; k <= digits + 2; k++, t++)
    {
      double xn, q;
      int first = k + 2;
/* Normalise only when overflow could have occured */
      if (overflow <= 1 || k % overflow == 0)
	{
	  norm_mp (w, first, digits_w);
	}
/* Estimate quotient digit */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      if (q != 0)
	{
/* Correct the nominator */
	  int j, len = k + digits + 1;
	  int lim = (len < digits_w ? len : digits_w);
	  mp_digit *u = t, *v = &MP_DIGIT (y, 1);
	  for (j = first; j <= lim; j++)
	    {
	      (u++)[0] -= q * (v++)[0];
	    }
	}
      t[0] += t[-1] * MP_RADIX;
      t[-1] = q;
    }
  norm_mp (w, 2, digits_w);
  round_mp (z, w, digits);
/* Exit */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (y, 1) = y1;
  MP_DIGIT (z, 1) = z1 * sign_x * sign_y;
  CHECK_MP_EXPONENT (p, z, "division");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
over_mp: set z to the integer quotient of x and y.                            */

mp_digit *
over_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *x_g, *y_g, *z_g;
/* Argument check */
  if (MP_DIGIT (y, 1) == 0)
    {
      return (NULL);
    }
/* Initialisations */
  x_g = stack_mp (p, digits_g);
  y_g = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  lengthen_mp (p, y_g, digits_g, y, digits);
  div_mp (p, z_g, x_g, y_g, digits_g);
  trunc_mp (z_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mod_mp: set z to x mod y.                                                     */

mp_digit *
mod_mp (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit * y, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *x_g, *y_g, *z_g;
  if (MP_DIGIT (y, 1) == 0)
    {
      return (NULL);
    }
  x_g = stack_mp (p, digits_g);
  y_g = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
  lengthen_mp (p, y_g, digits_g, y, digits);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* x mod y = x - y * trunc (x / y) */
  over_mp (p, z_g, x_g, y_g, digits_g);
  mul_mp (p, z_g, y_g, z_g, digits_g);
  sub_mp (p, z_g, x_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mul_mp_digit: set z to the product of x and digit y.
This is an O(N) routine for multiplication by a short value.                  */

mp_digit *
mul_mp_digit (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit y, int digits)
{
  mp_digit *w, z1, x1 = MP_DIGIT (x, 1), *u, *v;
  int j, digits_h, old_sp, sign_x, sign_y;
  old_sp = stack_pointer;
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  y = fabs (y);
/* Reserve a double precision result */
  digits_h = 2 + digits;
  w = stack_mp (p, digits_h);
  SET_MP_ZERO (w, digits_h);
/* Calculate "x * y" */
  MP_EXPONENT (w) = MP_EXPONENT (x) + 1;
  j = digits;
  u = &MP_DIGIT (w, 1 + digits);
  v = &MP_DIGIT (x, digits);
  while (j-- >= 1)
    {
      (u--)[0] += y * (v--)[0];
    }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Exit */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = z1 * sign_x * sign_y;
  CHECK_MP_EXPONENT (p, z, "multiplication");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
half_mp: set z to the half of x.
This is an O(N) routine for multiplication by a short value.                  */

mp_digit *
half_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  mp_digit *w, z1, x1 = MP_DIGIT (x, 1), *u, *v;
  int j, digits_h, old_sp, sign_x;
/* Argument check */
  old_sp = stack_pointer;
  sign_x = (x1 >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
/* Reserve a double precision result */
  digits_h = 2 + digits;
  w = stack_mp (p, digits_h);
  SET_MP_ZERO (w, digits_h);
/* Calculate "x * 0.5" */
  MP_EXPONENT (w) = MP_EXPONENT (x);
  j = digits;
  u = &MP_DIGIT (w, 1 + digits);
  v = &MP_DIGIT (x, digits);
  while (j-- >= 1)
    {
      (u--)[0] += (MP_RADIX / 2) * (v--)[0];
    }
  norm_mp (w, 2, digits_h);
  round_mp (z, w, digits);
/* Exit */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = z1 * sign_x;
  CHECK_MP_EXPONENT (p, z, "halving");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
div_mp_digit: set z to the quotient of x and digit y.
See notes with "div_mp".                                                      */

mp_digit *
div_mp_digit (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit y, int digits)
{
  double xd;
  mp_digit *t, *w, z1, x1 = MP_DIGIT (x, 1);
  int sign_x, sign_y, k, overflow, old_sp, digits_w;
/* Argument check */
  if (y == 0)
    {
      return (NULL);
    }
/* Initialisations */
  old_sp = stack_pointer;
/* Determine normalisation interval assuming that q < 2b in each step */
  overflow = (int) (floor) ((double) MAX_REPR_INT / (3 * (double) MP_RADIX * (double) MP_RADIX)) - 1;
/* Work with positive operands. */
  sign_x = (x1 >= 0 ? 1 : -1);
  sign_y = (y >= 0 ? 1 : -1);
  MP_DIGIT (x, 1) = fabs (x1);
  y = fabs (y);
/* "w" will be our working nominator in which the quotient develops */
  digits_w = 4 + digits;
  w = stack_mp (p, digits_w);
  MP_EXPONENT (w) = MP_EXPONENT (x);
  MP_DIGIT (w, 1) = 0;
  MOVE_DIGITS (&MP_DIGIT (w, 2), &MP_DIGIT (x, 1), digits);
  MP_DIGIT (w, digits + 2) = 0.0;
  MP_DIGIT (w, digits + 3) = 0.0;
  MP_DIGIT (w, digits + 4) = 0.0;
/* Estimate the denominator */
  xd = (double) y *MP_RADIX * MP_RADIX;
/* Main loop */
  t = &MP_DIGIT (w, 2);
  for (k = 1; k <= digits + 2; k++, t++)
    {
      double xn, q;
      int first = k + 2;
/* Normalise only when overflow could have occured */
      if (overflow <= 1 || k % overflow == 0)
	{
	  norm_mp (w, first, digits_w);
	}
/* Estimate quotient digit */
      xn = ((t[-1] * MP_RADIX + t[0]) * MP_RADIX + t[1]) * MP_RADIX + (digits_w >= (first + 2) ? t[2] : 0);
      q = (long) (xn / xd);
      t[0] += (t[-1] * MP_RADIX - q * y);
      t[-1] = q;
    }
  norm_mp (w, 2, digits_w);
  round_mp (z, w, digits);
/* Exit */
  stack_pointer = old_sp;
  z1 = MP_DIGIT (z, 1);
  MP_DIGIT (x, 1) = x1;
  MP_DIGIT (z, 1) = z1 * sign_x * sign_y;
  CHECK_MP_EXPONENT (p, z, "division");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
over_mp_digit: set z to the integer quotient of x and y.                      */

mp_digit *
over_mp_digit (NODE_T * p, mp_digit * z, mp_digit * x, mp_digit y, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *x_g, *z_g;
/* Argument check */
  if (y == 0)
    {
      return (NULL);
    }
/* Initialisations */
  x_g = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  div_mp_digit (p, z_g, x_g, y, digits_g);
  trunc_mp (z_g, z_g, digits_g);
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
rec_mp: set z to the reciprocal of x.                                         */

mp_digit *
rec_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer;
  mp_digit *one;
  if (MP_DIGIT (x, 1) == 0)
    {
      return (NULL);
    }
  one = stack_mp (p, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits);
  div_mp (p, z, one, x, digits);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pow_mp_int: set z to x ** n.
Use a binary shift.                                                           */

mp_digit *
pow_mp_int (NODE_T * p, mp_digit * z, mp_digit * x, int n, int digits)
{
  int old_sp = stack_pointer, bit, digits_g = digits + MP_GUARDS (digits);
  BOOL_T negative;
  mp_digit *z_g = stack_mp (p, digits_g);
  mp_digit *x_g = stack_mp (p, digits_g);
  set_mp_short (z_g, (mp_digit) 1, 0, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  negative = (n < 0);
  if (negative)
    {
      n = -n;
    }
  bit = 1;
  while ((unsigned) bit <= (unsigned) n)
    {
      if (n & bit)
	{
	  mul_mp (p, z_g, z_g, x_g, digits_g);
	}
      mul_mp (p, x_g, x_g, x_g, digits_g);
      bit *= 2;
    }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = old_sp;
  if (negative)
    {
      rec_mp (p, z, z, digits);
    }
  CHECK_MP_EXPONENT (p, z, "power");
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
eps_mp: test on |z| > 0.001 for argument reduction in "sin" and "exp".        */

static BOOL_T
eps_mp (mp_digit * z, int digits)
{
  if (MP_DIGIT (z, 1) == 0)
    {
      return (A_FALSE);
    }
  else if (MP_EXPONENT (z) > -1)
    {
      return (A_TRUE);
    }
  else if (MP_EXPONENT (z) < -1)
    {
      return (A_FALSE);
    }
  else
    {
#if (MP_RADIX == DEFAULT_MP_RADIX)
/* More or less optimised for LONG and default LONG LONG precisions */
      return (digits <= 10 ? fabs (MP_DIGIT (z, 1)) > 100000 : fabs (MP_DIGIT (z, 1)) > 10000);
#else
      switch (LOG_MP_BASE)
	{
	case 3:
	  {
	    return (fabs (MP_DIGIT (z, 1)) > 1);
	  }
	case 4:
	  {
	    return (fabs (MP_DIGIT (z, 1)) > 10);
	  }
	case 5:
	  {
	    return (fabs (MP_DIGIT (z, 1)) > 100);
	  }
	case 6:
	  {
	    return (fabs (MP_DIGIT (z, 1)) > 1000);
	  }
	default:
	  {
	    abend (A_TRUE, "unexpected mp base", "");
	    return (A_FALSE);
	  }
	}
#endif
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sqrt_mp: set z to sqrt (x).                                                   */

mp_digit *
sqrt_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = 2 * digits + MP_GUARDS (digits), digits_h;
  mp_digit *tmp, *x_g, *z_g;
  BOOL_T reciprocal = A_FALSE;
  if (MP_DIGIT (x, 1) == 0)
    {
      stack_pointer = old_sp;
      SET_MP_ZERO (z, digits);
      return (z);
    }
  if (MP_DIGIT (x, 1) < 0)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  z_g = stack_mp (p, digits_g);
  x_g = stack_mp (p, digits_g);
  tmp = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* Scaling for small x; sqrt (x) = 1 / sqrt (1 / x) */
  if ((reciprocal = MP_EXPONENT (x_g) < 0) == A_TRUE)
    {
      rec_mp (p, x_g, x_g, digits_g);
    }
  if (fabs (MP_EXPONENT (x_g)) >= 2)
    {
/* For extreme arguments we want accurate results as well */
      int expo = (int) MP_EXPONENT (x_g);
      MP_EXPONENT (x_g) = expo % 2;
      sqrt_mp (p, z_g, x_g, digits_g);
      MP_EXPONENT (z_g) += expo / 2;
    }
  else
    {
/* Argument is in range. Estimate the root as double */
      int decimals;
      double x_d = mp_to_real (p, x_g, digits_g);
      real_to_mp (p, z_g, sqrt (x_d), digits_g);
/* Newton's method: x<n+1> = (x<n> + a / x<n>) / 2 */
      decimals = DOUBLE_ACCURACY;
      do
	{
	  decimals <<= 1;
	  digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
	  div_mp (p, tmp, x_g, z_g, digits_h);
	  add_mp (p, tmp, z_g, tmp, digits_h);
	  half_mp (p, z_g, tmp, digits_h);
	}
      while (decimals < 2 * digits_g * LOG_MP_BASE);
    }
  if (reciprocal)
    {
      rec_mp (p, z_g, z_g, digits);
    }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
curt_mp: set z to curt (x), the cube root.                                    */

mp_digit *
curt_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits), digits_h;
  mp_digit *tmp, *x_g, *z_g;
  BOOL_T reciprocal = A_FALSE, change_sign = A_FALSE;
  if (MP_DIGIT (x, 1) == 0)
    {
      stack_pointer = old_sp;
      SET_MP_ZERO (z, digits);
      return (z);
    }
  if (MP_DIGIT (x, 1) < 0)
    {
      change_sign = A_TRUE;
      MP_DIGIT (x, 1) = -MP_DIGIT (x, 1);
    }
  z_g = stack_mp (p, digits_g);
  x_g = stack_mp (p, digits_g);
  tmp = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
/* Scaling for small x; curt (x) = 1 / curt (1 / x) */
  if ((reciprocal = MP_EXPONENT (x_g) < 0) == A_TRUE)
    {
      rec_mp (p, x_g, x_g, digits_g);
    }
  if (fabs (MP_EXPONENT (x_g)) >= 3)
    {
/* For extreme arguments we want accurate results as well */
      int expo = (int) MP_EXPONENT (x_g);
      MP_EXPONENT (x_g) = expo % 3;
      curt_mp (p, z_g, x_g, digits_g);
      MP_EXPONENT (z_g) += expo / 3;
    }
  else
    {
/* Argument is in range. Estimate the root as double */
      int decimals;
      real_to_mp (p, z_g, curt (mp_to_real (p, x_g, digits_g)), digits_g);
/* Newton's method: x<n+1> = (2 x<n> + a / x<n> ^ 2) / 3 */
      decimals = DOUBLE_ACCURACY;
      do
	{
	  decimals <<= 1;
	  digits_h = MINIMUM (1 + decimals / LOG_MP_BASE, digits_g);
	  mul_mp (p, tmp, z_g, z_g, digits_h);
	  div_mp (p, tmp, x_g, tmp, digits_h);
	  add_mp (p, tmp, z_g, tmp, digits_h);
	  add_mp (p, tmp, z_g, tmp, digits_h);
	  div_mp_digit (p, z_g, tmp, (mp_digit) 3, digits_h);
	}
      while (decimals < digits_g * LOG_MP_BASE);
    }
  if (reciprocal)
    {
      rec_mp (p, z_g, z_g, digits);
    }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  if (change_sign)
    {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
exp_mp: set z to exp (x).
Argument is reduced by using exp (z / (2 ** n)) ** (2 ** n) = exp(z).         */

mp_digit *
exp_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int m, n, old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *x_g, *sum, *pow, *fac, *tmp;
  BOOL_T iterate;
  if (MP_DIGIT (x, 1) == 0)
    {
      set_mp_short (z, (mp_digit) 1, 0, digits);
      return (z);
    }
  x_g = stack_mp (p, digits_g);
  sum = stack_mp (p, digits_g);
  pow = stack_mp (p, digits_g);
  fac = stack_mp (p, digits_g);
  tmp = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  m = 0;
/* Scale x down */
  while (eps_mp (x_g, digits_g))
    {
      m++;
      half_mp (p, x_g, x_g, digits_g);
    }
/* Calculate Taylor sum
   exp (z) = 1 + z / 1 ! + z ** 2 / 2 ! + ...  */
  set_mp_short (sum, (mp_digit) 1, 0, digits_g);
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
  set_mp_short (fac, (mp_digit) 3628800, 0, digits_g);
  n = 10;
#else
  set_mp_short (fac, (mp_digit) 2, 0, digits_g);
  n = 2;
#endif
  iterate = MP_DIGIT (pow, 1) != 0;
  while (iterate)
    {
      div_mp (p, tmp, pow, fac, digits_g);
      if (MP_EXPONENT (tmp) <= (MP_EXPONENT (sum) - digits_g))
	{
	  iterate = A_FALSE;
	}
      else
	{
	  add_mp (p, sum, sum, tmp, digits_g);
	  mul_mp (p, pow, pow, x_g, digits_g);
	  n++;
	  mul_mp_digit (p, fac, fac, (mp_digit) n, digits_g);
	}
    }
/* Square exp (x) up */
  while (m--)
    {
      mul_mp (p, sum, sum, sum, digits_g);
    }
  shorten_mp (p, z, digits, sum, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_ln_scale: ln scale with digits precision.                                  */

mp_digit *
mp_ln_scale (NODE_T * p, mp_digit * z, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *z_g = stack_mp (p, digits_g);
/* First see if we can restore a previous calculation */
  if (digits_g <= mp_ln_scale_size)
    {
      MOVE_MP (z_g, ref_mp_ln_scale, digits_g);
    }
  else
    {
/* No luck with the kept value, we generate a longer one */
      set_mp_short (z_g, (mp_digit) 1, 1, digits_g);
      ln_mp (p, z_g, z_g, digits_g);
      ref_mp_ln_scale = (mp_digit *) get_heap_space ((unsigned) SIZE_MP (digits_g));
      MOVE_MP (ref_mp_ln_scale, z_g, digits_g);
      mp_ln_scale_size = digits_g;
    }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_ln_10: ln 10 with digits precision.                                        */

mp_digit *
mp_ln_10 (NODE_T * p, mp_digit * z, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *z_g = stack_mp (p, digits_g);
/* First see if we can restore a previous calculation */
  if (digits_g <= mp_ln_10_size)
    {
      MOVE_MP (z_g, ref_mp_ln_10, digits_g);
    }
  else
    {
/* No luck with the kept value, we generate a longer one */
      set_mp_short (z_g, (mp_digit) 10, 0, digits_g);
      ln_mp (p, z_g, z_g, digits_g);
      ref_mp_ln_10 = (mp_digit *) get_heap_space ((unsigned) SIZE_MP (digits_g));
      MOVE_MP (ref_mp_ln_10, z_g, digits_g);
      mp_ln_10_size = digits_g;
    }
  shorten_mp (p, z, digits, z_g, digits_g);
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
ln_mp: set z to ln (x).
Depending on the argument we choose either Taylor or Newton.                  */

mp_digit *
ln_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  BOOL_T negative, scale;
  mp_digit *x_g, *z_g, expo = 0;
  if (MP_DIGIT (x, 1) <= 0)
    {
      return (NULL);
    }
  x_g = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  z_g = stack_mp (p, digits_g);
/* We use ln (1 / x) = - ln (x) */
  negative = MP_EXPONENT (x_g) < 0;
  if (negative)
    {
      rec_mp (p, x_g, x_g, digits);
    }
/* We want correct results for extreme arguments. We scale when "x_g" exceeds
   "MP_RADIX ** +- 2", using ln (x * MP_RADIX ** n) = ln (x) + n * ln (MP_RADIX).*/
  scale = (fabs (MP_EXPONENT (x_g)) >= 2);
  if (scale)
    {
      expo = MP_EXPONENT (x_g);
      MP_EXPONENT (x_g) = 0;
    }
  if (MP_EXPONENT (x_g) == 0 && MP_DIGIT (x_g, 1) == 1 && MP_DIGIT (x_g, 2) == 0)
    {
/* Taylor sum for x close to unity.
   ln (x) = (x - 1) - (x - 1) ** 2 / 2 + (x - 1) ** 3 / 3 - ...
   This is faster for small x and avoids cancellation.                        */
      mp_digit *one = stack_mp (p, digits_g);
      mp_digit *tmp = stack_mp (p, digits_g);
      mp_digit *pow = stack_mp (p, digits_g);
      int n = 2;
      BOOL_T iterate;
      set_mp_short (one, (mp_digit) 1, 0, digits_g);
      sub_mp (p, x_g, x_g, one, digits_g);
      mul_mp (p, pow, x_g, x_g, digits_g);
      MOVE_MP (z_g, x_g, digits_g);
      iterate = MP_DIGIT (pow, 1) != 0;
      while (iterate)
	{
	  div_mp_digit (p, tmp, pow, (mp_digit) n, digits_g);
	  if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g))
	    {
	      iterate = A_FALSE;
	    }
	  else
	    {
	      MP_DIGIT (tmp, 1) = (n % 2 == 0 ? -MP_DIGIT (tmp, 1) : MP_DIGIT (tmp, 1));
	      add_mp (p, z_g, z_g, tmp, digits_g);
	      mul_mp (p, pow, pow, x_g, digits_g);
	      n++;
	    }
	}
    }
  else
    {
/* Newton's method: x<n+1> = x<n> - 1 + a / exp (x<n>). */
      mp_digit *tmp, *z_0, *one;
      int decimals;
      tmp = stack_mp (p, digits_g);
      one = stack_mp (p, digits_g);
      z_0 = stack_mp (p, digits_g);
      set_mp_short (one, (mp_digit) 1, 0, digits_g);
      SET_MP_ZERO (z_0, digits_g);
/* Construct an estimate */
      real_to_mp (p, z_g, log (mp_to_real (p, x_g, digits_g)), digits_g);
      decimals = DOUBLE_ACCURACY;
      do
	{
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
/* Inverse scaling */
  if (scale)
    {
/* ln (x * MP_RADIX ** n) = ln (x) + n * ln (MP_RADIX) */
      mp_digit *ln_base = stack_mp (p, digits_g);
      (void) mp_ln_scale (p, ln_base, digits_g);
      mul_mp_digit (p, ln_base, ln_base, (mp_digit) expo, digits_g);
      add_mp (p, z_g, z_g, ln_base, digits_g);
    }
  if (negative)
    {
      MP_DIGIT (z_g, 1) = -MP_DIGIT (z_g, 1);
    }
  shorten_mp (p, z, digits, z_g, digits_g);
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
log_mp: set z to log (x).                                                     */

mp_digit *
log_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer;
  mp_digit *ln_10 = stack_mp (p, digits);
  if (ln_mp (p, z, x, digits) == NULL)
    {
      return (NULL);
    }
  (void) mp_ln_10 (p, ln_10, digits);
  div_mp (p, z, z, ln_10, digits);
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mp_pi: return pi with digits precision.
Uses Borwein & Borwein AGM.                                                   */

mp_digit *
mp_pi (NODE_T * p, mp_digit * api, int mult, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  BOOL_T iterate;
  mp_digit *pi_g, *one, *two, *x_g, *y_g, *u_g, *v_g;
  pi_g = stack_mp (p, digits_g);
/* First see if we can restore a previous calculation. */
  if (digits_g <= mp_pi_size)
    {
      MOVE_MP (pi_g, ref_mp_pi, digits_g);
    }
  else
    {
/* No luck with the kept value, hence we generate a longer "pi".
   Calculate "pi" using the Borwein & Borwein AGM algorithm.
   This AGM doubles the numbers of digits at every pass.                      */
      one = stack_mp (p, digits_g);
      two = stack_mp (p, digits_g);
      x_g = stack_mp (p, digits_g);
      y_g = stack_mp (p, digits_g);
      u_g = stack_mp (p, digits_g);
      v_g = stack_mp (p, digits_g);
      set_mp_short (one, (mp_digit) 1, 0, digits_g);
      set_mp_short (two, (mp_digit) 2, 0, digits_g);
      set_mp_short (x_g, (mp_digit) 2, 0, digits_g);
      sqrt_mp (p, x_g, x_g, digits_g);
      add_mp (p, pi_g, x_g, two, digits_g);
      sqrt_mp (p, y_g, x_g, digits_g);
      iterate = A_TRUE;
      while (iterate)
	{
/* New x */
	  sqrt_mp (p, u_g, x_g, digits_g);
	  div_mp (p, v_g, one, u_g, digits_g);
	  add_mp (p, u_g, u_g, v_g, digits_g);
	  half_mp (p, x_g, u_g, digits_g);
/* New pi */
	  add_mp (p, u_g, x_g, one, digits_g);
	  add_mp (p, v_g, y_g, one, digits_g);
	  div_mp (p, u_g, u_g, v_g, digits_g);
	  mul_mp (p, v_g, pi_g, u_g, digits_g);
/* Done yet? */
	  if (same_mp (p, v_g, pi_g, digits_g))
	    {
	      iterate = A_FALSE;
	    }
	  else
	    {
	      MOVE_MP (pi_g, v_g, digits_g);
/* New y */
	      sqrt_mp (p, u_g, x_g, digits_g);
	      div_mp (p, v_g, one, u_g, digits_g);
	      mul_mp (p, u_g, y_g, u_g, digits_g);
	      add_mp (p, u_g, u_g, v_g, digits_g);
	      add_mp (p, v_g, y_g, one, digits_g);
	      div_mp (p, y_g, u_g, v_g, digits_g);
	    }
	}
/* Keep the result for future restore */
      ref_mp_pi = (mp_digit *) get_heap_space ((unsigned) SIZE_MP (digits_g));
      MOVE_MP (ref_mp_pi, pi_g, digits_g);
      mp_pi_size = digits_g;
    }
  switch (mult)
    {
    case MP_PI:
      {
	break;
      }
    case MP_TWO_PI:
      {
	mul_mp_digit (p, pi_g, pi_g, (mp_digit) 2, digits_g);
	break;
      }
    case MP_HALF_PI:
      {
	half_mp (p, pi_g, pi_g, digits_g);
	break;
      }
    }
  shorten_mp (p, api, digits, pi_g, digits_g);
  stack_pointer = old_sp;
  return (api);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sin_mp: set z to sin (x).
Use triple-angle relation to reduce argument.                                 */

mp_digit *
sin_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, m, n, digits_g = digits + MP_GUARDS (digits);
  BOOL_T flip, negative, iterate, even;
  mp_digit *x_g, *pi, *tpi, *hpi, *sqr, *tmp, *pow, *fac, *z_g;
/* We will use "pi" */
  pi = stack_mp (p, digits_g);
  tpi = stack_mp (p, digits_g);
  hpi = stack_mp (p, digits_g);
  mp_pi (p, pi, MP_PI, digits_g);
  mp_pi (p, tpi, MP_TWO_PI, digits_g);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
/* Argument reduction (1): sin (x) = sin (x mod 2 pi) */
  x_g = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mod_mp (p, x_g, x_g, tpi, digits_g);
/* Argument reduction (2): sin (-x) = sin (x) 
                           sin (x) = - sin (x - pi); pi < x <= 2 pi
                           sin (x) = sin (pi - x);   pi / 2 < x <= pi */
  negative = MP_DIGIT (x_g, 1) < 0;
  if (negative)
    {
      MP_DIGIT (x_g, 1) = -MP_DIGIT (x_g, 1);
    }
  tmp = stack_mp (p, digits_g);
  sub_mp (p, tmp, x_g, pi, digits_g);
  flip = MP_DIGIT (tmp, 1) > 0;
  if (flip)
    {				/* x > pi */
      sub_mp (p, x_g, x_g, pi, digits_g);
    }
  sub_mp (p, tmp, x_g, hpi, digits_g);
  if (MP_DIGIT (tmp, 1) > 0)
    {				/* x > pi / 2 */
      sub_mp (p, x_g, pi, x_g, digits_g);
    }
/* Argument reduction (3): sin (3x) = sin (x) * (3 - 4 sin ^ 2 (x)) */
  m = 0;
  while (eps_mp (x_g, digits_g))
    {
      m++;
      div_mp_digit (p, x_g, x_g, (mp_digit) 3, digits_g);
    }
/* Taylor sum */
  sqr = stack_mp (p, digits_g);
  pow = stack_mp (p, digits_g);
  fac = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
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
  set_mp_short (fac, (mp_digit) 362880, 0, digits_g);
  n = 9;
  even = A_TRUE;
#else
  set_mp_short (fac, (mp_digit) 6, 0, digits_g);
  n = 3;
  even = A_FALSE;
#endif
  iterate = MP_DIGIT (pow, 1) != 0;
  while (iterate)
    {
      div_mp (p, tmp, pow, fac, digits_g);
      if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g))
	{
	  iterate = A_FALSE;
	}
      else
	{
	  if (even)
	    {
	      add_mp (p, z_g, z_g, tmp, digits_g);
	      even = A_FALSE;
	    }
	  else
	    {
	      sub_mp (p, z_g, z_g, tmp, digits_g);
	      even = A_TRUE;
	    }
	  mul_mp (p, pow, pow, sqr, digits_g);
	  mul_mp_digit (p, fac, fac, (mp_digit) (++n), digits_g);
	  mul_mp_digit (p, fac, fac, (mp_digit) (++n), digits_g);
	}
    }
/* Inverse scaling using sin (3x) = sin (x) * (3 - 4 sin ** 2 (x))
   Use existing mp's for intermediates. */
  set_mp_short (fac, (mp_digit) 3, 0, digits_g);
  while (m--)
    {
      mul_mp (p, pow, z_g, z_g, digits_g);
      mul_mp_digit (p, pow, pow, (mp_digit) 4, digits_g);
      sub_mp (p, pow, fac, pow, digits_g);
      mul_mp (p, z_g, pow, z_g, digits_g);
    }
  shorten_mp (p, z, digits, z_g, digits_g);
  if (negative ^ flip)
    {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
cos_mp: set z to cos (x).
Use cos (x) = sin (pi / 2 - x).                                               
Compute x mod 2 pi before subtracting to avoid cancellation.                  */

mp_digit *
cos_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *hpi, *tpi, *x_g;
  hpi = stack_mp (p, digits_g);
  tpi = stack_mp (p, digits_g);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
  mp_pi (p, tpi, MP_TWO_PI, digits_g);
  x_g = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mod_mp (p, x_g, x_g, tpi, digits_g);
  sub_mp (p, x_g, hpi, x_g, digits_g);
  shorten_mp (p, x, digits, x_g, digits_g);
  sin_mp (p, z, x, digits);
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tan_mp: set z to tan (x).
Use tan (x) = sin (x) / sqrt (1 - sin ^ 2 (x)).                               */

mp_digit *
tan_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *sns, *cns, *one, *pi, *hpi, *x_g, *y_g;
  BOOL_T negate;
  one = stack_mp (p, digits);
  pi = stack_mp (p, digits_g);
  hpi = stack_mp (p, digits_g);
  x_g = stack_mp (p, digits_g);
  y_g = stack_mp (p, digits_g);
  sns = stack_mp (p, digits);
  cns = stack_mp (p, digits);
/* Argument mod pi */
  mp_pi (p, pi, MP_PI, digits_g);
  mp_pi (p, hpi, MP_HALF_PI, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  mod_mp (p, x_g, x_g, pi, digits_g);
  if (MP_DIGIT (x_g, 1) >= 0)
    {
      sub_mp (p, y_g, x_g, hpi, digits_g);
      negate = MP_DIGIT (y_g, 1) > 0;
    }
  else
    {
      add_mp (p, y_g, x_g, hpi, digits_g);
      negate = MP_DIGIT (y_g, 1) < 0;
    }
  shorten_mp (p, x, digits, x_g, digits_g);
/* tan(x) = sin(x) / sqrt (1 - sin ** 2 (x)) */
  sin_mp (p, sns, x, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits);
  mul_mp (p, cns, sns, sns, digits);
  sub_mp (p, cns, one, cns, digits);
  sqrt_mp (p, cns, cns, digits);
  if (div_mp (p, z, sns, cns, digits) == NULL)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  stack_pointer = old_sp;
  if (negate)
    {
      MP_DIGIT (z, 1) = -MP_DIGIT (z, 1);
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
asin_mp: set z to arcsin (x).                                                 */

mp_digit *
asin_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *one, *x_g, *y, *z_g;
  y = stack_mp (p, digits);
  x_g = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
  one = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits_g);
  mul_mp (p, z_g, x_g, x_g, digits_g);
  sub_mp (p, z_g, one, z_g, digits_g);
  if (sqrt_mp (p, z_g, z_g, digits) == NULL)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  if (MP_DIGIT (z_g, 1) == 0)
    {
      mp_pi (p, z, MP_HALF_PI, digits);
      MP_DIGIT (z, 1) = (MP_DIGIT (x_g, 1) >= 0 ? MP_DIGIT (z, 1) : -MP_DIGIT (z, 1));
      stack_pointer = old_sp;
      return (z);
    }
  if (div_mp (p, x_g, x_g, z_g, digits_g) == NULL)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  shorten_mp (p, y, digits, x_g, digits_g);
  atan_mp (p, x, y, digits);
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
acos_mp: set z to arccos (x).                                                 */

mp_digit *
acos_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *one, *x_g, *y, *z_g;
  BOOL_T negative = MP_DIGIT (x, 1) < 0;
  if (MP_DIGIT (x, 1) == 0)
    {
      mp_pi (p, z, MP_HALF_PI, digits);
      stack_pointer = old_sp;
      return (z);
    }
  y = stack_mp (p, digits);
  x_g = stack_mp (p, digits_g);
  z_g = stack_mp (p, digits_g);
  one = stack_mp (p, digits_g);
  lengthen_mp (p, x_g, digits_g, x, digits);
  set_mp_short (one, (mp_digit) 1, 0, digits_g);
  mul_mp (p, z_g, x_g, x_g, digits_g);
  sub_mp (p, z_g, one, z_g, digits_g);
  if (sqrt_mp (p, z_g, z_g, digits) == NULL)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  if (div_mp (p, x_g, z_g, x_g, digits_g) == NULL)
    {
      stack_pointer = old_sp;
      return (NULL);
    }
  shorten_mp (p, y, digits, x_g, digits_g);
  atan_mp (p, x, y, digits);
  if (negative)
    {
      mp_pi (p, y, MP_PI, digits);
      add_mp (p, x, x, y, digits);
    }
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
atan_mp: set z to arctan (x).
Depending on the argument we choose either Taylor or Newton.                  */

mp_digit *
atan_mp (NODE_T * p, mp_digit * z, mp_digit * x, int digits)
{
  int old_sp = stack_pointer, digits_g = digits + MP_GUARDS (digits);
  mp_digit *x_g = stack_mp (p, digits_g);
  mp_digit *z_g = stack_mp (p, digits_g);
  BOOL_T flip, negative;
  if (MP_DIGIT (x, 1) == 0)
    {
      stack_pointer = old_sp;
      SET_MP_ZERO (z, digits);
      return (z);
    }
  lengthen_mp (p, x_g, digits_g, x, digits);
  negative = (MP_DIGIT (x_g, 1) < 0);
  if (negative)
    {
      MP_DIGIT (x_g, 1) = -MP_DIGIT (x_g, 1);
    }
/* For larger arguments we use atan(x) = pi/2 - atan(1/x) */
  flip = ((MP_EXPONENT (x_g) > 0) || (MP_EXPONENT (x_g) == 0 && MP_DIGIT (x_g, 1) > 1)) && (MP_DIGIT (x_g, 1) != 0);
  if (flip)
    {
      rec_mp (p, x_g, x_g, digits_g);
    }
  if (MP_EXPONENT (x_g) < -1 || (MP_EXPONENT (x_g) == -1 && MP_DIGIT (x_g, 1) < MP_RADIX / 100))
    {
/* Taylor sum for x close to zero.
   arctan (x) = x - x ** 3 / 3 + x ** 5 / 5 - x ** 7 / 7 + ..
   This is faster for small x and avoids cancellation. */
      mp_digit *tmp = stack_mp (p, digits_g);
      mp_digit *pow = stack_mp (p, digits_g);
      mp_digit *sqr = stack_mp (p, digits_g);
      int n = 3;
      BOOL_T iterate, even;
      mul_mp (p, sqr, x_g, x_g, digits_g);
      mul_mp (p, pow, sqr, x_g, digits_g);
      MOVE_MP (z_g, x_g, digits_g);
      even = A_FALSE;
      iterate = MP_DIGIT (pow, 1) != 0;
      while (iterate)
	{
	  div_mp_digit (p, tmp, pow, (mp_digit) n, digits_g);
	  if (MP_EXPONENT (tmp) <= (MP_EXPONENT (z_g) - digits_g))
	    {
	      iterate = A_FALSE;
	    }
	  else
	    {
	      if (even)
		{
		  add_mp (p, z_g, z_g, tmp, digits_g);
		  even = A_FALSE;
		}
	      else
		{
		  sub_mp (p, z_g, z_g, tmp, digits_g);
		  even = A_TRUE;
		}
	      mul_mp (p, pow, pow, sqr, digits_g);
	      n += 2;
	    }
	}
    }
  else
    {
/* Newton's method: x<n+1> = x<n> - cos (x<n>) * (sin (x<n>) - a cos (x<n>)) */
      mp_digit *tmp = stack_mp (p, digits_g);
      mp_digit *z_0 = stack_mp (p, digits_g);
      mp_digit *sns = stack_mp (p, digits_g);
      mp_digit *cns = stack_mp (p, digits_g);
      mp_digit *one = stack_mp (p, digits_g);
      int decimals, digits_h;
      SET_MP_ZERO (z_0, digits_g);
      set_mp_short (one, (mp_digit) 1, 0, digits_g);
/* Construct an estimate */
      real_to_mp (p, z_g, atan (mp_to_real (p, x_g, digits_g)), digits_g);
      decimals = DOUBLE_ACCURACY;
      do
	{
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
  if (flip)
    {
      mp_digit *hpi = stack_mp (p, digits_g);
      sub_mp (p, z_g, mp_pi (p, hpi, MP_HALF_PI, digits_g), z_g, digits_g);
    }
  shorten_mp (p, z, digits, z_g, digits_g);
  MP_DIGIT (z, 1) = (negative ? -MP_DIGIT (z, 1) : MP_DIGIT (z, 1));
/* Exit */
  stack_pointer = old_sp;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
stack_mp_bits: convert z to a row of unsigned in the stack.                   */

unsigned *
stack_mp_bits (NODE_T * p, mp_digit * z, MOID_T * m)
{
  int digits = get_mp_digits (m), words = get_mp_bits_words (m), k, lim;
  unsigned *row, mask;
  mp_digit *u, *v, *w;
  row = (unsigned *) STACK_ADDRESS (stack_pointer);
  INCREMENT_STACK_POINTER (p, words * SIZE_OF (unsigned));
  u = stack_mp (p, digits);
  v = stack_mp (p, digits);
  w = stack_mp (p, digits);
  MOVE_MP (u, z, digits);
/* Argument check. */
  if (MP_DIGIT (u, 1) < 0.0)
    {
      errno = EDOM;
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, (m == MODE (LONG_BITS) ? MODE (LONG_INT) : MODE (LONGLONG_INT)));
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Convert radix MP_BITS_RADIX number. */
  for (k = words - 1; k >= 0; k--)
    {
      MOVE_MP (w, u, digits);
      over_mp_digit (p, u, u, MP_BITS_RADIX, digits);
      mul_mp_digit (p, v, u, MP_BITS_RADIX, digits);
      sub_mp (p, v, w, v, digits);
      row[k] = (int) MP_DIGIT (v, 1);
    }
/* Test on overflow: too many bits or not reduced to 0. */
  mask = 0x1;
  lim = get_mp_bits_width (m) % MP_BITS_BITS;
  for (k = 1; k < lim; k++)
    {
      mask <<= 1;
      mask |= 0x1;
    }
  if ((row[0] & ~mask) != 0x0 || MP_DIGIT (u, 1) != 0.0)
    {
      errno = ERANGE;
      diagnostic (A_RUNTIME_ERROR, p, OUT_OF_BOUNDS, m);
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Exit. */
  return (row);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_long_bits_value                                                         */

void
check_long_bits_value (NODE_T * p, mp_digit * u, MOID_T * m)
{
  if (MP_EXPONENT (u) >= get_mp_digits (m) - 1)
    {
      ADDR_T save_sp = stack_pointer;
      stack_mp_bits (p, u, m);
      stack_pointer = save_sp;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pack_bits: convert row of unsigned to bits.                                   */

mp_digit *
pack_mp_bits (NODE_T * p, mp_digit * u, unsigned *row, MOID_T * m)
{
  int digits = get_mp_digits (m), words = get_mp_bits_words (m), k, lim;
  ADDR_T save_sp = stack_pointer;
  mp_digit *v = stack_mp (p, digits), *w = stack_mp (p, digits);
/* Discard excess bits. */
  unsigned mask = 0x1, musk = 0x0;
  lim = get_mp_bits_width (m) % MP_BITS_BITS;
  for (k = 1; k < lim; k++)
    {
      mask <<= 1;
      mask |= 0x1;
    }
  row[0] &= mask;
  for (k = 1; k < (BITS_WIDTH - MP_BITS_BITS); k++)
    {
      musk <<= 1;
    }
  for (k = 0; k < MP_BITS_BITS; k++)
    {
      musk <<= 1;
      musk |= 0x1;
    }
/* Convert. */
  SET_MP_ZERO (u, digits);
  set_mp_short (v, (mp_digit) 1, 0, digits);
  for (k = words - 1; k >= 0; k--)
    {
      mul_mp_digit (p, w, v, (mp_digit) (musk & row[k]), digits);
      add_mp (p, u, u, w, digits);
      if (k != 0)
	{
	  mul_mp_digit (p, v, v, MP_BITS_RADIX, digits);
	}
    }
  stack_pointer = save_sp;
  return (u);
}
