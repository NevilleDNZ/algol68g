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

/*-------1---------2---------3---------4---------5---------6---------7---------+
Static scope checker.                                                         */

typedef struct TUPLE_T TUPLE_T;
typedef struct SCOPE_T SCOPE_T;

struct TUPLE_T
{
  int level;
  BOOL_T transient;
};

struct SCOPE_T
{
  NODE_T *where;
  TUPLE_T tuple;
  SCOPE_T *next;
};

#define NOT_TRANSIENT 0x0
#define TRANSIENT 0x1
#define STRICT 0x10

int modifications;

static void format_environ (NODE_T *);
static void scan_format_environ (NODE_T *, SCOPE_T **);
static void proc_environ (NODE_T *);
static void scan_proc_environ (NODE_T *, SCOPE_T **);
static void scope_unit (NODE_T *, SCOPE_T **);
static void scope_enclosed_clause (NODE_T *, SCOPE_T **);
static void scope_formula (NODE_T *, SCOPE_T **);

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_make_tuple.                                                             */

static TUPLE_T
scope_make_tuple (int e, int t)
{
  TUPLE_T z;
  z.level = e;
  z.transient = t;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_add: link scope information into the list.                              */

static void
scope_add (SCOPE_T ** sl, NODE_T * p, TUPLE_T tup)
{
  if (sl != NULL)
    {
      SCOPE_T *ns = (SCOPE_T *) get_temp_heap_space ((unsigned) SIZE_OF (SCOPE_T));
      ns->where = p;
      ns->tuple = tup;
      NEXT (ns) = *sl;
      *sl = ns;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_check.                                                                  */

static BOOL_T
scope_check (SCOPE_T * top, int mask, int dest)
{
  SCOPE_T *s;
  int errors = 0;
  if (dest == PRIMAL_SCOPE)
    {
      return (A_TRUE);
    }
/* Transient names cannot be stored. */
  if (mask & TRANSIENT)
    {
      for (s = top; s != NULL; s = NEXT (s))
	{
	  if ((s->tuple.transient & TRANSIENT) && !s->where->error)
	    {
	      diagnostic (A_ERROR, s->where, "attempt to store transient name");
	      s->where->error = A_TRUE;
	      errors++;
	    }
	}
    }
/* Scope violations. */
  if (errors == 0)
    {
      int sum = 0;
      for (s = top; s != NULL; s = NEXT (s))
	{
	  sum++;
	  if (dest < s->tuple.level)
	    {
	      errors++;
	    }
	}
      for (s = top; s != NULL; s = NEXT (s))
	{
	  if (dest < s->tuple.level && !s->where->error)
	    {
	      int sev = (sum == errors || mask & STRICT ? A_ERROR : A_WARNING);
	      if (MOID (s->where) == NULL)
		{
		  diagnostic (sev, s->where, "A violates scope rule", ATTRIBUTE (s->where));
		}
	      else
		{
		  diagnostic (sev, s->where, "M A violates scope rule", MOID (s->where), ATTRIBUTE (s->where));
		}
	      s->where->error = A_TRUE;
	    }
	}
    }
  return (errors == 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_find_youngest: yield youngest scope.                                    */

static TUPLE_T
scope_find_youngest (SCOPE_T * s)
{
  TUPLE_T z = scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT);
  for (; s != NULL; s = NEXT (s))
    {
      if (s->tuple.level > z.level)
	{
	  z = s->tuple;
	}
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_find_youngest_outside: for scope of routine texts.                      */

static TUPLE_T
scope_find_youngest_outside (SCOPE_T * s, int treshold)
{
  TUPLE_T z = scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT);
  for (; s != NULL; s = NEXT (s))
    {
      if (s->tuple.level > z.level && s->tuple.level <= treshold)
	{
	  z = s->tuple;
	}
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_format_environ: drive scope checks in format texts.                      */

static void
scan_format_environ (NODE_T * p, SCOPE_T ** s)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case FORMAT_TEXT:
	  {
	    format_environ (p);
	    scope_add (s, p, scope_make_tuple (TAX (p)->youngest_environ, NOT_TRANSIENT));
	    break;
	  }
	case ROUTINE_TEXT:
	  {
	    proc_environ (p);
	    scope_add (s, p, scope_make_tuple (TAX (p)->youngest_environ, NOT_TRANSIENT));
	    break;
	  }
	case IDENTIFIER:
	case OPERATOR:
	  {
	    if (TAX (p) != NULL)
	      {
		scope_add (s, p, scope_make_tuple (LEX_LEVEL (TAX (p)), NOT_TRANSIENT));
	      }
	    break;
	  }
	default:
	  {
	    scan_format_environ (SUB (p), s);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
format_environ: drive scope checks in format text.                            */

static void
format_environ (NODE_T * p)
{
  SCOPE_T *s = NULL;
  scan_format_environ (SUB (p), &s);
  TAX (p)->youngest_environ = scope_find_youngest_outside (s, LEX_LEVEL (p)).level;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_format_environs: drive scope checks in format text.                       */

static void
get_format_environs (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, FORMAT_TEXT))
	{
	  format_environ (p);
	}
      get_format_environs (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_scope_to_format_tag: assign scope to tag.                                */

static void
bind_scope_to_format_tag (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, DEFINING_IDENTIFIER) && MOID (p) == MODE (FORMAT))
	{
	  if (WHETHER (NEXT (NEXT (p)), FORMAT_TEXT))
	    {
	      TAX (p)->scope = TAX (NEXT (NEXT (p)))->youngest_environ;
	      TAX (p)->scope_assigned = A_TRUE;
	    }
	  return;
	}
      else
	{
	  bind_scope_to_format_tag (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_format_environs: assign scope to tag.                                    */

static void
bind_format_environs (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  bind_scope_to_format_tag (SUB (p));
	}
      else
	{
	  bind_format_environs (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_proc_elements.                                                            */

static void
get_proc_elements (NODE_T * p, SCOPE_T ** r, BOOL_T no_ref)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case BOUNDS:
	  {
	    break;
	  }
	case INDICANT:
	  {
	    if (MOID (p) != NULL && TAX (p) != NULL && MOID (p)->has_rows && no_ref)
	      {
		scope_add (r, p, scope_make_tuple (LEX_LEVEL (TAX (p)), NOT_TRANSIENT));
	      }
	    break;
	  }
	case REF_SYMBOL:
	  {
	    get_proc_elements (NEXT (p), r, A_FALSE);
	    break;
	  }
	case PROC_SYMBOL:
	case UNION_SYMBOL:
	  {
	    /* FORMAL_DECLARER_MARK declarers */
	    break;
	  }
	default:
	  {
	    get_proc_elements (SUB (p), r, no_ref);
	    get_proc_elements (NEXT (p), r, no_ref);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_proc_environ.                                                            */

static void
scan_proc_environ (NODE_T * p, SCOPE_T ** s)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case ROUTINE_TEXT:
	  {
	    proc_environ (p);
	    scope_add (s, p, scope_make_tuple (TAX (p)->youngest_environ, NOT_TRANSIENT));
	    break;
	  }
	case FORMAT_TEXT:
	  {
	    format_environ (p);
	    scope_add (s, p, scope_make_tuple (TAX (p)->youngest_environ, NOT_TRANSIENT));
	    break;
	  }
	case IDENTIFIER:
	case OPERATOR:
	  {
	    if (TAX (p) != NULL)
	      {
		scope_add (s, p, scope_make_tuple (LEX_LEVEL (TAX (p)), NOT_TRANSIENT));
	      }
	    break;
	  }
	case DECLARER:
	  {
	    get_proc_elements (p, s, A_TRUE);
	    break;
	  }
	default:
	  {
	    scan_proc_environ (SUB (p), s);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
proc_environ.                                                                 */

static void
proc_environ (NODE_T * p)
{
  SCOPE_T *s = NULL;
  scan_proc_environ (SUB (p), &s);
  TAX (p)->youngest_environ = scope_find_youngest_outside (s, LEX_LEVEL (p)).level;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_proc_environs.                                                            */

static void
get_proc_environs (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, ROUTINE_TEXT))
	{
	  proc_environ (p);
	}
      get_proc_environs (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_scope_to_routine_tag.                                                    */

static void
bind_scope_to_routine_tag (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  if (WHETHER (NEXT (NEXT (p)), ROUTINE_TEXT))
	    {
	      TAX (p)->scope = TAX (NEXT (NEXT (p)))->youngest_environ;
	      TAX (p)->scope_assigned = A_TRUE;
	    }
	  return;
	}
      else
	{
	  bind_scope_to_routine_tag (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bind_proc_environs.                                                           */

static void
bind_proc_environs (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, PROCEDURE_DECLARATION))
	{
	  bind_scope_to_routine_tag (SUB (p));
	}
      else
	{
	  bind_proc_environs (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_bounds.                                                                 */

static void
scope_bounds (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  scope_unit (p, NULL);
	}
      else
	{
	  scope_bounds (SUB (p));
	}
      scope_bounds (NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_declarer.                                                               */

static void
scope_declarer (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, BOUNDS))
	{
	  scope_bounds (SUB (p));
	}
      else if (WHETHER (p, INDICANT))
	{
	  ;
	}
      else if (WHETHER (p, REF_SYMBOL))
	{
	  scope_declarer (NEXT (p));
	}
      else if (WHETHER (p, PROC_SYMBOL) || WHETHER (p, UNION_SYMBOL))
	{
	  ;
	}
      else
	{
	  scope_declarer (SUB (p));
	  scope_declarer (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_identifier_usage.                                                       */

static void
check_identifier_usage (TAG_T * t, NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, IDENTIFIER) && TAX (p) == t && ATTRIBUTE (MOID (t)) != PROC_SYMBOL)
	{
	  diagnostic (A_ERROR, p, "identifier S might be used uninitialised");
	}
      check_identifier_usage (t, SUB (p));
      check_identifier_usage (t, NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_identity_declaration.                                                   */

static void
scope_identity_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  ;
	}
      else if (WHETHER (p, DECLARER))
	{
	  scope_identity_declaration (NEXT (p));
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  NODE_T *unit = NEXT (NEXT (p));
	  SCOPE_T *s = NULL;
	  if (ATTRIBUTE (MOID (TAX (p))) != PROC_SYMBOL)
	    {
	      check_identifier_usage (TAX (p), unit);
	    }
	  scope_unit (unit, &s);
	  scope_check (s, TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else
	{
	  scope_identity_declaration (SUB (p));
	  scope_identity_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_variable_declaration.                                                   */

static void
scope_variable_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  ;
	}
      else if (WHETHER (p, DECLARER))
	{
	  scope_declarer (SUB (p));
	  scope_variable_declaration (NEXT (p));
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  if (whether (p, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0))
	    {
	      NODE_T *unit = NEXT (NEXT (p));
	      SCOPE_T *s = NULL;
	      check_identifier_usage (TAX (p), unit);
	      scope_unit (unit, &s);
	      scope_check (s, TRANSIENT | STRICT, LEX_LEVEL (p));
	    }
	}
      else
	{
	  scope_variable_declaration (SUB (p));
	  scope_variable_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_routine_text.                                                           */

static void
scope_routine_text (NODE_T * p, SCOPE_T ** s)
{
  NODE_T *q = SUB (p), *routine = WHETHER (q, PARAMETER_PACK) ? NEXT (q) : q;
  SCOPE_T *x = NULL;
  TUPLE_T routine_tuple;
  scope_unit (NEXT (NEXT (routine)), &x);
  scope_check (x, TRANSIENT | STRICT, LEX_LEVEL (p));
  routine_tuple = scope_make_tuple (TAX (p)->youngest_environ, NOT_TRANSIENT);
  scope_add (s, p, routine_tuple);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_procedure_var_declaration.                                              */

static void
scope_procedure_var_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  ;
	}
      if (WHETHER (p, ROUTINE_TEXT))
	{
	  SCOPE_T *s = NULL;
	  scope_routine_text (p, &s);
	  scope_check (s, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else
	{
	  scope_procedure_var_declaration (SUB (p));
	  scope_procedure_var_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_procedure_declaration.                                                  */

static void
scope_procedure_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, PROCEDURE_DECLARATION))
	{
	   /**/;
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER))
	{
	  SCOPE_T *s = NULL;
	  scope_routine_text (NEXT (NEXT (p)), &s);
	  scope_check (s, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	  if (!TAX (p)->scope_assigned)
	    {
	      TAX (p)->scope = scope_find_youngest (s).level;
	      TAX (p)->scope_assigned = A_TRUE;
	      modifications++;
	    }
	}
      else
	{
	  scope_procedure_declaration (SUB (p));
	  scope_procedure_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_operator_declaration.                                                   */

static void
scope_operator_declaration (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, BRIEF_OPERATOR_DECLARATION) || WHETHER (p, OPERATOR_DECLARATION))
	{
	   /**/;
	}
      else if (WHETHER (p, DEFINING_OPERATOR))
	{
	  SCOPE_T *s = NULL;
	  scope_unit (NEXT (NEXT (p)), &s);
	  scope_check (s, TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else
	{
	  scope_operator_declaration (SUB (p));
	  scope_operator_declaration (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_declaration_list.                                                       */

static void
scope_declaration_list (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  scope_identity_declaration (SUB (p));
	}
      else if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  scope_variable_declaration (SUB (p));
	}
      else if (WHETHER (p, MODE_DECLARATION))
	{
	  scope_declarer (SUB (p));
	}
      else if (WHETHER (p, PRIORITY_DECLARATION))
	{
	  ;
	}
      else if (WHETHER (p, PROCEDURE_DECLARATION))
	{
	  scope_procedure_declaration (SUB (p));
	}
      else if (WHETHER (p, PROCEDURE_VARIABLE_DECLARATION))
	{
	  scope_procedure_var_declaration (SUB (p));
	}
      else if (WHETHER (p, BRIEF_OPERATOR_DECLARATION) || WHETHER (p, OPERATOR_DECLARATION))
	{
	  scope_operator_declaration (SUB (p));
	}
      else
	{
	  scope_declaration_list (SUB (p));
	  scope_declaration_list (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_serial_clause.                                                          */

static void
scope_serial_clause (NODE_T * p, SCOPE_T ** s, BOOL_T terminator)
{
  if (p != NULL)
    {
      if (WHETHER (p, INITIALISER_SERIES))
	{
	  scope_serial_clause (SUB (p), s, A_FALSE);
	  scope_serial_clause (NEXT (p), s, terminator);
	}
      else if (WHETHER (p, DECLARATION_LIST))
	{
	  scope_declaration_list (SUB (p));
	}
      else if (WHETHER (p, LABEL) || WHETHER (p, SEMI_SYMBOL) || WHETHER (p, EXIT_SYMBOL))
	{
	  scope_serial_clause (NEXT (p), s, terminator);
	}
      else if (WHETHER (p, SERIAL_CLAUSE) || WHETHER (p, ENQUIRY_CLAUSE))
	{
	  if (NEXT (p) != NULL)
	    {
	      int j = ATTRIBUTE (NEXT (p));
	      if (j == EXIT_SYMBOL || j == END_SYMBOL || j == CLOSE_SYMBOL)
		{
		  scope_serial_clause (SUB (p), s, A_TRUE);
		}
	      else
		{
		  scope_serial_clause (SUB (p), s, A_FALSE);
		}
	    }
	  else
	    {
	      scope_serial_clause (SUB (p), s, A_TRUE);
	    }
	  scope_serial_clause (NEXT (p), s, terminator);
	}
      else if (WHETHER (p, LABELED_UNIT))
	{
	  scope_serial_clause (SUB (p), s, terminator);
	}
      else if (WHETHER (p, UNIT))
	{
	  if (terminator)
	    {
	      scope_unit (p, s);
	    }
	  else
	    {
	      scope_unit (p, NULL);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_closed_clause.                                                          */

static void
scope_closed_clause (NODE_T * p, SCOPE_T ** s)
{
  if (p != NULL)
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  scope_serial_clause (p, s, A_TRUE);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, BEGIN_SYMBOL))
	{
	  scope_closed_clause (NEXT (p), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_import_clause.                                                          */

static void
scope_import_clause (NODE_T * p, SCOPE_T ** s)
{
  if (p != NULL)
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  scope_serial_clause (p, s, A_TRUE);
	}
      else
	{
	  scope_import_clause (NEXT (p), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_export_clause.                                                          */

static void
scope_export_clause (NODE_T * p, SCOPE_T ** s)
{
  if (p != NULL)
    {
      if (WHETHER (p, SERIAL_CLAUSE))
	{
	  scope_serial_clause (p, s, A_TRUE);
	}
      else
	{
	  scope_export_clause (NEXT (p), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_arguments.                                                              */

static void
scope_arguments (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  SCOPE_T *s = NULL;
	  scope_unit (p, &s);
	  scope_check (s, TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else
	{
	  scope_arguments (SUB (p));
	}
      scope_arguments (NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_transient_row.                                                        */

static BOOL_T
whether_transient_row (MOID_T * m)
{
  if (WHETHER (m, REF_SYMBOL))
    {
      return (WHETHER (SUB (m), FLEX_SYMBOL));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_coercion.                                                             */

BOOL_T
whether_coercion (NODE_T * p)
{
  if (p != NULL)
    {
      switch (ATTRIBUTE (p))
	{
	case DEPROCEDURING:
	case DEREFERENCING:
	case UNITING:
	case ROWING:
	case WIDENING:
	case VOIDING:
	case PROCEDURING:
	  {
	    return (A_TRUE);
	  }
	default:
	  {
	    return (A_FALSE);
	  }
	}
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_coercion.                                                               */

static void
scope_coercion (NODE_T * p, SCOPE_T ** s)
{
  if (whether_coercion (p))
    {
      if (WHETHER (p, VOIDING))
	{
	  scope_coercion (SUB (p), NULL);
	}
      else if (WHETHER (p, DEREFERENCING))
	{
	  if (WHETHER (MOID (p), REF_SYMBOL))
	    {
	      scope_coercion (SUB (p), s);
	    }
	  else
	    {
	      scope_coercion (SUB (p), NULL);
	    }
	}
      else if (WHETHER (p, DEPROCEDURING))
	{
	  scope_coercion (SUB (p), NULL);
	}
      else if (WHETHER (p, ROWING))
	{
	  scope_coercion (SUB (p), s);
	  if (whether_transient_row (MOID (SUB (p))))
	    {
	      scope_add (s, p, scope_make_tuple (LEX_LEVEL (p), TRANSIENT));
	    }
	}
      else if (WHETHER (p, PROCEDURING))
	{
/* Can only be a JUMP */
	  NODE_T *q = SUB (SUB (p));
	  if (WHETHER (q, GOTO_SYMBOL))
	    {
	      q = NEXT (q);
	    }
	  scope_add (s, q, scope_make_tuple (LEX_LEVEL (TAX (q)), NOT_TRANSIENT));
	}
      else
	{
	  scope_coercion (SUB (p), s);
	}
    }
  else
    {
      scope_unit (p, s);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_format_text.                                                            */

static void
scope_format_text (NODE_T * p, SCOPE_T ** s)
{
  for (; p != NULL; p = NEXT (p))
    {
      scope_format_text (SUB (p), s);
      if (WHETHER (p, FORMAT_PATTERN))
	{
	  scope_enclosed_clause (SUB (NEXT_SUB (p)), s);
	}
      else if (WHETHER (p, FORMAT_ITEM_G) && NEXT (p) != NULL)
	{
	  scope_enclosed_clause (SUB (NEXT (p)), s);
	}
      else if (WHETHER (p, DYNAMIC_REPLICATOR))
	{
	  scope_enclosed_clause (SUB (NEXT_SUB (p)), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_primary.                                                                */

static void
scope_primary (NODE_T * p, SCOPE_T ** s)
{
  if (whether_coercion (p))
    {
      scope_coercion (p, s);
    }
  else
    {
      if (WHETHER (p, PRIMARY))
	{
	  scope_primary (SUB (p), s);
	}
      else if (WHETHER (p, DENOTER))
	{
	  scope_add (s, p, scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT));
	}
      else if (WHETHER (p, IDENTIFIER))
	{
	  if (WHETHER (MOID (p), REF_SYMBOL))
	    {
	      if (PRIO (TAX (p)) == PARAMETER_IDENTIFIER)
		{
		  scope_add (s, p, scope_make_tuple (LEX_LEVEL (TAX (p)) - 1, NOT_TRANSIENT));
		}
	      else
		{
		  if (HEAP (TAX (p)) == LOC_SYMBOL)
		    {
		      scope_add (s, p, scope_make_tuple (LEX_LEVEL (TAX (p)), NOT_TRANSIENT));
		    }
		  else
		    {
		      scope_add (s, p, scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT));
		    }
		}
	    }
	  else if (ATTRIBUTE (MOID (p)) == PROC_SYMBOL && TAX (p)->scope_assigned == A_TRUE)
	    {
	      scope_add (s, p, scope_make_tuple (TAX (p)->scope, NOT_TRANSIENT));
	    }
	  else if (MOID (p) == MODE (FORMAT) && TAX (p)->scope_assigned == A_TRUE)
	    {
	      scope_add (s, p, scope_make_tuple (TAX (p)->scope, NOT_TRANSIENT));
	    }
	}
      else if (WHETHER (p, ENCLOSED_CLAUSE))
	{
	  scope_enclosed_clause (SUB (p), s);
	}
      else if (WHETHER (p, CALL))
	{
	  SCOPE_T *x = NULL;
	  scope_primary (SUB (p), &x);
	  scope_check (x, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	  scope_arguments (NEXT_SUB (p));
	}
      else if (WHETHER (p, SLICE))
	{
	  SCOPE_T *x = NULL;
	  MOID_T *m = MOID (SUB (p));
	  if (WHETHER (m, REF_SYMBOL))
	    {
	      if (ATTRIBUTE (SUB (p)) == PRIMARY && ATTRIBUTE (SUB (SUB (p))) == SLICE)
		{
		  scope_primary (SUB (p), s);
		}
	      else
		{
		  scope_primary (SUB (p), &x);
		  scope_check (x, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
		}
	      if (WHETHER (SUB (m), FLEX_SYMBOL))
		{
		  scope_add (s, SUB (p), scope_make_tuple (LEX_LEVEL (p), TRANSIENT));
		}
	      scope_bounds (SUB (NEXT_SUB (p)));
	    }
	  if (WHETHER (MOID (p), REF_SYMBOL))
	    {
	      scope_add (s, p, scope_find_youngest (x));
	    }
	}
      else if (WHETHER (p, FORMAT_TEXT))
	{
	  SCOPE_T *x = NULL;
	  scope_format_text (SUB (p), &x);
	  scope_add (s, p, scope_find_youngest (x));
	}
      else if (WHETHER (p, CAST))
	{
	  SCOPE_T *x = NULL;
	  scope_enclosed_clause (SUB (NEXT_SUB (p)), &x);
	  scope_check (x, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_transient_selection.                                                  */

static BOOL_T
whether_transient_selection (MOID_T * m)
{
  if (WHETHER (m, REF_SYMBOL))
    {
      return (whether_transient_selection (SUB (m)));
    }
  else
    {
      return (WHETHER (m, FLEX_SYMBOL));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_secondary.                                                              */

static void
scope_secondary (NODE_T * p, SCOPE_T ** s)
{
  if (whether_coercion (p))
    {
      scope_coercion (p, s);
    }
  else
    {
      if (WHETHER (p, SECONDARY))
	{
	  scope_secondary (SUB (p), s);
	}
      else if (WHETHER (p, SELECTION))
	{
	  SCOPE_T *ns = NULL;
	  scope_secondary (NEXT_SUB (p), &ns);
	  scope_check (ns, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	  if (whether_transient_selection (MOID (NEXT_SUB (p))))
	    {
	      scope_add (s, p, scope_make_tuple (LEX_LEVEL (p), TRANSIENT));
	    }
	  scope_add (s, p, scope_find_youngest (ns));
	}
      else if (WHETHER (p, GENERATOR))
	{
	  if (WHETHER (SUB (p), LOC_SYMBOL))
	    {
	      scope_add (s, p, scope_make_tuple (LEX_LEVEL (p), NOT_TRANSIENT));
	    }
	  else
	    {
	      scope_add (s, p, scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT));
	    }
	  scope_declarer (SUB (NEXT_SUB (p)));
	}
      else
	{
	  scope_primary (p, s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_operand.                                                                */

static void
scope_operand (NODE_T * p, SCOPE_T ** s)
{
  SCOPE_T *x = NULL;
  if (WHETHER (p, MONADIC_FORMULA))
    {
      scope_operand (NEXT_SUB (p), &x);
    }
  else if (WHETHER (p, FORMULA))
    {
      scope_formula (p, &x);
    }
  else if (WHETHER (p, SECONDARY))
    {
      scope_secondary (SUB (p), &x);
      scope_check (x, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
    }
  if (WHETHER (MOID (p), REF_SYMBOL))
    {
      scope_add (s, p, scope_find_youngest (x));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_formula.                                                                */

static void
scope_formula (NODE_T * p, SCOPE_T ** s)
{
  NODE_T *q = SUB (p);
  SCOPE_T *s2 = NULL;
  int youngest;
  scope_operand (q, &s2);
  youngest = scope_find_youngest (s2).level;
  if (NEXT (q) != NULL)
    {
      SCOPE_T *s3 = NULL;
      int youngest2;
      scope_operand (NEXT (NEXT (q)), &s3);
      youngest2 = scope_find_youngest (s3).level;
      if (youngest2 > youngest)
	{
	  youngest = youngest2;
	}
    }
  if (WHETHER (MOID (p), REF_SYMBOL))
    {
      scope_add (s, p, scope_make_tuple (youngest, NOT_TRANSIENT));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_tertiary.                                                               */

static void
scope_tertiary (NODE_T * p, SCOPE_T ** s)
{
  if (whether_coercion (p))
    {
      scope_coercion (p, s);
    }
  else
    {
      if (WHETHER (p, TERTIARY))
	{
	  scope_tertiary (SUB (p), s);
	}
      else if (WHETHER (p, FORMULA))
	{
	  scope_formula (p, s);
	}
      else if (WHETHER (p, NIHIL))
	{
	  scope_add (s, p, scope_make_tuple (PRIMAL_SCOPE, NOT_TRANSIENT));
	}
      else
	{
	  scope_secondary (p, s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_unit.                                                                   */

static void
scope_unit (NODE_T * p, SCOPE_T ** s)
{
  if (whether_coercion (p))
    {
      scope_coercion (p, s);
    }
  else
    {
      if (WHETHER (p, UNIT))
	{
	  scope_unit (SUB (p), s);
	}
      else if (WHETHER (p, ASSIGNATION))
	{
	  NODE_T *unit = NEXT (NEXT_SUB (p));
	  SCOPE_T *ns = NULL;
	  int dest;
	  scope_tertiary (SUB (SUB (p)), &ns);
	  if (scope_check (ns, NOT_TRANSIENT | STRICT, LEX_LEVEL (p)))
	    {
	      dest = scope_find_youngest (ns).level;
	    }
	  else
	    {
	      dest = LEX_LEVEL (p);
	    }
	  ns = NULL;
	  scope_unit (unit, &ns);
	  scope_check (ns, TRANSIENT | STRICT, dest);
	  scope_add (s, p, scope_make_tuple (dest, NOT_TRANSIENT));
	}
      else if (WHETHER (p, ROUTINE_TEXT))
	{
	  scope_routine_text (p, s);
	}
      else if (WHETHER (p, IDENTITY_RELATION))
	{
	  SCOPE_T *n = NULL;
	  scope_tertiary (SUB (p), &n);
	  scope_tertiary (NEXT (NEXT_SUB (p)), &n);
	  scope_check (n, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else if (WHETHER (p, AND_FUNCTION))
	{
	  SCOPE_T *n = NULL;
	  scope_tertiary (SUB (p), &n);
	  scope_tertiary (NEXT (NEXT_SUB (p)), &n);
	  scope_check (n, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else if (WHETHER (p, OR_FUNCTION))
	{
	  SCOPE_T *n = NULL;
	  scope_tertiary (SUB (p), &n);
	  scope_tertiary (NEXT (NEXT_SUB (p)), &n);
	  scope_check (n, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else if (WHETHER (p, ASSERTION))
	{
	  SCOPE_T *n = NULL;
	  scope_enclosed_clause (SUB (NEXT_SUB (p)), &n);
	  scope_check (n, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
	}
      else if (WHETHER (p, JUMP))
	{
	  ;
	}
      else if (ATTRIBUTE (p) != JUMP && ATTRIBUTE (p) != SKIP)
	{
	  scope_tertiary (p, s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_unit_list.                                                              */

static void
scope_unit_list (NODE_T * p, SCOPE_T ** s)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNIT))
	{
	  scope_unit (p, s);
	}
      else
	{
	  scope_unit_list (SUB (p), s);
	}
      scope_unit_list (NEXT (p), s);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_collateral_clause.                                                      */

static void
scope_collateral_clause (NODE_T * p, SCOPE_T ** s)
{
  if (p != NULL)
    {
      if (!(whether (p, BEGIN_SYMBOL, END_SYMBOL, 0) || whether (p, OPEN_SYMBOL, CLOSE_SYMBOL, 0)))
	{
	  scope_unit_list (p, s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_conditional_clause.                                                     */

static void
scope_conditional_clause (NODE_T * p, SCOPE_T ** s)
{
  scope_serial_clause (NEXT_SUB (p), NULL, A_TRUE);
  p = NEXT (p);
  scope_serial_clause (NEXT_SUB (p), s, A_TRUE);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, ELSE_PART) || WHETHER (p, CHOICE))
	{
	  scope_serial_clause (NEXT_SUB (p), s, A_TRUE);
	}
      else if (WHETHER (p, ELIF_PART) || WHETHER (p, BRIEF_ELIF_IF_PART))
	{
	  scope_conditional_clause (SUB (p), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_case.                                                                   */

static void
scope_case (NODE_T * p, SCOPE_T ** s)
{
  SCOPE_T *n = NULL;
  scope_serial_clause (NEXT_SUB (p), &n, A_TRUE);
  scope_check (n, NOT_TRANSIENT | STRICT, LEX_LEVEL (p));
  p = NEXT (p);
  scope_unit_list (NEXT_SUB (p), s);
  if ((p = NEXT (p)) != NULL)
    {
      if (WHETHER (p, OUT_PART) || WHETHER (p, CHOICE))
	{
	  scope_serial_clause (NEXT_SUB (p), s, A_TRUE);
	}
      else if (WHETHER (p, INTEGER_OUT_PART) || WHETHER (p, BRIEF_INTEGER_OUSE_PART))
	{
	  scope_case (SUB (p), s);
	}
      else if (WHETHER (p, UNITED_OUSE_PART) || WHETHER (p, BRIEF_UNITED_OUSE_PART))
	{
	  scope_case (SUB (p), s);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_loop_clause.                                                            */

static void
scope_loop_clause (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, FOR_PART))
	{
	  scope_loop_clause (NEXT (p));
	}
      else if (WHETHER (p, FROM_PART) || WHETHER (p, BY_PART) || WHETHER (p, TO_PART))
	{
	  scope_unit (NEXT_SUB (p), NULL);
	  scope_loop_clause (NEXT (p));
	}
      else if (WHETHER (p, WHILE_PART))
	{
	  scope_serial_clause (NEXT_SUB (p), NULL, A_TRUE);
	  scope_loop_clause (NEXT (p));
	}
      else if (WHETHER (p, DO_PART) || WHETHER (p, ALT_DO_PART))
	{
	  scope_serial_clause (NEXT_SUB (p), NULL, A_TRUE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_enclosed_clause.                                                        */

static void
scope_enclosed_clause (NODE_T * p, SCOPE_T ** s)
{
  if (WHETHER (p, ENCLOSED_CLAUSE))
    {
      scope_enclosed_clause (SUB (p), s);
    }
  else if (WHETHER (p, CLOSED_CLAUSE))
    {
      scope_closed_clause (SUB (p), s);
    }
  else if (WHETHER (p, COLLATERAL_CLAUSE))
    {
      scope_collateral_clause (SUB (p), s);
    }
  else if (WHETHER (p, PARALLEL_CLAUSE))
    {
      scope_collateral_clause (NEXT_SUB (p), s);
    }
  else if (WHETHER (p, CONDITIONAL_CLAUSE))
    {
      scope_conditional_clause (SUB (p), s);
    }
  else if (WHETHER (p, INTEGER_CASE_CLAUSE) || WHETHER (p, UNITED_CASE_CLAUSE))
    {
      scope_case (SUB (p), s);
    }
  else if (WHETHER (p, LOOP_CLAUSE))
    {
      scope_loop_clause (SUB (p));
    }
  else if (WHETHER (p, CODE_CLAUSE))
    {
      scope_import_clause (SUB (p), s);
    }
  else if (WHETHER (p, EXPORT_CLAUSE))
    {
      scope_export_clause (SUB (p), s);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scope_checker.                                                                */

void
scope_checker (NODE_T * p)
{
  get_proc_environs (p);
  get_format_environs (p);
  bind_proc_environs (p);
  bind_format_environs (p);
  do
    {
      modifications = 0;
      scope_enclosed_clause (SUB (p), NULL);
    }
  while (modifications);
}
