//! @file rts-int128.c
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
//! 128-bit INT support.

// A68G keeps this code since some (for instance 32-bit) platforms do not
// natively support a 128-bit int type.

#include "a68g.h"

#if (A68_LEVEL >= 3)

#include "a68g-prelude.h"
#include "a68g-genie.h"
#include "a68g-double.h"

void m64to128 (DOUBLE_NUM_T * w, UNSIGNED_T u, UNSIGNED_T v)
{
// Knuth Algorithm M, multiprecision multiplication of natural numbers.
#define M (0xffffffff)
#define N 32
  UNSIGNED_T hu = u >> N, lu = u & M, hv = v >> N, lv = v & M;
  UNSIGNED_T t = lu * lv;
  UNSIGNED_T w3 = t & M, k = t >> N;
  t = hu * lv + k;
  UNSIGNED_T w2 = t & M, w1 = t >> N;
  t = lu * hv + w2;
  k = t >> N;
  HW (*w) = hu * hv + w1 + k;
  LW (*w) = (t << N) + w3;
#undef M
#undef N
}

void m128to128 (NODE_T * p, MOID_T * m, DOUBLE_NUM_T * w, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
// Knuth Algorithm M, multiprecision multiplication of natural numbers.
  UNSIGNED_T hu = HW (u), lu = LW (u), hv = HW (v), lv = LW (v);
  DOUBLE_NUM_T k, t, h, w1, w2, w3;
  if (lu == 0 || lv == 0) {
    set_lw (t, 0);
  } else {
    m64to128 (&t, lu, lv);
  }
  set_lw (w3, LW (t));
  set_lw (k, HW (t));
  if (hu == 0 || lv == 0) {
    set_lw (t, 0);
  } else {
    m64to128 (&t, hu, lv);
  }
  add_double (p, m, t, t, k);
  set_lw (w2, LW (t));
  set_lw (w1, HW (t));
  if (lu == 0 || hv == 0) {
    set_lw (t, 0);
  } else {
    m64to128 (&t, lu, hv);
  }
  add_double (p, m, t, t, w2);
  set_lw (k, HW (t));
  if (hu == 0 || hv == 0) {
    set_lw (h, 0);
  } else {
    m64to128 (&h, hu, hv);
  }
  add_double (p, m, h, h, w1);
  add_double (p, m, h, h, k);
  set_hw (*w, LW (t));
  add_double (p, m, *w, *w, w3);
  PRELUDE_ERROR (MODCHK (p, m, HW (h) != 0 || LW (h) != 0), p, ERROR_MATH, M_LONG_INT)
}

DOUBLE_NUM_T double_udiv_deep (NODE_T * p, MOID_T * m, DOUBLE_NUM_T n, DOUBLE_NUM_T d, int mode)
{
// A bit naive long division.
  DOUBLE_NUM_T q, r;
// Special cases.
  PRELUDE_ERROR (IS_ZERO (d), p, ERROR_DIVISION_BY_ZERO, M_LONG_INT);
  if (IS_ZERO (n)) {
    if (mode == 0) {
      set_lw (q, 0);
      return q;
    } else {
      set_lw (r, 0);
      return r;
    }
  }
// Assuming random n and d, circa half of the divisions is trivial:
// n / d = 1 when n = d, n / d = 0 when n < d.
  if (EQ (n, d)) {
    if (mode == 0) {
      set_lw (q, 1);
      return q;
    } else {
      set_lw (r, 0);
      return r;
    }
  } else if (GT (d, n)) {
    if (mode == 0) {
      set_lw (q, 0);
      return q;
    } else {
      return n;
    }
  }
// Halfword divide.
  if (HW (n) == 0 && HW (d) == 0) {
    if (mode == 0) {
      set_lw (q, LW (n) / LW (d));
      return q;
    } else {
      set_lw (r, LW (n) % LW (d));
      return r;
    }
  }
// We now know that n and d both have > 64 bits.
// Full divide.
  set_lw (q, 0);
  set_lw (r, 0);
  for (int k = 128; k > 0; k--) {
    UNSIGNED_T carry = (LW (q) & D_SIGN) ? 0x1 : 0x0;
    LW (q) <<= 1;
    HW (q) = (HW (q) << 1) | carry;
// Left-shift r
    carry = (LW (r) & D_SIGN) ? 0x1 : 0x0;
    LW (r) <<= 1;
    HW (r) = (HW (r) << 1) | carry;
// r[0] = n[k]
    if (HW (n) & D_SIGN) {
      LW (r) |= 0x1;
    }
    carry = (LW (n) & D_SIGN) ? 0x1 : 0x0;
    LW (n) <<= 1;
    HW (n) = (HW (n) << 1) | carry;
// if r >= d
    if (GE (r, d)) {
// r = r - d
      sub_double (p, m, r, r, d);
// q[k] = 1
      LW (q) |= 0x1;
    }
  }
  if (mode == 0) {
    return q;
  } else {
    return r;
  }
}

