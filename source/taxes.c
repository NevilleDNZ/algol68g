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
This file contains routines that work with TAXes and symbol tables.           */

#include "algol68g.h"

static void tax_tags (NODE_T *);
static void tax_specifier_list (NODE_T *);
static void tax_parameter_list (NODE_T *);
static void tax_format_texts (NODE_T *);

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_identifiers: bind identifier in the tree to the symbol table.   */

static void
bind_identifiers (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      bind_identifiers (SUB (p));
      if (WHETHER (p, IDENTIFIER) || WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *z = find_tag_global (SYMBOL_TABLE (p), IDENTIFIER, SYMBOL (p));
	  if (z != NULL)
	    {
	      MOID (p) = MOID (z);
	    }
	  else if ((z = find_tag_global (SYMBOL_TABLE (p), LABEL, SYMBOL (p))) == NULL)
	    {
	      diagnostic (A_ERROR, p, "identifier S has not been declared in this range");
	      z = add_tag (SYMBOL_TABLE (p), IDENTIFIER, p, MODE (ERROR), NORMAL_IDENTIFIER);
	      MOID (p) = MODE (ERROR);
	    }
	  TAX (p) = z;
	  if (WHETHER (p, DEFINING_IDENTIFIER))
	    {
	      NODE (z) = p;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_indicants: bind indicants to the symbol table.                           */

static void
bind_indicants (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      bind_indicants (SUB (p));
      if (WHETHER (p, INDICANT) || WHETHER (p, DEFINING_INDICANT))
	{
	  TAG_T *z = find_tag_global (SYMBOL_TABLE (p), INDICANT, SYMBOL (p));
	  if (z != NULL)
	    {
	      MOID (p) = MOID (z);
	      TAX (p) = z;
	      if (WHETHER (p, DEFINING_INDICANT))
		{
		  NODE (z) = p;
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_specifiers:  enter specifier identifiers in the symbol table.             */

static void
tax_specifiers (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_specifiers (SUB (p));
      if (SUB (p) != NULL && WHETHER (p, SPECIFIER))
	{
	  tax_specifier_list (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_specifier_list: enter specifier identifiers in the symbol table.          */

static void
tax_specifier_list (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, OPEN_SYMBOL))
	{
	  tax_specifier_list (NEXT (p));
	}
      else if (WHETHER (p, CLOSE_SYMBOL) || WHETHER (p, VOID_SYMBOL))
	{
/* skip */ ;
	}
      else if (WHETHER (p, IDENTIFIER))
	{
	  TAG_T *z = add_tag (SYMBOL_TABLE (p), IDENTIFIER, p, NULL,
			      SPECIFIER_IDENTIFIER);
	  HEAP (z) = LOC_SYMBOL;
	}
      else if (WHETHER (p, DECLARER))
	{
	  tax_specifiers (SUB (p));
	  tax_specifier_list (NEXT (p));
/* last identifier entry is identifier with this declarer */
	  if (SYMBOL_TABLE (p)->identifiers != NULL && PRIO (SYMBOL_TABLE (p)->identifiers) == SPECIFIER_IDENTIFIER)
	    MOID (SYMBOL_TABLE (p)->identifiers) = MOID (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_parameters: enter parameter identifiers in the symbol table.              */

static void
tax_parameters (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL)
	{
	  tax_parameters (SUB (p));
	  if (WHETHER (p, PARAMETER_PACK))
	    {
	      tax_parameter_list (SUB (p));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_parameter_list: enter parameter identifiers in the symbol table.          */

static void
tax_parameter_list (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  tax_parameter_list (NEXT (p));
	}
      else if (WHETHER (p, CLOSE_SYMBOL))
	{;
	}
      else if (WHETHER (p, PARAMETER_LIST) || WHETHER (p, PARAMETER))
	{
	  tax_parameter_list (NEXT (p));
	  tax_parameter_list (SUB (p));
	}
      else if (WHETHER (p, IDENTIFIER))
	{
/* parameters are always local */
	  HEAP (add_tag (SYMBOL_TABLE (p), IDENTIFIER, p, NULL, PARAMETER_IDENTIFIER)) = LOC_SYMBOL;
	}
      else if (WHETHER (p, DECLARER))
	{
	  TAG_T *s;
	  tax_parameter_list (NEXT (p));
/* last identifier entries are identifiers with this declarer */
	  for (s = SYMBOL_TABLE (p)->identifiers; s != NULL && MOID (s) == NULL; s = NEXT (s))
	    {
	      MOID (s) = MOID (p);
	    }
	  tax_parameters (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_for_identifiers: enter FOR identifiers in the symbol table.               */

static void
tax_for_identifiers (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_for_identifiers (SUB (p));
      if (WHETHER (p, FOR_SYMBOL))
	{
	  if ((p = NEXT (p)) != NULL)
	    {
	      add_tag (SYMBOL_TABLE (p), IDENTIFIER, p, MODE (INT), LOOP_IDENTIFIER);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_routine_texts: enter routine texts in the symbol table.                   */

static void
tax_routine_texts (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_routine_texts (SUB (p));
      if (WHETHER (p, ROUTINE_TEXT))
	{
	  TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, MOID (p), ROUTINE_TEXT);
	  TAX (p) = z;
	  HEAP (z) = LOC_SYMBOL;
	  z->use = A_TRUE;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_format_texts: enter format texts in the symbol table.                     */

static void
tax_format_texts (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_format_texts (SUB (p));
      if (WHETHER (p, FORMAT_TEXT))
	{
	  TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, MODE (FORMAT), FORMAT_TEXT);
	  TAX (p) = z;
	  z->use = A_TRUE;
	}
      else if (WHETHER (p, FORMAT_DELIMITER_SYMBOL) && NEXT (p) != NULL)
	{
	  TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, MODE (FORMAT), FORMAT_IDENTIFIER);
	  TAX (p) = z;
	  z->use = A_TRUE;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_pictures: enter FORMAT pictures in the symbol table.                      */

static void
tax_pictures (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_pictures (SUB (p));
      if (WHETHER (p, PICTURE))
	{
	  TAX (p) = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, MODE (COLLITEM), FORMAT_IDENTIFIER);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_generators: enter generators in the symbol table.                         */

static void
tax_generators (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      tax_generators (SUB (p));
      if (WHETHER (p, GENERATOR))
	{
	  if (WHETHER (SUB (p), LOC_SYMBOL))
	    {
	      TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, SUB (MOID (SUB (p))), GENERATOR);
	      HEAP (z) = LOC_SYMBOL;
	      z->use = A_TRUE;
	      TAX (p) = z;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
structure_fields_test: consistency check on fields in structured modes.
For instance, STRUCT (REAL x, INT n, REAL x) is wrong.                        */

static void
structure_fields_test (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  MOID_T *m = SYMBOL_TABLE (SUB (p))->moids;
	  for (; m != NULL; m = NEXT (m))
	    {
	      if (WHETHER (m, STRUCT_SYMBOL) && m->equivalent_mode == NULL)
		{
/* check on identically named fields */
		  PACK_T *s = PACK (m);
		  for (; s != NULL; s = NEXT (s))
		    {
		      PACK_T *t = NEXT (s);
		      BOOL_T k = A_TRUE;
		      for (t = NEXT (s); t != NULL && k; t = NEXT (t))
			{
			  if (s->text == t->text)
			    {
			      diagnostic (A_ERROR, p, "multiple declaration of field S");
			      while (NEXT (s) != NULL && NEXT (s)->text == t->text)
				{
				  s = NEXT (s);
				}
			      k = A_FALSE;
			    }
			}
		    }
		}
	    }
	}
      structure_fields_test (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
incestuous_union_test: consistency check on united modes.
  UNION (INT),
  UNION (REF INT, PROC INT),
  UNION (STRING, [] CHAR),
  UNION (INT, REAL, REF UNION (INT, REAL)) are all wrong united modes.        */

static void
incestuous_union_test (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE_T *symbol_table = SYMBOL_TABLE (SUB (p));
	  MOID_T *m = symbol_table->moids;
	  for (; m != NULL; m = NEXT (m))
	    {
	      if (WHETHER (m, UNION_SYMBOL) && m->equivalent_mode == NULL)
		{
		  PACK_T *s = PACK (m);
		  BOOL_T x = A_TRUE;
/* Discard unions with one member */
		  if (count_pack_members (s) == 1)
		    {
		      SOID_T a;
		      make_soid (&a, NO_SORT, m, 0);
		      diagnostic (A_ERROR, NODE (m), "M must have at least two components", m);
		      x = A_FALSE;
		    }
/* Discard unions with firmly related modes */
		  for (; s != NULL && x; s = NEXT (s))
		    {
		      PACK_T *t = NEXT (s);
		      for (; t != NULL; t = NEXT (t))
			{
			  if (MOID (t) != MOID (s))
			    {
			      if (whether_firm (MOID (s), MOID (t)))
				{
				  diagnostic (A_ERROR, p, "M has firmly related components", m);
				}
			    }
			}
		    }
/* Discard unions with firmly related subsets */
		  for (s = PACK (m); s != NULL && x; s = NEXT (s))
		    {
		      MOID_T *n = depref_completely (MOID (s));
		      if (WHETHER (n, UNION_SYMBOL) && whether_subset (n, m, NO_DEFLEXING))
			{
			  SOID_T z;
			  make_soid (&z, NO_SORT, n, 0);
			  diagnostic (A_ERROR, p, "M contains firmly related subset M", m, n);
			}
		    }
		}
	    }
	}
      incestuous_union_test (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_firmly_related_op: find a firmly related operator for operands.          */

static TAG_T *
find_firmly_related_op (SYMBOL_TABLE_T * c, char *n, MOID_T * l, MOID_T * r, TAG_T * self)
{
  if (c != NULL)
    {
      TAG_T *s = c->operators;
      for (; s != NULL; s = NEXT (s))
	{
	  if (s != self && SYMBOL (NODE (s)) == n)
	    {
	      PACK_T *t = PACK (MOID (s));
	      if (t != NULL && whether_firm (MOID (t), l))
		{
/* catch monadic operator */
		  if ((t = NEXT (t)) == NULL)
		    {
		      if (r == NULL)
			{
			  return (s);
			}
		    }
		  else
		    {
/* catch dyadic operator */
		      if (r != NULL && whether_firm (MOID (t), r))
			{
			  return (s);
			}
		    }
		}
	    }
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
test_firmly_related_ops_local: firmly related operators in this range.        */

static void
test_firmly_related_ops_local (NODE_T * p, TAG_T * s)
{
  if (s != NULL)
    {
      PACK_T *u = PACK (MOID (s));
      MOID_T *l = MOID (u);
      MOID_T *r = NEXT (u) != NULL ? MOID (NEXT (u)) : NULL;
      TAG_T *t = find_firmly_related_op (SYMBOL_TABLE (s), SYMBOL (NODE (s)), l, r, s);
      if (t != NULL)
	{
	  if (SYMBOL_TABLE (t) == stand_env)
	    {
	      diagnostic (A_ERROR, p, "M Z is firmly related to M Z in standard environ", MOID (s), SYMBOL (NODE (s)), MOID (t), SYMBOL (NODE (t)), NULL);
	      abend (A_TRUE, "standard environ error", NULL);
	    }
	  else
	    {
	      diagnostic (A_ERROR, p, "M Z is firmly related to M Z", MOID (s), SYMBOL (NODE (s)), MOID (t), SYMBOL (NODE (t)), NULL);
	    }
	}
      if (NEXT (s) != NULL)
	{
	  test_firmly_related_ops_local (p == NULL ? NULL : NODE (NEXT (s)), NEXT (s));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
test_firmly_related_ops: find firmly related operators in this program.       */

static void
test_firmly_related_ops (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  TAG_T *oops = SYMBOL_TABLE (SUB (p))->operators;
	  if (oops != NULL)
	    {
	      test_firmly_related_ops_local (NODE (oops), oops);
	    }
	}
      test_firmly_related_ops (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
collect_taxes: driver for the processing of TAXes.                            */

void
collect_taxes (NODE_T * p)
{
  tax_tags (p);
  tax_specifiers (p);
  tax_parameters (p);
  tax_for_identifiers (p);
  tax_routine_texts (p);
  tax_pictures (p);
  tax_format_texts (p);
  tax_generators (p);
  bind_identifiers (p);
  bind_indicants (p);
  structure_fields_test (p);
  incestuous_union_test (p);
  test_firmly_related_ops (p);
  test_firmly_related_ops_local (NULL, stand_env->operators);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
already_declared: whether tag has already been declared in this range.        */

static void
already_declared (NODE_T * n, int a)
{
  if (find_tag_local (SYMBOL_TABLE (n), a, SYMBOL (n)) != NULL)
    {
      diagnostic (A_ERROR, n, "multiple declaration of tag S");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_tag: add tag to local symbol table.                                       */

#define INSERT_TAG(l, n) {NEXT (n) = *(l); *(l) = (n);}

TAG_T *
add_tag (SYMBOL_TABLE_T * s, int a, NODE_T * n, MOID_T * m, int p)
{
  if (s != NULL)
    {
      TAG_T *z = new_tag ();
      ACCESS (z) = PRIVATE_SYMBOL;
      SYMBOL_TABLE (z) = s;
      PRIO (z) = p;
      MOID (z) = m;
      NODE (z) = n;
/*    TAX(n) = z; */
      if (a == IDENTIFIER)
	{
	  already_declared (n, IDENTIFIER);
	  already_declared (n, LABEL);
	  INSERT_TAG (&s->identifiers, z);
	}
      else if (a == OP_SYMBOL)
	{
	  already_declared (n, INDICANT);
	  INSERT_TAG (&s->operators, z);
	}
      else if (a == PRIO_SYMBOL)
	{
	  already_declared (n, PRIO_SYMBOL);
	  already_declared (n, INDICANT);
	  INSERT_TAG (&PRIO (s), z);
	}
      else if (a == INDICANT)
	{
	  already_declared (n, INDICANT);
	  already_declared (n, OP_SYMBOL);
	  already_declared (n, PRIO_SYMBOL);
	  INSERT_TAG (&s->indicants, z);
	}
      else if (a == LABEL)
	{
	  already_declared (n, LABEL);
	  already_declared (n, IDENTIFIER);
	  INSERT_TAG (&s->labels, z);
	}
      else if (a == ANONYMOUS)
	{
	  INSERT_TAG (&s->anonymous, z);
	}
      else
	{
	  abend (A_TRUE, INTERNAL_ERROR, "add tag");
	}
      return (z);
    }
  else
    {
      return (NULL);
    }
}

#undef INSERT_TAG

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_tag_global: find a tag, searching symbol tables towards the root.        */

TAG_T *
find_tag_global (SYMBOL_TABLE_T * table, int a, char *name)
{
  if (table != NULL)
    {
      TAG_T *s = NULL;
      if (a == OP_SYMBOL)
	{
	  s = table->operators;
	}
      else if (a == PRIO_SYMBOL)
	{
	  s = PRIO (table);
	}
      else if (a == IDENTIFIER)
	{
	  s = table->identifiers;
	}
      else if (a == INDICANT)
	{
	  s = table->indicants;
	}
      else if (a == LABEL)
	{
	  s = table->labels;
	}
      else
	{
	  abend (A_TRUE, "impossible state in find_tag_global", NULL);
	}
      for (; s != NULL; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      return (s);
	    }
	}
      return (find_tag_global (PREVIOUS (table), a, name));
    }
  else
    {
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_identifier_or_label_global: find a tag, searching global symbol table.     */

int
whether_identifier_or_label_global (SYMBOL_TABLE_T * table, char *name)
{
  if (table != NULL)
    {
      TAG_T *s;
      for (s = table->identifiers; s != NULL; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      return (IDENTIFIER);
	    }
	}
      for (s = table->labels; s != NULL; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      return (LABEL);
	    }
	}
      return (whether_identifier_or_label_global (PREVIOUS (table), name));
    }
  else
    {
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_tag_local: find a tag, searching only local symbol table.                */

TAG_T *
find_tag_local (SYMBOL_TABLE_T * table, int a, char *name)
{
  if (table != NULL)
    {
      TAG_T *s = NULL;
      if (a == OP_SYMBOL)
	{
	  s = table->operators;
	}
      else if (a == PRIO_SYMBOL)
	{
	  s = PRIO (table);
	}
      else if (a == IDENTIFIER)
	{
	  s = table->identifiers;
	}
      else if (a == INDICANT)
	{
	  s = table->indicants;
	}
      else if (a == LABEL)
	{
	  s = table->labels;
	}
      else
	{
	  abend (A_TRUE, "impossible state in find_tag_local", NULL);
	}
      for (; s != NULL; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      return (s);
	    }
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tab_qualifier: whether context specifies HEAP or LOC for an identifier.       */

static int
tab_qualifier (NODE_T * p)
{
  if (p != NULL)
    {
      int k = ATTRIBUTE (p);
      if (k == UNIT || k == ASSIGNATION || k == TERTIARY || k == SECONDARY || k == GENERATOR)
	{
	  return (tab_qualifier (SUB (p)));
	}
      else if (k == LOC_SYMBOL || k == HEAP_SYMBOL)
	{
	  return (k);
	}
      else
	{
	  return (LOC_SYMBOL);
	}
    }
  else
    {
      return (LOC_SYMBOL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_identity_dec: enter identity declarations in the symbol table.            */

static void
tax_identity_dec (NODE_T * p, MOID_T ** m, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  tax_identity_dec (SUB (p), m, access);
	  tax_identity_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_identity_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, DECLARER))
	{
	  tax_tags (SUB (p));
	  *m = MOID (p);
	  tax_identity_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  tax_identity_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *entry = find_tag_local (SYMBOL_TABLE (p), IDENTIFIER, SYMBOL (p));
	  MOID (p) = *m;
	  HEAP (entry) = LOC_SYMBOL;
	  ACCESS (entry) = *access;
	  TAX (p) = entry;
	  MOID (entry) = *m;
	  if ((*m)->attribute == REF_SYMBOL)
	    {
	      HEAP (entry) = tab_qualifier (NEXT (NEXT (p)));
	    }
	  tax_identity_dec (NEXT (NEXT (p)), m, access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_variable_dec: enter variable declarations in the symbol table.            */

static void
tax_variable_dec (NODE_T * p, int *q, MOID_T ** m, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  tax_variable_dec (SUB (p), q, m, access);
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else if (WHETHER (p, DECLARER))
	{
	  tax_tags (SUB (p));
	  *m = MOID (p);
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else if (WHETHER (p, QUALIFIER))
	{
	  *q = ATTRIBUTE (SUB (p));
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *entry = find_tag_local (SYMBOL_TABLE (p), IDENTIFIER, SYMBOL (p));
	  MOID (p) = *m;
	  TAX (p) = entry;
	  HEAP (entry) = *q;
	  ACCESS (entry) = *access;
	  if (*q == LOC_SYMBOL)
	    {
	      TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, SUB (*m), GENERATOR);
	      HEAP (z) = LOC_SYMBOL;
	      z->use = A_TRUE;
	      entry->body = z;
	    }
	  else
	    {
	      entry->body = NULL;
	    }
	  MOID (entry) = *m;
	  tax_variable_dec (NEXT (p), q, m, access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_proc_variable_dec: enter proc variable declarations in the symbol table.  */

static void
tax_proc_variable_dec (NODE_T * p, int *q, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  tax_proc_variable_dec (SUB (p), q, access);
	  tax_proc_variable_dec (NEXT (p), q, access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_proc_variable_dec (NEXT (p), q, access);
	}
      else if (WHETHER (p, QUALIFIER))
	{
	  *q = ATTRIBUTE (SUB (p));
	  tax_proc_variable_dec (NEXT (p), q, access);
	}
      else if (WHETHER (p, PROC_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  tax_proc_variable_dec (NEXT (p), q, access);
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *entry = find_tag_local (SYMBOL_TABLE (p), IDENTIFIER, SYMBOL (p));
	  TAX (p) = entry;
	  HEAP (entry) = *q;
	  ACCESS (entry) = *access;
	  MOID (entry) = MOID (p);
	  if (*q == LOC_SYMBOL)
	    {
	      TAG_T *z = add_tag (SYMBOL_TABLE (p), ANONYMOUS, p, SUB (MOID (p)),
				  GENERATOR);
	      HEAP (z) = LOC_SYMBOL;
	      z->use = A_TRUE;
	      entry->body = z;
	    }
	  else
	    {
	      entry->body = NULL;
	    }
	  tax_proc_variable_dec (NEXT (p), q, access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_proc_dec: enter proc declarations in the symbol table.                    */

static void
tax_proc_dec (NODE_T * p, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, PROCEDURE_DECLARATION))
	{
	  tax_proc_dec (SUB (p), access);
	  tax_proc_dec (NEXT (p), access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_proc_dec (NEXT (p), access);
	}
      else if (WHETHER (p, PROC_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  tax_proc_dec (NEXT (p), access);
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  TAG_T *entry = find_tag_local (SYMBOL_TABLE (p), IDENTIFIER, SYMBOL (p));
	  MOID_T *m = MOID (NEXT (NEXT (p)));
	  MOID (p) = m;
	  TAX (p) = entry;
	  HEAP (entry) = LOC_SYMBOL;
	  ACCESS (entry) = *access;
	  MOID (entry) = m;
	  tax_proc_dec (NEXT (p), access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_operands: count number of operands in operator parameter list.          */

static int
count_operands (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, DECLARER))
	{
	  return (count_operands (NEXT (p)));
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  return (1 + count_operands (NEXT (p)));
	}
      else
	{
	  return (count_operands (NEXT (p)) + count_operands (SUB (p)));
	}
    }
  else
    {
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_operator_dec */

static void
check_operator_dec (NODE_T * p)
{
  int k = 0;
  NODE_T *pack = SUB (SUB (NEXT (NEXT (p))));	/* That's where the parameter pack is */
  if (ATTRIBUTE (NEXT (NEXT (p))) != ROUTINE_TEXT)
    {
      pack = SUB (pack);
    }
  k = 1 + count_operands (pack);
  if (!(k == 1 || k == 2))
    {
      diagnostic (A_SYNTAX_ERROR, p, "operator S cannot have D operands", k);
      k = 0;
    }
  if (k == 1 && strchr ("></=*", *(SYMBOL (p))) != NULL)
    {
      diagnostic (A_SYNTAX_ERROR, p, "monadic operator S cannot begin with character from `></=*'");
    }
  else if (k == 2 && !find_tag_global (SYMBOL_TABLE (p), PRIO_SYMBOL, SYMBOL (p)))
    {
      diagnostic (A_SYNTAX_ERROR, p, "dyadic operator S has no priority declaration");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_op_dec: enter operator declarations in the symbol table.                  */

static void
tax_op_dec (NODE_T * p, MOID_T ** m, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, OPERATOR_DECLARATION))
	{
	  tax_op_dec (SUB (p), m, access);
	  tax_op_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_op_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, OPERATOR_PLAN))
	{
	  tax_tags (SUB (p));
	  *m = MOID (p);
	  tax_op_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, OP_SYMBOL))
	{
	  tax_op_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, COMMA_SYMBOL))
	{
	  tax_op_dec (NEXT (p), m, access);
	}
      else if (WHETHER (p, DEFINING_OPERATOR))
	{
	  TAG_T *entry = SYMBOL_TABLE (p)->operators;
	  check_operator_dec (p);
	  while (entry != NULL && NODE (entry) != p)
	    {
	      entry = NEXT (entry);
	    }
	  MOID (p) = *m;
	  TAX (p) = entry;
	  HEAP (entry) = LOC_SYMBOL;
	  ACCESS (entry) = *access;
	  MOID (entry) = *m;
	  tax_op_dec (NEXT (p), m, access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_brief_op_dec: enter brief operator declarations in the symbol table.      */

static void
tax_brief_op_dec (NODE_T * p, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, BRIEF_OPERATOR_DECLARATION))
	{
	  tax_brief_op_dec (SUB (p), access);
	  tax_brief_op_dec (NEXT (p), access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_brief_op_dec (NEXT (p), access);
	}
      else if (WHETHER (p, OP_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  tax_brief_op_dec (NEXT (p), access);
	}
      else if (WHETHER (p, DEFINING_OPERATOR))
	{
	  TAG_T *entry = SYMBOL_TABLE (p)->operators;
	  MOID_T *m = MOID (NEXT (NEXT (p)));
	  check_operator_dec (p);
	  while (entry != NULL && NODE (entry) != p)
	    {
	      entry = NEXT (entry);
	    }
	  MOID (p) = m;
	  TAX (p) = entry;
	  HEAP (entry) = LOC_SYMBOL;
	  ACCESS (entry) = *access;
	  MOID (entry) = m;
	  tax_brief_op_dec (NEXT (p), access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_prio_dec: enter priority declarations in the symbol table.                */

static void
tax_prio_dec (NODE_T * p, int *access)
{
  if (p != NULL)
    {
      if (WHETHER (p, PRIORITY_DECLARATION))
	{
	  tax_prio_dec (SUB (p), access);
	  tax_prio_dec (NEXT (p), access);
	}
      else if (WHETHER (p, ACCESS))
	{
	  *access = ATTRIBUTE (SUB (p));
	  tax_prio_dec (NEXT (p), access);
	}
      else if (WHETHER (p, PRIO_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  tax_prio_dec (NEXT (p), access);
	}
      else if (WHETHER (p, DEFINING_OPERATOR))
	{
	  TAG_T *entry = PRIO (SYMBOL_TABLE (p));
	  while (entry != NULL && NODE (entry) != p)
	    {
	      entry = NEXT (entry);
	    }
	  MOID (p) = NULL;
	  TAX (p) = entry;
	  HEAP (entry) = LOC_SYMBOL;
	  ACCESS (entry) = *access;
	  tax_prio_dec (NEXT (p), access);
	}
      else
	{
	  tax_tags (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tax_tags: enter TAXes in the symbol table.                                    */

static void
tax_tags (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      int heap = LOC_SYMBOL, access = PRIVATE_SYMBOL;
      MOID_T *m = NULL;
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  tax_identity_dec (p, &m, &access);
	}
      else if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  tax_variable_dec (p, &heap, &m, &access);
	}
      else if (WHETHER (p, PROCEDURE_DECLARATION))
	{
	  tax_proc_dec (p, &access);
	}
      else if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  tax_proc_variable_dec (p, &heap, &access);
	}
      else if (WHETHER (p, OPERATOR_DECLARATION))
	{
	  tax_op_dec (p, &m, &access);
	}
      else if (WHETHER (p, BRIEF_OPERATOR_DECLARATION))
	{
	  tax_brief_op_dec (p, &access);
	}
      else if (WHETHER (p, PRIORITY_DECLARATION))
	{
	  tax_prio_dec (p, &access);
	}
      else
	{
	  tax_tags (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_symbol_table_nest_count*/

void
reset_symbol_table_nest_count (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  SYMBOL_TABLE (SUB (p))->nest = symbol_table_count++;
	}
      reset_symbol_table_nest_count (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_routine_tags_to_tree: bind routines in symbol table to the tree.
By inserting coercions etc. some may have shifted.                            */

void
bind_routine_tags_to_tree (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, ROUTINE_TEXT) && TAX (p) != NULL)
	{
	  NODE (TAX (p)) = p;
	}
      bind_routine_tags_to_tree (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_format_tags_to_tree: bind formats in symbol table to tree.
By inserting coercions etc. some may have shifted.                            */

void
bind_format_tags_to_tree (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, FORMAT_TEXT) && TAX (p) != NULL)
	{
	  NODE (TAX (p)) = p;
	}
      else if (WHETHER (p, FORMAT_DELIMITER_SYMBOL) && NEXT (p) != NULL && TAX (p) != NULL)
	{
	  NODE (TAX (p)) = p;
	}
      bind_format_tags_to_tree (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
flood_with_symbol_table_restricted: flood tree with local symbol table "s".   */

static void
flood_with_symbol_table_restricted (NODE_T * p, SYMBOL_TABLE_T * s)
{
  for (; p != NULL; p = NEXT (p))
    {
      SYMBOL_TABLE (p) = s;
      if (ATTRIBUTE (p) != ROUTINE_TEXT && ATTRIBUTE (p) != SPECIFIED_UNIT)
	{
	  if (whether_new_lexical_level (p))
	    {
	      PREVIOUS (SYMBOL_TABLE (SUB (p))) = s;
	    }
	  else
	    {
	      flood_with_symbol_table_restricted (SUB (p), s);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
finalise_symbol_table_setup: final structure of symbol table after parsing.   */

void
finalise_symbol_table_setup (NODE_T * p, int l)
{
  SYMBOL_TABLE_T *s = SYMBOL_TABLE (p);
  NODE_T *q = p;
  while (q != NULL)
    {
/* routine texts are ranges */
      if (WHETHER (q, ROUTINE_TEXT))
	{
	  flood_with_symbol_table_restricted (SUB (q), new_symbol_table (s));
	}
/* specifiers are ranges */
      else if (WHETHER (q, SPECIFIED_UNIT))
	{
	  flood_with_symbol_table_restricted (SUB (q), new_symbol_table (s));
	}
/* level count and recursion */
      if (SUB (q) != NULL)
	{
	  if (whether_new_lexical_level (q))
	    {
	      LEX_LEVEL (SUB (q)) = l + 1;
	      PREVIOUS (SYMBOL_TABLE (SUB (q))) = s;
	      finalise_symbol_table_setup (SUB (q), l + 1);
	      if (WHETHER (q, WHILE_PART))
		{
		  if ((q = NEXT (q)) == NULL)
		    {
		      return;
		    }
		  if (WHETHER (q, ALT_DO_PART))
		    {
		      LEX_LEVEL (SUB (q)) = l + 2;
		      finalise_symbol_table_setup (SUB (q), l + 2);
		    }
		}
	    }
	  else
	    {
	      SYMBOL_TABLE (SUB (q)) = s;
	      finalise_symbol_table_setup (SUB (q), l);
	    }
	}
      SYMBOL_TABLE (q) = s;
      if (WHETHER (q, FOR_SYMBOL))
	{
	  q = NEXT (q);
	}
      q = NEXT (q);
    }
/* FOR identifiers are in the DO ... OD range */
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, FOR_SYMBOL))
	{
	  SYMBOL_TABLE (NEXT (q)) = SYMBOL_TABLE (NEXT (q)->do_od_part);
	}
    }

}

/*-------1---------2---------3---------4---------5---------6---------7---------+
preliminary_symbol_table_setup: first structure of symbol table for parsing.  */

void
preliminary_symbol_table_setup (NODE_T * p)
{
  NODE_T *q;
  SYMBOL_TABLE_T *s = SYMBOL_TABLE (p);
  BOOL_T not_a_for_range = A_FALSE;
/* let the tree point to the current symbol table */
  for (q = p; q != NULL; q = NEXT (q))
    {
      SYMBOL_TABLE (q) = s;
    }
/* insert new tables when required */
  for (q = p; q != NULL && !not_a_for_range; q = NEXT (q))
    {
      if (SUB (q) != NULL)
	{
/* BEGIN ... END, CODE ... EDOC, DEF ... FED, DO ... OD, $ ... $, { ... } are ranges */
	  if (WHETHER (q, BEGIN_SYMBOL) || WHETHER (q, CODE_SYMBOL) || WHETHER (q, DEF_SYMBOL) || WHETHER (q, DO_SYMBOL) || WHETHER (q, ALT_DO_SYMBOL) || WHETHER (q, FORMAT_DELIMITER_SYMBOL) || WHETHER (q, ACCO_SYMBOL))
	    {
	      SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
	      preliminary_symbol_table_setup (SUB (q));
	    }
/* ( ... ) is a range */
	  else if (WHETHER (q, OPEN_SYMBOL))
	    {
	      if (whether (q, OPEN_SYMBOL, THEN_BAR_SYMBOL, 0))
		{
		  SYMBOL_TABLE (SUB (q)) = s;
		  preliminary_symbol_table_setup (SUB (q));
		  q = NEXT (q);
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		  if ((q = NEXT (q)) == NULL)
		    {
		      not_a_for_range = A_TRUE;
		    }
		  else
		    {
		      if (WHETHER (q, THEN_BAR_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		      if (WHETHER (q, OPEN_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		    }
		}
	      else
		{
/* don't worry about STRUCT (...), UNION (...), PROC (...) yet */
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		}
	    }
/* IF ... THEN ... ELSE ... FI are ranges */
	  else if (WHETHER (q, IF_SYMBOL))
	    {
	      if (whether (q, IF_SYMBOL, THEN_SYMBOL, 0))
		{
		  SYMBOL_TABLE (SUB (q)) = s;
		  preliminary_symbol_table_setup (SUB (q));
		  q = NEXT (q);
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		  if ((q = NEXT (q)) == NULL)
		    {
		      not_a_for_range = A_TRUE;
		    }
		  else
		    {
		      if (WHETHER (q, ELSE_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		      if (WHETHER (q, IF_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		    }
		}
	      else
		{
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		}
	    }
/* CASE ... IN ... OUT ... ESAC are ranges */
	  else if (WHETHER (q, CASE_SYMBOL))
	    {
	      if (whether (q, CASE_SYMBOL, IN_SYMBOL, 0))
		{
		  SYMBOL_TABLE (SUB (q)) = s;
		  preliminary_symbol_table_setup (SUB (q));
		  q = NEXT (q);
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		  if ((q = NEXT (q)) == NULL)
		    {
		      not_a_for_range = A_TRUE;
		    }
		  else
		    {
		      if (WHETHER (q, OUT_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		      if (WHETHER (q, CASE_SYMBOL))
			{
			  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
			  preliminary_symbol_table_setup (SUB (q));
			}
		    }
		}
	      else
		{
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (s);
		  preliminary_symbol_table_setup (SUB (q));
		}
	    }
/* WHILE ... DO ... OD are ranges */
	  else if (WHETHER (q, WHILE_SYMBOL))
	    {
	      SYMBOL_TABLE_T *u = new_symbol_table (s);
	      SYMBOL_TABLE (SUB (q)) = u;
	      preliminary_symbol_table_setup (SUB (q));
	      if ((q = NEXT (q)) == NULL)
		{
		  not_a_for_range = A_TRUE;
		}
	      else if (WHETHER (q, ALT_DO_SYMBOL))
		{
		  SYMBOL_TABLE (SUB (q)) = new_symbol_table (u);
		  preliminary_symbol_table_setup (SUB (q));
		}
	    }
	  else
	    {
	      SYMBOL_TABLE (SUB (q)) = s;
	      preliminary_symbol_table_setup (SUB (q));
	    }
	}
    }
/* FOR identifiers will go to the DO ... OD range */
  if (!not_a_for_range)
    {
      for (q = p; q != NULL; q = NEXT (q))
	{
	  if (WHETHER (q, FOR_SYMBOL))
	    {
	      NODE_T *r = q;
	      SYMBOL_TABLE (NEXT (q)) = NULL;
	      for (; r != NULL && SYMBOL_TABLE (NEXT (q)) == NULL; r = NEXT (r))
		{
		  if ((WHETHER (r, WHILE_SYMBOL) || WHETHER (r, ALT_DO_SYMBOL)) && (NEXT (q) != NULL && SUB (r) != NULL))
		    {
		      SYMBOL_TABLE (NEXT (q)) = SYMBOL_TABLE (SUB (r));
		      NEXT (q)->do_od_part = SUB (r);
		    }
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mark_mode: mark a mode as in use.                                             */

static void
mark_mode (MOID_T * m)
{
  if (m != NULL && m->use == A_FALSE)
    {
      PACK_T *p = PACK (m);
      m->use = A_TRUE;
      for (; p != NULL; p = NEXT (p))
	{
	  mark_mode (MOID (p));
	  mark_mode (SUB (m));
	  mark_mode (m->slice);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mark_moids: traverse tree and mark modes as used.                             */

void
mark_moids (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      mark_moids (SUB (p));
      if (MOID (p) != NULL)
	{
	  mark_mode (MOID (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
mark_auxilliary: mark various tags as used.                                   */

void
mark_auxilliary (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL)
	{
/* You get no warnings on unused PROC parameters. That is ok since A68 has some 
   parameters that you may not use at all - think of PROC (REF FILE) BOOL event 
   routines in transput.                           */
	  mark_auxilliary (SUB (p));
	}
      else if (WHETHER (p, OPERATOR))
	{
	  TAG_T *z;
	  if (TAX (p) != NULL)
	    {
	      TAX (p)->use = A_TRUE;
	    }
	  if ((z = find_tag_global (SYMBOL_TABLE (p), PRIO_SYMBOL, SYMBOL (p))) != NULL)
	    {
	      z->use = A_TRUE;
	    }
	}
      else if (WHETHER (p, INDICANT))
	{
	  TAG_T *z = find_tag_global (SYMBOL_TABLE (p), INDICANT, SYMBOL (p));
	  if (z != NULL)
	    {
	      TAX (p) = z;
	      z->use = A_TRUE;
	    }
	}
      else if (WHETHER (p, IDENTIFIER))
	{
	  if (TAX (p) != NULL)
	    {
	      TAX (p)->use = A_TRUE;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unused: check a single tag.                                                   */

static void
unused (TAG_T * s)
{
  for (; s != NULL; s = NEXT (s))
    {
      if (!s->use)
	{
	  diagnostic (A_WARNING, NODE (s), "#tag S is not used", NODE (s));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
warn_for_unused_tags: driver for traversing tree and warn for unused tags.    */

void
warn_for_unused_tags (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && LINE (p)->number != 0)
	{
	  if (whether_new_lexical_level (p) && ATTRIBUTE (SYMBOL_TABLE (SUB (p))) != ENVIRON_SYMBOL)
	    {
	      unused (SYMBOL_TABLE (SUB (p))->operators);
	      unused (PRIO (SYMBOL_TABLE (SUB (p))));
	      unused (SYMBOL_TABLE (SUB (p))->identifiers);
	      unused (SYMBOL_TABLE (SUB (p))->indicants);
	    }
	}
      warn_for_unused_tags (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
jumps_from_procs */

void
jumps_from_procs (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, PROCEDURING))
	{
	  NODE_T *u = SUB (SUB (p));
	  if (WHETHER (u, GOTO_SYMBOL))
	    {
	      u = NEXT (u);
	    }
	  if (u->info->PROCEDURE_NUMBER != NODE (TAX (u))->info->PROCEDURE_NUMBER)
	    {
	      PRIO (TAX (u)) = EXTERN_LABEL;
	    }
	  TAX (u)->use = A_TRUE;
	}
      else if (WHETHER (p, JUMP))
	{
	  NODE_T *u = SUB (p);
	  if (WHETHER (u, GOTO_SYMBOL))
	    {
	      u = NEXT (u);
	    }
	  if (u->info->PROCEDURE_NUMBER != NODE (TAX (u))->info->PROCEDURE_NUMBER)
	    {
	      PRIO (TAX (u)) = EXTERN_LABEL;
	    }
	  TAX (u)->use = A_TRUE;
	}
      else
	{
	  jumps_from_procs (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
assign_offset_tags */

static ADDR_T
assign_offset_tags (TAG_T * t, ADDR_T base)
{
  ADDR_T sum = base;
  for (; t != NULL; t = NEXT (t))
    {
      t->size = moid_size (MOID (t));
      if (t->value == NULL)
	{
	  t->offset = sum;
	  sum += t->size;
	}
    }
  return (sum);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
assign_offsets_table */

void
assign_offsets_table (SYMBOL_TABLE_T * c)
{
  ADDR_T k = assign_offset_tags (c->operators, 0);
  k = assign_offset_tags (c->identifiers, k);
  c->ap_increment = assign_offset_tags (c->anonymous, k);
  c->ap_increment = ALIGN (c->ap_increment);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
assign_offsets */

void
assign_offsets (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL && whether_new_lexical_level (p))
	{
	  assign_offsets_table (SYMBOL_TABLE (SUB (p)));
	}
      assign_offsets (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
assign_offsets_packs */

void
assign_offsets_packs (MOID_LIST_T * moid_list)
{
  MOID_LIST_T *q;
  for (q = moid_list; q != NULL; q = NEXT (q))
    {
      if (EQUIVALENT (MOID (q)) == NULL && WHETHER (MOID (q), STRUCT_SYMBOL))
	{
	  PACK_T *p = PACK (MOID (q));
	  ADDR_T offset = 0;
	  for (; p != NULL; p = NEXT (p))
	    {
	      p->size = moid_size (MOID (p));
	      p->offset = offset;
	      offset += p->size;
	    }
	}
    }
}
