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
This is the mode checker and coercion inserter. The syntax tree is traversed to 
determine and check all modes. Next the tree is traversed again to insert 
coercions.

Algol 68 contexts are SOFT, WEAK, MEEK, FIRM and STRONG.
These contexts are increasing in strength:

  SOFT: Deproceduring
  WEAK: Dereferencing to REF [] or REF STRUCT
  MEEK: Deproceduring and dereferencing
  FIRM: MEEK followed by uniting
  STRONG: FIRM followed by rowing, widening or voiding

Furthermore you will see in this file next switches:

(1) FORCE_DEFLEXING allows assignment compatibility between FLEX and non FLEX 
rows. This can only be the case when there is no danger of altering bounds of a 
non FLEX row.

(2) ALIAS_DEFLEXING prohibits aliasing a FLEX row to a non FLEX row (vice versa 
is no problem) so that one cannot alter the bounds of a non FLEX row by 
aliasing it to a FLEX row. This is particularly the case when passing names as 
parameters to procedures:

   PROC x = (REF STRING s) VOID: ..., PROC y = (REF [] CHAR c) VOID: ...;
   x (LOC STRING);    # OK #
   x (LOC [10] CHAR); # Not OK, suppose x changes bounds of s! #
   y (LOC STRING);    # OK #
   y (LOC [10] CHAR); # OK #

(3) SAFE_DEFLEXING sets FLEX row apart from non FLEX row. This holds for names, 
not for values, so common things are not rejected, for instance

   STRING x = read string;
   [] CHAR y = read string

(4) NO_DEFLEXING sets FLEX row apart from non FLEX row.                       */

#include "algol68g.h"

TAG_T *error_tag;

static BOOL_T basic_coercions (MOID_T *, MOID_T *, int, int);
static BOOL_T whether_coercible (MOID_T *, MOID_T *, int, int);
static BOOL_T whether_nonproc (MOID_T *);

static void mode_check_particular (NODE_T *, SOID_T *, SOID_T *);
static void mode_check_enclosed (NODE_T *, SOID_T *, SOID_T *);
static void mode_check_primary (NODE_T *, SOID_T *, SOID_T *);
static void mode_check_secondary (NODE_T *, SOID_T *, SOID_T *);
static void mode_check_unit (NODE_T *, SOID_T *, SOID_T *);
static void mode_check_formula (NODE_T *, SOID_T *, SOID_T *);

static void coerce_enclosed (NODE_T *, SOID_T *);
static void coerce_operand (NODE_T *, SOID_T *);
static void coerce_formula (NODE_T *, SOID_T *);
static void coerce_primary (NODE_T *, SOID_T *);
static void coerce_secondary (NODE_T *, SOID_T *);
static void coerce_unit (NODE_T *, SOID_T *);

#define DEPREF A_TRUE
#define NO_DEPREF A_FALSE

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_checker: check modes.                                                    */