DOUBLE_NUM_T double_udiv (NODE_T * p, MOID_T * m, DOUBLE_NUM_T n, DOUBLE_NUM_T d, int mode)
{
  DOUBLE_NUM_T w = double_udiv_deep (p, m, n, d, mode);
  return abs_double_zero (w);
}

DOUBLE_NUM_T double_uadd (NODE_T * p, MOID_T * m, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  DOUBLE_NUM_T w;
  (void) p;
  add_double (p, m, w, u, v);
  return w;
}

DOUBLE_NUM_T double_usub (NODE_T * p, MOID_T * m, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  DOUBLE_NUM_T w;
  (void) p;
  sub_double (p, m, w, u, v);
  return w;
}

DOUBLE_NUM_T double_umul (NODE_T * p, MOID_T * m, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  DOUBLE_NUM_T w;
  m128to128 (p, m, &w, u, v);
  return w;
}

// Signed integer.

DOUBLE_NUM_T double_sadd (NODE_T * p, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  int neg_u = D_NEG (u), neg_v = D_NEG (v);
  if (neg_u) {
    u = neg_double_int (u);
  }
  if (neg_v) {
    v = neg_double_int (v);
  }
  DOUBLE_NUM_T w;
  set_lw (w, 0);
  if (!neg_u && !neg_v) {
    w = double_uadd (p, M_LONG_INT, u, v);
    PRELUDE_ERROR (D_NEG (w), p, ERROR_MATH, M_LONG_INT);
  } else if (neg_u && neg_v) {
    w = neg_double_int (double_sadd (p, u, v));
  } else if (neg_u) {
    w = double_ssub (p, v, u);
  } else if (neg_v) {
    w = double_ssub (p, u, v);
  }
  return w;
}

DOUBLE_NUM_T double_ssub (NODE_T * p, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  int neg_u = D_NEG (u), neg_v = D_NEG (v);
  if (neg_u) {
    u = neg_double_int (u);
  }
  if (neg_v) {
    v = neg_double_int (v);
  }
  DOUBLE_NUM_T w;
  set_lw (w, 0);
  if (!neg_u && !neg_v) {
    if (D_LT (u, v)) {
      w = neg_double_int (double_usub (p, M_LONG_INT, v, u));
    } else {
      w = double_usub (p, M_LONG_INT, u, v);
    }
  } else if (neg_u && neg_v) {
    w = double_ssub (p, v, u);
  } else if (neg_u) {
    w = neg_double_int (double_sadd (p, u, v));
  } else if (neg_v) {
    w = double_sadd (p, u, v);
  }
  return w;
}

DOUBLE_NUM_T double_smul (NODE_T * p, DOUBLE_NUM_T u, DOUBLE_NUM_T v)
{
  int neg_u = D_NEG (u), neg_v = D_NEG (v);
  DOUBLE_NUM_T w;
  if (neg_u) {
    u = neg_double_int (u);
  }
  if (neg_v) {
    v = neg_double_int (v);
  }
  w = double_umul (p, M_LONG_INT, u, v);
  if (neg_u != neg_v) {
    w = neg_double_int (w);
  }
  return w;
}

DOUBLE_NUM_T double_sdiv (NODE_T * p, DOUBLE_NUM_T u, DOUBLE_NUM_T v, int mode)
{
  int neg_u = D_NEG (u), neg_v = D_NEG (v);
  if (neg_u) {
    u = neg_double_int (u);
  }
  if (neg_v) {
    v = neg_double_int (v);
  }
  DOUBLE_NUM_T w = double_udiv (p, M_LONG_INT, u, v, mode);
  if (mode == 0 && neg_u != neg_v) {
    w = neg_double_int (w);
  } else if (mode == 1 && D_NEG (w)) {
    w = double_sadd (p, w, v);
  }
  return abs_double_zero (w);
}

#endif
