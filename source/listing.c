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

#define SHOW_EQ A_FALSE
#define LINE_LENGTH 100

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_level: fill out lexical level of TAXes.                                   */

void
get_level (NODE_T * t)
{
  for (; t != NULL; t = NEXT (t))
    {
      if (SUB (t) != NULL)
	{
	  get_level (SUB (t));
	}
      switch (ATTRIBUTE (t))
	{
	case DENOTER:
	case IDENTIFIER:
	case DEFINING_IDENTIFIER:
	case FIELD_IDENTIFIER:
/*      case LABEL_IDENTIFIER: */
	case INDICANT:
	case DEFINING_INDICANT:
	case NIHIL:
	case OPERATOR:
	case DEFINING_OPERATOR:
	case SKIP:
	  {
	    if (t->info->PROCEDURE_LEVEL < LINE (t)->min_proc_level)
	      {
		LINE (t)->min_proc_level = t->info->PROCEDURE_LEVEL;
	      }
	    if (t->info->PROCEDURE_LEVEL > LINE (t)->max_proc_level)
	      {
		LINE (t)->max_proc_level = t->info->PROCEDURE_LEVEL;
	      }
	    if (SYMBOL_TABLE (t) != NULL && (LEX_LEVEL (t) < LINE (t)->min_level))
	      {
		LINE (t)->min_level = LEX_LEVEL (t);
	      }
	    if (SYMBOL_TABLE (t) != NULL && (LEX_LEVEL (t) > LINE (t)->max_level))
	      {
		LINE (t)->max_level = LEX_LEVEL (t);
	      }
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
number_proc_levels: number procedure levels for listing.                      */

void
number_proc_levels (NODE_T * p, int l)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, ROUTINE_TEXT) || WHETHER (p, PROCEDURING))
	{
	  number_proc_levels (SUB (p), l + 1);
	}
      else
	{
	  number_proc_levels (SUB (p), l);
	}
      p->info->PROCEDURE_LEVEL = l;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
number_this_proc: start numbering a new procedure level.                      */

static void
number_this_proc (NODE_T * p, int k)
{
  if (p != NULL && ATTRIBUTE (p) != ROUTINE_TEXT && ATTRIBUTE (p) != PROCEDURING)
    {
      p->info->PROCEDURE_NUMBER = k;
      number_this_proc (SUB (p), k);
      number_this_proc (NEXT (p), k);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
number_procs: driver for numbering procedure levels.                          */

void
number_procs (NODE_T * p, int *k)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, ROUTINE_TEXT) || WHETHER (p, PROCEDURING))
	{
	  (*k)++;
	  number_this_proc (SUB (p), *k);
	  p->info->PROCEDURE_NUMBER = *k;
	}
      number_procs (SUB (p), k);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
own_strncpy: copy max n chars of s to d.
Returns pointer to char after last char copied.                               */

static char *
own_strncpy (char *d, char *s, int n)
{
  while (*s != '\0' && (n--) != 0)
    {
      *(d++) = *(s++);
    }
  *d = '\0';
  return (s);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_numbers*/

static char *
make_numbers (int a, int b)
{
  char z[3];
  strcpy (z, "   ");
  if (a == 0 && b == 0)
    {
      return (new_string (z));
    }
  else
    {
      if (a == b && a != 0)
	{
	  z[1] = digit_to_char (a);
	}
      else
	{
	  if (a != INT_MAX)
	    {
	      if (a == 0)
		{
		  z[1] = digit_to_char (b);
		}
	      else
		{
		  z[0] = digit_to_char (a);
		  z[1] = '-';
		  z[2] = digit_to_char (b);
		}
	    }
	}
    }
  return (new_string (z));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
brief_mode_string*/

static char *
brief_mode_string (MOID_T * p)
{
  static char q[32];
  sprintf (q, "MODE#%X", p->number);
  return (new_string (q));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
brief_mode_flat */

static void
brief_mode_flat (FILE_T f, MOID_T * z)
{
  if (WHETHER (z, STANDARD) || WHETHER (z, INDICANT))
    {
      int i = z->dimensions;
      if (i > 0)
	{
	  while (i--)
	    {
	      io_write_string (f, "LONG ");
	    }
	}
      else if (i < 0)
	{
	  while (i++)
	    {
	      io_write_string (f, "SHORT ");
	    }
	}
      sprintf (output_line, "%s", SYMBOL (NODE (z)));
      io_write_string (f, output_line);
    }
  else
    {
      sprintf (output_line, "%s", brief_mode_string (z));
      io_write_string (f, output_line);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
brief_fields_flat */

static void
brief_fields_flat (FILE_T f, PACK_T * pack)
{
  if (pack != NULL)
    {
      brief_mode_flat (f, MOID (pack));
      if (NEXT (pack) != NULL)
	{
	  sprintf (output_line, ", ");
	  io_write_string (f, output_line);
	  brief_fields_flat (f, NEXT (pack));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
brief_moid_flat */

static void
brief_moid_flat (FILE_T f, MOID_T * z)
{
  if (z != NULL)
    {
      if (WHETHER (z, STANDARD) || WHETHER (z, INDICANT))
	{
	  brief_mode_flat (f, z);
	}
      else if (z == MODE (COLLITEM))
	{
	  io_write_string (f, "\"COLLITEM\"");
	}
      else if (WHETHER (z, REF_SYMBOL))
	{
	  io_write_string (f, "REF ");
	  brief_mode_flat (f, SUB (z));
	}
      else if (WHETHER (z, FLEX_SYMBOL))
	{
	  io_write_string (f, "FLEX ");
	  brief_mode_flat (f, SUB (z));
	}
      else if (WHETHER (z, ROW_SYMBOL))
	{
	  int i = z->dimensions;
	  io_write_string (f, "[");
	  while (--i)
	    {
	      io_write_string (f, ", ");
	    }
	  io_write_string (f, "] ");
	  brief_mode_flat (f, SUB (z));
	}
      else if (WHETHER (z, STRUCT_SYMBOL))
	{
	  io_write_string (f, "STRUCT (");
	  brief_fields_flat (f, PACK (z));
	  io_write_string (f, ")");
	}
      else if (WHETHER (z, UNION_SYMBOL))
	{
	  io_write_string (f, "UNION (");
	  brief_fields_flat (f, PACK (z));
	  io_write_string (f, ")");
	}
      else if (WHETHER (z, PROC_SYMBOL))
	{
	  io_write_string (f, "PROC ");
	  if (PACK (z) != NULL)
	    {
	      io_write_string (f, "(");
	      brief_fields_flat (f, PACK (z));
	      io_write_string (f, ") ");
	    }
	  brief_mode_flat (f, SUB (z));
	}
      else if (WHETHER (z, IN_TYPE_MODE))
	{
	  io_write_string (f, "\"SIMPLIN\"");
	}
      else if (WHETHER (z, OUT_TYPE_MODE))
	{
	  io_write_string (f, "\"SIMPLOUT\"");
	}
      else if (WHETHER (z, ROWS_SYMBOL))
	{
	  io_write_string (f, "\"ROWS\"");
	}
      else if (WHETHER (z, SERIES_MODE))
	{
	  io_write_string (f, "\"SERIES\" (");
	  brief_fields_flat (f, PACK (z));
	  io_write_string (f, ")");
	}
      else if (WHETHER (z, STOWED_MODE))
	{
	  io_write_string (f, "\"STOWED\" (");
	  brief_fields_flat (f, PACK (z));
	  io_write_string (f, ")");
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
print_mode_flat */

static void
print_mode_flat (FILE_T f, MOID_T * m)
{
  if (m != NULL)
    {
      brief_moid_flat (f, m);
      sprintf (output_line, " ");
      io_write_string (f, output_line);
/*
      sprintf (output_line, moid_to_string (m, 100));
      io_write_string (f, output_line);
*/
      if (m->equivalent_mode != NULL)
	{
	  sprintf (output_line, " equi: %s", brief_mode_string (EQUIVALENT (m)));
	  io_write_string (f, output_line);
	}
      if (m->slice != NULL)
	{
	  sprintf (output_line, " slice: %s", brief_mode_string (m->slice));
	  io_write_string (f, output_line);
	}
      if (m->deflexed_mode != NULL)
	{
	  sprintf (output_line, " deflex: %s", brief_mode_string (m->deflexed_mode));
	  io_write_string (f, output_line);
	}
      if (m->multiple_mode != NULL)
	{
	  sprintf (output_line, " multiple: %s", brief_mode_string (m->multiple_mode));
	  io_write_string (f, output_line);
	}
      if (m->name != NULL)
	{
	  sprintf (output_line, " name: %s", brief_mode_string (m->name));
	  io_write_string (f, output_line);
	}
      if (m->trim != NULL)
	{
	  sprintf (output_line, " trim: %s", brief_mode_string (m->trim));
	  io_write_string (f, output_line);
	}
      if (m->use == A_FALSE)
	{
	  sprintf (output_line, " unused");
	  io_write_string (f, output_line);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
xref_tags */

static void
xref_tags (FILE_T f, TAG_T * s, int a)
{
  for (; s != NULL; s = NEXT (s))
    {
      NODE_T *where = NODE (s);
      if (where != NULL && ((MASK (where) & CROSS_REFERENCE_MASK)))
	{
	  io_write_string (f, "\n ");
	  switch (a)
	    {
	    case IDENTIFIER:
	      {
		sprintf (output_line, "identifier %s ", SYMBOL (NODE (s)));
		io_write_string (f, output_line);
		brief_moid_flat (f, MOID (s));
		break;
	      }
	    case INDICANT:
	      {
		sprintf (output_line, "indicant %s ", SYMBOL (NODE (s)));
		io_write_string (f, output_line);
		brief_moid_flat (f, MOID (s));
		break;
	      }
	    case PRIO_SYMBOL:
	      {
		sprintf (output_line, "PRIO %s %d", SYMBOL (NODE (s)), PRIO (s));
		io_write_string (f, output_line);
		break;
	      }
	    case OP_SYMBOL:
	      {
		sprintf (output_line, "operator %s ", SYMBOL (NODE (s)));
		io_write_string (f, output_line);
		brief_moid_flat (f, MOID (s));
		break;
	      }
	    case LABEL:
	      {
		sprintf (output_line, "label %s", SYMBOL (NODE (s)));
		io_write_string (f, output_line);
		break;
	      }
	    case ANONYMOUS:
	      {
		switch (PRIO (s))
		  {
		  case ROUTINE_TEXT:
		    {
		      sprintf (output_line, "routine text ");
		      break;
		    }
		  case FORMAT_TEXT:
		    {
		      sprintf (output_line, "format text ");
		      break;
		    }
		  case FORMAT_IDENTIFIER:
		    {
		      sprintf (output_line, "format item ");
		      break;
		    }
		  case COLLATERAL_CLAUSE:
		    {
		      sprintf (output_line, "display ");
		      break;
		    }
		  case GENERATOR:
		    {
		      sprintf (output_line, "generator ");
		      break;
		    }
		  case PROTECT_FROM_SWEEP:
		    {
		      sprintf (output_line, "sweep protect ");
		      break;
		    }
		  }
		io_write_string (f, output_line);
		brief_moid_flat (f, MOID (s));
		break;
	      }
	    default:
	      {
		sprintf (output_line, "internal %d ", a);
		io_write_string (f, output_line);
		brief_moid_flat (f, MOID (s));
		break;
	      }
	    }
	  if (where != NULL && where->info != NULL && where->info->line != NULL)
	    {
	      sprintf (output_line, " line %d", where->info->line->number);
	      io_write_string (f, output_line);
	    }
	  switch (ACCESS (s))
	    {
	    case PUBLIC_SYMBOL:
	      {
		sprintf (output_line, " public");
		io_write_string (f, output_line);
		break;
	      }
	    case PRELUDE_SYMBOL:
	      {
		sprintf (output_line, " prelude ");
		io_write_string (f, output_line);
		break;
	      }
	    case POSTLUDE_SYMBOL:
	      {
		sprintf (output_line, " postlude");
		io_write_string (f, output_line);
		break;
	      }
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
xref_decs */

static void
xref_decs (FILE_T f, SYMBOL_TABLE_T * next)
{
  if (next->indicants != NULL)
    {
      xref_tags (f, next->indicants, INDICANT);
    }
  if (next->operators != NULL)
    {
      xref_tags (f, next->operators, OP_SYMBOL);
    }
  if (PRIO (next) != NULL)
    {
      xref_tags (f, PRIO (next), PRIO_SYMBOL);
    }
  if (next->identifiers != NULL)
    {
      xref_tags (f, next->identifiers, IDENTIFIER);
    }
  if (next->labels != NULL)
    {
      xref_tags (f, next->labels, LABEL);
    }
  if (next->anonymous != NULL)
    {
      xref_tags (f, next->anonymous, ANONYMOUS);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
xref1_moid */

static void
xref1_moid (FILE_T f, MOID_T * p)
{
  if (EQUIVALENT (p) == NULL || SHOW_EQ)
    {
      sprintf (output_line, "\n  %s ", brief_mode_string (p));
      io_write_string (f, output_line);
      print_mode_flat (f, p);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
xref_moids */

static void
xref_moids (FILE_T f, MOID_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      xref1_moid (f, p);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_listing */

static void
moid_listing (FILE_T f, MOID_LIST_T * m)
{
  for (; m != NULL; m = NEXT (m))
    {
      xref1_moid (f, MOID (m));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
cross_reference */

static void
cross_reference (FILE_T f, NODE_T * p, SOURCE_LINE_T * l)
{
  if (cross_reference_safe)
    {
      for (; p != NULL; p = NEXT (p))
	{
	  if (whether_new_lexical_level (p) && l == LINE (p))
	    {
	      SYMBOL_TABLE_T *c = SYMBOL_TABLE (SUB (p));
	      sprintf (output_line, "\n%scross reference [level %d", BARS, c->level);
	      io_write_string (f, output_line);
	      if (PREVIOUS (c) == stand_env)
		{
		  sprintf (output_line, ", in standard environ]");
		}
	      else
		{
		  sprintf (output_line, ", in level %d]", PREVIOUS (c)->level);
		}
	      io_write_string (f, output_line);
	      if (c->moids != NULL)
		{
		  xref_moids (f, c->moids);
		}
	      if (c != NULL)
		{
		  xref_decs (f, c);
		}
	    }
	  cross_reference (f, SUB (p), l);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
low_level */

static void
low_level (NODE_T * p, int x, int *y, SOURCE_LINE_T * l)
{
  for (; p != NULL; p = NEXT (p))
    {
      if ((MASK (p) & TREE_MASK) && l == LINE (p))
	{
	  if (x < *y)
	    {
	      *y = x;
	    }
	}
      low_level (SUB (p), x + 1, y, l);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tree_listing */

static void
tree_listing (FILE_T f, NODE_T * p, int x, int *y, SOURCE_LINE_T * l)
{
  if (tree_listing_safe)
    {
      for (; p != NULL; p = NEXT (p))
	{
	  if ((MASK (p) & TREE_MASK) && l == LINE (p))
	    {
	      if (MASK (p) & TREE_MASK)
		{
		  io_write_string (f, "\n");
		  sprintf (output_line, "%p %03x ", p, x);
		  io_write_string (f, output_line);
		  if (SYMBOL_TABLE (p) != NULL)
		    {
		      sprintf (output_line, "%c ", digit_to_char (LEX_LEVEL (p)));
		    }
		  else
		    {
		      sprintf (output_line, "  ");
		    }
		  io_write_string (f, output_line);
		  sprintf (output_line, "%c %c ", digit_to_char (p->info->PROCEDURE_NUMBER), digit_to_char (p->info->PROCEDURE_LEVEL));
		  io_write_string (f, output_line);
/* Print grammatical stuff */
		  sprintf (output_line, "%-24s ", non_terminal_string (edit_line, ATTRIBUTE (p)));
		  io_write_string (f, output_line);
		  if (SUB (p) == NULL)
		    {
		      sprintf (output_line, "%-24s ", SYMBOL (p));
		    }
		  else
		    {
		      sprintf (output_line, "%-24s ", " ");
		    }
		  io_write_string (f, output_line);
		  if (MOID (p) != NULL)
		    {
		      sprintf (output_line, "#%03X %s ", MOID (p)->number, moid_to_string (MOID (p), 32));
		      io_write_string (f, output_line);
		    }
/* Tell what the interpreter does */
		  if (p->genie.propagator.unit != genie_unit && genie_function_name (p->genie.propagator.unit) != NULL)
		    {
		      sprintf (output_line, "<%s>", genie_function_name (p->genie.propagator.unit));
		      io_write_string (f, output_line);
		    }
		}
	    }
	  tree_listing (f, SUB (p), x + 1, y, l);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
leaves_to_print */

static int
leaves_to_print (NODE_T * p, SOURCE_LINE_T * l)
{
  int z = 0;
  for (; p != NULL && z == 0; p = NEXT (p))
    {
      if (l == LINE (p) && (MASK (p) & TREE_MASK))
	{
	  z++;
	}
      else
	{
	  z += leaves_to_print (SUB (p), l);
	}
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
source_listing */

void
source_listing (MODULE_T * module)
{
  SOURCE_LINE_T *line = module->top_line;
  FILE_T f = module->files.listing.fd;
  int listed = 0;
  if (module->files.listing.opened == 0)
    {
      diagnostic (A_ERROR, NULL, "cannot write source listing", NULL);
      return;
    }
  for (; line != NULL; line = NEXT (line))
    {
      int k = strlen (line->string) - 1;
      if (line->number <= 0)
	{
/* Mask the prelude and postlude */
	  continue;
	}
      if ((line->string)[k] == '\n')
	{
	  (line->string)[k] = '\0';
	}
      if (line->list)
	{
	  listed++;
	  sprintf (output_line, "\n%s\t%-4d", BARS, line->number);
	  io_write_string (f, output_line);
	  if (error_count)
	    {
	      sprintf (output_line, "      ");
	      io_write_string (f, output_line);
	    }
	  else
	    {
	      sprintf (output_line, " %2s %2s", make_numbers (line->min_proc_level, line->max_proc_level), make_numbers (line->min_level, line->max_level));
	      io_write_string (f, output_line);
	    }
/* Print source line and fragment long lines */
	  {
	    char d[LINE_LENGTH + 1], *s = line->string;
	    int i = 0;
	    do
	      {
		s = own_strncpy (d, s, LINE_LENGTH);
		if (i++ > 0)
		  {
		    sprintf (output_line, "\n\t      ");
		    io_write_string (f, output_line);
		  }
		sprintf (output_line, " %s", d);
		io_write_string (f, output_line);
	      }
	    while (strlen (d) == LINE_LENGTH);
	  };
/* Print diagnostics */
	  if (line->messages != NULL)
	    {
	      MESSAGE_T *d = line->messages;
	      char *p = line->string;
	      sprintf (output_line, "\n%s\t           ", BARS);
	      io_write_string (f, output_line);
	      while (*p != '\0')
		{
		  if (*p == ' ' || *p == '\t')
		    {
		      sprintf (output_line, "%c", *p++);
		      io_write_string (f, output_line);
		    }
		  else
		    {
		      int first = 0, length, more = 0;
		      for (d = line->messages; d != NULL; d = NEXT (d))
			{
			  if (d->where->info->char_in_line == p && ++more == 1)
			    {
			      first = d->number;
			      length = strlen (SYMBOL (d->where));
			    }
			}
		      p++;
		      if (more == 0)
			{
			  sprintf (output_line, " ");
			}
		      else if (more == 1)
			{
			  sprintf (output_line, "%c", digit_to_char (first));
			}
		      else
			{
			  sprintf (output_line, "*");
			}
		      io_write_string (f, output_line);
		    }
		}
	      for (d = line->messages; d != NULL; d = NEXT (d))
		{
		  sprintf (output_line, "\n%s%s", BARS, d->text);
		  io_write_string (f, output_line);
		}
	    }
	  if (module->options.cross_reference)
	    {
	      cross_reference (f, line->top_node, line);
	    }
	  if (module->options.tree_listing && leaves_to_print (module->top_node, line))
	    {
	      int y = INT_MAX;
	      sprintf (output_line, "\n%ssyntax tree", BARS);
	      io_write_string (f, output_line);
	      low_level (module->top_node, 1, &y, line);
	      tree_listing (f, module->top_node, 1, &y, line);
	    }
	}
    }
  if (listed == 0)
    {
      sprintf (output_line, "\n  no lines to list");
      io_write_string (f, output_line);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_listing */

void
write_listing (MODULE_T * module)
{
  SOURCE_LINE_T *z;
  FILE_T f = module->files.listing.fd;
  if (module->options.moid_listing && top_moid_list != NULL)
    {
      sprintf (output_line, "\n%smoid listing", BARS);
      io_write_string (f, output_line);
      moid_listing (f, top_moid_list);
    }
  if (module->options.standard_prelude_listing && stand_env != NULL)
    {
      sprintf (output_line, "\n%sstandard prelude listing", BARS);
      io_write_string (f, output_line);
      xref_decs (f, stand_env);
    }
  if (module->top_refinement != NULL)
    {
      REFINEMENT_T *x = module->top_refinement;
      sprintf (output_line, "\n%srefinements", BARS);
      io_write_string (f, output_line);
      while (x != NULL)
	{
	  sprintf (output_line, "\n  \"%s\"", x->name);
	  io_write_string (f, output_line);
	  if (x->line_defined != NULL)
	    {
	      sprintf (output_line, ", defined in line %d", x->line_defined->number);
	      io_write_string (f, output_line);
	    }
	  if (x->line_applied != NULL)
	    {
	      sprintf (output_line, ", applied in line %d", x->line_applied->number);
	      io_write_string (f, output_line);
	    }
	  switch (x->applications)
	    {
	    case 0:
	      sprintf (output_line, ", not applied");
	      io_write_string (f, output_line);
	      break;
	    case 1:
	      break;
	    default:
	      sprintf (output_line, ", applied more than once");
	      io_write_string (f, output_line);
	      break;
	    }
	  x = NEXT (x);
	}
    }
  if (module->options.list != NULL)
    {
      OPTION_LIST_T *i;
      sprintf (output_line, "\n%scommands options", BARS);
      io_write_string (f, output_line);
      for (i = module->options.list; i != NULL; i = NEXT (i))
	{
	  sprintf (output_line, " %s", i->str);
	  io_write_string (f, output_line);
	}
    }
  if (module->options.statistics_listing)
    {
      if (error_count + warning_count > 0)
	{
	  sprintf (output_line, "\n%smessages", BARS);
	  io_write_string (f, output_line);
	  for (z = module->top_line; z != NULL; z = NEXT (z))
	    {
	      MESSAGE_T *d;
	      for (d = z->messages; d != NULL; d = NEXT (d))
		{
		  sprintf (output_line, "\n  line %d: %s", z->number, d->text);
		  io_write_string (f, output_line);
		}
	    }
	}
      sprintf (output_line, "\n%sgarbage collections %d", BARS, garbage_collects);
      io_write_string (f, output_line);
    }
  io_write_string (f, "\n");
}
