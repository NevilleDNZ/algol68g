//! @file parser-moids-equivalence.c
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
//! Prove equivalence of modes.

#include "a68g.h"
#include "a68g-postulates.h"
#include "a68g-parser.h"

// Routines for establishing equivalence of modes.
//
// After I made this mode equivalencer (in 1993), I found:
// Algol Bulletin 30.3.3 C.H.A. Koster: On infinite modes, 86-89 [1969],
// which essentially concurs with this test on mode equivalence I wrote.
// It is elementary logic anyway: prove equivalence, assuming equivalence.

//! @brief Whether packs are equivalent, same sequence of equivalence modes.

BOOL_T are_packs_equivalent (PACK_T * s, PACK_T * t)
{
  for (; s != NO_PACK && t != NO_PACK; FORWARD (s), FORWARD (t)) {
    if (!are_modes_equivalent (MOID (s), MOID (t))) {
      return A68_FALSE;
    }
    if (TEXT (s) != TEXT (t)) {
      return A68_FALSE;
    }
  }
  return (BOOL_T) (s == NO_PACK && t == NO_PACK);
}

//! @brief Whether packs are subsets.

BOOL_T is_united_subset (PACK_T * s, PACK_T * t)
{
// For all modes in 's' there must be an equivalent in 't'.
  for (PACK_T *p = s; p != NO_PACK; FORWARD (p)) {
    BOOL_T f = A68_FALSE; 
    for (PACK_T *q = t; q != NO_PACK && !f; FORWARD (q)) {
      f = are_modes_equivalent (MOID (p), MOID (q));
    }
    if (!f) {
      return A68_FALSE;
    }
  }
  return A68_TRUE;
}

//! @brief Whether packs are subsets.

BOOL_T are_united_packs_equivalent (PACK_T * s, PACK_T * t)
{
  return is_united_subset (s, t) && is_united_subset (t, s);
}

//! @brief Whether moids a and b are structurally equivalent.

BOOL_T are_modes_equivalent (MOID_T * a, MOID_T * b)
{
// Heuristics.
  if (a == NO_MOID || b == NO_MOID) {
// Modes can be NO_MOID in partial argument lists.
    return A68_FALSE;
  } else if (a == M_ERROR || b == M_ERROR) {
    return A68_FALSE;
  } else if (a == b) {
    return A68_TRUE;
  } else if (ATTRIBUTE (a) != ATTRIBUTE (b)) {
    return A68_FALSE;
  } else if (DIM (a) != DIM (b)) {
    return A68_FALSE;
  } else if (IS (a, STANDARD)) {
    return (BOOL_T) (a == b);
  } else if (EQUIVALENT (a) == b || EQUIVALENT (b) == a) {
    return A68_TRUE;
  } else if (is_postulated_pair (A68 (top_postulate), a, b) || is_postulated_pair (A68 (top_postulate), b, a)) {
    return A68_TRUE;
  } else if (IS (a, INDICANT)) {
    if (NODE (a) == NO_NODE || NODE (b) == NO_NODE) {
      return A68_FALSE;
    } else {
      return NODE (a) == NODE (b);
    }
  }
// Investigate structure.
// We now know that 'a' and 'b' have same attribute, dimension, ...
  if (IS (a, REF_SYMBOL)) {
// REF MODE
    return are_modes_equivalent (SUB (a), SUB (b));
  } else if (IS (a, ROW_SYMBOL)) {
// [] MODE
    return are_modes_equivalent (SUB (a), SUB (b));
  } else if (IS (a, FLEX_SYMBOL)) {
// FLEX [...] MODE
    return are_modes_equivalent (SUB (a), SUB (b));
  } else if (IS (a, STRUCT_SYMBOL)) {
// STRUCT (...)
    POSTULATE_T *save = A68 (top_postulate);
    make_postulate (&A68 (top_postulate), a, b);
    BOOL_T z = are_packs_equivalent (PACK (a), PACK (b));
    free_postulate_list (A68 (top_postulate), save);
    A68 (top_postulate) = save;
    return z;
  } else if (IS (a, UNION_SYMBOL)) {
// UNION (...)
    return are_united_packs_equivalent (PACK (a), PACK (b));
  } else if (IS (a, PROC_SYMBOL) && PACK (a) == NO_PACK && PACK (b) == NO_PACK) {
// PROC MOID
    return are_modes_equivalent (SUB (a), SUB (b));
  } else if (IS (a, PROC_SYMBOL) && PACK (a) != NO_PACK && PACK (b) != NO_PACK) {
// PROC (...) MOID
    POSTULATE_T *save = A68 (top_postulate);
    make_postulate (&A68 (top_postulate), a, b);
    BOOL_T z = are_modes_equivalent (SUB (a), SUB (b));
    if (z) {
      z = are_packs_equivalent (PACK (a), PACK (b));
    }
    free_postulate_list (A68 (top_postulate), save);
    A68 (top_postulate) = save;
    return z;
  } else if (IS (a, SERIES_MODE) || IS (a, STOWED_MODE)) {
// Modes occurring in displays.
    return are_packs_equivalent (PACK (a), PACK (b));
  }
  return A68_FALSE;
}

//! @brief Whether two modes are structurally equivalent.

BOOL_T prove_moid_equivalence (MOID_T * p, MOID_T * q)
{
// Prove two modes to be equivalent under assumption that they indeed are.
  POSTULATE_T *save = A68 (top_postulate);
  BOOL_T z = are_modes_equivalent (p, q);
  free_postulate_list (A68 (top_postulate), save);
  A68 (top_postulate) = save;
  return z;
}
