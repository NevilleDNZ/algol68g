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

#include "algol68g.h"
#include "genie.h"
#include "mp.h"

enum
{ PUT_IN_THIS_LEVEL, PUT_IN_PARENT_LEVEL };

static MOID_T *get_mode_from_declarer (NODE_T *, int);
BOOL_T check_yin_yang (NODE_T *, MOID_T *, BOOL_T, BOOL_T);
static void make_deflexed_pack (PACK_T *, PACK_T **, MOID_T **);
static void moid_to_string_2 (char *, MOID_T *, int);

MOID_LIST_T *top_moid_list, *old_moid_list = NULL;
static int max_simplout_size;
static POSTULATE_T *postulates;

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_mode: add mode "sub" to chain "z".                                        */

MOID_T *
add_mode (MOID_T ** z, int att, int dimensions, NODE_T * node, MOID_T * sub, PACK_T * pack)
{
  MOID_T *x = new_moid ();
  x->in_standard_environ = (z == &(stand_env->moids));
  x->use = A_FALSE;
  x->size = 0;
  NUMBER (x) = mode_count++;
  ATTRIBUTE (x) = att;
  DIMENSION (x) = dimensions;
  NODE (x) = node;
  x->well_formed = A_TRUE;
  x->has_rows = (att == ROW_SYMBOL);
  SUB (x) = sub;
  PACK (x) = pack;
  NEXT (x) = *z;
  EQUIVALENT (x) = NULL;
  SLICE (x) = NULL;
  DEFLEXED (x) = NULL;
  NAME (x) = NULL;
  MULTIPLE (x) = NULL;
  TRIM (x) = NULL;
/* Link to chain and exit */
  *z = x;
  return (x);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_row: add row and its slices to chain, recursively.                        */

static MOID_T *
add_row (MOID_T ** p, int k, MOID_T * f, NODE_T * n)
{
  add_mode (p, ROW_SYMBOL, k, n, f, NULL);
  if (k > 1)
    {
      SLICE (*p) = add_row (&NEXT (*p), k - 1, f, n);
    }
  else
    {
      SLICE (*p) = f;
    }
  return (*p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_moid_list.                                                               */

void
init_moid_list ()
{
  top_moid_list = NULL;
  old_moid_list = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_moid_list.                                                              */

void
reset_moid_list ()
{
  old_moid_list = top_moid_list;
  top_moid_list = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_single_moid_to_list.                                                      */

void
add_single_moid_to_list (MOID_LIST_T ** p, MOID_T * q, SYMBOL_TABLE_T * c)
{
  MOID_LIST_T *m;
  if (old_moid_list == NULL)
    {
      m = (MOID_LIST_T *) get_fixed_heap_space (SIZE_OF (MOID_LIST_T));
    }
  else
    {
      m = old_moid_list;
      old_moid_list = NEXT (old_moid_list);
    }
  m->coming_from_level = c;
  MOID (m) = q;
  NEXT (m) = *p;
  *p = m;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_moid_list.                                                                */

void
add_moid_list (MOID_LIST_T ** p, SYMBOL_TABLE_T * c)
{
  if (c != NULL)
    {
      MOID_T *q;
      for (q = c->moids; q != NULL; q = NEXT (q))
	{
	  add_single_moid_to_list (p, q, c);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_moid_moid_list.                                                           */

void
add_moid_moid_list (NODE_T * p, MOID_LIST_T ** q)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL)
	{
	  add_moid_moid_list (SUB (p), q);
	  if (whether_new_lexical_level (p))
	    {
	      add_moid_list (q, SYMBOL_TABLE (SUB (p)));
	    }
	}
    }
}
/*-------1---------2---------3---------4---------5---------6---------7---------+
count_pack_members: count moids in a pack.                                    */

int
count_pack_members (PACK_T * u)
{
  int k = 0;
  for (; u != NULL; u = NEXT (u))
    {
      k++;
    }
  return (k);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_mode_to_pack: add a moid to a pack, maybe with a (field) name.            */

void
add_mode_to_pack (PACK_T ** p, MOID_T * m, char *text, NODE_T * node)
{
  PACK_T *z = new_pack ();
  MOID (z) = m;
  TEXT (z) = text;
  NODE (z) = node;
  NEXT (z) = *p;
  PREVIOUS (z) = NULL;
  if (NEXT (z) != NULL)
    {
      PREVIOUS (NEXT (z)) = z;
    }
/* Link in chain */
  *p = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_formal_bounds: count formal bounds in declarer in tree.                 */

static int
count_formal_bounds (NODE_T * p)
{
  if (p == NULL)
    {
      return (0);
    }
  else
    {
      if (WHETHER (p, COMMA_SYMBOL))
	{
	  return (1);
	}
      else
	{
	  return (count_formal_bounds (NEXT (p)) + count_formal_bounds (SUB (p)));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_bounds: count bounds in declarer in tree.                               */

static int
count_bounds (NODE_T * p)
{
  if (p == NULL)
    {
      return (0);
    }
  else
    {
      if (WHETHER (p, BOUND))
	{
	  return (1 + count_bounds (NEXT (p)));
	}
      else
	{
	  return (count_bounds (NEXT (p)) + count_bounds (SUB (p)));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_sizety : count number of SHORTs or LONGs.                               */

static int
count_sizety (NODE_T * p)
{
  if (p == NULL)
    {
      return (0);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case LONGETY:
	  {
	    return (count_sizety (SUB (p)) + count_sizety (NEXT (p)));
	  }
	case SHORTETY:
	  {
	    return (count_sizety (SUB (p)) + count_sizety (NEXT (p)));
	  }
	case LONG_SYMBOL:
	  {
	    return (1);
	  }
	case SHORT_SYMBOL:
	  {
	    return (-1);
	  }
	default:
	  {
	    return (0);
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Routines to collect MOIDs from the program text.                              */

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_standard_moid: collect standard mode.                           */

static MOID_T *
get_mode_from_standard_moid (int sizety, NODE_T * indicant, BOOL_T supported_precision)
{
  MOID_T *p = stand_env->moids, *q = NULL;
  for (; p != NULL && q == NULL; p = NEXT (p))
    {
      if (WHETHER (p, STANDARD) && DIMENSION (p) == sizety && SYMBOL (NODE (p)) == SYMBOL (indicant))
	{
	  q = p;
	}
    }
  if (q != NULL)
    {
      if (supported_precision)
	{
	  return (q);
	}
      else
	{
	  diagnostic (A_WARNING, indicant, PRECISION_NOT_IMPLEMENTED, q);
	  return (q);
	}
    }
  else
    {
      if (sizety < 0)
	{
	  return (get_mode_from_standard_moid (sizety + 1, indicant, A_FALSE));
	}
      else if (sizety > 0)
	{
	  return (get_mode_from_standard_moid (sizety - 1, indicant, A_FALSE));
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_struct_field: collect mode from STRUCT field.                   */

static void
get_mode_from_struct_field (NODE_T * p, PACK_T ** u, MOID_T ** m)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case IDENTIFIER:
	  {
	    ATTRIBUTE (p) = FIELD_IDENTIFIER;
	    add_mode_to_pack (u, NULL, SYMBOL (p), p);
	    break;
	  }
	case DECLARER:
	  {
	    MOID_T *new_one = get_mode_from_declarer (p, PUT_IN_THIS_LEVEL);
	    PACK_T *t;
	    get_mode_from_struct_field (NEXT (p), u, m);
	    for (t = *u; t && MOID (t) == NULL; t = NEXT (t))
	      {
		MOID (t) = new_one;
		MOID (NODE (t)) = new_one;
	      }
	    break;
	  }
	default:
	  {
	    get_mode_from_struct_field (NEXT (p), u, m);
	    get_mode_from_struct_field (SUB (p), u, m);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_formal_pack: collect MODE from formal pack.                     */

static void
get_mode_from_formal_pack (NODE_T * p, PACK_T ** u, MOID_T ** m)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	  {
	    MOID_T *z;
	    get_mode_from_formal_pack (NEXT (p), u, m);
	    z = get_mode_from_declarer (p, PUT_IN_THIS_LEVEL);
	    add_mode_to_pack (u, z, NULL, p);
	    break;
	  }
	default:
	  {
	    get_mode_from_formal_pack (NEXT (p), u, m);
	    get_mode_from_formal_pack (SUB (p), u, m);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_formal_pack: collect MODE or VOID from formal UNION pack.       */

static void
get_mode_from_union_pack (NODE_T * p, PACK_T ** u, MOID_T ** m)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DECLARER:
	case VOID_SYMBOL:
	  {
	    MOID_T *z;
	    get_mode_from_union_pack (NEXT (p), u, m);
	    z = get_mode_from_declarer (p, PUT_IN_THIS_LEVEL);
	    add_mode_to_pack (u, z, NULL, p);
	    break;
	  }
	default:
	  {
	    get_mode_from_union_pack (NEXT (p), u, m);
	    get_mode_from_union_pack (SUB (p), u, m);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_routine_pack: collect mode from PROC, OP pack.                  */

static void
get_mode_from_routine_pack (NODE_T * p, PACK_T ** u, MOID_T ** m)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case IDENTIFIER:
	  {
	    add_mode_to_pack (u, NULL, NULL, p);
	    break;
	  }
	case DECLARER:
	  {
	    MOID_T *z = get_mode_from_declarer (p, PUT_IN_PARENT_LEVEL);
	    PACK_T *t = *u;
	    for (; t != NULL && MOID (t) == NULL; t = NEXT (t))
	      {
		MOID (t) = z;
		MOID (NODE (t)) = z;
	      }
	    add_mode_to_pack (u, z, NULL, p);
	    break;
	  }
	default:
	  {
	    get_mode_from_routine_pack (NEXT (p), u, m);
	    get_mode_from_routine_pack (SUB (p), u, m);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_declarer: collect MODE from DECLARER.                           */

static MOID_T *
get_mode_from_declarer (NODE_T * p, int put_where)
{
  if (p == NULL)
    {
      return (NULL);
    }
  else
    {
      if (WHETHER (p, DECLARER))
	{
	  if (MOID (p) != NULL)
	    {
	      return (MOID (p));
	    }
	  else
	    {
	      return (MOID (p) = get_mode_from_declarer (SUB (p), put_where));
	    }
	}
      else
	{
	  MOID_T **m = NULL;
	  if (put_where == PUT_IN_THIS_LEVEL)
	    {
	      m = &(SYMBOL_TABLE (p)->moids);
	    }
	  else if (put_where == PUT_IN_PARENT_LEVEL)
	    {
	      m = &(PREVIOUS (SYMBOL_TABLE (p))->moids);
	    }
	  if (WHETHER (p, VOID_SYMBOL))
	    {
	      MOID (p) = MODE (VOID);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, LONGETY))
	    {
	      if (whether (p, LONGETY, INDICANT, 0))
		{
		  int k = count_sizety (SUB (p));
		  MOID (p) = get_mode_from_standard_moid (k, NEXT (p), A_TRUE);
		  return (MOID (p));
		}
	      else
		{
		  return (NULL);
		}
	    }
	  else if (WHETHER (p, SHORTETY))
	    {
	      if (whether (p, SHORTETY, INDICANT, 0))
		{
		  int k = count_sizety (SUB (p));
		  MOID (p) = get_mode_from_standard_moid (k, NEXT (p), A_TRUE);
		  return (MOID (p));
		}
	      else
		{
		  return (NULL);
		}
	    }
	  else if (WHETHER (p, INDICANT))
	    {
	      MOID_T *q = get_mode_from_standard_moid (0, p, A_TRUE);
	      if (q != NULL)
		{
		  MOID (p) = q;
		}
	      else
		{
		  MOID (p) = add_mode (m, INDICANT, 0, p, NULL, NULL);
		}
	      return (MOID (p));
	    }
	  else if (WHETHER (p, REF_SYMBOL))
	    {
	      MOID_T *new_one = get_mode_from_declarer (NEXT (p), put_where);
	      MOID (p) = add_mode (m, REF_SYMBOL, 0, p, new_one, NULL);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, FLEX_SYMBOL))
	    {
	      MOID_T *new_one = get_mode_from_declarer (NEXT (p), put_where);
	      MOID (p) = add_mode (m, FLEX_SYMBOL, 0, NULL, new_one, NULL);
	      SLICE (MOID (p)) = SLICE (new_one);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, FORMAL_BOUNDS))
	    {
	      MOID_T *new_one = get_mode_from_declarer (NEXT (p), put_where);
	      MOID (p) = add_row (m, 1 + count_formal_bounds (SUB (p)), new_one, p);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, BOUNDS))
	    {
	      MOID_T *new_one = get_mode_from_declarer (NEXT (p), put_where);
	      MOID (p) = add_row (m, count_bounds (SUB (p)), new_one, p);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, STRUCT_SYMBOL))
	    {
	      PACK_T *u = NULL;
	      get_mode_from_struct_field (NEXT (p), &u, m);
	      MOID (p) = add_mode (m, STRUCT_SYMBOL, count_pack_members (u), p, NULL, u);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, UNION_SYMBOL))
	    {
	      PACK_T *u = NULL;
	      get_mode_from_union_pack (NEXT (p), &u, m);
	      MOID (p) = add_mode (m, UNION_SYMBOL, count_pack_members (u), p, NULL, u);
	      return (MOID (p));
	    }
	  else if (WHETHER (p, PROC_SYMBOL))
	    {
	      NODE_T *save = p;
	      PACK_T *u = NULL;
	      MOID_T *new_one;
	      if (WHETHER (NEXT (p), FORMAL_DECLARERS))
		{
		  get_mode_from_formal_pack (SUB (NEXT (p)), &u, m);
		  p = NEXT (p);
		}
	      new_one = get_mode_from_declarer (NEXT (p), put_where);
	      MOID (p) = add_mode (m, PROC_SYMBOL, count_pack_members (u), save, new_one, u);
	      return (MOID (p));
	    }
	  else
	    {
	      return (NULL);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_routine_text: collect MODEs from a routine-text header.         */

static MOID_T *
get_mode_from_routine_text (NODE_T * p)
{
  PACK_T *u = NULL;
  MOID_T **m, *n;
  NODE_T *q = p;
  m = &(PREVIOUS (SYMBOL_TABLE (p))->moids);
  if (WHETHER (p, PARAMETER_PACK))
    {
      get_mode_from_routine_pack (SUB (p), &u, m);
      p = NEXT (p);
    }
  n = get_mode_from_declarer (p, PUT_IN_PARENT_LEVEL);
  return (add_mode (m, PROC_SYMBOL, count_pack_members (u), q, n, u));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_operator: collect modes from operator-plan.                     */

static MOID_T *
get_mode_from_operator (NODE_T * p)
{
  PACK_T *u = NULL;
  MOID_T **m = &(SYMBOL_TABLE (p)->moids), *new_one;
  NODE_T *save = p;
  if (WHETHER (NEXT (p), FORMAL_DECLARERS))
    {
      get_mode_from_formal_pack (SUB (NEXT (p)), &u, m);
      p = NEXT (p);
    }
  new_one = get_mode_from_declarer (NEXT (p), PUT_IN_THIS_LEVEL);
  MOID (p) = add_mode (m, PROC_SYMBOL, count_pack_members (u), save, new_one, u);
  return (MOID (p));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_denoter: collect modes from denoters.                           */

static void
get_mode_from_denoter (NODE_T * p, int sizety)
{
  if (p != NULL)
    {
      if (WHETHER (p, ROW_CHAR_DENOTER))
	{
	  if (strlen (SYMBOL (p)) == 1)
	    {
	      MOID (p) = MODE (CHAR);
	    }
	  else
	    {
	      MOID (p) = MODE (ROW_CHAR);
	    }
	}
      else if (WHETHER (p, TRUE_SYMBOL) || WHETHER (p, FALSE_SYMBOL))
	{
	  MOID (p) = MODE (BOOL);
	}
      else if (WHETHER (p, INT_DENOTER))
	{
	  switch (sizety)
	    {
	    case 0:
	      {
		MOID (p) = MODE (INT);
		break;
	      }
	    case 1:
	      {
		MOID (p) = MODE (LONG_INT);
		break;
	      }
	    case 2:
	      {
		MOID (p) = MODE (LONGLONG_INT);
		break;
	      }
	    default:
	      {
		diagnostic (A_WARNING, p, PRECISION_NOT_IMPLEMENTED, MODE (INT));
		MOID (p) = sizety > 0 ? MODE (LONGLONG_INT) : MODE (INT);
		break;
	      }
	    }
	}
      else if (WHETHER (p, REAL_DENOTER))
	{
	  switch (sizety)
	    {
	    case 0:
	      {
		MOID (p) = MODE (REAL);
		break;
	      }
	    case 1:
	      {
		MOID (p) = MODE (LONG_REAL);
		break;
	      }
	    case 2:
	      {
		MOID (p) = MODE (LONGLONG_REAL);
		break;
	      }
	    default:
	      {
		diagnostic (A_WARNING, p, PRECISION_NOT_IMPLEMENTED, MODE (REAL));
		MOID (p) = sizety > 0 ? MODE (LONGLONG_REAL) : MODE (REAL);
		break;
	      }
	    }
	}
      else if (WHETHER (p, BITS_DENOTER))
	{
	  switch (sizety)
	    {
	    case 0:
	      {
		MOID (p) = MODE (BITS);
		break;
	      }
	    case 1:
	      {
		MOID (p) = MODE (LONG_BITS);
		break;
	      }
	    case 2:
	      {
		MOID (p) = MODE (LONGLONG_BITS);
		break;
	      }
	    default:
	      {
		diagnostic (A_WARNING, p, PRECISION_NOT_IMPLEMENTED, MODE (BITS));
		MOID (p) = MODE (BITS);
		break;
	      }
	    }
	}
      else if (WHETHER (p, LONGETY) || WHETHER (p, SHORTETY))
	{
	  get_mode_from_denoter (NEXT (p), count_sizety (SUB (p)));
	  MOID (p) = MOID (NEXT (p));
	}
      else if (WHETHER (p, EMPTY_SYMBOL))
	{
	  MOID (p) = MODE (VOID);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_modes: collect modes from the syntax tree.                      */

static void
get_mode_from_modes (NODE_T * p, int attribute)
{
  NODE_T *q = p;
  BOOL_T z = A_TRUE;
  for (; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, VOID_SYMBOL))
	{
	  MOID (q) = MODE (VOID);
	}
      else if (WHETHER (q, DECLARER))
	{
	  if (attribute != VARIABLE_DECLARATION)
	    {
	      MOID (q) = get_mode_from_declarer (q, PUT_IN_THIS_LEVEL);
	    }
	  else
	    {
	      MOID_T **m = &(SYMBOL_TABLE (q)->moids), *new_one = get_mode_from_declarer (q, PUT_IN_THIS_LEVEL);
	      MOID (q) = add_mode (m, REF_SYMBOL, 0, NULL, new_one, NULL);
	    }
	}
      else if (WHETHER (q, ROUTINE_TEXT))
	{
	  MOID (q) = get_mode_from_routine_text (SUB (q));
	}
      else if (WHETHER (q, OPERATOR_PLAN))
	{
	  MOID (q) = get_mode_from_operator (SUB (q));
	}
      else if (WHETHER (q, LOC_SYMBOL) || WHETHER (q, HEAP_SYMBOL))
	{
	  if (attribute == GENERATOR)
	    {
	      MOID_T **m = &(SYMBOL_TABLE (q)->moids), *new_one = get_mode_from_declarer (NEXT (q), PUT_IN_THIS_LEVEL);
	      MOID (NEXT (q)) = new_one;
	      MOID (q) = add_mode (m, REF_SYMBOL, 0, NULL, new_one, NULL);
	    }
	}
      else
	{
	  if (attribute == DENOTER)
	    {
	      get_mode_from_denoter (q, 0);
	      z = A_FALSE;
	    }
	}
    }
  if (z)
    {
      for (q = p; q != NULL; q = NEXT (q))
	{
	  if (SUB (q) != NULL)
	    {
	      get_mode_from_modes (SUB (q), ATTRIBUTE (q));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_proc_variables: collect modes from proc variables.              */

static void
get_mode_from_proc_variables (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  get_mode_from_proc_variables (SUB (p));
	  get_mode_from_proc_variables (NEXT (p));
	}
      else if (WHETHER (p, QUALIFIER) || WHETHER (p, PROC_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  get_mode_from_proc_variables (NEXT (p));
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  MOID_T **m = &(SYMBOL_TABLE (p)->moids), *new_one = MOID (NEXT (NEXT (p)));
	  MOID (p) = add_mode (m, REF_SYMBOL, 0, p, new_one, NULL);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_mode_from_proc_variable_declarations: 
  collect modes from proc variable declarations.                              */

static void
get_mode_from_proc_variable_declarations (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      get_mode_from_proc_variable_declarations (SUB (p));
      if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  get_mode_from_proc_variables (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Various routines to test modes.                                               */

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_flex_modes: FLEX must be followed by [] AMODE.                          */

static void
check_flex_modes (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, FLEX_SYMBOL) && ATTRIBUTE (MOID (NEXT (p))) != ROW_SYMBOL)
	{
	  diagnostic (A_ERROR, p, "only rows can be flexible", NULL);
	}
      check_flex_modes (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_void: whether a MODE shows VOID.                             */

static BOOL_T
whether_mode_has_void (MOID_T * m)
{
  if (m == MODE (VOID))
    {
      return (A_TRUE);
    }
  else if (whether_postulated_pair (top_postulate, m, NULL))
    {
      return (A_FALSE);
    }
  else
    {
      int z = ATTRIBUTE (m);
      make_postulate (&top_postulate, m, NULL);
      if (z == REF_SYMBOL || z == FLEX_SYMBOL || z == ROW_SYMBOL)
	{
	  return whether_mode_has_void (SUB (m));
	}
      else if (z == STRUCT_SYMBOL)
	{
	  PACK_T *p;
	  for (p = PACK (m); p != NULL; p = NEXT (p))
	    {
	      if (whether_mode_has_void (MOID (p)))
		{
		  return (A_TRUE);
		}
	    }
	  return (A_FALSE);
	}
      else if (z == UNION_SYMBOL)
	{
	  PACK_T *p;
	  for (p = PACK (m); p != NULL; p = NEXT (p))
	    {
	      if (MOID (p) != MODE (VOID) && whether_mode_has_void (MOID (p)))
		{
		  return (A_TRUE);
		}
	    }
	  return (A_FALSE);
	}
      else if (z == PROC_SYMBOL)
	{
	  PACK_T *p;
	  for (p = PACK (m); p != NULL; p = NEXT (p))
	    {
	      if (whether_mode_has_void (MOID (p)))
		{
		  return (A_TRUE);
		}
	    }
	  if (SUB (m) == MODE (VOID))
	    {
	      return (A_FALSE);
	    }
	  else
	    {
	      return (whether_mode_has_void (SUB (m)));
	    }
	}
      else
	{
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_relation_to_void: check for modes that are related to VOID.             */

static void
check_relation_to_void (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  MOID_T *m;
	  for (m = SYMBOL_TABLE (SUB (p))->moids; m != NULL; m = NEXT (m))
	    {
	      reset_postulates ();
	      if (NODE (m) != NULL && whether_mode_has_void (m))
		{
		  diagnostic (A_ERROR, NODE (m), "M is related to M", m, MODE (VOID));
		}
	    }
	}
      check_relation_to_void (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
absorb_union_pack: absorb UNION pack.                                         */

PACK_T *
absorb_union_pack (PACK_T * t, int *modifications)
{
  PACK_T *z = NULL;
  for (; t != NULL; t = NEXT (t))
    {
      if (WHETHER (MOID (t), UNION_SYMBOL))
	{
	  PACK_T *s;
	  (*modifications)++;
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
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
absorb_unions: absorb UNIONs in the program.

  UNION (A, UNION (B, C)) = UNION (A, B, C) or
  UNION (A, UNION (A, B)) = UNION (A, B).                                     */

static void
absorb_unions (NODE_T * p, int *modifications)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  MOID_T *m;
	  for (m = SYMBOL_TABLE (SUB (p))->moids; m != NULL; m = NEXT (m))
	    {
	      if (WHETHER (m, UNION_SYMBOL))
		{
		  PACK (m) = absorb_union_pack (PACK (m), modifications);
		}
	    }
	}
      absorb_unions (SUB (p), modifications);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
contract_union: contract this UNION.                                          */

void
contract_union (MOID_T * u, int *modifications)
{
  PACK_T *s = PACK (u);
  for (; s != NULL; s = NEXT (s))
    {
      PACK_T *t = s;
      while (t != NULL)
	{
	  if (NEXT (t) != NULL && MOID (NEXT (t)) == MOID (s))
	    {
	      (*modifications)++;
	      MOID (t) = MOID (t);
	      NEXT (t) = NEXT (NEXT (t));
	    }
	  else
	    {
	      t = NEXT (t);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
contract_unions: contract UNIONs in program.

  UNION (A, B, A) -> UNION (A, B).                                            */

static void
contract_unions (NODE_T * p, int *modifications)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  MOID_T *m;
	  for (m = SYMBOL_TABLE (SUB (p))->moids; m != NULL; m = NEXT (m))
	    {
	      if (WHETHER (m, UNION_SYMBOL) && EQUIVALENT (m) == NULL)
		{
		  contract_union (m, modifications);
		}
	    }
	}
      contract_unions (SUB (p), modifications);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
cyclic_declaration: whether a mode declaration refers to self.                */

static BOOL_T
cyclic_declaration (TAG_T * table, MOID_T * p)
{
  if (WHETHER (p, VOID_SYMBOL))
    {
      return (A_TRUE);
    }
  else if (WHETHER (p, INDICANT))
    {
      if (whether_postulated (top_postulate, p))
	{
	  return (A_TRUE);
	}
      else
	{
	  TAG_T *z = table;
	  while (z != NULL && (SYMBOL (NODE (z)) != SYMBOL (NODE (p))))
	    {
	      z = NEXT (z);
	    }
	  if (z == NULL)
	    {
	      return (A_FALSE);
	    }
	  else
	    {
	      make_postulate (&top_postulate, p, NULL);
	      return (cyclic_declaration (table, MOID (z)));
	    }
	}
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_cyclic_modes: check self-referring modes in the program.

MODE A = B, B = C, C = A.                                                     */

static void
check_cyclic_modes (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  TAG_T *z, *table;
	  table = SYMBOL_TABLE (SUB (p))->indicants;
	  for (z = table; z != NULL; z = NEXT (z))
	    {
	      reset_postulates ();
	      if (cyclic_declaration (table, MOID (z)))
		{
		  diagnostic (A_ERROR, NODE (z), "M specifies a cyclic mode", MOID (z));
		}
	    }
	}
      check_cyclic_modes (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_yin_yang_pack: whether pack is well-formed.                             */

static BOOL_T
check_yin_yang_pack (NODE_T * indi, PACK_T * s, BOOL_T yin, BOOL_T yang)
{
  BOOL_T good = A_TRUE;
  for (; s != NULL && good; s = NEXT (s))
    {
      good = good && check_yin_yang (indi, MOID (s), yin, yang);
    }
  return (good);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_yin_yang: whether mode is well-formed.                                  */

BOOL_T
check_yin_yang (NODE_T * def, MOID_T * dec, BOOL_T yin, BOOL_T yang)
{
  if (dec->well_formed == A_FALSE)
    {
      return (A_TRUE);
    }
  else
    {
      if (WHETHER (dec, VOID_SYMBOL))
	{
	  return (A_TRUE);
	}
      else if (WHETHER (dec, STANDARD))
	{
	  return (A_TRUE);
	}
      else if (WHETHER (dec, INDICANT))
	{
	  if (SYMBOL (def) == SYMBOL (NODE (dec)))
	    {
	      return (yin && yang);
	    }
	  else
	    {
	      TAG_T *s = SYMBOL_TABLE (def)->indicants;
	      BOOL_T z = A_TRUE;
	      while (s != NULL && z)
		{
		  if (SYMBOL (NODE (s)) == SYMBOL (NODE (dec)))
		    {
		      z = A_FALSE;
		    }
		  else
		    {
		      s = NEXT (s);
		    }
		}
	      return (s == NULL ? A_TRUE : check_yin_yang (def, MOID (s), yin, yang));
	    }
	}
      else if (WHETHER (dec, REF_SYMBOL))
	{
	  return (yang ? A_TRUE : check_yin_yang (def, SUB (dec), A_TRUE, yang));
	}
      else if (WHETHER (dec, FLEX_SYMBOL) || WHETHER (dec, ROW_SYMBOL))
	{
	  return (check_yin_yang (def, SUB (dec), yin, yang));
	}
      else if (WHETHER (dec, STRUCT_SYMBOL))
	{
	  return (yin ? A_TRUE : check_yin_yang_pack (def, PACK (dec), yin, A_TRUE));
	}
      else if (WHETHER (dec, UNION_SYMBOL))
	{
	  return (check_yin_yang_pack (def, PACK (dec), yin, yang));
	}
      else if (WHETHER (dec, PROC_SYMBOL))
	{
	  if (PACK (dec) != NULL)
	    {
	      return (A_TRUE);
	    }
	  else
	    {
	      return (yang ? A_TRUE : check_yin_yang (def, SUB (dec), A_TRUE, yang));
	    }
	}
      else
	{
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_well_formedness: check well-formedness of modes in the program.         */

static void
check_well_formedness (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      check_well_formedness (SUB (p));
      if (WHETHER (p, DEFINING_INDICANT))
	{
	  MOID_T *z = NULL;
	  if (NEXT (p) != NULL && NEXT (NEXT (p)) != NULL)
	    {
	      z = MOID (NEXT (NEXT (p)));
	    }
	  if (!check_yin_yang (p, z, A_FALSE, A_FALSE))
	    {
	      diagnostic (A_ERROR, p, "S is not a well formed mode");
	      z->well_formed = A_FALSE;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
After the initial version of the mode equivalencer was made to work (1993), I 
found: Algol Bulletin 30.3.3 C.H.A. Koster: On infinite modes, 86-89 [1969],
which essentially concurs with the algorithm on mode equivalence I wrote (and
which is still here). It is basic logic anyway: prove equivalence of things 
postulating their equivalence.                                                */

/*-------1---------2---------3---------4---------5---------6---------7---------+
packs_equivalent: whether packs are equivalent.                               */

static BOOL_T
packs_equivalent (PACK_T * s, PACK_T * t)
{
  while (s != NULL && t != NULL)
    {
      if (modes_equivalent (MOID (s), MOID (t)) && TEXT (s) == TEXT (t))
	{
	  s = NEXT (s);
	  t = NEXT (t);
	}
      else
	{
	  return (A_FALSE);
	}
    }
  return (s == NULL && t == NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
united_moids_equivalent: whether united moids are equivalent.                 */

static BOOL_T
united_moids_equivalent (PACK_T * s, PACK_T * t)
{
  BOOL_T z = A_TRUE;
  for (; s != NULL && z; s = NEXT (s))
    {
      PACK_T *q = t;
      BOOL_T f = A_FALSE;
      for (; q != NULL && !f; q = NEXT (q))
	{
	  f = modes_equivalent (MOID (s), MOID (q));
	}
      z = z && f;
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
modes_equivalent: whether modes are structurally equivalent.                  */

BOOL_T
modes_equivalent (MOID_T * a, MOID_T * b)
{
  if (a == NULL || b == NULL)
    {
      abend (A_TRUE, "NULL pointer in modes_equivalent", NULL);
    }
  if (a == b)
    {
      return (A_TRUE);
    }
  else if (ATTRIBUTE (a) != ATTRIBUTE (b))
    {
      return (A_FALSE);
    }
  else if (WHETHER (a, STANDARD) && WHETHER (b, STANDARD))
    {
      return (a == b);
    }
  else if (EQUIVALENT (a) == b || EQUIVALENT (b) == a)
    {
      return (A_TRUE);
    }
  else if (whether_postulated_pair (top_postulate, a, b) || whether_postulated_pair (top_postulate, b, a))
    {
      return (A_TRUE);
    }
  else if (WHETHER (a, INDICANT))
    {
      return (modes_equivalent (EQUIVALENT (a), EQUIVALENT (b)));
    }
  else
    {
      make_postulate (&top_postulate, a, b);
      if (WHETHER (a, REF_SYMBOL) || WHETHER (a, FLEX_SYMBOL))
	{
	  return modes_equivalent (SUB (a), SUB (b));
	}
      else if (WHETHER (a, ROW_SYMBOL))
	{
	  return (DIMENSION (a) == DIMENSION (b) && modes_equivalent (SUB (a), SUB (b)));
	}
      else if (WHETHER (a, STRUCT_SYMBOL))
	{
	  return (DIMENSION (a) == DIMENSION (b) && packs_equivalent (PACK (a), PACK (b)));
	}
      else if (WHETHER (a, UNION_SYMBOL))
	{
	  return (united_moids_equivalent (PACK (a), PACK (b)) && united_moids_equivalent (PACK (b), PACK (a)));
	}
      else if (WHETHER (a, PROC_SYMBOL))
	{
	  return (DIMENSION (a) == DIMENSION (b) && modes_equivalent (SUB (a), SUB (b)) && packs_equivalent (PACK (a), PACK (b)));
	}
      else if (WHETHER (a, SERIES_MODE))
	{
	  return (DIMENSION (a) == DIMENSION (b) && packs_equivalent (PACK (a), PACK (b)));
	}
      else if (WHETHER (a, STOWED_MODE))
	{
	  return (DIMENSION (a) == DIMENSION (b) && packs_equivalent (PACK (a), PACK (b)));
	}
      else
	{
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_equivalent_moids: whether modes are structurally equivalent.
Proof that two modes are equivalent under assumption that they are.           */

static BOOL_T
check_equivalent_moids (MOID_T * p, MOID_T * q)
{
  POSTULATE_T *save = top_postulate;
  BOOL_T z;
/* Optimise a bit since most will be comparing PROCs in standenv. */
  if (ATTRIBUTE (p) == ATTRIBUTE (q))
    {
      if (WHETHER (p, PROC_SYMBOL))
	{
	  z = (ATTRIBUTE (SUB (p)) == ATTRIBUTE (SUB (q)) && DIMENSION (p) == DIMENSION (q)) ? modes_equivalent (p, q) : A_FALSE;
	}
      else
	{
	  z = modes_equivalent (p, q);
	}
    }
  else
    {
      z = A_FALSE;
    }
  if (z)
    {
      if (q->in_standard_environ && p->in_standard_environ)
	{
	  EQUIVALENT (p) = q;
	}
      else
	{
	  EQUIVALENT (q) = p;
	}
    }
  top_postulate = save;
/* give back core
  while (top_postulate != save) {
    top_postulate = NEXT (top_postulate);
  }
*/
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_equivalent_moids: indentify equivalent modes in program.                 */

static void
find_equivalent_moids (MOID_LIST_T * start, MOID_LIST_T * stop)
{
  for (; start != stop; start = NEXT (start))
    {
      MOID_LIST_T *p = NEXT (start);
      BOOL_T z = A_TRUE;
      for (; p != NULL && z; p = NEXT (p))
	{
	  if (EQUIVALENT (MOID (p)) != MOID (start))
	    {
	      z = !check_equivalent_moids (MOID (p), MOID (start));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_indicants_to_tags: bind indicants in symbol tables to tree.              */

static void
bind_indicants_to_tags (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *s = SYMBOL_TABLE (SUB (p));
	  TAG_T *z;
	  for (z = s->indicants; z != NULL; z = NEXT (z))
	    {
	      TAG_T *y = find_tag_global (s, INDICANT, SYMBOL (NODE (z)));
	      if (y != NULL && NODE (y) != NULL)
		{
		  MOID (z) = MOID (NEXT (NEXT (NODE (y))));
		}
	    }
	}
      bind_indicants_to_tags (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_indicants_to_modes: bind indicants in symbol tables to tree.             */

static void
bind_indicants_to_modes (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *s = SYMBOL_TABLE (SUB (p));
	  MOID_T *z;
	  for (z = s->moids; z != NULL; z = NEXT (z))
	    {
	      if (WHETHER (z, INDICANT))
		{
		  TAG_T *y = find_tag_global (s, INDICANT, SYMBOL (NODE (z)));
		  if (y != NULL && NODE (y) != NULL)
		    {
		      EQUIVALENT (z) = MOID (NEXT (NEXT (NODE (y))));
		    }
		  else
		    {
		      diagnostic (A_ERROR, p, "no declaration for tag Z in this range", SYMBOL (NODE (z)));
		    }
		}
	    }
	}
      bind_indicants_to_modes (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
track_equivalent_modes: replace a mode by its equivalent mode.                */

static void
track_equivalent_modes (MOID_T ** m)
{
  while ((*m) != NULL && EQUIVALENT ((*m)) != NULL)
    {
      (*m) = EQUIVALENT (*m);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
track_equivalent_one_moid: replace a mode by its equivalent mode.             */

static void
track_equivalent_one_moid (MOID_T * q)
{
  PACK_T *p;
  track_equivalent_modes (&SUB (q));
  track_equivalent_modes (&DEFLEXED (q));
  track_equivalent_modes (&MULTIPLE (q));
  track_equivalent_modes (&NAME (q));
  track_equivalent_modes (&SLICE (q));
  track_equivalent_modes (&TRIM (q));
  for (p = PACK (q); p != NULL; p = NEXT (p))
    {
      track_equivalent_modes (&MOID (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_list_track_equivalent.                                                   */

static void
moid_list_track_equivalent (MOID_T * q)
{
  for (; q != NULL; q = NEXT (q))
    {
      track_equivalent_one_moid (q);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
track_equivalent_tags.                                                        */

static void
track_equivalent_tags (TAG_T * z)
{
  for (; z != NULL; z = NEXT (z))
    {
      while (EQUIVALENT (MOID (z)) != NULL)
	{
	  MOID (z) = EQUIVALENT (MOID (z));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
track_equivalent_tree.                                                        */

static void
track_equivalent_tree (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (MOID (p) != NULL)
	{
	  while (EQUIVALENT (MOID (p)) != NULL)
	    {
	      MOID (p) = EQUIVALENT (MOID (p));
	    }
	}
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  if (SYMBOL_TABLE (SUB (p)) != NULL)
	    {
	      track_equivalent_tags (SYMBOL_TABLE (SUB (p))->indicants);
	      moid_list_track_equivalent (SYMBOL_TABLE (SUB (p))->moids);
	    }
	}
      track_equivalent_tree (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
track_equivalent_standard_modes.                                              */

static void
track_equivalent_standard_modes (void)
{
  track_equivalent_modes (&MODE (COMPLEX));
  track_equivalent_modes (&MODE (REF_COMPLEX));
  track_equivalent_modes (&MODE (LONG_COMPLEX));
  track_equivalent_modes (&MODE (REF_LONG_COMPLEX));
  track_equivalent_modes (&MODE (LONGLONG_COMPLEX));
  track_equivalent_modes (&MODE (REF_LONGLONG_COMPLEX));
  track_equivalent_modes (&MODE (REF_ROW_CHAR));
  track_equivalent_modes (&MODE (REF_STRING));
  track_equivalent_modes (&MODE (STRING));
  track_equivalent_modes (&MODE (REF_PIPE));
  track_equivalent_modes (&MODE (PIPE));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Routines for calculating subordinates for selections, for instance selection 
from REF STRUCT (A) yields REF A fields and selection from [] STRUCT (A) yields 
[] A fields.                                                                  */

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_name_pack.                                                               */

static void
make_name_pack (PACK_T * src, PACK_T ** dst, MOID_T ** p)
{
  if (src != NULL)
    {
      MOID_T *z;
      make_name_pack (NEXT (src), dst, p);
      z = add_mode (p, REF_SYMBOL, 0, NULL, MOID (src), NULL);
      add_mode_to_pack (dst, z, TEXT (src), NODE (src));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_name_struct.                                                             */

static MOID_T *
make_name_struct (MOID_T * m, MOID_T ** p)
{
  MOID_T *save;
  PACK_T *u = NULL;
  add_mode (p, STRUCT_SYMBOL, DIMENSION (m), NULL, NULL, NULL);
  save = *p;
  make_name_pack (PACK (m), &u, p);
  PACK (save) = u;
  return (save);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_name_row.                                                                */

static MOID_T *
make_name_row (MOID_T * m, MOID_T ** p)
{
  if (SLICE (m) != NULL)
    {
      return (add_mode (p, REF_SYMBOL, 0, NULL, SLICE (m), NULL));
    }
  else
    {
      return (add_mode (p, REF_SYMBOL, 0, NULL, SUB (m), NULL));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_structured_names.                                                        */

static void
make_structured_names (NODE_T * p, int *modifications)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *symbol_table = SYMBOL_TABLE (SUB (p));
	  MOID_T **topmoid = &(symbol_table->moids);
	  BOOL_T k = A_TRUE;
	  while (k)
	    {
	      MOID_T *m = symbol_table->moids;
	      k = A_FALSE;
	      for (; m != NULL; m = NEXT (m))
		{
		  if (NAME (m) == NULL && WHETHER (m, REF_SYMBOL))
		    {
		      if (WHETHER (SUB (m), STRUCT_SYMBOL))
			{
			  k = A_TRUE;
			  (*modifications)++;
			  NAME (m) = make_name_struct (SUB (m), topmoid);
			}
		      else if (WHETHER (SUB (m), ROW_SYMBOL))
			{
			  k = A_TRUE;
			  (*modifications)++;
			  NAME (m) = make_name_row (SUB (m), topmoid);
			}
		      else if (WHETHER (SUB (m), FLEX_SYMBOL))
			{
			  k = A_TRUE;
			  (*modifications)++;
			  NAME (m) = make_name_row (SUB (SUB (m)), topmoid);
			}
		    }
		}
	    }
	}
      make_structured_names (SUB (p), modifications);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_multiple_row_pack.                                                       */

static void
make_multiple_row_pack (PACK_T * src, PACK_T ** dst, MOID_T ** p, int dimensions)
{
  if (src != NULL)
    {
      make_multiple_row_pack (NEXT (src), dst, p, dimensions);
      add_mode_to_pack (dst, add_row (p, dimensions, MOID (src), NULL), TEXT (src), NODE (src));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_multiple_struct.                                                         */

static MOID_T *
make_multiple_struct (MOID_T * m, MOID_T ** p, int dimensions)
{
  MOID_T *save;
  PACK_T *u = NULL;
  add_mode (p, STRUCT_SYMBOL, DIMENSION (m), NULL, NULL, NULL);
  save = *p;
  make_multiple_row_pack (PACK (m), &u, p, dimensions);
  PACK (save) = u;
  return (save);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_flex_multiple_row_pack.                                                  */

static void
make_flex_multiple_row_pack (PACK_T * src, PACK_T ** dst, MOID_T ** p, int dimensions)
{
  if (src != NULL)
    {
      MOID_T *z;
      make_flex_multiple_row_pack (NEXT (src), dst, p, dimensions);
      z = add_row (p, dimensions, MOID (src), NULL);
      z = add_mode (p, FLEX_SYMBOL, 0, NULL, z, NULL);
      add_mode_to_pack (dst, z, TEXT (src), NODE (src));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_flex_multiple_struct.                                                    */

static MOID_T *
make_flex_multiple_struct (MOID_T * m, MOID_T ** p, int dimensions)
{
  MOID_T *x;
  PACK_T *u = NULL;
  add_mode (p, STRUCT_SYMBOL, DIMENSION (m), NULL, NULL, NULL);
  x = *p;
  make_flex_multiple_row_pack (PACK (m), &u, p, dimensions);
  PACK (x) = u;
  return (x);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_multiple_modes.                                                          */

static void
make_multiple_modes (NODE_T * p, int *modifications)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *symbol_table = SYMBOL_TABLE (SUB (p));
	  MOID_T **top = &symbol_table->moids;
	  BOOL_T z = A_TRUE;
	  while (z)
	    {
	      MOID_T *q = symbol_table->moids;
	      z = A_FALSE;
	      for (; q != NULL; q = NEXT (q))
		{
		  if (MULTIPLE (q) != NULL)
		    {
		      ;
		    }
		  else if (WHETHER (q, REF_SYMBOL))
		    {
		      if (MULTIPLE (SUB (q)) != NULL)
			{
			  (*modifications)++;
			  MULTIPLE (q) = make_name_struct (MULTIPLE (SUB (q)), top);
			}
		    }
		  else if (WHETHER (q, ROW_SYMBOL))
		    {
		      if (WHETHER (SUB (q), STRUCT_SYMBOL))
			{
			  z = A_TRUE;
			  (*modifications)++;
			  MULTIPLE (q) = make_multiple_struct (SUB (q), top, DIMENSION (q));
			}
		    }
		  else if (WHETHER (q, FLEX_SYMBOL))
		    {
		      if (SUB (SUB (q)) == NULL)
			{
			  (*modifications)++;	/* as yet unresolved FLEX INDICANT */
			}
		      else
			{
			  if (WHETHER (SUB (SUB (q)), STRUCT_SYMBOL))
			    {
			      z = A_TRUE;
			      (*modifications)++;
			      MULTIPLE (q) = make_flex_multiple_struct (SUB (SUB (q)), top, DIMENSION (SUB (q)));
			    }
			}
		    }
		}
	    }
	}
      make_multiple_modes (SUB (p), modifications);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Deflexing removes all FLEX from a mode, 
for instance REF STRING -> REF [] CHAR.                                       */

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_flex_2.                                                      */

static BOOL_T
whether_mode_has_flex_2 (MOID_T * m)
{
  if (whether_postulated (top_postulate, m))
    {
      return (A_FALSE);
    }
  else
    {
      make_postulate (&top_postulate, m, NULL);
      if (WHETHER (m, FLEX_SYMBOL))
	{
	  return (A_TRUE);
	}
      else if (WHETHER (m, REF_SYMBOL))
	{
	  return (whether_mode_has_flex_2 (SUB (m)));
	}
      else if (WHETHER (m, PROC_SYMBOL))
	{
	  return (whether_mode_has_flex_2 (SUB (m)));
	}
      else if (WHETHER (m, ROW_SYMBOL))
	{
	  return (whether_mode_has_flex_2 (SUB (m)));
	}
      else if (WHETHER (m, STRUCT_SYMBOL))
	{
	  PACK_T *t = PACK (m);
	  BOOL_T z = A_FALSE;
	  for (; t != NULL && !z; t = NEXT (t))
	    {
	      z |= whether_mode_has_flex_2 (MOID (t));
	    }
	  return (z);
	}
      else
	{
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_flex.                                                        */

static BOOL_T
whether_mode_has_flex (MOID_T * m)
{
  reset_postulates ();
  return (whether_mode_has_flex_2 (m));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_deflexed: make a deflexed mode.                                          */

static MOID_T *
make_deflexed (MOID_T * m, MOID_T ** p)
{
  if (DEFLEXED (m) != NULL)
    {				/* Keep this condition on top */
      return (DEFLEXED (m));
    }
  else if (WHETHER (m, REF_SYMBOL))
    {
      MOID_T *new_one = make_deflexed (SUB (m), p);
      add_mode (p, REF_SYMBOL, DIMENSION (m), NULL, new_one, NULL);
      SUB (*p) = new_one;
      return (DEFLEXED (m) = *p);
    }
  else if (WHETHER (m, PROC_SYMBOL))
    {
      MOID_T *new_one = make_deflexed (SUB (m), p);
      add_mode (p, PROC_SYMBOL, DIMENSION (m), NULL, new_one, PACK (m));
      return (DEFLEXED (m) = *p);
    }
  else if (WHETHER (m, FLEX_SYMBOL))
    {
      abend (SUB (m) == NULL, "NULL mode while deflexing", NULL);
      DEFLEXED (m) = make_deflexed (SUB (m), p);
      return (DEFLEXED (m));
    }
  else if (WHETHER (m, ROW_SYMBOL))
    {
      MOID_T *new_sub, *new_slice;
      if (DIMENSION (m) > 1)
	{
	  new_slice = make_deflexed (SLICE (m), p);
	  add_mode (p, ROW_SYMBOL, DIMENSION (m) - 1, NULL, new_slice, NULL);
	  new_sub = make_deflexed (SUB (m), p);
	}
      else
	{
	  new_sub = make_deflexed (SUB (m), p);
	  new_slice = new_sub;
	}
      add_mode (p, ROW_SYMBOL, DIMENSION (m), NULL, new_sub, NULL);
      SLICE (*p) = new_slice;
      return (DEFLEXED (m) = *p);
    }
  else if (WHETHER (m, STRUCT_SYMBOL))
    {
      MOID_T *save;
      PACK_T *u = NULL;
      add_mode (p, STRUCT_SYMBOL, DIMENSION (m), NULL, NULL, NULL);
      save = *p;
/* Mark to prevent eventual cyclic references */
      DEFLEXED (m) = save;
      make_deflexed_pack (PACK (m), &u, p);
      PACK (save) = u;
      return (save);
    }
  else if (WHETHER (m, INDICANT))
    {
      MOID_T *n = EQUIVALENT (m);
      abend (n == NULL, "NULL equivalent mode while deflexing", NULL);
      return (DEFLEXED (m) = make_deflexed (n, p));
    }
  else if (WHETHER (m, STANDARD))
    {
      MOID_T *n = (DEFLEXED (m) != NULL ? DEFLEXED (m) : m);
      return (n);
    }
  else
    {
      return (m);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_deflexed_pack .                                                          */

static void
make_deflexed_pack (PACK_T * src, PACK_T ** dst, MOID_T ** p)
{
  if (src != NULL)
    {
      make_deflexed_pack (NEXT (src), dst, p);
      add_mode_to_pack (dst, make_deflexed (MOID (src), p), TEXT (src), NODE (src));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_deflexed_modes.                                                          */

static void
make_deflexed_modes (NODE_T * p, int *modifications)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *s = SYMBOL_TABLE (SUB (p));
	  MOID_T *m, **top = &s->moids;
	  for (m = s->moids; m != NULL; m = NEXT (m))
	    {
/* 'Complete' deflexing */
	      if (!m->has_flex)
		{
		  m->has_flex = whether_mode_has_flex (m);
		}
	      if (m->has_flex && DEFLEXED (m) == NULL)
		{
		  (*modifications)++;
		  DEFLEXED (m) = make_deflexed (m, top);
		  abend (whether_mode_has_flex (DEFLEXED (m)), "deflexing failed for ", moid_to_string (DEFLEXED (m), 80));
		}
/* 'Light' deflexing needed for trims */
	      if (TRIM (m) == NULL && WHETHER (m, FLEX_SYMBOL))
		{
		  (*modifications)++;
		  TRIM (m) = SUB (m);
		}
	      else if (TRIM (m) == NULL && WHETHER (m, REF_SYMBOL) && WHETHER (SUB (m), FLEX_SYMBOL))
		{
		  (*modifications)++;
		  add_mode (top, REF_SYMBOL, DIMENSION (m), NULL, SUB (SUB (m)), NULL);
		  TRIM (m) = *top;
		}
	    }
	}
      make_deflexed_modes (SUB (p), modifications);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_ref_2.                                                       */

static BOOL_T
whether_mode_has_ref_2 (MOID_T * m)
{
  if (whether_postulated (top_postulate, m))
    {
      return (A_FALSE);
    }
  else
    {
      make_postulate (&top_postulate, m, NULL);
      if (WHETHER (m, FLEX_SYMBOL))
	{
	  return (whether_mode_has_ref_2 (SUB (m)));
	}
      else if (WHETHER (m, REF_SYMBOL))
	{
	  return (A_TRUE);
	}
      else if (WHETHER (m, ROW_SYMBOL))
	{
	  return (whether_mode_has_ref_2 (SUB (m)));
	}
      else if (WHETHER (m, STRUCT_SYMBOL))
	{
	  PACK_T *t = PACK (m);
	  BOOL_T z = A_FALSE;
	  for (; t != NULL && !z; t = NEXT (t))
	    {
	      z |= whether_mode_has_ref_2 (MOID (t));
	    }
	  return (z);
	}
      else
	{
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_ref.                                                         */

static BOOL_T
whether_mode_has_ref (MOID_T * m)
{
  reset_postulates ();
  return (whether_mode_has_ref_2 (m));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Routines setting properties of modes.                                         */

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_moid.                                                                   */

static void
reset_moid (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      MOID (p) = NULL;
      reset_moid (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
renumber_moids.                                                               */

static int
renumber_moids (MOID_LIST_T * p)
{
  if (p == NULL)
    {
      return (1);
    }
  else
    {
      return (1 + (MOID (p)->number = renumber_moids (NEXT (p))));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_mode_has_row.                                                         */

static int
whether_mode_has_row (MOID_T * m)
{
  if (WHETHER (m, STRUCT_SYMBOL) || WHETHER (m, UNION_SYMBOL))
    {
      BOOL_T k = A_FALSE;
      PACK_T *p = PACK (m);
      for (; p != NULL && k == A_FALSE; p = NEXT (p))
	{
	  MOID (p)->has_rows = whether_mode_has_row (MOID (p));
	  k |= (MOID (p)->has_rows);
	}
      return (k);
    }
  else
    {
      return (m->has_rows || WHETHER (m, ROW_SYMBOL) || WHETHER (m, FLEX_SYMBOL));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mark_row_modes.                                                               */

static void
mark_row_modes (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
/* Dive into lexical levels */
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  MOID_T *m;
	  for (m = SYMBOL_TABLE (SUB (p))->moids; m != NULL; m = NEXT (m))
	    {
	      m->has_rows = whether_mode_has_row (m);
	    }
	}
      mark_row_modes (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_moid_attributes.                                                          */

static void
set_moid_attributes (MOID_LIST_T * start)
{
  for (; start != NULL; start = NEXT (start))
    {
      if (!(MOID (start)->has_ref))
	{
	  MOID (start)->has_ref = whether_mode_has_ref (MOID (start));
	}
      if (!(MOID (start)->has_flex))
	{
	  MOID (start)->has_flex = whether_mode_has_flex (MOID (start));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_moid_list.                                                                */

void
get_moid_list (MOID_LIST_T ** top_moid_list, NODE_T * top_node)
{
  reset_moid_list ();
  add_moid_list (top_moid_list, stand_env);
  add_moid_moid_list (top_node, top_moid_list);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
expand_contract_moids: calculate derived modes and simplify table.            */

static int
expand_contract_moids (NODE_T * top_node, int cycle_no)
{
  int modifications = 0;
  reset_postulates ();
  if (cycle_no >= 0)
    {				/* Just experimental, might remove */
/* Calculate derived modes */
      absorb_unions (top_node, &modifications);
      contract_unions (top_node, &modifications);
      make_multiple_modes (top_node, &modifications);
      make_structured_names (top_node, &modifications);
      make_deflexed_modes (top_node, &modifications);
    }
/* Calculate equivalent modes */
  get_moid_list (&top_moid_list, top_node);
  bind_indicants_to_modes (top_node);
  reset_postulates ();
  find_equivalent_moids (top_moid_list, NULL);
  track_equivalent_tree (top_node);
  track_equivalent_tags (stand_env->indicants);
  track_equivalent_tags (stand_env->identifiers);
  track_equivalent_tags (stand_env->operators);
  moid_list_track_equivalent (stand_env->moids);
  contract_unions (top_node, &modifications);
  set_moid_attributes (top_moid_list);
  set_moid_sizes (top_moid_list);
  return (modifications);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
maintain_mode_table.                                                          */

void
maintain_mode_table (NODE_T * p)
{
  (void) p;
  renumber_moids (top_moid_list);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_up_mode_table: make list of all modes in the program.                     */

void
set_up_mode_table (NODE_T * top_node)
{
  reset_moid (top_node);
  get_mode_from_modes (top_node, 0);
  get_mode_from_proc_variable_declarations (top_node);
/* Tie MODE declarations to their respective a68_modes ... */
  bind_indicants_to_tags (top_node);
  bind_indicants_to_modes (top_node);
/* ... and check for cyclic definitions as MODE A = B, B = C, C = A */
  check_cyclic_modes (top_node);
  if (error_count == 0)
    {
/* Check yin-yang of modes */
      reset_postulates ();
      check_well_formedness (top_node);
/* Construct the full moid list */
      if (error_count == 0)
	{
	  int cycle = 0;
	  track_equivalent_standard_modes ();
	  while (expand_contract_moids (top_node, cycle) > 0 || cycle <= 1)
	    {
	      abend (cycle++ > 16, "apparent indefinite loop in set_up_mode_table", NULL);
	    }
/* Set standard modes */
	  track_equivalent_standard_modes ();
/* Postlude */
	  check_flex_modes (top_node);
	  check_relation_to_void (top_node);
	  mark_row_modes (top_node);
	}
    }
  init_postulates ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Next are routines to calculate the size of a mode.                            */

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_max_simplout_size.                                                      */

void
reset_max_simplout_size (void)
{
  max_simplout_size = 0;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
max_unitings_to_simplout.                                                     */

static void
max_unitings_to_simplout (NODE_T * p, int *max)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, UNITING) && MOID (p) == MODE (SIMPLOUT))
	{
	  MOID_T *q = MOID (SUB (p));
	  if (q != MODE (SIMPLOUT))
	    {
	      int size = moid_size (q);
	      if (size > *max)
		{
		  *max = size;
		}
	    }
	}
      max_unitings_to_simplout (SUB (p), max);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_max_simplout_size.                                                        */

void
get_max_simplout_size (NODE_T * p)
{
  max_simplout_size = 0;
  max_unitings_to_simplout (p, &max_simplout_size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_moid_sizes.                                                               */

void
set_moid_sizes (MOID_LIST_T * start)
{
  for (; start != NULL; start = NEXT (start))
    {
      MOID (start)->size = moid_size (MOID (start));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_size_2.                                                                   */

static int
moid_size_2 (MOID_T * p)
{
  if (p == NULL)
    {
      return (0);
    }
  else if (EQUIVALENT (p) != NULL)
    {
      return (moid_size_2 (EQUIVALENT (p)));
    }
  else if (p == MODE (HIP))
    {
      return (0);
    }
  else if (p == MODE (VOID))
    {
      return (0);
    }
  else if (p == MODE (INT))
    {
      return (SIZE_OF (A68_INT));
    }
  else if (p == MODE (LONG_INT))
    {
      return ((int) size_long_mp ());
    }
  else if (p == MODE (LONGLONG_INT))
    {
      return ((int) size_longlong_mp ());
    }
  else if (p == MODE (REAL))
    {
      return (SIZE_OF (A68_REAL));
    }
  else if (p == MODE (LONG_REAL))
    {
      return ((int) size_long_mp ());
    }
  else if (p == MODE (LONGLONG_REAL))
    {
      return ((int) size_longlong_mp ());
    }
  else if (p == MODE (BOOL))
    {
      return (SIZE_OF (A68_BOOL));
    }
  else if (p == MODE (CHAR))
    {
      return (SIZE_OF (A68_CHAR));
    }
  else if (p == MODE (ROW_CHAR))
    {
      return (SIZE_OF (A68_REF));
    }
  else if (p == MODE (BITS))
    {
      return (SIZE_OF (A68_BITS));
    }
  else if (p == MODE (LONG_BITS))
    {
      return ((int) size_long_mp ());
    }
  else if (p == MODE (LONGLONG_BITS))
    {
      return ((int) size_longlong_mp ());
    }
  else if (p == MODE (BYTES))
    {
      return (SIZE_OF (A68_BYTES));
    }
  else if (p == MODE (LONG_BYTES))
    {
      return (SIZE_OF (A68_LONG_BYTES));
    }
  else if (p == MODE (FILE))
    {
      return (SIZE_OF (A68_FILE));
    }
  else if (p == MODE (CHANNEL))
    {
      return (SIZE_OF (A68_CHANNEL));
    }
  else if (p == MODE (FORMAT))
    {
      return (SIZE_OF (A68_FORMAT));
    }
  else if (p == MODE (COLLITEM))
    {
      return (SIZE_OF (A68_COLLITEM));
    }
  else if (p == MODE (NUMBER))
    {
      int k = 0;
      if (SIZE_OF (A68_INT) > k)
	{
	  k = SIZE_OF (A68_INT);
	}
      if ((int) size_long_mp () > k)
	{
	  k = (int) size_long_mp ();
	}
      if ((int) size_longlong_mp () > k)
	{
	  k = (int) size_longlong_mp ();
	}
      if (SIZE_OF (A68_REAL) > k)
	{
	  k = SIZE_OF (A68_REAL);
	}
      if ((int) size_long_mp () > k)
	{
	  k = (int) size_long_mp ();
	}
      if ((int) size_longlong_mp () > k)
	{
	  k = (int) size_longlong_mp ();
	}
      if (SIZE_OF (A68_REF) > k)
	{
	  k = SIZE_OF (A68_REF);
	}
      return (SIZE_OF (A68_POINTER) + k);
    }
  else if (p == MODE (SIMPLIN))
    {
      int k = 0;
      if (SIZE_OF (A68_REF) > k)
	{
	  k = SIZE_OF (A68_REF);
	}
      if (SIZE_OF (A68_FORMAT) > k)
	{
	  k = SIZE_OF (A68_FORMAT);
	}
      if (SIZE_OF (A68_PROCEDURE) > k)
	{
	  k = SIZE_OF (A68_PROCEDURE);
	}
      return (SIZE_OF (A68_POINTER) + k);
    }
  else if (p == MODE (SIMPLOUT))
    {
      return (SIZE_OF (A68_POINTER) + max_simplout_size);
    }
  else if (WHETHER (p, REF_SYMBOL))
    {
      return (SIZE_OF (A68_REF));
    }
  else if (WHETHER (p, PROC_SYMBOL))
    {
      return (SIZE_OF (A68_PROCEDURE));
    }
  else if (WHETHER (p, ROW_SYMBOL) && p != MODE (ROWS))
    {
      return (SIZE_OF (A68_REF));
    }
  else if (p == MODE (ROWS))
    {
      return (SIZE_OF (A68_POINTER) + SIZE_OF (A68_REF));
    }
  else if (WHETHER (p, FLEX_SYMBOL))
    {
      return moid_size (SUB (p));
    }
  else if (WHETHER (p, STRUCT_SYMBOL))
    {
      PACK_T *z = PACK (p);
      int size = 0;
      for (; z != NULL; z = NEXT (z))
	{
	  size += moid_size (MOID (z));
	}
      return (size);
    }
  else if (WHETHER (p, UNION_SYMBOL))
    {
      PACK_T *z = PACK (p);
      int size = 0;
      for (; z != NULL; z = NEXT (z))
	{
	  if (moid_size (MOID (z)) > size)
	    {
	      size = moid_size (MOID (z));
	    }
	}
      return (SIZE_OF (A68_POINTER) + size);
    }
  else if (PACK (p) != NULL)
    {
      PACK_T *z = PACK (p);
      int size = 0;
      for (; z != NULL; z = NEXT (z))
	{
	  size += moid_size (MOID (z));
	}
      return (size);
    }
  else
    {
/* ? */
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_size.                                                                    */

int
moid_size (MOID_T * p)
{
  p->size = moid_size_2 (p);
  return (p->size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
A pretty printer for moids.                                                   */

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_to_string_3.                                                             */

static void
moid_to_string_3 (char *dst, char *str, int w)
{
  if (w >= (int) strlen (str))
    {
      strcat (dst, str);
    }
  else
    {
      strcat (dst, "..");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pack_to_string.                                                               */

static void
pack_to_string (char *b, PACK_T * p, int w, BOOL_T text)
{
  if (w > (int) strlen (".."))
    {
      while (p != NULL && w > 0)
	{
	  if (w > (int) strlen (".."))
	    {
	      int l = (int) strlen (b);
	      moid_to_string_2 (b, MOID (p), w);
	      if (text && TEXT (p) != NULL)
		{
		  strcat (b, " ");
		  strcat (b, TEXT (p));
		}
	      w -= (int) strlen (b) - l;
	    }
	  else
	    {
	      strcat (b, "..");
	      w = 0;
	    }
	  p = NEXT (p);
	  if (p != NULL)
	    {
	      strcat (b, ", ");
	      if (w <= (int) strlen (", ") + (int) strlen (".."))
		{
		  strcat (b, "..");
		  w = 0;
		}
	    }
	}
    }
  else
    {
      strcat (b, "..");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_to_string_2.                                                             */

static void
moid_to_string_2 (char *b, MOID_T * n, int w)
{
  if (n != NULL)
    {
      if (whether_postulated (postulates, n))
	{
	  strcat (b, "\"SELF\"");
	}
      else if (n == MODE (HIP))
	{
	  moid_to_string_3 (b, "HIP", w);
	}
      else if (n == MODE (ERROR))
	{
	  moid_to_string_3 (b, "ERROR", w);
	}
      else if (n == MODE (UNDEFINED))
	{
	  moid_to_string_3 (b, "UNDEFINED", w);
	}
      else if (n == MODE (C_STRING))
	{
	  moid_to_string_3 (b, "C-STRING", w);
	}
      else if (n == MODE (COMPLEX) || n == MODE (COMPL))
	{
	  moid_to_string_3 (b, "COMPLEX", w);
	}
      else if (n == MODE (LONG_COMPLEX) || n == MODE (LONG_COMPL))
	{
	  moid_to_string_3 (b, "LONG COMPLEX", w);
	}
      else if (n == MODE (LONGLONG_COMPLEX) || n == MODE (LONGLONG_COMPL))
	{
	  moid_to_string_3 (b, "LONG LONG COMPLEX", w);
	}
      else if (n == MODE (STRING))
	{
	  moid_to_string_3 (b, "STRING", w);
	}
      else if (n == MODE (PIPE))
	{
	  moid_to_string_3 (b, "PIPE", w);
	}
      else if (n == MODE (COLLITEM))
	{
	  moid_to_string_3 (b, "COLLITEM", w);
	}
      else if (WHETHER (n, IN_TYPE_MODE))
	{
	  moid_to_string_3 (b, "\"SIMPLIN\"", w);
	}
      else if (WHETHER (n, OUT_TYPE_MODE))
	{
	  moid_to_string_3 (b, "\"SIMPLOUT\"", w);
	}
      else if (WHETHER (n, ROWS_SYMBOL))
	{
	  moid_to_string_3 (b, "\"ROWS\"", w);
	}
      else if (n == MODE (VACUUM))
	{
	  moid_to_string_3 (b, "\"VACUUM\"", w);
	}
      else if (WHETHER (n, VOID_SYMBOL) || WHETHER (n, STANDARD) || WHETHER (n, INDICANT))
	{
	  int i = 1;
	  for (; i <= abs (n->dimensions) && w > 0; i++)
	    {
	      if (n->dimensions < 0)
		{
		  if (w >= (int) strlen ("SHORT .."))
		    {
		      strcat (b, "SHORT ");
		      w -= (int) strlen ("SHORT ");
		    }
		  else
		    {
		      strcat (b, "..");
		      w = 0;
		    }
		}
	      else if (w >= (int) strlen ("LONG .."))
		{
		  strcat (b, "LONG ");
		  w -= (int) strlen ("LONG ");
		}
	      else
		{
		  strcat (b, "..");
		  w = 0;
		}
	    }
	  moid_to_string_3 (b, SYMBOL (NODE (n)), w);
	}
      else if (WHETHER (n, REF_SYMBOL))
	{
	  if (w >= (int) strlen ("REF .."))
	    {
	      strcat (b, "REF ");
	      moid_to_string_2 (b, SUB (n), w - (int) strlen ("REF .."));
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	}
      else if (WHETHER (n, FLEX_SYMBOL))
	{
	  if (w >= (int) strlen ("FLEX .."))
	    {
	      strcat (b, "FLEX ");
	      moid_to_string_2 (b, SUB (n), w - (int) strlen ("FLEX .."));
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	}
      else if (WHETHER (n, ROW_SYMBOL))
	{
	  int j = (int) strlen ("[] ..") + 2 * (n->dimensions - 1);
	  if (w >= j)
	    {
	      int i;
	      strcat (b, "[");
	      for (i = 1; i < n->dimensions; i++)
		{
		  strcat (b, ", ");
		}
	      strcat (b, "] ");
	      moid_to_string_2 (b, SUB (n), w - j);
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	}
      else if (WHETHER (n, STRUCT_SYMBOL))
	{
	  POSTULATE_T *save = postulates;
	  make_postulate (&postulates, n, NULL);
	  if (w >= (int) strlen ("STRUCT (..)"))
	    {
	      strcat (b, "STRUCT (");
	      pack_to_string (b, PACK (n), w - (int) strlen ("STRUCT (..)"), A_TRUE);
	      strcat (b, ")");
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	  postulates = save;
	}
      else if (WHETHER (n, UNION_SYMBOL))
	{
	  POSTULATE_T *save = postulates;
	  make_postulate (&postulates, n, NULL);
	  if (w >= (int) strlen ("UNION (..)"))
	    {
	      strcat (b, "UNION (");
	      pack_to_string (b, PACK (n), w - (int) strlen ("UNION (..)"), A_FALSE);
	      strcat (b, ")");
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	  postulates = save;
	}
      else if (WHETHER (n, PROC_SYMBOL))
	{
	  POSTULATE_T *save = postulates;
	  make_postulate (&postulates, n, NULL);
	  if (PACK (n) != NULL)
	    {
	      if (w >= (int) strlen ("PROC (..) .."))
		{
		  strcat (b, "PROC (");
		  pack_to_string (b, PACK (n), w - (int) strlen ("PROC (..) .."), A_FALSE);
		  strcat (b, ") ");
		  moid_to_string_2 (b, SUB (n), w - (int) strlen (b));
		}
	      else
		{
		  strcat (b, "..");
		}
	    }
	  else
	    {
	      if (w >= (int) strlen ("PROC .."))
		{
		  strcat (b, "PROC ");
		  moid_to_string_2 (b, SUB (n), w - (int) strlen ("PROC .."));
		}
	      else
		{
		  strcat (b, "..");
		}
	    }
	  postulates = save;
	}
      else if (WHETHER (n, SERIES_MODE))
	{
	  if (w >= (int) strlen ("(..)"))
	    {
	      strcat (b, "(");
	      pack_to_string (b, PACK (n), w - (int) strlen ("(..)"), A_FALSE);
	      strcat (b, ")");
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	}
      else if (WHETHER (n, STOWED_MODE))
	{
	  if (w >= (int) strlen ("(..)"))
	    {
	      strcat (b, "(");
	      pack_to_string (b, PACK (n), w - (int) strlen ("(..)"), A_FALSE);
	      strcat (b, ")");
	    }
	  else
	    {
	      strcat (b, "..");
	    }
	}
    }
  else
    {
      strcat (b, "NULL");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_to_string: pretty-formatted mode "n"; "w" is a measure of width.         */

char *
moid_to_string (MOID_T * n, int w)
{
  char a[BUFFER_SIZE];
  a[0] = '\0';
  if (w >= BUFFER_SIZE)
    {
      return (new_string (a));
    }
  postulates = NULL;
  if (n != NULL)
    {
      moid_to_string_2 (a, n, w);
    }
  else
    {
      strcat (a, "NULL");
    }
  return (new_string (a));
}
