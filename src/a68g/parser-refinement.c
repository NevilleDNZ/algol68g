//! @file parser-refinement.c
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
//! Refinement preprocessor.

#include "a68g.h"
#include "a68g-parser.h"

// This code implements a small refinement preprocessor for A68G.
// It is included for educational purposes only.
// At the University of Nijmegen a preprocessor much like this one
// was used as a front-end to FLACC in freshman programming courses.
//
// See: 
//   C.H.A. Koster et al., 
//   Systematisch programmeren in Algol 68, Deel I en II.
//   Kluwer, Deventer [1978, 1981]
//
// The superimposed 'refinement grammar' is trivial:
//   refined-program-option:
//     refined-algol-68-source-code;
//     point-symbol;
//     refinement-definition-sequence-option.
//   refinement-definition:
//     defining-identifier;
//     colon-symbol;
//     refined-Algol-68-source-code;
//     point-symbol.
//   refined-algol-68-source-code:
//     # valid source code,
//       with applied-refinements,
//       without refinement-definitions #.
//   applied-refinement:
//     identifier.
//
// An applied-refinement is textually substituted for its definition.
// Note that refinement-definitions cannot be nested.
// Nested refinement-definitions would allow conflict with Algol 68 labels.
// The naive approach (no nesting) was chosen here to keep matters simple.
//
// Wirth had another approach to refinements in Pascal: procedures.
// That works for Algol 68 as well, of course.

//! @brief Whether refinement terminator.

BOOL_T is_refinement_terminator (NODE_T * p)
{
  if (IS (p, POINT_SYMBOL)) {
    if (IN_PRELUDE (NEXT (p))) {
      return A68_TRUE;
    } else {
      return whether (p, POINT_SYMBOL, IDENTIFIER, COLON_SYMBOL, STOP);
    }
  } else {
    return A68_FALSE;
  }
}

//! @brief Get refinement definitions in the internal source.

void get_refinements (void)
{
  TOP_REFINEMENT (&A68_JOB) = NO_REFINEMENT;
// First look where the prelude ends.
  NODE_T *p = TOP_NODE (&A68_JOB);
  while (p != NO_NODE && IN_PRELUDE (p)) {
    FORWARD (p);
  }
// Determine whether the program contains refinements at all.
  while (p != NO_NODE && !IN_PRELUDE (p) && !is_refinement_terminator (p)) {
    FORWARD (p);
  }
  if (p == NO_NODE || IN_PRELUDE (p)) {
    return;
  }
  FORWARD (p);
  if (p == NO_NODE || IN_PRELUDE (p)) {
// A program without refinements.
    return;
  }
// Apparently this is code with refinements.
  while (p != NO_NODE && !IN_PRELUDE (p) && whether (p, IDENTIFIER, COLON_SYMBOL, STOP)) {
    REFINEMENT_T *new_one = (REFINEMENT_T *) get_fixed_heap_space ((size_t) SIZE_ALIGNED (REFINEMENT_T));
    NEXT (new_one) = NO_REFINEMENT;
    NAME (new_one) = NSYMBOL (p);
    APPLICATIONS (new_one) = 0;
    LINE_DEFINED (new_one) = LINE (INFO (p));
    LINE_APPLIED (new_one) = NO_LINE;
    NODE_DEFINED (new_one) = p;
    BEGIN (new_one) = END (new_one) = NO_NODE;
    p = NEXT_NEXT (p);
    if (p == NO_NODE) {
      diagnostic (A68_SYNTAX_ERROR, NODE_DEFINED (new_one), ERROR_REFINEMENT_EMPTY);
      return;
    } else {
      BEGIN (new_one) = p;
    }
    while (p != NO_NODE && ATTRIBUTE (p) != POINT_SYMBOL) {
      END (new_one) = p;
      FORWARD (p);
    }
    if (p == NO_NODE) {
      diagnostic (A68_SYNTAX_ERROR, NODE_DEFINED (new_one), ERROR_REFINEMENT_INVALID);
      return;
    } else {
      FORWARD (p);
    }
// Do we already have one by this name.
    REFINEMENT_T *x = TOP_REFINEMENT (&A68_JOB);
    BOOL_T exists = A68_FALSE;
    while (x != NO_REFINEMENT && !exists) {
      if (NAME (x) == NAME (new_one)) {
        diagnostic (A68_SYNTAX_ERROR, NODE_DEFINED (new_one), ERROR_REFINEMENT_DEFINED);
        exists = A68_TRUE;
      }
      FORWARD (x);
    }
// Straight insertion in chain.
    if (!exists) {
      NEXT (new_one) = TOP_REFINEMENT (&A68_JOB);
      TOP_REFINEMENT (&A68_JOB) = new_one;
    }
  }
  if (p != NO_NODE && !IN_PRELUDE (p)) {
    diagnostic (A68_SYNTAX_ERROR, p, ERROR_REFINEMENT_INVALID);
  }
}