void
mode_checker (NODE_T * p)
{
  if (WHETHER (p, PARTICULAR_PROGRAM))
    {
      SOID_T x, y;
      make_soid (&x, STRONG, MODE (VOID), 0);
      mode_check_particular (p, &x, &y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_particular: check particular program.                              */

static void
mode_check_particular (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (WHETHER (p, PARTICULAR_PROGRAM))
    {
      mode_check_enclosed (SUB (p), x, y);
    }
  MOID (p) = MOID (y);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coercion_inserter: insert coercions.                                          */

void
coercion_inserter (NODE_T * p)
{
  if (WHETHER (p, PARTICULAR_PROGRAM))
    {
      SOID_T q;
      make_soid (&q, STRONG, MODE (VOID), 0);
      coerce_enclosed (SUB (p), &q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_is_well: whether "p" is well defined.                            */

static BOOL_T
whether_mode_is_well (MOID_T * n)
{
  return (!(n == MODE (ERROR) || n == MODE (UNDEFINED)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_isnt_well: whether "p" is not well defined.                      */

static BOOL_T
whether_mode_isnt_well (MOID_T * p)
{
  if (p == NULL)
    {
      return (A_TRUE);
    }
  else if (!whether_mode_is_well (p))
    {
      return (A_TRUE);
    }
  else if (PACK (p) != NULL)
    {
      PACK_T *q = PACK (p);
      for (; q != NULL; q = NEXT (q))
	{
	  if (!whether_mode_is_well (MOID (q)))
	    {
	      return (A_TRUE);
	    }
	}
    }
  return (A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_soid.                                                                    */

void
make_soid (SOID_T * s, int sort, MOID_T * type, int attribute)
{
  ATTRIBUTE (s) = attribute;
  SORT (s) = sort;
  MOID (s) = type;
  s->cast = A_FALSE;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_to_soid_list.                                                             */

static void
add_to_soid_list (SOID_LIST_T ** root, NODE_T * where, SOID_T * soid)
{
  if (*root != NULL)
    {
      add_to_soid_list (&(NEXT (*root)), where, soid);
    }
  else
    {
      SOID_LIST_T *new_one = (SOID_LIST_T *) get_temp_heap_space (SIZE_OF (SOID_LIST_T));
      new_one->where = where;
      new_one->yield = (SOID_T *) get_temp_heap_space (SIZE_OF (SOID_T));
      make_soid (new_one->yield, SORT (soid), MOID (soid), 0);
      NEXT (new_one) = NULL;
      *root = new_one;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
absorb_series_pack: absorb nested series modes recursively.                   */

static void
absorb_series_pack (MOID_T ** p)
{
  BOOL_T go_on;
  do
    {
      PACK_T *z = NULL, *t;
      go_on = A_FALSE;
      for (t = PACK (*p); t != NULL; t = NEXT (t))
	{
	  if (MOID (t) != NULL && WHETHER (MOID (t), SERIES_MODE))
	    {
	      PACK_T *s;
	      go_on = A_TRUE;
	      for (s = PACK (MOID (t)); s != NULL; s = NEXT (s))
		{
		  add_mode_to_pack (&z, MOID (s), NULL, NODE (s));
		}
	    }
	  else
	    {
	      add_mode_to_pack (&z, MOID (t), NULL, NODE (t));
	    }
	}
      PACK (*p) = z;
    }
  while (go_on);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_series_from_moids: make SERIES (u, v).                                   */

static MOID_T *
make_series_from_moids (MOID_T * u, MOID_T * v)
{
  MOID_T *x = new_moid ();
  ATTRIBUTE (x) = SERIES_MODE;
  add_mode_to_pack (&(PACK (x)), u, NULL, NODE (u));
  add_mode_to_pack (&(PACK (x)), v, NULL, NODE (v));
  absorb_series_pack (&x);
  DIMENSION (x) = count_pack_members (PACK (x));
  add_single_moid_to_list (&top_moid_list, x, NULL);
  if (DIMENSION (x) == 1)
    {
      return (MOID (PACK (x)));
    }
  else
    {
      return (x);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
absorb_related_subsets: absorb firmly related unions in m.

For instance invalid UNION (PROC REF UNION (A, B), A, B) -> valid UNION (A, B),
which is used in balancing conformity clauses.                                */

static MOID_T *
absorb_related_subsets (MOID_T * m)
{
  int mods;
  do
    {
      PACK_T *u = NULL, *v;
      mods = 0;
      for (v = PACK (m); v != NULL; v = NEXT (v))
	{
	  MOID_T *n = depref_completely (MOID (v));
	  if (WHETHER (n, UNION_SYMBOL) && whether_subset (n, m, SAFE_DEFLEXING))
	    {
/* Unpack it */
	      PACK_T *w;
	      for (w = PACK (n); w != NULL; w = NEXT (w))
		{
		  add_mode_to_pack (&u, MOID (w), NULL, NODE (w));
		}
	      mods++;
	    }
	  else
	    {
	      add_mode_to_pack (&u, MOID (v), NULL, NODE (v));
	    }
	}
      PACK (m) = absorb_union_pack (u, &mods);
    }
  while (mods != 0);
  return (m);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
register_extra_mode: register u in the global mode table, if u is unique.     */

static MOID_T *
register_extra_mode (MOID_T * u)
{
  MOID_LIST_T *z;
/* Check for equivalency */
  for (z = top_moid_list; z != NULL; z = NEXT (z))
    {
      MOID_T *v = MOID (z);
      POSTULATE_T *save = top_postulate;
      BOOL_T w = EQUIVALENT (v) == NULL && modes_equivalent (v, u);
      top_postulate = save;
      if (w)
	{
	  return (v);
	}
    }
/* Mode u is unique - include in the global moid list */
  z = (MOID_LIST_T *) get_fixed_heap_space (SIZE_OF (MOID_LIST_T));
  z->coming_from_level = NULL;
  MOID (z) = u;
  NEXT (z) = top_moid_list;
  abend (z == NULL, "NULL pointer", "register_extra_mode");
  top_moid_list = z;
  add_single_moid_to_list (&top_moid_list, u, NULL);
  return (u);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_united_mode.                                                             */

static MOID_T *
make_united_mode (MOID_T * m)
{
  MOID_T *u;
  PACK_T *v, *w;
  int mods;
  if (m == NULL)
    {
      return (MODE (ERROR));
    };
  if (ATTRIBUTE (m) != SERIES_MODE)
    {
      return (m);
    }
/* Straighten the series */
  absorb_series_pack (&m);
/* Copy the series into a UNION */
  u = new_moid ();
  ATTRIBUTE (u) = UNION_SYMBOL;
  PACK (u) = NULL;
  v = PACK (u);
  w = PACK (m);
  for (w = PACK (m); w != NULL; w = NEXT (w))
    {
      add_mode_to_pack (&(PACK (u)), MOID (w), NULL, NODE (m));
    }
/* Absorb and contract the new UNION */
  do
    {
      mods = 0;
      u->dimensions = count_pack_members (PACK (u));
      PACK (u) = absorb_union_pack (PACK (u), &mods);
      contract_union (u, &mods);
    }
  while (mods != 0);
/* A UNION of one mode is that mode itself */
  if (DIMENSION (u) == 1)
    {
      return (MOID (PACK (u)));
    }
  else
    {
      return (register_extra_mode (u));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pack_soids_in_moid.                                                           */

static MOID_T *
pack_soids_in_moid (SOID_LIST_T * top_sl, int attribute)
{
  MOID_T *x;
  PACK_T *t, **p;
  x = new_moid ();
  NUMBER (x) = mode_count++;
  ATTRIBUTE (x) = attribute;
  DIMENSION (x) = 0;
  SUB (x) = NULL;
  EQUIVALENT (x) = NULL;
  SLICE (x) = NULL;
  DEFLEXED (x) = NULL;
  NAME (x) = NULL;
  NEXT (x) = NULL;
  PACK (x) = NULL;
  p = &(PACK (x));
  for (; top_sl != NULL; top_sl = NEXT (top_sl))
    {
      t = new_pack ();
      MOID (t) = MOID (top_sl->yield);
      t->text = NULL;
      NODE (t) = top_sl->where;
      NEXT (t) = NULL;
      x->dimensions++;
      *p = t;
      p = &NEXT (t);
    }
  add_single_moid_to_list (&top_moid_list, x, NULL);
  return (x);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_deprefable.                                                           */

BOOL_T
whether_deprefable (MOID_T * p)
{
  if (WHETHER (p, REF_SYMBOL))
    {
      return (A_TRUE);
    }
  else
    {
      return (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
depref_once.                                                                  */

static MOID_T *
depref_once (MOID_T * p)
{
  if (WHETHER (p, REF_SYMBOL))
    {
      return (SUB (p));
    }
  else if (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL)
    {
      return (SUB (p));
    }
  else
    {
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
depref_completely.                                                            */

MOID_T *
depref_completely (MOID_T * p)
{
  while (whether_deprefable (p))
    {
      p = depref_once (p);
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
deproc_completely.                                                            */

static MOID_T *
deproc_completely (MOID_T * p)
{
  while (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL)
    {
      p = depref_once (p);
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
depref_rows.                                                                  */

static MOID_T *
depref_rows (MOID_T * p, MOID_T * q)
{
  if (q == MODE (ROWS))
    {
      while (whether_deprefable (p))
	{
	  p = depref_once (p);
	}
      return (p);
    }
  else
    {
      return (q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
derow.                                                                        */

static MOID_T *
derow (MOID_T * p)
{
  if (WHETHER (p, ROW_SYMBOL) || WHETHER (p, FLEX_SYMBOL))
    {
      return (derow (SUB (p)));
    }
  else
    {
      return (p);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_rows_type.                                                            */

static BOOL_T
whether_rows_type (MOID_T * p)
{
  switch (ATTRIBUTE (p))
    {
    case ROW_SYMBOL:
    case FLEX_SYMBOL:
      {
	return (A_TRUE);
      }
    case UNION_SYMBOL:
      {
	PACK_T *t = PACK (p);
	BOOL_T go_on = A_TRUE;
	while (t != NULL && go_on)
	  {
	    go_on &= whether_rows_type (MOID (t));
	    t = NEXT (t);
	  }
	return (go_on);
      }
    default:
      {
	return (A_FALSE);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_aux_transput_mode.                                                    */

static BOOL_T
whether_aux_transput_mode (MOID_T * p)
{
  if (p == MODE (PROC_REF_FILE_VOID))
    {
      return (A_TRUE);
    }
  else if (p == MODE (FORMAT))
    {
      return (A_TRUE);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_transput_mode.                                                        */

static BOOL_T
whether_transput_mode (MOID_T * p)
{
  if (p == MODE (INT))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONG_INT))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONGLONG_INT))
    {
      return (A_TRUE);
    }
  else if (p == MODE (REAL))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONG_REAL))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONGLONG_REAL))
    {
      return (A_TRUE);
    }
  else if (p == MODE (BOOL))
    {
      return (A_TRUE);
    }
  else if (p == MODE (CHAR))
    {
      return (A_TRUE);
    }
  else if (p == MODE (BITS))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONG_BITS))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONGLONG_BITS))
    {
      return (A_TRUE);
    }
  else if (p == MODE (BYTES))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONG_BYTES))
    {
      return (A_TRUE);
    }
  else if (p == MODE (COMPLEX))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONG_COMPLEX))
    {
      return (A_TRUE);
    }
  else if (p == MODE (LONGLONG_COMPLEX))
    {
      return (A_TRUE);
    }
  else if (p == MODE (ROW_CHAR))
    {
      return (A_TRUE);
    }
  else if (p == MODE (STRING))
    {
      return (A_TRUE);		/* Not conform RR */
    }
  else if (WHETHER (p, UNION_SYMBOL) || WHETHER (p, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (p);
      BOOL_T k = A_TRUE;
      for (; q != NULL && k; q = NEXT (q))
	{
	  k &= (whether_transput_mode (MOID (q)) || whether_aux_transput_mode (MOID (q)));
	}
      return (k);
    }
  else if (WHETHER (p, ROW_SYMBOL))
    {
      return (whether_transput_mode (SUB (p)) || whether_aux_transput_mode (SUB (p)));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_printable_mode.                                                       */

static BOOL_T
whether_printable_mode (MOID_T * p)
{
  if (whether_aux_transput_mode (p))
    {
      return (A_TRUE);
    }
  else
    {
      return (whether_transput_mode (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_readable_mode.                                                        */

static BOOL_T
whether_readable_mode (MOID_T * p)
{
  if (whether_aux_transput_mode (p))
    {
      return (A_TRUE);
    }
  else
    {
      return (WHETHER (p, REF_SYMBOL) ? whether_transput_mode (SUB (p)) : A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_name_struct.                                                          */

static BOOL_T
whether_name_struct (MOID_T * p)
{
  return (p->name != NULL ? WHETHER (DEFLEX (SUB (p)), STRUCT_SYMBOL) : A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_modes_equal.                                                          */

BOOL_T
whether_modes_equal (MOID_T * u, MOID_T * v, int context)
{
  if (u == v)
    {
      return (A_TRUE);
    }
  else
    {
      switch (context)
	{
	case FORCE_DEFLEXING:
	  {
/* Allow any interchange between FLEX [] A and [] A */
	    return (DEFLEX (u) == DEFLEX (v));
	  }
	case ALIAS_DEFLEXING:
/* Cannot alias [] A to FLEX [] A, but vice versa is ok */
	  {
	    if (u->has_ref)
	      {
		return (DEFLEX (u) == v);
	      }
	    else
	      {
		return (whether_modes_equal (u, v, SAFE_DEFLEXING));
	      }
	  }
	case SAFE_DEFLEXING:
	  {
/* Cannot alias [] A to FLEX [] A but values are ok */
	    if (!u->has_ref && !v->has_ref)
	      {
		return (whether_modes_equal (u, v, FORCE_DEFLEXING));
	      }
	    else
	      {
		return (A_FALSE);
	      }
	case NO_DEFLEXING:
	    return (A_FALSE);
	  }
	}
    }
  return (A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unites_to: yield mode to unite to.                                            */

MOID_T *
unites_to (MOID_T * m, MOID_T * u)
{
/* Uniting m->u */
  MOID_T *v = NULL;
  PACK_T *p;
  if (u == MODE (SIMPLIN) || u == MODE (SIMPLOUT))
    {
      return (m);
    }
  for (p = PACK (u); p != NULL; p = NEXT (p))
    {
/* Prefer []->[] over []->FLEX []. */
      if (m == MOID (p))
	{
	  v = MOID (p);
	}
      else if (v == NULL && DEFLEX (m) == DEFLEX (MOID (p)))
	{
	  v = MOID (p);
	}
    }
  return (v);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_subset: whether "p" is a subset of "q".                               */

BOOL_T
whether_subset (MOID_T * p, MOID_T * q, int context)
{
  PACK_T *u = PACK (p);
  BOOL_T j = A_TRUE;
  for (; u != NULL && j; u = NEXT (u))
    {
      PACK_T *v = PACK (q);
      BOOL_T k = A_FALSE;
      for (; v != NULL && !k; v = NEXT (v))
	{
	  k |= whether_modes_equal (MOID (u), MOID (v), context);
	}
      j = j && k;
    }
  return (j);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_unitable: whether "p" can be united to UNION "q".                     */

BOOL_T
whether_unitable (MOID_T * p, MOID_T * q, int context)
{
  if (WHETHER (q, UNION_SYMBOL))
    {
      if (WHETHER (p, UNION_SYMBOL))
	{
	  return (whether_subset (p, q, context));
	}
      else
	{
	  PACK_T *t = PACK (q);
	  for (; t != NULL; t = NEXT (t))
	    {
	      if (whether_modes_equal (p, MOID (t), context))
		{
		  return (A_TRUE);
		}
	    }
	}
    }
  return (A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
investigate_firm_relations: whether all or some components of "u" can be firmly 
                            coerced to a component mode of v.                 */

static void
investigate_firm_relations (PACK_T * u, PACK_T * v, BOOL_T * all, BOOL_T * some)
{
  *all = A_TRUE;
  *some = A_FALSE;
  for (; v != NULL; v = NEXT (v))
    {
      PACK_T *w;
      BOOL_T k = A_FALSE;
/* Check whether any component of u matches this component of v */
      for (w = u; w != NULL; w = NEXT (w))
	{
	  BOOL_T coercible = whether_coercible (MOID (w), MOID (v), FIRM, FORCE_DEFLEXING);
	  *some |= coercible;
	  k |= coercible;
	}
      *all &= k;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_softly_coercible.                                                     */

static BOOL_T
whether_softly_coercible (MOID_T * p, MOID_T * q, int context)
{
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL)
    {
      return (whether_softly_coercible (SUB (p), q, context));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_weakly_coercible.                                                     */

static BOOL_T
whether_weakly_coercible (MOID_T * p, MOID_T * q, int context)
{
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (whether_deprefable (p))
    {
      return (whether_weakly_coercible (depref_once (p), q, context));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_meekly_coercible.                                                     */

static BOOL_T
whether_meekly_coercible (MOID_T * p, MOID_T * q, int context)
{
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (whether_deprefable (p))
    {
      return (whether_meekly_coercible (depref_once (p), q, context));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_firmly_coercible: whether there is a firm path from "p" to "q".       */

static BOOL_T
whether_firmly_coercible (MOID_T * p, MOID_T * q, int context)
{
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (q == MODE (ROWS) && whether_rows_type (p))
    {
      return (A_TRUE);
    }
  else if (whether_unitable (p, q, context))
    {
      return (A_TRUE);
    }
  else if (whether_deprefable (p))
    {
      return (whether_firmly_coercible (depref_once (p), q, context));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
widens_to: whether "p" widens to "q".                                         */

static MOID_T *
widens_to (MOID_T * p, MOID_T * q)
{
  if (p == MODE (INT))
    {
      if (q == MODE (LONG_INT) || q == MODE (LONGLONG_INT) || q == MODE (LONG_REAL) || q == MODE (LONGLONG_REAL) || q == MODE (LONG_COMPLEX) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONG_INT));
	}
      else if (q == MODE (REAL) || q == MODE (COMPLEX))
	{
	  return (MODE (REAL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONG_INT))
    {
      if (q == MODE (LONGLONG_INT))
	{
	  return (MODE (LONGLONG_INT));
	}
      else if (q == MODE (LONG_REAL) || q == MODE (LONGLONG_REAL) || q == MODE (LONG_COMPLEX) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONG_REAL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONGLONG_INT))
    {
      if (q == MODE (LONGLONG_REAL) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONGLONG_REAL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (REAL))
    {
      if (q == MODE (LONG_REAL) || q == MODE (LONGLONG_REAL) || q == MODE (LONG_COMPLEX) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONG_REAL));
	}
      else if (q == MODE (COMPLEX))
	{
	  return (MODE (COMPLEX));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (COMPLEX))
    {
      if (q == MODE (LONG_COMPLEX) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONG_COMPLEX));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONG_REAL))
    {
      if (q == MODE (LONGLONG_REAL) || q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONGLONG_REAL));
	}
      else if (q == MODE (LONG_COMPLEX))
	{
	  return (MODE (LONG_COMPLEX));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONG_COMPLEX))
    {
      if (q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONGLONG_COMPLEX));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONGLONG_REAL))
    {
      if (q == MODE (LONGLONG_COMPLEX))
	{
	  return (MODE (LONGLONG_COMPLEX));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (BITS))
    {
      if (q == MODE (LONG_BITS) || q == MODE (LONGLONG_BITS))
	{
	  return (MODE (LONG_BITS));
	}
      else if (q == MODE (ROW_BOOL))
	{
	  return (MODE (ROW_BOOL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONG_BITS))
    {
      if (q == MODE (LONGLONG_BITS))
	{
	  return (MODE (LONGLONG_BITS));
	}
      else if (q == MODE (ROW_BOOL))
	{
	  return (MODE (ROW_BOOL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (LONGLONG_BITS))
    {
      if (q == MODE (ROW_BOOL))
	{
	  return (MODE (ROW_BOOL));
	}
      else
	{
	  return (NULL);
	}
    }
  else if (p == MODE (BYTES) && q == MODE (ROW_CHAR))
    {
      return (MODE (ROW_CHAR));
    }
  else if (p == MODE (LONG_BYTES) && q == MODE (ROW_CHAR))
    {
      return (MODE (ROW_CHAR));
    }
  else
    {
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_widenable.                                                            */

static BOOL_T
whether_widenable (MOID_T * p, MOID_T * q)
{
  MOID_T *z = widens_to (p, q);
  if (z != NULL)
    {
      return (z == q ? A_TRUE : whether_widenable (z, q));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_ref_row.                                                              */

static BOOL_T
whether_ref_row (MOID_T * p)
{
  return (p->name != NULL ? WHETHER (DEFLEX (SUB (p)), ROW_SYMBOL) : A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_strong_name.                                                               */

static BOOL_T
whether_strong_name (MOID_T * p, MOID_T * q)
{
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (whether_ref_row (q))
    {
      return (whether_strong_name (p, q->name));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_strong_slice                                        */

static BOOL_T
whether_strong_slice (MOID_T * p, MOID_T * q)
{
  if (p == q || whether_widenable (p, q))
    {
      return (A_TRUE);
    }
  else if (SLICE (q) != NULL)
    {
      return (whether_strong_slice (p, SLICE (q)));
    }
  else if (WHETHER (q, FLEX_SYMBOL))
    {
      return (whether_strong_slice (p, SUB (q)));
    }
  else if (whether_ref_row (q))
    {
      return (whether_strong_name (p, q));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_strongly_coercible.                                                   */

static BOOL_T
whether_strongly_coercible (MOID_T * p, MOID_T * q, int context)
{
/* Keep this sequence of statements */
  if (p == q)
    {
      return (A_TRUE);
    }
  else if (q == MODE (VOID))
    {
      return (A_TRUE);
    }
  else if ((q == MODE (SIMPLIN) || q == MODE (ROW_SIMPLIN)) && whether_readable_mode (p))
    {
      return (A_TRUE);
    }
  else if (q == MODE (ROWS) && whether_rows_type (p))
    {
      return (A_TRUE);
    }
  else if (whether_unitable (p, derow (q), context))
    {
      return (A_TRUE);
    }
  else if (whether_ref_row (q) && whether_strong_name (p, q))
    {
      return (A_TRUE);
    }
  else if (SLICE (q) != NULL && whether_strong_slice (p, q))
    {
      return (A_TRUE);
    }
  else if (WHETHER (q, FLEX_SYMBOL) && whether_strong_slice (p, q))
    {
      return (A_TRUE);
    }
  else if (whether_widenable (p, q))
    {
      return (A_TRUE);
    }
  else if (whether_deprefable (p))
    {
      return (whether_strongly_coercible (depref_once (p), q, context));
    }
  else if (q == MODE (SIMPLOUT) || q == MODE (ROW_SIMPLOUT))
    {
      return (whether_printable_mode (p));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_firm                                       */

BOOL_T
whether_firm (MOID_T * p, MOID_T * q)
{
  return (whether_firmly_coercible (p, q, SAFE_DEFLEXING) || whether_firmly_coercible (q, p, SAFE_DEFLEXING));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_coercible_stowed.                                                     */

static BOOL_T
whether_coercible_stowed (MOID_T * p, MOID_T * q, int c, int context)
{
  if (c == STRONG)
    {
      if (c == STRONG && q == MODE (VOID))
	{
	  return (A_TRUE);
	}
      else if (WHETHER (q, FLEX_SYMBOL))
	{
	  PACK_T *u = PACK (p);
	  BOOL_T j = A_TRUE;
	  for (; u != NULL && j; u = NEXT (u))
	    {
	      j &= whether_coercible (MOID (u), SLICE (SUB (q)), c, context);
	    }
	  return (j);
	}
      else if (WHETHER (q, ROW_SYMBOL))
	{
	  PACK_T *u = PACK (p);
	  BOOL_T j = A_TRUE;
	  for (; u != NULL && j; u = NEXT (u))
	    {
	      j &= whether_coercible (MOID (u), SLICE (q), c, context);
	    }
	  return (j);
	}
      else if (WHETHER (q, PROC_SYMBOL) || WHETHER (q, STRUCT_SYMBOL))
	{
	  PACK_T *u = PACK (p), *v = PACK (q);
	  if (p->dimensions != q->dimensions)
	    {
	      return (A_FALSE);
	    }
	  else
	    {
	      BOOL_T j = A_TRUE;
	      while (u != NULL && v != NULL && j)
		{
		  j &= whether_coercible (MOID (u), MOID (v), c, context);
		  u = NEXT (u);
		  v = NEXT (v);
		}
	      return (j);
	    }
	}
      else
	{
	  return (A_FALSE);
	}
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_coercible_series                                         */

static BOOL_T
whether_coercible_series (MOID_T * p, MOID_T * q, int c, int context)
{
  if (c == STRONG)
    {
      PACK_T *u = PACK (p);
      BOOL_T j = A_TRUE;
      for (; u != NULL && j; u = NEXT (u))
	{
	  if (MOID (u) != NULL)
	    {
	      j &= whether_coercible (MOID (u), q, c, context);
	    }
	}
      return (j);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
basic_coercions                                        */

static BOOL_T
basic_coercions (MOID_T * p, MOID_T * q, int c, int context)
{
  if (p == q)
    {
/* A can be coerced to A in any context */
      return (A_TRUE);
    }
  else if (c == NO_SORT)
    {
      return (p == q);
    }
  else if (c == SOFT)
    {
      return (whether_softly_coercible (p, q, context));
    }
  else if (c == WEAK)
    {
      return (whether_weakly_coercible (p, q, context));
    }
  else if (c == MEEK)
    {
      return (whether_meekly_coercible (p, q, context));
    }
  else if (c == FIRM)
    {
      return (whether_firmly_coercible (p, q, context));
    }
  else if (c == STRONG)
    {
      return (whether_strongly_coercible (p, q, context));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_coercible: whether "p" can be coerced to "q" in a "c" context.             */

BOOL_T
whether_coercible (MOID_T * p, MOID_T * q, int c, int context)
{
  if (whether_mode_isnt_well (p) || whether_mode_isnt_well (q))
    {
      return (A_TRUE);
    }
  else if (p == q)
    {
      return (A_TRUE);
    }
  else if (p == MODE (HIP))
    {
      return (A_TRUE);
    }
  else if (WHETHER (p, STOWED_MODE))
    {
      return (whether_coercible_stowed (p, q, c, context));
    }
  else if (WHETHER (p, SERIES_MODE))
    {
      return (whether_coercible_series (p, q, c, context));
    }
  else if (p == MODE (VACUUM) && WHETHER (DEFLEX (q), ROW_SYMBOL))
    {
      return (A_TRUE);
    }
  else if (basic_coercions (p, q, c, context))
    {
      return (A_TRUE);
    }
  else if (context == FORCE_DEFLEXING)
    {
/* Allow for any interchange between FLEX [] A and [] A */
      return (basic_coercions (DEFLEX (p), DEFLEX (q), c, FORCE_DEFLEXING));
    }
  else if (context == ALIAS_DEFLEXING)
    {
/* No aliasing of REF [] A and REF FLEX [] A, but vice versa is ok and values too */
      if (p->has_ref)
	{
	  return (basic_coercions (DEFLEX (p), q, c, ALIAS_DEFLEXING));
	}
      else
	{
	  return (whether_coercible (p, q, c, SAFE_DEFLEXING));
	}
    }
  else if (context == SAFE_DEFLEXING)
    {
/* No aliasing of FLEX [] A and [] A names, but allow for values */
      if (!p->has_ref && !q->has_ref)
	{
	  return (whether_coercible (p, q, c, FORCE_DEFLEXING));
	}
      else
	{
	  return (basic_coercions (p, q, c, SAFE_DEFLEXING));
	}
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_coercible_in_context                                         */

static BOOL_T
whether_coercible_in_context (SOID_T * p, SOID_T * q, int context)
{
  if (SORT (p) != SORT (q))
    {
      return (A_FALSE);
    }
  else if (MOID (p) != MOID (q))
    {
      return (whether_coercible (MOID (p), MOID (q), SORT (q), context));
    }
  else
    {
      return (A_TRUE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_balanced: whether list "y" is balanced.                                    */

static BOOL_T
whether_balanced (NODE_T * n, SOID_LIST_T * y, int sort)
{
  if (sort == STRONG)
    {
      return (A_TRUE);
    }
  else
    {
      BOOL_T k = A_FALSE;
      for (; y != NULL && !k; y = NEXT (y))
	{
	  SOID_T *z = y->yield;
	  k = WHETHER_NOT (MOID (z), STOWED_MODE);
/*
      if (!(SORT (z) == STRONG && MOID (z) == MODE (HIP))) {
	k = WHETHER_NOT (MOID (z), STOWED_MODE);
      }
*/
	}
      if (k == A_FALSE)
	{
	  diagnostic (A_ERROR, n, "cannot find unique mode for construct");
	}
      return (k);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_balanced_mode: return a moid in "m"'s pack to which all other memberss can 
                   be coerced.                                                */

MOID_T *
get_balanced_mode (MOID_T * m, int sort, BOOL_T return_depreffed, int context)
{
  MOID_T *common = NULL;
  if (m != NULL && !whether_mode_isnt_well (m) && WHETHER (m, UNION_SYMBOL))
    {
      int depref_level;
      BOOL_T go_on = A_TRUE;
/* Test for increasing depreffing */
      for (depref_level = 0; go_on; depref_level++)
	{
	  PACK_T *p;
	  go_on = A_FALSE;
/* Test the whole pack */
	  for (p = PACK (m); p != NULL; p = NEXT (p))
	    {
/* HIPs are not eligible of course */
	      if (MOID (p) != MODE (HIP))
		{
		  MOID_T *candidate = MOID (p);
		  int k;
/* Depref as far as allowed */
		  for (k = depref_level; k > 0 && whether_deprefable (candidate); k--)
		    {
		      candidate = depref_once (candidate);
		    }
/* Only need testing if all allowed deprefs succeeded */
		  if (k == 0)
		    {
		      PACK_T *q;
		      MOID_T *to = return_depreffed ? depref_completely (candidate) : candidate;
		      BOOL_T all_coercible = A_TRUE;
		      go_on = A_TRUE;
		      for (q = PACK (m); q != NULL && all_coercible; q = NEXT (q))
			{
			  MOID_T *from = MOID (q);
			  if (p != q && from != to)
			    {
			      all_coercible &= whether_coercible (from, to, sort, context);
			    }
			}
/* If the whole pack is coercible to the candidate, we mark the candidate.
   We continue searching since we want the longest series of REF REF PROC REF ..    */
		      if (all_coercible)
			{
			  MOID_T *mark = (return_depreffed ? MOID (p) : candidate);
			  if (common == NULL)
			    {
			      common = mark;
			    }
			  else if (WHETHER (candidate, FLEX_SYMBOL) && DEFLEX (candidate) == common)
			    {
/* We prefer FLEX */
			      common = mark;
			    }
			}
		    }
		}
	    }			/* for */
	}			/* for */
    }
  return (common == NULL ? m : common);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
allowed_clause: whether we can search a common mode from a clause or not.     */

static BOOL_T
allowed_clause (int att)
{
  switch (att)
    {
    case CONDITIONAL_CLAUSE:
    case INTEGER_CASE_CLAUSE:
    case SERIAL_CLAUSE:
    case UNITED_CASE_CLAUSE:
      {
	return (A_TRUE);
      }
    default:
      {
	return (A_FALSE);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
determine_unique_mode: return a unique mode from "z".                         */

static MOID_T *
determine_unique_mode (SOID_T * z, int context)
{
  MOID_T *x;
  if (z == NULL)
    {
      return (NULL);
    }
  x = MOID (z);
  if (whether_mode_isnt_well (x))
    {
      return (MODE (ERROR));
    }
  x = make_united_mode (x);
  if (allowed_clause (ATTRIBUTE (z)))
    {
      return (get_balanced_mode (x, STRONG, NO_DEPREF, context));
    }
  else
    {
      return (x);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
warn_for_voiding: give a warning when a value is silently discarded.          */

static void
warn_for_voiding (NODE_T * p, SOID_T * x, SOID_T * y, int c)
{
  (void) c;
  if (x->cast == A_FALSE)
    {
      if (MOID (x) == MODE (VOID) && !(MOID (y) == MODE (VOID) || !whether_nonproc (MOID (y))))
	{
	  diagnostic (A_WARNING, p, "value from M @ will be voided", MOID (y));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_coercion: insert coercion "a" in the tree.                               */

static void
make_coercion (NODE_T * l, int a, MOID_T * m)
{
  make_sub (l, l, a);
  MOID (l) = depref_rows (MOID (l), m);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_widening_coercion                  */

static void
make_widening_coercion (NODE_T * n, MOID_T * p, MOID_T * q)
{
  MOID_T *z = widens_to (p, q);
  make_coercion (n, WIDENING, z);
  if (z != q)
    {
      make_widening_coercion (n, z, q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_ref_rowing_coercion                                        */

static void
make_ref_rowing_coercion (NODE_T * n, MOID_T * p, MOID_T * q)
{
  if (DEFLEX (p) != DEFLEX (q))
    {
      if (whether_widenable (p, q))
	{
	  make_widening_coercion (n, p, q);
	}
      else if (whether_ref_row (q))
	{
	  make_ref_rowing_coercion (n, p, q->name);
	  make_coercion (n, ROWING, q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_rowing_coercion                                        */

static void
make_rowing_coercion (NODE_T * n, MOID_T * p, MOID_T * q)
{
  if (DEFLEX (p) != DEFLEX (q))
    {
      if (whether_widenable (p, q))
	{
	  make_widening_coercion (n, p, q);
	}
      else if (SLICE (q) != NULL)
	{
	  make_rowing_coercion (n, p, SLICE (q));
	  make_coercion (n, ROWING, q);
	}
      else if (WHETHER (q, FLEX_SYMBOL))
	{
	  make_rowing_coercion (n, p, SUB (q));
	}
      else if (whether_ref_row (q))
	{
	  make_ref_rowing_coercion (n, p, q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_uniting_coercion                                        */

static void
make_uniting_coercion (NODE_T * n, MOID_T * q)
{
  make_coercion (n, UNITING, derow (q));
  if (WHETHER (q, ROW_SYMBOL))
    {
      make_rowing_coercion (n, derow (q), q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_depreffing_coercion                                         */

static void
make_depreffing_coercion (NODE_T * n, MOID_T * p, MOID_T * q)
{
  if (DEFLEX (p) == DEFLEX (q))
    {
      return;
    }
  else if (q == MODE (SIMPLOUT) && whether_printable_mode (p))
    {
      make_coercion (n, UNITING, q);
    }
  else if (q == MODE (ROW_SIMPLOUT) && whether_printable_mode (p))
    {
      make_coercion (n, UNITING, MODE (SIMPLOUT));
      make_coercion (n, ROWING, MODE (ROW_SIMPLOUT));
    }
  else if (q == MODE (SIMPLIN) && whether_readable_mode (p))
    {
      make_coercion (n, UNITING, q);
    }
  else if (q == MODE (ROW_SIMPLIN) && whether_readable_mode (p))
    {
      make_coercion (n, UNITING, MODE (SIMPLIN));
      make_coercion (n, ROWING, MODE (ROW_SIMPLIN));
    }
  else if (q == MODE (ROWS) && whether_rows_type (p))
    {
      make_coercion (n, UNITING, MODE (ROWS));
      MOID (n) = MODE (ROWS);
    }
  else if (whether_widenable (p, q))
    {
      make_widening_coercion (n, p, q);
    }
  else if (whether_unitable (p, derow (q), SAFE_DEFLEXING))
    {
      make_uniting_coercion (n, q);
    }
  else if (whether_ref_row (q) && whether_strong_name (p, q))
    {
      make_ref_rowing_coercion (n, p, q);
    }
  else if (SLICE (q) != NULL && whether_strong_slice (p, q))
    {
      make_rowing_coercion (n, p, q);
    }
  else if (WHETHER (q, FLEX_SYMBOL) && whether_strong_slice (p, q))
    {
      make_rowing_coercion (n, p, q);
    }
  else if (WHETHER (p, REF_SYMBOL))
    {
      MOID_T *r = DEFLEX (SUB (p));
      make_coercion (n, DEREFERENCING, r);
      make_depreffing_coercion (n, r, q);
    }
  else if (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL)
    {
      MOID_T *r = SUB (p);
      make_coercion (n, DEPROCEDURING, r);
      make_depreffing_coercion (n, r, q);
    }
  else if (p != q)
    {
      diagnostic (A_ERROR, n, CANNOT_COERCE_ERROR, p, q, NO_SORT);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_nonproc: whether p is a nonproc mode (that is voided directly).       */

static BOOL_T
whether_nonproc (MOID_T * p)
{
  if (WHETHER (p, PROC_SYMBOL) && PACK (p) == NULL)
    {
      return (A_FALSE);
    }
  else if (WHETHER (p, REF_SYMBOL))
    {
      return (whether_nonproc (SUB (p)));
    }
  else
    {
      return (A_TRUE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_void: voiden COMORFs are voided by eliminating their values.             */

static void
make_void (NODE_T * p, MOID_T * q)
{
  switch (ATTRIBUTE (p))
    {
    case ASSIGNATION:
    case IDENTITY_RELATION:
    case GENERATOR:
    case CAST:
    case DENOTER:
      {
	make_coercion (p, VOIDING, MODE (VOID));
	return;
      }
    }
/* MORFs are an involved case */
  switch (ATTRIBUTE (p))
    {
    case SELECTION:
    case SLICE:
    case ROUTINE_TEXT:
    case FORMULA:
    case CALL:
    case IDENTIFIER:
      {
/* A nonproc moid value is eliminated directly */
	if (whether_nonproc (q))
	  {
	    make_coercion (p, VOIDING, MODE (VOID));
	    return;
	  }
	else
	  {
/* Descend the chain of e.g. "REF PROC PROC REF INT" until a nonproc moid value remains */
	    MOID_T *z = q;
	    while (!whether_nonproc (z))
	      {
		if (WHETHER (z, REF_SYMBOL))
		  {
		    make_coercion (p, DEREFERENCING, SUB (z));
		  }
		if (WHETHER (z, PROC_SYMBOL) && PACK (p) == NULL)
		  {
		    make_coercion (p, DEPROCEDURING, SUB (z));
		  }
		z = SUB (z);
	      }
	    if (z != MODE (VOID))
	      {
		make_coercion (p, VOIDING, MODE (VOID));
	      }
	    return;
	  }
      }
    }
/* All other is voided straight away */
  make_coercion (p, VOIDING, MODE (VOID));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_strong                                        */

static void
make_strong (NODE_T * n, MOID_T * p, MOID_T * q)
{
  if (q == MODE (VOID) && p != MODE (VOID))
    {
      make_void (n, p);
    }
  else
    {
      make_depreffing_coercion (n, p, q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
insert_coercions                                        */

static void
insert_coercions (NODE_T * n, MOID_T * p, SOID_T * q)
{
  make_strong (n, p, MOID (q));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_bounds                                        */

static void
mode_check_bounds (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  SOID_T x, y;
	  make_soid (&x, STRONG, MODE (INT), 0);
	  mode_check_unit (p, &x, &y);
	  if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&y), MODE (INT), MEEK);
	    }
	  mode_check_bounds (NEXT (p));
	}
      else
	{
	  mode_check_bounds (SUB (p));
	  mode_check_bounds (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_declarer                                        */

static void
mode_check_declarer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, BOUNDS))
	{
	  mode_check_bounds (SUB (p));
	  mode_check_declarer (NEXT (p));
	}
      else
	{
	  mode_check_declarer (SUB (p));
	  mode_check_declarer (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_identity_declaration                                        */

static void
mode_check_identity_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    mode_check_declarer (SUB (p));
	    mode_check_identity_declaration (NEXT (p));
	    break;
	  }
	case DEFINING_IDENTIFIER:
	  {
	    SOID_T x, y;
	    make_soid (&x, STRONG, MOID (p), 0);
	    mode_check_unit (NEXT (NEXT (p)), &x, &y);
	    if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	      {
		diagnostic (A_ERROR, NEXT (NEXT (p)), CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	      }
	    break;
	  }
	default:
	  {
	    mode_check_identity_declaration (SUB (p));
	    mode_check_identity_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_variable_declaration                                        */

static void
mode_check_variable_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    mode_check_declarer (SUB (p));
	    mode_check_variable_declaration (NEXT (p));
	    break;
	  }
	case DEFINING_IDENTIFIER:
	  {
	    if (whether (p, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0))
	      {
		SOID_T x, y;
		make_soid (&x, STRONG, (SUB (MOID (p))), 0);
		mode_check_unit (NEXT (NEXT (p)), &x, &y);
		if (!whether_coercible_in_context (&y, &x, FORCE_DEFLEXING))
		  {
		    diagnostic (A_ERROR, NEXT (NEXT (p)), CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
		  }
	      }
	    break;
	  }
	default:
	  {
	    mode_check_variable_declaration (SUB (p));
	    mode_check_variable_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_routine_text                                        */

static void
mode_check_routine_text (NODE_T * p, SOID_T * y)
{
  SOID_T w;
  if (WHETHER (p, PARAMETER_PACK))
    {
      mode_check_declarer (SUB (p));
      p = NEXT (p);
    }
  mode_check_declarer (SUB (p));
  make_soid (&w, STRONG, MOID (p), 0);
  mode_check_unit (NEXT (NEXT (p)), &w, y);
  if (!whether_coercible_in_context (y, &w, ALIAS_DEFLEXING))
    {
      diagnostic (A_ERROR, NEXT (NEXT (p)), CANNOT_COERCE_ERROR, MOID (y), MOID (&w), STRONG);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_proc_declaration             */

static void
mode_check_proc_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case ROUTINE_TEXT:
	  {
	    SOID_T x, y;
	    make_soid (&x, STRONG, NULL, 0);
	    mode_check_routine_text (SUB (p), &y);
	    break;
	  }
	default:
	  {
	    mode_check_proc_declaration (SUB (p));
	    mode_check_proc_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_brief_op_declaration         */

static void
mode_check_brief_op_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DEFINING_OPERATOR:
	  {
	    SOID_T y;
	    if (MOID (p) != MOID (NEXT (NEXT (p))))
	      {
		SOID_T y, x;
		make_soid (&y, NO_SORT, MOID (NEXT (NEXT (p))), 0);
		make_soid (&x, NO_SORT, MOID (p), 0);
		diagnostic (A_ERROR, NEXT (NEXT (p)), CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	      }
	    mode_check_routine_text (SUB (NEXT (NEXT (p))), &y);
	    break;
	  }
	default:
	  mode_check_brief_op_declaration (SUB (p));
	  mode_check_brief_op_declaration (NEXT (p));
	  break;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_op_declaration                                        */

static void
mode_check_op_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DEFINING_OPERATOR:
	  {
	    SOID_T y, x;
	    make_soid (&x, STRONG, MOID (p), 0);
	    mode_check_unit (NEXT (NEXT (p)), &x, &y);
	    if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	      {
		diagnostic (A_ERROR, NEXT (NEXT (p)), CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	      }
	    break;
	  }
	default:
	  {
	    mode_check_op_declaration (SUB (p));
	    mode_check_op_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_declaration_list                                        */

static void
mode_check_declaration_list (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case IDENTITY_DECLARATION:
	  {
	    mode_check_identity_declaration (SUB (p));
	    break;
	  }
	case VARIABLE_DECLARATION:
	  {
	    mode_check_variable_declaration (SUB (p));
	    break;
	  }
	case MODE_DECLARATION:
	  {
	    mode_check_declarer (SUB (p));
	    break;
	  }
	case PROCEDURE_DECLARATION:
	case PROCEDURE_VARIABLE_DECLARATION:
	  {
	    mode_check_proc_declaration (SUB (p));
	    break;
	  }
	case BRIEF_OPERATOR_DECLARATION:
	  {
	    mode_check_brief_op_declaration (SUB (p));
	    break;
	  }
	case OPERATOR_DECLARATION:
	  {
	    mode_check_op_declaration (SUB (p));
	    break;
	  }
	default:
	  {
	    mode_check_declaration_list (SUB (p));
	    mode_check_declaration_list (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_serial                                        */

static void
mode_check_serial (SOID_LIST_T ** r, NODE_T * p, SOID_T * x, BOOL_T k)
{
  if (p != NULL)
    {
      if (WHETHER (p, INITIALISER_SERIES))
	{
	  mode_check_serial (r, SUB (p), x, A_FALSE);
	  mode_check_serial (r, NEXT (p), x, k);
	}
      else if (WHETHER (p, DECLARATION_LIST))
	{
	  mode_check_declaration_list (SUB (p));
	}
      else if (WHETHER (p, LABEL) || WHETHER (p, SEMI_SYMBOL) || WHETHER (p, EXIT_SYMBOL))
	{
	  mode_check_serial (r, NEXT (p), x, k);
	}
      else if (WHETHER (p, SERIAL_CLAUSE) || WHETHER (p, ENQUIRY_CLAUSE))
	{
	  if (NEXT (p) != NULL)
	    {
	      if (WHETHER (NEXT (p), EXIT_SYMBOL) || WHETHER (NEXT (p), END_SYMBOL) || WHETHER (NEXT (p), CLOSE_SYMBOL))
		{
		  mode_check_serial (r, SUB (p), x, A_TRUE);
		}
	      else
		{
		  mode_check_serial (r, SUB (p), x, A_FALSE);
		}
	      mode_check_serial (r, NEXT (p), x, k);
	    }
	  else
	    {
	      mode_check_serial (r, SUB (p), x, A_TRUE);
	    }
	}
      else if (WHETHER (p, LABELED_UNIT))
	{
	  mode_check_serial (r, SUB (p), x, k);
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T y;
	  if (k)
	    {
	      mode_check_unit (p, x, &y);
	    }
	  else
	    {
	      SOID_T w;
	      make_soid (&w, STRONG, MODE (VOID), 0);
	      mode_check_unit (p, &w, &y);
	    }
	  if (NEXT (p) != NULL)
	    {
	      mode_check_serial (r, NEXT (p), x, k);
	    }
	  else
	    {
	      if (k)
		{
		  add_to_soid_list (r, p, &y);
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_serial_units.                                                      */

static void
mode_check_serial_units (NODE_T * p, SOID_T * x, SOID_T * y, int att)
{
  SOID_LIST_T *top_sl = NULL;
  (void) att;
  mode_check_serial (&top_sl, SUB (p), x, A_TRUE);
  if (whether_balanced (p, top_sl, SORT (x)))
    {
      MOID_T *result = pack_soids_in_moid (top_sl, SERIES_MODE);
      make_soid (y, SORT (x), result, SERIAL_CLAUSE);
    }
  else
    {
      make_soid (y, SORT (x), MOID (x) != NULL ? MOID (x) : MODE (ERROR), 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_unit_list.                                                         */

static void
mode_check_unit_list (SOID_LIST_T ** r, NODE_T * p, SOID_T * x)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT_LIST))
	{
	  mode_check_unit_list (r, SUB (p), x);
	  mode_check_unit_list (r, NEXT (p), x);
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  mode_check_unit_list (r, NEXT (p), x);
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T y;
	  mode_check_unit (p, x, &y);
	  add_to_soid_list (r, p, &y);
	  mode_check_unit_list (r, NEXT (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_struct_display.                                                    */

static void
mode_check_struct_display (SOID_LIST_T ** r, NODE_T * p, PACK_T ** fields)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT_LIST))
	{
	  mode_check_struct_display (r, SUB (p), fields);
	  mode_check_struct_display (r, NEXT (p), fields);
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  mode_check_struct_display (r, NEXT (p), fields);
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T x, y;
	  if (*fields != NULL)
	    {
	      make_soid (&x, STRONG, MOID (*fields), 0);
	      *fields = NEXT (*fields);
	    }
	  else
	    {
	      make_soid (&x, STRONG, NULL, 0);
	    }
	  mode_check_unit (p, &x, &y);
	  add_to_soid_list (r, p, &y);
	  mode_check_struct_display (r, NEXT (p), fields);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_get_specified_moids.                                               */

static void
mode_check_get_specified_moids (NODE_T * p, MOID_T * u)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, SPECIFIED_UNIT_LIST) || WHETHER (p, SPECIFIED_UNIT))
	{
	  mode_check_get_specified_moids (SUB (p), u);
	}
      else if (WHETHER (p, SPECIFIER))
	{
/* Fetch moid of specifier and test this moid for firm relations with others */
	  MOID_T *m = MOID (NEXT_SUB (p));
	  PACK_T *v;
/* First prevent e.g. (A): ..., (REF A): ..., (PROC A): ..., (UNION (A, B)): ... */
	  for (v = PACK (u); v != NULL; v = NEXT (v))
	    {
	      if (whether_firm (m, MOID (v)))
		{
		  diagnostic (A_ERROR, p, "ambiguous mode M in A", m, SPECIFIED_UNIT_LIST);
		}
	    }
/* Then prevent e.g. (UNION (A, B)): ..., (UNION (A, C)): ... */
	  if (WHETHER (m, UNION_SYMBOL))
	    {
	      BOOL_T all, some;
	      investigate_firm_relations (PACK (m), PACK (u), &all, &some);
	      if (some)
		{
		  diagnostic (A_ERROR, p, "ambiguous mode M in A", m, SPECIFIED_UNIT_LIST);
		}
	    }
	  add_mode_to_pack (&(PACK (u)), m, NULL, NODE (m));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_specified_unit_list                                        */

static void
mode_check_specified_unit_list (SOID_LIST_T ** r, NODE_T * p, SOID_T * x, MOID_T * u)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, SPECIFIED_UNIT_LIST) || WHETHER (p, SPECIFIED_UNIT))
	{
	  mode_check_specified_unit_list (r, SUB (p), x, u);
	}
      else if (WHETHER (p, SPECIFIER))
	{
	  MOID_T *m = MOID (NEXT_SUB (p));
	  if (u != NULL && !whether_unitable (m, u, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, "M cannot be united to M", m, u);
	    }
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T y;
	  mode_check_unit (p, x, &y);
	  add_to_soid_list (r, p, &y);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_united_case_parts                                        */

static void
mode_check_united_case_parts (SOID_LIST_T ** ry, NODE_T * p, SOID_T * x)
{
  SOID_T enq_expct, enq_yield;
  MOID_T *u = NULL, *v = NULL, *w = NULL;
/* Check the CASE part and deduce the united mode */
  make_soid (&enq_expct, STRONG, NULL, 0);
  mode_check_serial_units (NEXT_SUB (p), &enq_expct, &enq_yield, ENQUIRY_CLAUSE);
/* Deduce the united mode from the enquiry clause */
  u = make_united_mode (MOID (&enq_yield));
  u = depref_completely (u);
/* Also deduce the united mode from the specifiers */
  v = new_moid ();
  ATTRIBUTE (v) = SERIES_MODE;
  mode_check_get_specified_moids (NEXT (SUB (NEXT (p))), v);
  v = make_united_mode (v);
/* Determine a resulting union */
  if (u == MODE (HIP))
    {
      w = v;
    }
  else
    {
      if (WHETHER (u, UNION_SYMBOL))
	{
	  BOOL_T uv, vu, some;
	  investigate_firm_relations (PACK (u), PACK (v), &uv, &some);
	  investigate_firm_relations (PACK (v), PACK (u), &vu, &some);
	  if (uv && vu)
	    {
/* Every component has a specifier */
	      w = u;
	    }
	  else if (!uv && !vu)
	    {
/* Hmmmm ... let the coercer sort it out */
	      w = u;
	    }
	  else
	    {
/*  This is all the balancing we allow here for the moment. Firmly related 
subsets are not valid so we absorb them. If this doesn't solve it then we 
get a coercion-error later.                                                  */
	      w = absorb_related_subsets (u);
	    }
	}
      else
	{
	  diagnostic (A_ERROR, NEXT_SUB (p), "M is not a united mode", u);
	  return;
	}
    }
  MOID (SUB (p)) = w;
  p = NEXT (p);
/* Check the IN part */
  mode_check_specified_unit_list (ry, NEXT_SUB (p), x, w);
/* OUSE, OUT, ESAC */
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, OUT_PART) || WHETHER (p, CHOICE))
	{
	  mode_check_serial (ry, NEXT_SUB (p), x, A_TRUE);
	}
      else if (WHETHER (p, UNITED_OUSE_PART) || WHETHER (p, BRIEF_UNITED_OUSE_PART))
	{
	  mode_check_united_case_parts (ry, SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_united_case                                        */

static void
mode_check_united_case (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_LIST_T *top_sl = NULL;
  MOID_T *z;
  mode_check_united_case_parts (&top_sl, p, x);
  if (!whether_balanced (p, top_sl, SORT (x)))
    {
      if (MOID (x) != NULL)
	{
	  make_soid (y, SORT (x), MOID (x), UNITED_CASE_CLAUSE);

	}
      else
	{
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
    }
  else
    {
      z = pack_soids_in_moid (top_sl, SERIES_MODE);
      make_soid (y, SORT (x), z, UNITED_CASE_CLAUSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_unit_list_2                                        */

static void
mode_check_unit_list_2 (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_LIST_T *top_sl = NULL;
/* Will hopefully get balanced.
  if (SORT (x) != STRONG || MOID (x) == NULL) {
    diagnostic (A_ERROR, p, "display must be C", STRONG);
  }
*/
  if (MOID (x) != NULL)
    {
      if (WHETHER (MOID (x), FLEX_SYMBOL))
	{
	  SOID_T y;
	  make_soid (&y, SORT (x), SLICE (SUB (MOID (x))), 0);
	  mode_check_unit_list (&top_sl, SUB (p), &y);
	}
      else if (WHETHER (MOID (x), ROW_SYMBOL))
	{
	  SOID_T y;
	  make_soid (&y, SORT (x), SLICE (MOID (x)), 0);
	  mode_check_unit_list (&top_sl, SUB (p), &y);
	}
      else if (WHETHER (MOID (x), STRUCT_SYMBOL))
	{
	  PACK_T *y = PACK (MOID (x));
	  mode_check_struct_display (&top_sl, SUB (p), &y);
	}
      else
	{
	  mode_check_unit_list (&top_sl, SUB (p), x);
	}
    }
  else
    {
      mode_check_unit_list (&top_sl, SUB (p), x);
    }
  make_soid (y, STRONG, pack_soids_in_moid (top_sl, STOWED_MODE), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_closed                                        */

static void
mode_check_closed (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  mode_check_serial_units (p, x, y, SERIAL_CLAUSE);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL))
	{
	  mode_check_closed (NEXT (p), x, y);
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_export_clause                                        */

static void
mode_check_export_clause (NODE_T * p, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, INITIALISER_SERIES))
	{
	  mode_check_declaration_list (p);
	}
      else
	{
	  mode_check_export_clause (NEXT (p), y);
	  make_soid (y, STRONG, MODE (VOID), 0);
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_collateral                                        */

static void
mode_check_collateral (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      if (whether (p, BEGIN_SYMBOL, END_SYMBOL, 0) || whether (p, OPEN_SYMBOL, CLOSE_SYMBOL, 0))
	{
	  if (SORT (x) == STRONG)
	    {
	      make_soid (y, STRONG, MODE (VACUUM), 0);
	    }
	  else
	    {
	      make_soid (y, STRONG, MODE (UNDEFINED), 0);
	    }
	}
      else
	{
	  if (WHETHER (p, UNIT_LIST))
	    {
	      mode_check_unit_list_2 (p, x, y);
	    }
	  else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL))
	    {
	      mode_check_collateral (NEXT (p), x, y);
	    }
	  MOID (p) = MOID (y);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_conditional_2                                        */

static void
mode_check_conditional_2 (SOID_LIST_T ** ry, NODE_T * p, SOID_T * x)
{
  SOID_T enq_expct, enq_yield;
  make_soid (&enq_expct, STRONG, MODE (BOOL), 0);
  mode_check_serial_units (NEXT_SUB (p), &enq_expct, &enq_yield, ENQUIRY_CLAUSE);
  if (!whether_coercible_in_context (&enq_yield, &enq_expct, SAFE_DEFLEXING))
    {
      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&enq_yield), MOID (&enq_expct), MEEK);
    }
  p = NEXT (p);
  mode_check_serial (ry, NEXT_SUB (p), x, A_TRUE);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, ELSE_PART) || WHETHER (p, CHOICE))
	{
	  mode_check_serial (ry, NEXT_SUB (p), x, A_TRUE);
	}
      else if (WHETHER (p, ELIF_PART) || WHETHER (p, BRIEF_ELIF_IF_PART))
	{
	  mode_check_conditional_2 (ry, SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_conditional                                        */

static void
mode_check_conditional (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_LIST_T *top_sl = NULL;
  MOID_T *z;
  mode_check_conditional_2 (&top_sl, p, x);
  if (!whether_balanced (p, top_sl, SORT (x)))
    {
      if (MOID (x) != NULL)
	{
	  make_soid (y, SORT (x), MOID (x), CONDITIONAL_CLAUSE);
	}
      else
	{
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
    }
  else
    {
      z = pack_soids_in_moid (top_sl, SERIES_MODE);
      make_soid (y, SORT (x), z, CONDITIONAL_CLAUSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_int_case_2                                        */

static void
mode_check_int_case_2 (SOID_LIST_T ** ry, NODE_T * p, SOID_T * x)
{
  SOID_T enq_expct, enq_yield;
  make_soid (&enq_expct, STRONG, MODE (INT), 0);
  mode_check_serial_units (NEXT_SUB (p), &enq_expct, &enq_yield, ENQUIRY_CLAUSE);
  if (!whether_coercible_in_context (&enq_yield, &enq_expct, SAFE_DEFLEXING))
    {
      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&enq_yield), MOID (&enq_expct), MEEK);
    }
  p = NEXT (p);
  mode_check_unit_list (ry, NEXT_SUB (p), x);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, OUT_PART) || WHETHER (p, CHOICE))
	{
	  mode_check_serial (ry, NEXT_SUB (p), x, A_TRUE);
	}
      else if (WHETHER (p, INTEGER_OUT_PART) || WHETHER (p, BRIEF_INTEGER_OUSE_PART))
	{
	  mode_check_int_case_2 (ry, SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_int_case                                        */

static void
mode_check_int_case (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_LIST_T *top_sl = NULL;
  MOID_T *z;
  mode_check_int_case_2 (&top_sl, p, x);
  if (!whether_balanced (p, top_sl, SORT (x)))
    {
      if (MOID (x) != NULL)
	{
	  make_soid (y, SORT (x), MOID (x), INTEGER_CASE_CLAUSE);
	}
      else
	{
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
    }
  else
    {
      z = pack_soids_in_moid (top_sl, SERIES_MODE);
      make_soid (y, SORT (x), z, INTEGER_CASE_CLAUSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_loop_2                                        */

static void
mode_check_loop_2 (NODE_T * p, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, FOR_PART))
	{
	  mode_check_loop_2 (NEXT (p), y);
	}
      else if (WHETHER (p, FROM_PART) || WHETHER (p, BY_PART) || WHETHER (p, TO_PART))
	{
	  SOID_T ix, iy;
	  make_soid (&ix, STRONG, MODE (INT), 0);
	  mode_check_unit (NEXT_SUB (p), &ix, &iy);
	  if (!whether_coercible_in_context (&iy, &ix, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, NEXT_SUB (p), CANNOT_COERCE_ERROR, MOID (&iy), MODE (INT), MEEK);
	    }
	  mode_check_loop_2 (NEXT (p), y);
	}
      else if (WHETHER (p, WHILE_PART))
	{
	  SOID_T enq_expct, enq_yield;
	  make_soid (&enq_expct, STRONG, MODE (BOOL), 0);
	  mode_check_serial_units (NEXT_SUB (p), &enq_expct, &enq_yield, ENQUIRY_CLAUSE);
	  if (!whether_coercible_in_context (&enq_yield, &enq_expct, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&enq_yield), MOID (&enq_expct), MEEK);
	    }
	  mode_check_loop_2 (NEXT (p), y);
	}
      else if (WHETHER (p, DO_PART) || WHETHER (p, ALT_DO_PART))
	{
	  SOID_LIST_T *z = NULL;
	  SOID_T ix;
	  make_soid (&ix, STRONG, MODE (VOID), 0);
	  mode_check_serial (&z, NEXT_SUB (p), &ix, A_TRUE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_loop                                        */

static void
mode_check_loop (NODE_T * p, SOID_T * y)
{
  SOID_T *z = NULL;
  mode_check_loop_2 (p, /* y */ z);
  make_soid (y, STRONG, MODE (VOID), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_enclosed                                        */

void
mode_check_enclosed (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, ENCLOSED_CLAUSE))
	{
	  mode_check_enclosed (SUB (p), x, y);
	}
      else if (WHETHER (p, CLOSED_CLAUSE))
	{
	  mode_check_closed (SUB (p), x, y);
	}
      else if (WHETHER (p, PARALLEL_CLAUSE))
	{
	  mode_check_collateral (NEXT_SUB (p), x, y);
	  make_soid (y, STRONG, MODE (VOID), 0);
	  MOID (NEXT_SUB (p)) = MODE (VOID);
	}
      else if (WHETHER (p, COLLATERAL_CLAUSE))
	{
	  mode_check_collateral (SUB (p), x, y);
	}
      else if (WHETHER (p, CONDITIONAL_CLAUSE))
	{
	  mode_check_conditional (SUB (p), x, y);
	}
      else if (WHETHER (p, INTEGER_CASE_CLAUSE))
	{
	  mode_check_int_case (SUB (p), x, y);
	}
      else if (WHETHER (p, UNITED_CASE_CLAUSE))
	{
	  mode_check_united_case (SUB (p), x, y);
	}
      else if (WHETHER (p, LOOP_CLAUSE))
	{
	  mode_check_loop (SUB (p), y);
	}
      else if (WHETHER (p, EXPORT_CLAUSE))
	{
	  mode_check_export_clause (SUB (p), y);
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
search_table_for_operator: search table and return operator "x n y" or "n x". */

static TAG_T *
search_table_for_operator (TAG_T * t, char *n, MOID_T * x, MOID_T * y, int context)
{
  if (whether_mode_isnt_well (x))
    {
      return (error_tag);
    }
  else if (y != NULL && whether_mode_isnt_well (y))
    {
      return (error_tag);
    }
  for (; t != NULL; t = NEXT (t))
    {
      if (SYMBOL (NODE (t)) == n)
	{
	  PACK_T *p = PACK (MOID (t));
	  if (whether_coercible (x, MOID (p), FIRM, context))
	    {
	      p = NEXT (p);
	      if (p == NULL && y == NULL)
		{
/* Matched in case of a monad */
		  return (t);
		}
	      else if (p != NULL && y != NULL && whether_coercible (y, MOID (p), FIRM, context))
		{
/* Matched in case of a nomad */
		  return (t);
		}
	    }
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
search_table_chain_for_operator: search chain of symbol tables and return 
                                 matching operator "x n y" or "n x".          */

static TAG_T *
search_table_chain_for_operator (SYMBOL_TABLE_T * s, char *n, MOID_T * x, MOID_T * y, int context)
{
  if (whether_mode_isnt_well (x))
    {
      return (error_tag);
    }
  else if (y != NULL && whether_mode_isnt_well (y))
    {
      return (error_tag);
    }
  while (s != NULL)
    {
      TAG_T *z = search_table_for_operator (s->operators, n, x, y, context);
      if (z != NULL)
	{
	  return (z);
	}
      s = PREVIOUS (s);
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_operator: return a matching operator "x n y"

Coercions to operand modes are FIRM.                                          */

static TAG_T *
find_operator (SYMBOL_TABLE_T * s, char *n, MOID_T * x, MOID_T * y)
{
  TAG_T *z;
  MOID_T *u, *v;
/* (A) Catch exceptions first */
  if (x == NULL && y == NULL)
    {
      return (NULL);
    }
  else if (whether_mode_isnt_well (x))
    {
      return (error_tag);
    }
  else if (y != NULL && whether_mode_isnt_well (y))
    {
      return (error_tag);
    }
/* (B) MONADs */
  if (x != NULL && y == NULL)
    {
      z = search_table_chain_for_operator (s, n, x, NULL, SAFE_DEFLEXING);
      return (z);
    }
/* (C) NOMADs */
  z = search_table_chain_for_operator (s, n, x, y, SAFE_DEFLEXING);
  if (z != NULL)
    {
      return (z);
    }
/* (D) Look in standenv for an appropriate cross-term */
  u = make_series_from_moids (x, y);
  u = make_united_mode (u);
  v = get_balanced_mode (u, STRONG, NO_DEPREF, SAFE_DEFLEXING);
  z = search_table_for_operator (stand_env->operators, n, v, v, ALIAS_DEFLEXING);
  if (z != NULL)
    {
      return (z);
    }
/* (E) Now allow for depreffing for REF REAL +:= INT and alike */
  v = get_balanced_mode (u, STRONG, DEPREF, SAFE_DEFLEXING);
  z = search_table_for_operator (stand_env->operators, n, v, v, ALIAS_DEFLEXING);
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_monadic_operator */

static void
mode_check_monadic_operator (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      TAG_T *t;
      MOID_T *u;
      u = determine_unique_mode (y, SAFE_DEFLEXING);
      if (whether_mode_isnt_well (u))
	{
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else if (u == MODE (HIP))
	{
	  diagnostic (A_ERROR, NEXT (p), "M construct is not a valid operand", u);
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else
	{
	  t = find_operator (SYMBOL_TABLE (p), SYMBOL (p), u, NULL);
	  if (t == NULL)
	    {
	      diagnostic (A_ERROR, p, "operator S O has not been declared in this range", u);
	      make_soid (y, SORT (x), MODE (ERROR), 0);
	    }
	  if (t != NULL)
	    {
	      MOID (p) = MOID (t);
	    }
	  TAX (p) = t;
	  if (t != NULL && t != error_tag)
	    {
	      MOID (p) = MOID (t);
	      make_soid (y, SORT (x), SUB (MOID (t)), 0);
	    }
	  else
	    {
	      MOID (p) = MODE (ERROR);
	      make_soid (y, SORT (x), MODE (ERROR), 0);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_monadic_formula                                        */

static void
mode_check_monadic_formula (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T e;
  make_soid (&e, FIRM, NULL, 0);
  mode_check_formula (NEXT (p), &e, y);
  mode_check_monadic_operator (p, &e, y);
  make_soid (y, SORT (x), MOID (y), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_formula                                        */

static void
mode_check_formula (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T ls, rs;
  TAG_T *op;
  MOID_T *u, *v;
  if (WHETHER (p, MONADIC_FORMULA))
    {
      mode_check_monadic_formula (SUB (p), x, &ls);
    }
  else if (WHETHER (p, FORMULA))
    {
      mode_check_formula (SUB (p), x, &ls);
    }
  else if (WHETHER (p, SECONDARY))
    {
      SOID_T e;
      make_soid (&e, FIRM, NULL, 0);
      mode_check_secondary (SUB (p), &e, &ls);
    }
  u = determine_unique_mode (&ls, SAFE_DEFLEXING);
  MOID (p) = u;
  if (NEXT (p) == NULL)
    {
      make_soid (y, SORT (x), u, 0);
    }
  else
    {
      NODE_T *q = NEXT (NEXT (p));
      if (WHETHER (q, MONADIC_FORMULA))
	{
	  mode_check_monadic_formula (SUB (NEXT (NEXT (p))), x, &rs);
	}
      else if (WHETHER (q, FORMULA))
	{
	  mode_check_formula (SUB (NEXT (NEXT (p))), x, &rs);
	}
      else if (WHETHER (q, SECONDARY))
	{
	  SOID_T e;
	  make_soid (&e, FIRM, NULL, 0);
	  mode_check_secondary (SUB (q), &e, &rs);
	}
      v = determine_unique_mode (&rs, SAFE_DEFLEXING);
      MOID (q) = v;
      if (whether_mode_isnt_well (u) || whether_mode_isnt_well (v))
	{
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else if (u == MODE (HIP))
	{
	  diagnostic (A_ERROR, p, "M construct is not a valid operand", u);
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else if (v == MODE (HIP))
	{
	  diagnostic (A_ERROR, q, "M construct is not a valid operand", u);
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else
	{
	  op = find_operator (SYMBOL_TABLE (NEXT (p)), SYMBOL (NEXT (p)), u, v);
	  if (op == NULL)
	    {
	      diagnostic (A_ERROR, NEXT (p), "operator O S O has not been declared in this range", u, v);
	      make_soid (y, SORT (x), MODE (ERROR), 0);
	    }
	  if (op != NULL)
	    {
	      MOID (NEXT (p)) = MOID (op);
	    }
	  TAX (NEXT (p)) = op;
	  if (op != NULL && op != error_tag)
	    {
	      make_soid (y, SORT (x), SUB (MOID (op)), 0);
	    }
	  else
	    {
	      make_soid (y, SORT (x), MODE (ERROR), 0);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_tertiary                                        */

static void
mode_check_tertiary (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (WHETHER (p, TERTIARY))
    {
      mode_check_tertiary (SUB (p), x, y);
    }
  else if (WHETHER (p, SECONDARY))
    {
      mode_check_secondary (SUB (p), x, y);
      MOID (p) = MOID (y);
    }
  else if (WHETHER (p, NIHIL))
    {
      make_soid (y, STRONG, MODE (HIP), 0);
      MOID (p) = MOID (y);
    }
  else if (WHETHER (p, FORMULA))
    {
      mode_check_formula (p, x, y);
      if (ATTRIBUTE (MOID (y)) != REF_SYMBOL)
	{
	  warn_for_voiding (p, x, y, FORMULA);
	}
    }
  else if (WHETHER (p, JUMP) || WHETHER (p, SKIP))
    {
      make_soid (y, STRONG, MODE (HIP), 0);
      MOID (p) = MODE (HIP);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_assignation                                        */

static void
mode_check_assignation (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T name, tmp, value;
  MOID_T *name_moid, *source_moid, *dest_moid, *ori;
/* Get destination mode */
  make_soid (&name, SOFT, NULL, 0);
  mode_check_tertiary (SUB (p), &name, &tmp);
  dest_moid = MOID (&tmp);
/* SOFT coercion */
  ori = determine_unique_mode (&tmp, SAFE_DEFLEXING);
  name_moid = deproc_completely (ori);
  if (ATTRIBUTE (name_moid) != REF_SYMBOL)
    {
      if (whether_mode_is_well (name_moid))
	{
	  diagnostic (A_ERROR, p, "M A cannot yield a name", ori, ATTRIBUTE (SUB (p)));
	}
      make_soid (y, SORT (x), MODE (ERROR), 0);
      return;
    }
  MOID (p) = name_moid;
  make_soid (&name, STRONG, SUB (name_moid), 0);
/* Get source mode */
  mode_check_unit (NEXT (NEXT (p)), &name, &value);
  if (!whether_coercible_in_context (&value, &name, FORCE_DEFLEXING))
    {
      source_moid = MOID (&value);
      diagnostic (A_ERROR, p, "M cannot be assigned to M C", source_moid, dest_moid, STRONG);
      make_soid (y, SORT (x), MODE (ERROR), 0);
    }
  else
    {
      make_soid (y, SORT (x), name_moid, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_identity_relation                                        */

static void
mode_check_identity_relation (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T e, l, r;
  MOID_T *lhs, *rhs, *oril, *orir;
  NODE_T *ln = p, *rn = NEXT (NEXT (p));
  make_soid (&e, SOFT, NULL, 0);
  mode_check_tertiary (SUB (ln), &e, &l);
  mode_check_tertiary (SUB (rn), &e, &r);
/* SOFT coercion */
  oril = determine_unique_mode (&l, SAFE_DEFLEXING);
  orir = determine_unique_mode (&r, SAFE_DEFLEXING);
  lhs = deproc_completely (oril);
  rhs = deproc_completely (orir);
  if (whether_mode_is_well (lhs) && lhs != MODE (HIP) && ATTRIBUTE (lhs) != REF_SYMBOL)
    {
      diagnostic (A_ERROR, ln, "M A cannot yield a name", oril, ATTRIBUTE (SUB (ln)));
      lhs = MODE (ERROR);
    }
  if (whether_mode_is_well (rhs) && rhs != MODE (HIP) && ATTRIBUTE (rhs) != REF_SYMBOL)
    {
      diagnostic (A_ERROR, rn, "M A cannot yield a name", orir, ATTRIBUTE (SUB (rn)));
      rhs = MODE (ERROR);
    }
  if (lhs == MODE (HIP) && rhs == MODE (HIP))
    {
      diagnostic (A_ERROR, p, "cannot find unique mode for tertiaries");
    }
  if (whether_coercible (lhs, rhs, STRONG, SAFE_DEFLEXING))
    {
      lhs = rhs;
    }
  else if (whether_coercible (rhs, lhs, STRONG, SAFE_DEFLEXING))
    {
      rhs = lhs;
    }
  else
    {
      diagnostic (A_ERROR, NEXT (p), CANNOT_COERCE_ERROR, rhs, lhs, STRONG);
      lhs = rhs = MODE (ERROR);
    }
  MOID (ln) = lhs;
  MOID (rn) = rhs;
  make_soid (y, SORT (x), MODE (BOOL), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_bool_function                                        */

static void
mode_check_bool_function (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T e, l, r;
  NODE_T *ln = p, *rn = NEXT (NEXT (p));
  make_soid (&e, STRONG, MODE (BOOL), 0);
  mode_check_tertiary (SUB (ln), &e, &l);
  if (!whether_coercible_in_context (&l, &e, SAFE_DEFLEXING))
    {
      diagnostic (A_ERROR, ln, CANNOT_COERCE_ERROR, MOID (&l), MOID (&e), MEEK);
    }
  mode_check_tertiary (SUB (rn), &e, &r);
  if (!whether_coercible_in_context (&r, &e, SAFE_DEFLEXING))
    {
      diagnostic (A_ERROR, rn, CANNOT_COERCE_ERROR, MOID (&r), MOID (&e), MEEK);
    }
  MOID (ln) = MODE (BOOL);
  MOID (rn) = MODE (BOOL);
  make_soid (y, SORT (x), MODE (BOOL), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_cast                                        */

static void
mode_check_cast (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T w;
  mode_check_declarer (p);
  make_soid (&w, STRONG, MOID (p), 0);
  w.cast = A_TRUE;
  mode_check_enclosed (SUB_NEXT (p), &w, y);
  if (!whether_coercible_in_context (y, &w, ALIAS_DEFLEXING))
    {
      diagnostic (A_ERROR, NEXT (p), CANNOT_COERCE_ERROR, MOID (y), MOID (&w), STRONG);
    }
  make_soid (y, SORT (x), MOID (p), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_assertion                                        */

static void
mode_check_assertion (NODE_T * p)
{
  SOID_T w, y;
  make_soid (&w, STRONG, MODE (BOOL), 0);
  mode_check_enclosed (SUB_NEXT (p), &w, &y);
  SORT (&y) = SORT (&w);	/* Patch */
  if (!whether_coercible_in_context (&y, &w, NO_DEFLEXING))
    {
      diagnostic (A_ERROR, NEXT (p), CANNOT_COERCE_ERROR, MOID (&y), MOID (&w), MEEK);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_argument_list                                        */

static void
mode_check_argument_list (SOID_LIST_T ** r, NODE_T * p, PACK_T ** x)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, GENERIC_ARGUMENT_LIST))
	{
	  ATTRIBUTE (p) = ARGUMENT_LIST;
	}
      if (WHETHER (p, ARGUMENT_LIST))
	{
	  mode_check_argument_list (r, SUB (p), x);
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T y, z;
	  if (*x != NULL)
	    {
	      make_soid (&z, STRONG, MOID (*x), 0);
	      *x = NEXT (*x);
	    }
	  else
	    {
	      make_soid (&z, STRONG, NULL, 0);
	    }
	  mode_check_unit (p, &z, &y);
	  add_to_soid_list (r, p, &y);
	}
      else if (WHETHER (p, TRIMMER))
	{
	  SOID_T z;
	  if (*x != NULL)
	    {
	      make_soid (&z, STRONG, MODE (ERROR), 0);
	      *x = NEXT (*x);
	    }
	  else
	    {
	      make_soid (&z, STRONG, MODE (ERROR), 0);
	    }
	  add_to_soid_list (r, p, &z);
	  diagnostic (A_SYNTAX_ERROR, p, SYNTAX_ERROR, CALL);
	}
      else if (WHETHER (p, SUB_SYMBOL) && !p->info->module->options.brackets)
	{
	  diagnostic (A_SYNTAX_ERROR, p, SYNTAX_ERROR, CALL);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_argument_list_2                                        */

static void
mode_check_argument_list_2 (NODE_T * p, PACK_T * x, SOID_T * y)
{
  SOID_LIST_T *top_sl = NULL;
  mode_check_argument_list (&top_sl, SUB (p), &x);
  make_soid (y, STRONG, pack_soids_in_moid (top_sl, STOWED_MODE), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_meek_int                                        */

static void
mode_check_meek_int (NODE_T * p)
{
  SOID_T x, y;
  make_soid (&x, STRONG, MODE (INT), 0);
  mode_check_unit (p, &x, &y);
  if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
    {
      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), MEEK);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_trimmer                                        */

static void
mode_check_trimmer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, TRIMMER))
	{
	  mode_check_trimmer (SUB (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  mode_check_meek_int (p);
	  mode_check_trimmer (NEXT (p));
	}
      else
	{
	  mode_check_trimmer (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_indexer                                        */

static void
mode_check_indexer (NODE_T * p, int *subs, int *trims)
{
  if (p != NULL)
    {
      if (WHETHER (p, TRIMMER))
	{
	  (*trims)++;
	  mode_check_trimmer (SUB (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  (*subs)++;
	  mode_check_meek_int (p);
	}
      else
	{
	  mode_check_indexer (SUB (p), subs, trims);
	  mode_check_indexer (NEXT (p), subs, trims);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_call                                        */

static void
mode_check_call (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T w, d;
  MOID_T *n, *ori;
  make_soid (&w, MEEK, NULL, 0);
  mode_check_primary (SUB (p), &w, &d);
/* MEEK coercion */
  ori = determine_unique_mode (&d, SAFE_DEFLEXING);
  n = depref_completely (ori);
  if (ATTRIBUTE (n) != PROC_SYMBOL)
    {
      if (whether_mode_is_well (n))
	{
	  diagnostic (A_ERROR, p, "M A cannot yield a procedure with arguments", ori, ATTRIBUTE (SUB (p)));
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	  return;
	}
      make_soid (y, SORT (x), n, 0);
    }
  else
    {
      MOID (p) = n;
      p = NEXT (p);
      mode_check_argument_list_2 (p, PACK (n), &d);
      if (DIMENSION (MOID (&d)) != DIMENSION (n))
	{
	  if (DIMENSION (n) == 1)
	    {
	      diagnostic (A_ERROR, p, "call of M requires D argument", n, DIMENSION (n));
	    }
	  else
	    {
	      diagnostic (A_ERROR, p, "call of M requires D arguments", n, DIMENSION (n));
	    }
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else
	{
	  if (!whether_coercible (MOID (&d), n, STRONG, ALIAS_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, "M is not an argument list for M", MOID (&d), n);
	    }
	  make_soid (y, SORT (x), SUB (n), 0);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_slice                                        */

static int
mode_check_slice (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T w, d;
  MOID_T *m, *ori;
  make_soid (&w, WEAK, NULL, 0);
  mode_check_primary (SUB (p), &w, &d);
  ori = determine_unique_mode (&d, SAFE_DEFLEXING);
  m = depref_completely (ori);
  if (ATTRIBUTE (m) == PROC_SYMBOL)
    {
/* Assume CALL */
      MOID_T *n = m;
      MOID (p) = n;
      p = NEXT (p);
      mode_check_argument_list_2 (p, PACK (n), &d);
      if (DIMENSION (MOID (&d)) != DIMENSION (n))
	{
	  if (DIMENSION (n) == 1)
	    {
	      diagnostic (A_ERROR, p, "call of M requires D argument", n, DIMENSION (n));
	    }
	  else
	    {
	      diagnostic (A_ERROR, p, "call of M requires D arguments", n, DIMENSION (n));
	    }
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else
	{
	  if (!whether_coercible (MOID (&d), n, STRONG, ALIAS_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, "M is not an argument list for M", MOID (&d), n);
	    }
	  make_soid (y, SORT (x), SUB (n), 0);
	}
      return (CALL);
    }
  else
    {
/* assume SLICE */
      BOOL_T whether_ref;
      int rowdim, subs, trims;
/* WEAK coercion */
      MOID_T *n = ori;
      while ((WHETHER (n, REF_SYMBOL) && !whether_ref_row (n)) || (WHETHER (n, PROC_SYMBOL) && PACK (n) == NULL))
	{
	  n = depref_once (n);
	}
      if (n == NULL || !(SLICE (DEFLEX (n)) != NULL || whether_ref_row (n)))
	{
	  if (whether_mode_is_well (n))
	    {
	      diagnostic (A_ERROR, p, "M A cannot be sliced nor called", ori, ATTRIBUTE (SUB (p)));
	    }
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	  return (PRIMARY);
	}
      MOID (p) = n;
      subs = trims = 0;
      mode_check_indexer (SUB (NEXT (p)), &subs, &trims);
      if ((whether_ref = whether_ref_row (n)) != 0)
	{
	  rowdim = DEFLEX (SUB (n))->dimensions;
	}
      else
	{
	  rowdim = DEFLEX (n)->dimensions;
	}
      if ((subs + trims) != rowdim)
	{
	  diagnostic (A_ERROR, p, "wrong number of indexers for M", n);
	  make_soid (y, SORT (x), MODE (ERROR), 0);
	}
      else
	{
	  if (subs > 0 && trims == 0)
	    {
	      ANNOTATION (NEXT (p)) = SLICE;
	      m = n;
	    }
	  else
	    {
	      ANNOTATION (NEXT (p)) = TRIMMER;
	      m = n;
	    }
	  while (subs > 0)
	    {
	      if (whether_ref)
		{
		  m = m->name;
		}
	      else
		{
		  if (WHETHER (m, FLEX_SYMBOL))
		    {
		      m = SUB (m);
		    }
		  m = SLICE (m);
		}
	      if (m == NULL)
		{
		  abend (m == NULL, "NULL mode in mode_check_slice", NULL);
		}
	      subs--;
	    }
/* A trim cannot be but deflexed
      make_soid (y, SORT (x), ANNOTATION (NEXT (p)) == TRIMMER && m->deflexed_mode != NULL ? m->deflexed_mode : m,0);
*/
	  make_soid (y, SORT (x), ANNOTATION (NEXT (p)) == TRIMMER && m->trim != NULL ? m->trim : m, 0);
	  return (SLICE);
	}
    }
  return (PRIMARY);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_selection                                        */

static void
mode_check_selection (NODE_T * p, SOID_T * x, SOID_T * y)
{
  SOID_T w, d;
  BOOL_T coerce;
  MOID_T *n, *str, *ori;
  PACK_T *t, *t_2;
  char *fs;
  NODE_T *secondary = SUB (NEXT (p));
  make_soid (&w, WEAK, NULL, 0);
  mode_check_secondary (secondary, &w, &d);
  n = ori = determine_unique_mode (&d, SAFE_DEFLEXING);
  coerce = A_TRUE;
  while (coerce)
    {
      if (WHETHER (n, STRUCT_SYMBOL))
	{
	  coerce = A_FALSE;
	  t = PACK (n);
	}
      else if (WHETHER (n, REF_SYMBOL) && (WHETHER (SUB (n), ROW_SYMBOL) || WHETHER (SUB (n), FLEX_SYMBOL)) && n->multiple_mode != NULL)
	{
	  coerce = A_FALSE;
	  t = PACK (n->multiple_mode);
	}
      else if ((WHETHER (n, ROW_SYMBOL) || WHETHER (n, FLEX_SYMBOL)) && n->multiple_mode != NULL)
	{
	  coerce = A_FALSE;
	  t = PACK (n->multiple_mode);
	}
      else if (WHETHER (n, REF_SYMBOL) && whether_name_struct (n))
	{
	  coerce = A_FALSE;
	  t = PACK (n->name);
	}
      else if (whether_deprefable (n))
	{
	  coerce = A_TRUE;
	  n = SUB (n);
	  t = NULL;
	}
      else
	{
	  coerce = A_FALSE;
	  t = NULL;
	}
    }
  if (t == NULL)
    {
      if (whether_mode_is_well (MOID (&d)))
	{
	  diagnostic (A_ERROR, secondary, "M A cannot yield a structured value", ori, ATTRIBUTE (secondary));
	}
      make_soid (y, SORT (x), MODE (ERROR), 0);
      return;
    }
  MOID (NEXT (p)) = n;
  fs = SYMBOL (SUB (p));
  str = n;
  while (WHETHER (str, REF_SYMBOL))
    {
      str = SUB (str);
    }
  if (WHETHER (str, FLEX_SYMBOL))
    {
      str = SUB (str);
    }
  if (WHETHER (str, ROW_SYMBOL))
    {
      str = SUB (str);
    }
  t_2 = PACK (str);
  while (t != NULL && t_2 != NULL)
    {
      if (t->text == fs)
	{
	  make_soid (y, SORT (x), MOID (t), 0);
	  MOID (p) = MOID (t);
	  PACK (SUB (p)) = t_2;
	  return;
	}
      t = NEXT (t);
      t_2 = NEXT (t_2);
    }
  make_soid (&d, NO_SORT, n, 0);
  diagnostic (A_ERROR, p, "mode M has no structured field Z", str, fs);
  make_soid (y, SORT (x), MODE (ERROR), 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_format_text                                        */

static void
mode_check_format_text (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      mode_check_format_text (SUB (p));
      if (WHETHER (p, FORMAT_PATTERN))
	{
	  SOID_T x, y;
	  make_soid (&x, STRONG, MODE (FORMAT), 0);
	  mode_check_enclosed (SUB (NEXT_SUB (p)), &x, &y);
	  if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	    }
	}
      else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
	{
	  SOID_T x, y;
	  make_soid (&x, STRONG, MODE (ROW_INT), 0);
	  mode_check_enclosed (SUB (NEXT_SUB (p)), &x, &y);
	  if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	    }
	}
      else if (WHETHER (p, DYNAMIC_REPLICATOR))
	{
	  SOID_T x, y;
	  make_soid (&x, STRONG, MODE (INT), 0);
	  mode_check_enclosed (SUB (NEXT_SUB (p)), &x, &y);
	  if (!whether_coercible_in_context (&y, &x, SAFE_DEFLEXING))
	    {
	      diagnostic (A_ERROR, p, CANNOT_COERCE_ERROR, MOID (&y), MOID (&x), STRONG);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_primary                                        */

static void
mode_check_primary (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, CALL))
	{
	  mode_check_call (SUB (p), x, y);
	  warn_for_voiding (p, x, y, CALL);
	}
      else if (WHETHER (p, SLICE))
	{
	  int attr = mode_check_slice (SUB (p), x, y);
	  ATTRIBUTE (p) = attr;
	  warn_for_voiding (p, x, y, attr);
	}
      else if (WHETHER (p, CAST))
	{
	  mode_check_cast (SUB (p), x, y);
	  warn_for_voiding (p, x, y, CAST);
	}
      else if (WHETHER (p, DENOTER))
	{
	  make_soid (y, SORT (x), MOID (SUB (p)), 0);
	  warn_for_voiding (p, x, y, DENOTER);
	}
      else if (WHETHER (p, IDENTIFIER))
	{
	  make_soid (y, SORT (x), MOID (p), 0);
	  warn_for_voiding (p, x, y, IDENTIFIER);
	}
      else if (WHETHER (p, ENCLOSED_CLAUSE))
	{
	  mode_check_enclosed (SUB (p), x, y);
	}
      else if (WHETHER (p, FORMAT_TEXT))
	{
	  mode_check_format_text (p);
	  make_soid (y, SORT (x), MODE (FORMAT), 0);
	  warn_for_voiding (p, x, y, FORMAT_TEXT);
	}
      else if (WHETHER (p, JUMP) || WHETHER (p, SKIP))
	{
	  make_soid (y, STRONG, MODE (HIP), 0);
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_secondary                                        */

static void
mode_check_secondary (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      if (WHETHER (p, PRIMARY))
	{
	  mode_check_primary (SUB (p), x, y);
	}
      else if (WHETHER (p, GENERATOR))
	{
	  mode_check_declarer (SUB (p));
	  make_soid (y, SORT (x), MOID (SUB (p)), 0);
	  warn_for_voiding (p, x, y, GENERATOR);
	}
      else if (WHETHER (p, SELECTION))
	{
	  mode_check_selection (SUB (p), x, y);
	  warn_for_voiding (p, x, y, SELECTION);
	}
      else if (WHETHER (p, JUMP) || WHETHER (p, SKIP))
	{
	  make_soid (y, STRONG, MODE (HIP), 0);
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mode_check_unit                                        */

static void
mode_check_unit (NODE_T * p, SOID_T * x, SOID_T * y)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case UNIT:
	  {
	    mode_check_unit (SUB (p), x, y);
	    break;
	  }
	case TERTIARY:
	  {
	    mode_check_tertiary (SUB (p), x, y);
	    break;
	  }
	case JUMP:
	case SKIP:
	  {
	    make_soid (y, STRONG, MODE (HIP), 0);
	    break;
	  }
	case ASSIGNATION:
	  {
	    mode_check_assignation (SUB (p), x, y);
	    break;
	  }
	case IDENTITY_RELATION:
	  {
	    mode_check_identity_relation (SUB (p), x, y);
	    warn_for_voiding (p, x, y, IDENTITY_RELATION);
	    break;
	  }
	case ROUTINE_TEXT:
	  {
	    mode_check_routine_text (SUB (p), y);
	    make_soid (y, SORT (x), MOID (p), 0);
	    warn_for_voiding (p, x, y, ROUTINE_TEXT);
	    break;
	  }
	case ASSERTION:
	  {
	    mode_check_assertion (SUB (p));
	    make_soid (y, STRONG, MODE (VOID), 0);
	    break;
	  }
	case AND_FUNCTION:
	  {
	    mode_check_bool_function (SUB (p), x, y);
	    warn_for_voiding (p, x, y, AND_FUNCTION);
	    break;
	  }
	case OR_FUNCTION:
	  {
	    mode_check_bool_function (SUB (p), x, y);
	    warn_for_voiding (p, x, y, OR_FUNCTION);
	    break;
	  }
	}
      MOID (p) = MOID (y);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_bounds                                        */

static void
coerce_bounds (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  SOID_T q;
	  make_soid (&q, MEEK, MODE (INT), 0);
	  coerce_unit (p, &q);
	}
      else
	{
	  coerce_bounds (SUB (p));
	}
      coerce_bounds (NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_declarer                                        */

static void
coerce_declarer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, BOUNDS))
	{
	  coerce_bounds (SUB (p));
	}
      else
	{
	  coerce_declarer (SUB (p));
	}
      coerce_declarer (NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_identity_declaration                                        */

static void
coerce_identity_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    coerce_declarer (SUB (p));
	    coerce_identity_declaration (NEXT (p));
	    break;
	  }
	case DEFINING_IDENTIFIER:
	  {
	    SOID_T q;
	    make_soid (&q, STRONG, MOID (p), 0);
	    coerce_unit (NEXT (NEXT (p)), &q);
	    break;
	  }
	default:
	  {
	    coerce_identity_declaration (SUB (p));
	    coerce_identity_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_variable_declaration                                        */

static void
coerce_variable_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    coerce_declarer (SUB (p));
	    coerce_variable_declaration (NEXT (p));
	    break;
	  }
	case DEFINING_IDENTIFIER:
	  {
	    if (whether (p, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0))
	      {
		SOID_T q;
		make_soid (&q, STRONG, SUB (MOID (p)), 0);
		coerce_unit (NEXT (NEXT (p)), &q);
		break;
	      }
	  }
	default:
	  {
	    coerce_variable_declaration (SUB (p));
	    coerce_variable_declaration (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_routine_text                                        */

static void
coerce_routine_text (NODE_T * p)
{
  SOID_T w;
  if (WHETHER (p, PARAMETER_PACK))
    {
      p = NEXT (p);
    }
  make_soid (&w, STRONG, MOID (p), 0);
  coerce_unit (NEXT (NEXT (p)), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_proc_declaration                                        */

static void
coerce_proc_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, ROUTINE_TEXT))
	{
	  coerce_routine_text (SUB (p));
	}
      else
	{
	  coerce_proc_declaration (SUB (p));
	  coerce_proc_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_op_declaration                                        */

static void
coerce_op_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, DEFINING_OPERATOR))
	{
	  SOID_T q;
	  make_soid (&q, STRONG, MOID (p), 0);
	  coerce_unit (NEXT (NEXT (p)), &q);
	}
      else
	{
	  coerce_op_declaration (SUB (p));
	  coerce_op_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_brief_op_declaration                                        */

static void
coerce_brief_op_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, DEFINING_OPERATOR))
	{
	  coerce_routine_text (SUB (NEXT (NEXT (p))));
	}
      else
	{
	  coerce_brief_op_declaration (SUB (p));
	  coerce_brief_op_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_declaration_list                                        */

static void
coerce_declaration_list (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case IDENTITY_DECLARATION:
	  {
	    coerce_identity_declaration (SUB (p));
	    break;
	  }
	case VARIABLE_DECLARATION:
	  {
	    coerce_variable_declaration (SUB (p));
	    break;
	  }
	case MODE_DECLARATION:
	  {
	    coerce_declarer (SUB (p));
	    break;
	  }
	case PROCEDURE_DECLARATION:
	case PROCEDURE_VARIABLE_DECLARATION:
	  {
	    coerce_proc_declaration (SUB (p));
	    break;
	  }
	case BRIEF_OPERATOR_DECLARATION:
	  {
	    coerce_brief_op_declaration (SUB (p));
	    break;
	  }
	case OPERATOR_DECLARATION:
	  {
	    coerce_op_declaration (SUB (p));
	    break;
	  }
	default:
	  {
	    coerce_declaration_list (SUB (p));
	    coerce_declaration_list (NEXT (p));
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_serial                                        */

static void
coerce_serial (NODE_T * p, SOID_T * q, int k)
{
  if (p != NULL)
    {
      if (WHETHER (p, INITIALISER_SERIES))
	{
	  coerce_serial (SUB (p), q, 0);
	  coerce_serial (NEXT (p), q, k);
	}
      else if (WHETHER (p, DECLARATION_LIST))
	{
	  coerce_declaration_list (SUB (p));
	}
      else if (WHETHER (p, LABEL) || WHETHER (p, SEMI_SYMBOL) || WHETHER (p, EXIT_SYMBOL))
	{
	  coerce_serial (NEXT (p), q, k);
	}
      else if (WHETHER (p, SERIAL_CLAUSE) || WHETHER (p, ENQUIRY_CLAUSE))
	{
	  NODE_T *z = NEXT (p);
	  if (z != NULL)
	    {
	      if (WHETHER (z, EXIT_SYMBOL) || WHETHER (z, END_SYMBOL) || WHETHER (z, CLOSE_SYMBOL) || WHETHER (z, OCCA_SYMBOL))
		{
		  coerce_serial (SUB (p), q, 1);
		}
	      else
		{
		  coerce_serial (SUB (p), q, 0);
		}
	    }
	  else
	    {
	      coerce_serial (SUB (p), q, 1);
	    }
	  coerce_serial (NEXT (p), q, k);
	}
      else if (WHETHER (p, LABELED_UNIT))
	{
	  coerce_serial (SUB (p), q, k);
	}
      else if (WHETHER (p, UNIT))
	{
	  if (k)
	    {
	      coerce_unit (p, q);
	    }
	  else
	    {
	      SOID_T strongvoid;
	      make_soid (&strongvoid, STRONG, MODE (VOID), 0);
	      coerce_unit (p, &strongvoid);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_closed                                         */

static void
coerce_closed (NODE_T * p, SOID_T * q)
{
  if (WHETHER (p, SERIAL_CLAUSE))
    {
      coerce_serial (p, q, 1);
    }
  else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL))
    {
      coerce_closed (NEXT (p), q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_export_clause                                        */

static void
coerce_export_clause (NODE_T * p, SOID_T * q)
{
  if (WHETHER (p, INITIALISER_SERIES))
    {
      coerce_declaration_list (SUB (p));
    }
  else
    {
      coerce_export_clause (NEXT (p), q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_conditional                                        */

static void
coerce_conditional (NODE_T * p, SOID_T * q)
{
  SOID_T w;
  make_soid (&w, MEEK, MODE (BOOL), 0);
  coerce_serial (NEXT_SUB (p), &w, 1);
  p = NEXT (p);
  coerce_serial (NEXT_SUB (p), q, 1);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, ELSE_PART) || WHETHER (p, CHOICE))
	{
	  coerce_serial (NEXT_SUB (p), q, 1);
	}
      else if (WHETHER (p, ELIF_PART) || WHETHER (p, BRIEF_ELIF_IF_PART))
	{
	  coerce_conditional (SUB (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_unit_list                                        */

static void
coerce_unit_list (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT_LIST))
	{
	  coerce_unit_list (SUB (p), q);
	  coerce_unit_list (NEXT (p), q);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  coerce_unit_list (NEXT (p), q);
	}
      else if (WHETHER (p, UNIT))
	{
	  coerce_unit (p, q);
	  coerce_unit_list (NEXT (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_int_case                                        */

static void
coerce_int_case (NODE_T * p, SOID_T * q)
{
  SOID_T w;
  make_soid (&w, MEEK, MODE (INT), 0);
  coerce_serial (NEXT_SUB (p), &w, 1);
  p = NEXT (p);
  coerce_unit_list (NEXT_SUB (p), q);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, OUT_PART) || WHETHER (p, CHOICE))
	{
	  coerce_serial (NEXT_SUB (p), q, 1);
	}
      else if (WHETHER (p, INTEGER_OUT_PART) || WHETHER (p, BRIEF_INTEGER_OUSE_PART))
	{
	  coerce_int_case (SUB (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_spec_unit_list                                        */

static void
coerce_spec_unit_list (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      if (WHETHER (p, SPECIFIED_UNIT_LIST) || WHETHER (p, SPECIFIED_UNIT))
	{
	  coerce_spec_unit_list (SUB (p), q);
	  coerce_spec_unit_list (NEXT (p), q);
	}
      else if (WHETHER (p, COLON_SYMBOL) || WHETHER (p, COMMA_SYMBOL) || WHETHER (p, SPECIFIER))
	{
	  coerce_spec_unit_list (NEXT (p), q);
	}
      else if (WHETHER (p, UNIT))
	{
	  coerce_unit (p, q);
	  coerce_spec_unit_list (NEXT (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_united_case                                        */

static void
coerce_united_case (NODE_T * p, SOID_T * q)
{
  SOID_T w;
  make_soid (&w, MEEK, MOID (SUB (p)), 0);
  coerce_serial (NEXT_SUB (p), &w, 1);
  p = NEXT (p);
  coerce_spec_unit_list (NEXT_SUB (p), q);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, OUT_PART) || WHETHER (p, CHOICE))
	{
	  coerce_serial (NEXT_SUB (p), q, 1);
	}
      else if (WHETHER (p, UNITED_OUSE_PART) || WHETHER (p, BRIEF_UNITED_OUSE_PART))
	{
	  coerce_united_case (SUB (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_loop                                        */

static void
coerce_loop (NODE_T * p)
{
  if (WHETHER (p, FOR_PART))
    {
      coerce_loop (NEXT (p));
    }
  else if (WHETHER (p, FROM_PART) || WHETHER (p, BY_PART) || WHETHER (p, TO_PART))
    {
      SOID_T w;
      make_soid (&w, MEEK, MODE (INT), 0);
      coerce_unit (NEXT_SUB (p), &w);
      coerce_loop (NEXT (p));
    }
  else if (WHETHER (p, WHILE_PART))
    {
      SOID_T w;
      make_soid (&w, MEEK, MODE (BOOL), 0);
      coerce_serial (NEXT_SUB (p), &w, 1);
      coerce_loop (NEXT (p));
    }
  else if (WHETHER (p, DO_PART) || WHETHER (p, ALT_DO_PART))
    {
      SOID_T w;
      make_soid (&w, STRONG, MODE (VOID), 0);
      coerce_serial (NEXT_SUB (p), &w, 1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_struct_display                                        */

static void
coerce_struct_display (PACK_T ** r, NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT_LIST))
	{
	  coerce_struct_display (r, SUB (p));
	  coerce_struct_display (r, NEXT (p));
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  coerce_struct_display (r, NEXT (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T s;
	  make_soid (&s, STRONG, MOID (*r), 0);
	  coerce_unit (p, &s);
	  (*r) = NEXT (*r);
	  coerce_struct_display (r, NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_collateral                                        */

static void
coerce_collateral (NODE_T * p, SOID_T * q)
{
  if (!(whether (p, BEGIN_SYMBOL, END_SYMBOL, 0) || whether (p, OPEN_SYMBOL, CLOSE_SYMBOL, 0)))
    {
      if (WHETHER (MOID (q), STRUCT_SYMBOL))
	{
	  PACK_T *t = PACK (MOID (q));
	  coerce_struct_display (&t, p);
	}
      else if (WHETHER (MOID (q), FLEX_SYMBOL))
	{
	  SOID_T w;
	  make_soid (&w, STRONG, SLICE (SUB (MOID (q))), 0);
	  coerce_unit_list (p, &w);
	}
      else if (WHETHER (MOID (q), ROW_SYMBOL))
	{
	  SOID_T w;
	  make_soid (&w, STRONG, SLICE (MOID (q)), 0);
	  coerce_unit_list (p, &w);
	}
      else
	{
/* if (MOID (q) != MODE (VOID)) */
	  coerce_unit_list (p, q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_enclosed                                        */

void
coerce_enclosed (NODE_T * p, SOID_T * q)
{
  if (WHETHER (p, ENCLOSED_CLAUSE))
    {
      coerce_enclosed (SUB (p), q);
    }
  else if (WHETHER (p, CLOSED_CLAUSE))
    {
      coerce_closed (SUB (p), q);
    }
  else if (WHETHER (p, COLLATERAL_CLAUSE))
    {
      coerce_collateral (SUB (p), q);
    }
  else if (WHETHER (p, PARALLEL_CLAUSE))
    {
      coerce_collateral (SUB (NEXT_SUB (p)), q);
    }
  else if (WHETHER (p, CONDITIONAL_CLAUSE))
    {
      coerce_conditional (SUB (p), q);
    }
  else if (WHETHER (p, INTEGER_CASE_CLAUSE))
    {
      coerce_int_case (SUB (p), q);
    }
  else if (WHETHER (p, UNITED_CASE_CLAUSE))
    {
      coerce_united_case (SUB (p), q);
    }
  else if (WHETHER (p, LOOP_CLAUSE))
    {
      coerce_loop (SUB (p));
    }
  else if (WHETHER (p, EXPORT_CLAUSE))
    {
      coerce_export_clause (SUB (p), q);
    }
  MOID (p) = depref_rows (MOID (p), MOID (q));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_monad_moid                                        */

static MOID_T *
get_monad_moid (NODE_T * p)
{
  if (TAX (p) != NULL && TAX (p) != error_tag)
    {
      MOID (p) = MOID (TAX (p));
      return (MOID (PACK (MOID (p))));
    }
  else
    {
      return (MODE (ERROR));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_monad_oper                                        */

static void
coerce_monad_oper (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      SOID_T z;
      make_soid (&z, FIRM, MOID (PACK (MOID (TAX (p)))), 0);
      insert_coercions (NEXT (p), MOID (q), &z);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_monad_formula                                        */

static void
coerce_monad_formula (NODE_T * p)
{
  SOID_T e;
  make_soid (&e, STRONG, get_monad_moid (p), 0);
  coerce_operand (NEXT (p), &e);
  MOID (NEXT (p)) = MOID (SUB (NEXT (p)));
  coerce_monad_oper (p, &e);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_operand                                        */

static void
coerce_operand (NODE_T * p, SOID_T * q)
{
  if (WHETHER (p, MONADIC_FORMULA))
    {
      coerce_monad_formula (SUB (p));
      if (MOID (p) != MOID (q))
	{
	  make_sub (p, p, FORMULA);
	  insert_coercions (p, MOID (p), q);
	  make_sub (p, p, TERTIARY);
	}
      MOID (p) = depref_rows (MOID (p), MOID (q));
    }
  else if (WHETHER (p, FORMULA))
    {
      coerce_formula (SUB (p), q);
      insert_coercions (p, MOID (p), q);
      MOID (p) = depref_rows (MOID (p), MOID (q));
    }
  else if (WHETHER (p, SECONDARY))
    {
      coerce_secondary (SUB (p), q);
      MOID (p) = MOID (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_formula                                        */

static void
coerce_formula (NODE_T * p, SOID_T * q)
{
  (void) q;
  if (WHETHER (p, MONADIC_FORMULA) && NEXT (p) == NULL)
    {
      coerce_monad_formula (SUB (p));
    }
  else
    {
      if (TAX (NEXT (p)) != NULL && TAX (NEXT (p)) != error_tag)
	{
	  SOID_T s;
	  NODE_T *op = NEXT (p), *q = NEXT (NEXT (p));
	  MOID_T *w = MOID (op);
	  MOID_T *u = MOID (PACK (w)), *v = MOID (NEXT (PACK (w)));
	  make_soid (&s, STRONG, u, 0);
	  coerce_operand (p, &s);
	  make_soid (&s, STRONG, v, 0);
	  coerce_operand (q, &s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_tertiary                                         */

static void
coerce_tertiary (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      if (WHETHER (p, SECONDARY))
	{
	  coerce_secondary (SUB (p), q);
	  MOID (p) = MOID (SUB (p));
	}
      else if (WHETHER (p, NIHIL))
	{
	  if (ATTRIBUTE (MOID (q)) != REF_SYMBOL && MOID (q) != MODE (VOID))
	    {
	      diagnostic (A_ERROR, p, "context does not require a name");
	    }
	  MOID (p) = depref_rows (MOID (p), MOID (q));
	}
      else if (WHETHER (p, FORMULA))
	{
	  coerce_formula (SUB (p), q);
	  insert_coercions (p, MOID (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_assignation                                        */

static void
coerce_assignation (NODE_T * p)
{
  SOID_T w;
  make_soid (&w, SOFT, MOID (p), 0);
  coerce_tertiary (SUB (p), &w);
  make_soid (&w, STRONG, SUB (MOID (p)), 0);
  coerce_unit (NEXT (NEXT (p)), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_relation                                        */

static void
coerce_relation (NODE_T * p)
{
  SOID_T w;
  make_soid (&w, STRONG, MOID (p), 0);
  coerce_tertiary (SUB (p), &w);
  make_soid (&w, STRONG, MOID (NEXT (NEXT (p))), 0);
  coerce_tertiary (SUB (NEXT (NEXT (p))), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_bool_function                                        */

static void
coerce_bool_function (NODE_T * p)
{
  SOID_T w;
  make_soid (&w, STRONG, MODE (BOOL), 0);
  coerce_tertiary (SUB (p), &w);
  coerce_tertiary (SUB (NEXT (NEXT (p))), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_assertion                                        */

static void
coerce_assertion (NODE_T * p)
{
  SOID_T w;
  make_soid (&w, MEEK, MODE (BOOL), 0);
  coerce_enclosed (SUB_NEXT (p), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_unit                                        */

static void
coerce_unit (NODE_T * p, SOID_T * q)
{
  if (WHETHER (p, UNIT))
    {
      coerce_unit (SUB (p), q);
      MOID (p) = MOID (SUB (p));
    }
  else if (WHETHER (p, TERTIARY))
    {
      coerce_tertiary (SUB (p), q);
      MOID (p) = MOID (SUB (p));
    }
  else if (WHETHER (p, JUMP))
    {
      if (MOID (q) == MODE (PROC_VOID))
	{
	  make_sub (p, p, PROCEDURING);
	}
      MOID (p) = depref_rows (MOID (p), MOID (q));
    }
  else if (WHETHER (p, SKIP))
    {
      MOID (p) = depref_rows (MOID (p), MOID (q));
    }
  else if (WHETHER (p, ASSIGNATION))
    {
      coerce_assignation (SUB (p));
      insert_coercions (p, MOID (p), q);
      MOID (p) = depref_rows (MOID (p), MOID (q));
    }
  else if (WHETHER (p, IDENTITY_RELATION))
    {
      coerce_relation (SUB (p));
      insert_coercions (p, MOID (p), q);
    }
  else if (WHETHER (p, AND_FUNCTION))
    {
      coerce_bool_function (SUB (p));
      insert_coercions (p, MOID (p), q);
    }
  else if (WHETHER (p, OR_FUNCTION))
    {
      coerce_bool_function (SUB (p));
      insert_coercions (p, MOID (p), q);
    }
  else if (WHETHER (p, ROUTINE_TEXT))
    {
      coerce_routine_text (SUB (p));
      insert_coercions (p, MOID (p), q);
    }
  else if (WHETHER (p, ASSERTION))
    {
      coerce_assertion (SUB (p));
      insert_coercions (p, MOID (p), q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_selection                                         */

static void
coerce_selection (NODE_T * p)
{
  SOID_T w;
  make_soid (&w, /* WEAK */ STRONG, MOID (NEXT (p)), 0);
  coerce_secondary (SUB (NEXT (p)), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_secondary                                        */

static void
coerce_secondary (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      if (WHETHER (p, PRIMARY))
	{
	  coerce_primary (SUB (p), q);
	  if (ATTRIBUTE (p) != DEREFERENCING)
	    {
	      MOID (p) = MOID (SUB (p));
	    }
	}
      else if (WHETHER (p, SELECTION))
	{
	  coerce_selection (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, GENERATOR))
	{
	  coerce_declarer (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_cast                                        */

static void
coerce_cast (NODE_T * p)
{
  SOID_T w;
  coerce_declarer (p);
  make_soid (&w, STRONG, MOID (p), 0);
  coerce_enclosed (SUB_NEXT (p), &w);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_argument_list                                        */

static void
coerce_argument_list (PACK_T ** r, NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, ARGUMENT_LIST))
	{
	  coerce_argument_list (r, SUB (p));
	  coerce_argument_list (r, NEXT (p));
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  coerce_argument_list (r, NEXT (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  SOID_T s;
	  make_soid (&s, STRONG, MOID (*r), 0);
	  coerce_unit (p, &s);
	  *r = NEXT (*r);
	  coerce_argument_list (r, NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_call                                        */

static void
coerce_call (NODE_T * p)
{
  MOID_T *proc = MOID (p);
  SOID_T w;
  PACK_T *t;
  make_soid (&w, MEEK, proc, 0);
  coerce_primary (SUB (p), &w);
  p = NEXT (p);
  t = PACK (proc);
  coerce_argument_list (&t, SUB (p));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_meek_int                                        */

static void
coerce_meek_int (NODE_T * p)
{
  SOID_T x;
  make_soid (&x, MEEK, MODE (INT), 0);
  coerce_unit (p, &x);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_trimmer                                        */

static void
coerce_trimmer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  coerce_meek_int (p);
	  coerce_trimmer (NEXT (p));
	}
      else
	{
	  coerce_trimmer (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_indexer                                        */

static void
coerce_indexer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, TRIMMER))
	{
	  coerce_trimmer (SUB (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  coerce_meek_int (p);
	}
      else
	{
	  coerce_indexer (SUB (p));
	  coerce_indexer (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_slice                                        */

static void
coerce_slice (NODE_T * p)
{
  SOID_T w;
  MOID_T *row;
  row = MOID (p);
  make_soid (&w, /* WEAK */ STRONG, row, 0);
  coerce_primary (SUB (p), &w);
  coerce_indexer (SUB (NEXT (p)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_format_text                                        */

static void
coerce_format_text (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      coerce_format_text (SUB (p));
      if (WHETHER (p, FORMAT_PATTERN))
	{
	  SOID_T x;
	  make_soid (&x, STRONG, MODE (FORMAT), 0);
	  coerce_enclosed (SUB (NEXT_SUB (p)), &x);
	}
      else if (WHETHER (p, GENERAL_PATTERN) && NEXT_SUB (p) != NULL)
	{
	  SOID_T x;
	  make_soid (&x, STRONG, MODE (ROW_INT), 0);
	  coerce_enclosed (SUB (NEXT_SUB (p)), &x);
	}
      else if (WHETHER (p, DYNAMIC_REPLICATOR))
	{
	  SOID_T x;
	  make_soid (&x, STRONG, MODE (INT), 0);
	  coerce_enclosed (SUB (NEXT_SUB (p)), &x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
coerce_primary                                         */

static void
coerce_primary (NODE_T * p, SOID_T * q)
{
  if (p != NULL)
    {
      if (WHETHER (p, CALL))
	{
	  coerce_call (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, SLICE))
	{
	  coerce_slice (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, CAST))
	{
	  coerce_cast (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, DENOTER) || WHETHER (p, IDENTIFIER))
	{
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, FORMAT_TEXT))
	{
	  coerce_format_text (SUB (p));
	  insert_coercions (p, MOID (p), q);
	}
      else if (WHETHER (p, ENCLOSED_CLAUSE))
	{
	  coerce_enclosed (p, q);
	}
    }
}