//! @brief Put refinement applications in the internal source.

void put_refinements (void)
{
// If there are no refinements, there's little to do.
  if (TOP_REFINEMENT (&A68_JOB) == NO_REFINEMENT) {
    return;
  }
// Initialisation.
  REFINEMENT_T *x = TOP_REFINEMENT (&A68_JOB);
  while (x != NO_REFINEMENT) {
    APPLICATIONS (x) = 0;
    FORWARD (x);
  }
// Before we introduce infinite loops, find where closing-prelude starts.
  NODE_T *p = TOP_NODE (&A68_JOB);
  while (p != NO_NODE && IN_PRELUDE (p)) {
    FORWARD (p);
  }
  while (p != NO_NODE && !IN_PRELUDE (p)) {
    FORWARD (p);
  }
  ABEND (p == NO_NODE, ERROR_INTERNAL_CONSISTENCY, __func__);
  NODE_T *point = p;
// We need to substitute until the first point.
  p = TOP_NODE (&A68_JOB);
  while (p != NO_NODE && ATTRIBUTE (p) != POINT_SYMBOL) {
    if (IS (p, IDENTIFIER)) {
// See if we can find its definition.
      REFINEMENT_T *y = NO_REFINEMENT;
      x = TOP_REFINEMENT (&A68_JOB);
      while (x != NO_REFINEMENT && y == NO_REFINEMENT) {
        if (NAME (x) == NSYMBOL (p)) {
          y = x;
        } else {
          FORWARD (x);
        }
      }
      if (y != NO_REFINEMENT) {
// We found its definition.
        APPLICATIONS (y)++;
        if (APPLICATIONS (y) > 1) {
          diagnostic (A68_SYNTAX_ERROR, NODE_DEFINED (y), ERROR_REFINEMENT_APPLIED);
          FORWARD (p);
        } else {
// Tie the definition in the tree.
          LINE_APPLIED (y) = LINE (INFO (p));
          if (PREVIOUS (p) != NO_NODE) {
            NEXT (PREVIOUS (p)) = BEGIN (y);
          }
          if (BEGIN (y) != NO_NODE) {
            PREVIOUS (BEGIN (y)) = PREVIOUS (p);
          }
          if (NEXT (p) != NO_NODE) {
            PREVIOUS (NEXT (p)) = END (y);
          }
          if (END (y) != NO_NODE) {
            NEXT (END (y)) = NEXT (p);
          }
          p = BEGIN (y);        // So we can substitute the refinements within
        }
      } else {
        FORWARD (p);
      }
    } else {
      FORWARD (p);
    }
  }
// After the point we ignore it all until the prelude.
  if (p != NO_NODE && IS (p, POINT_SYMBOL)) {
    if (PREVIOUS (p) != NO_NODE) {
      NEXT (PREVIOUS (p)) = point;
    }
    if (PREVIOUS (point) != NO_NODE) {
      PREVIOUS (point) = PREVIOUS (p);
    }
  } else {
    diagnostic (A68_SYNTAX_ERROR, p, ERROR_SYNTAX_EXPECTED, POINT_SYMBOL);
  }
// Has the programmer done it well?.
  if (ERROR_COUNT (&A68_JOB) == 0) {
    x = TOP_REFINEMENT (&A68_JOB);
    while (x != NO_REFINEMENT) {
      if (APPLICATIONS (x) == 0) {
        diagnostic (A68_SYNTAX_ERROR, NODE_DEFINED (x), ERROR_REFINEMENT_NOT_APPLIED);
      }
      FORWARD (x);
    }
  }
}
