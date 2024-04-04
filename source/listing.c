/*!
\file listing.c
\brief routines for making a listing file
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2006 J. Marcel van der Veer <algol68g@xs4all.nl>.

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


#include "algol68g.h"
#include "genie.h"

#define SHOW_EQ A_FALSE

/*!
\brief brief_mode_string
\param p
\return
**/

static char *brief_mode_string (MOID_T * p)
{
  static char q[32];
  sprintf (q, "MODE#%X", p->number);
  return (new_string (q));
}

/*!
\brief brief_mode_flat
\param f
\param z
**/

static void brief_mode_flat (FILE_T f, MOID_T * z)
{
  if (WHETHER (z, STANDARD) || WHETHER (z, INDICANT)) {
    int i = z->dimensions;
    if (i > 0) {
      while (i--) {
	io_write_string (f, "LONG ");
      }
    } else if (i < 0) {
      while (i++) {
	io_write_string (f, "SHORT ");
      }
    }
    sprintf (output_line, "%s", SYMBOL (NODE (z)));
    io_write_string (f, output_line);
  } else {
    sprintf (output_line, "%s", brief_mode_string (z));
    io_write_string (f, output_line);
  }
}

/*!
\brief brief_fields_flat
\param f
\param pack
**/

static void brief_fields_flat (FILE_T f, PACK_T * pack)
{
  if (pack != NULL) {
    brief_mode_flat (f, MOID (pack));
    if (NEXT (pack) != NULL) {
      sprintf (output_line, ", ");
      io_write_string (f, output_line);
      brief_fields_flat (f, NEXT (pack));
    }
  }
}

/*!
\brief brief_moid_flat
\param f
\param z
**/

static void brief_moid_flat (FILE_T f, MOID_T * z)
{
  if (z != NULL) {
    if (WHETHER (z, STANDARD) || WHETHER (z, INDICANT)) {
      brief_mode_flat (f, z);
    } else if (z == MODE (COLLITEM)) {
      io_write_string (f, "\"COLLITEM\"");
    } else if (WHETHER (z, REF_SYMBOL)) {
      io_write_string (f, "REF ");
      brief_mode_flat (f, SUB (z));
    } else if (WHETHER (z, FLEX_SYMBOL)) {
      io_write_string (f, "FLEX ");
      brief_mode_flat (f, SUB (z));
    } else if (WHETHER (z, ROW_SYMBOL)) {
      int i = z->dimensions;
      io_write_string (f, "[");
      while (--i) {
	io_write_string (f, ", ");
      }
      io_write_string (f, "] ");
      brief_mode_flat (f, SUB (z));
    } else if (WHETHER (z, STRUCT_SYMBOL)) {
      io_write_string (f, "STRUCT (");
      brief_fields_flat (f, PACK (z));
      io_write_string (f, ")");
    } else if (WHETHER (z, UNION_SYMBOL)) {
      io_write_string (f, "UNION (");
      brief_fields_flat (f, PACK (z));
      io_write_string (f, ")");
    } else if (WHETHER (z, PROC_SYMBOL)) {
      io_write_string (f, "PROC ");
      if (PACK (z) != NULL) {
	io_write_string (f, "(");
	brief_fields_flat (f, PACK (z));
	io_write_string (f, ") ");
      }
      brief_mode_flat (f, SUB (z));
    } else if (WHETHER (z, IN_TYPE_MODE)) {
      io_write_string (f, "\"SIMPLIN\"");
    } else if (WHETHER (z, OUT_TYPE_MODE)) {
      io_write_string (f, "\"SIMPLOUT\"");
    } else if (WHETHER (z, ROWS_SYMBOL)) {
      io_write_string (f, "\"ROWS\"");
    } else if (WHETHER (z, SERIES_MODE)) {
      io_write_string (f, "\"SERIES\" (");
      brief_fields_flat (f, PACK (z));
      io_write_string (f, ")");
    } else if (WHETHER (z, STOWED_MODE)) {
      io_write_string (f, "\"STOWED\" (");
      brief_fields_flat (f, PACK (z));
      io_write_string (f, ")");
    }
  }
}

/*!
\brief print_mode_flat
\param f
\param m
**/

static void print_mode_flat (FILE_T f, MOID_T * m)
{
  if (m != NULL) {
    brief_moid_flat (f, m);
    sprintf (output_line, " ");
    io_write_string (f, output_line);
    if (m->equivalent_mode != NULL) {
      sprintf (output_line, " equi: %s", brief_mode_string (EQUIVALENT (m)));
      io_write_string (f, output_line);
    }
    if (m->slice != NULL) {
      sprintf (output_line, " slice: %s", brief_mode_string (m->slice));
      io_write_string (f, output_line);
    }
    if (m->deflexed_mode != NULL) {
      sprintf (output_line, " deflex: %s", brief_mode_string (m->deflexed_mode));
      io_write_string (f, output_line);
    }
    if (m->multiple_mode != NULL) {
      sprintf (output_line, " multiple: %s", brief_mode_string (m->multiple_mode));
      io_write_string (f, output_line);
    }
    if (m->name != NULL) {
      sprintf (output_line, " name: %s", brief_mode_string (m->name));
      io_write_string (f, output_line);
    }
    if (m->trim != NULL) {
      sprintf (output_line, " trim: %s", brief_mode_string (m->trim));
      io_write_string (f, output_line);
    }
    if (m->use == A_FALSE) {
      sprintf (output_line, " unused");
      io_write_string (f, output_line);
    }
  }
}

/*!
\brief xref_tags
\param f
\param s
\param a
**/

static void xref_tags (FILE_T f, TAG_T * s, int a)
{
  for (; s != NULL; FORWARD (s)) {
    NODE_T *where = NODE (s);
    if (where != NULL && ((MASK (where) & CROSS_REFERENCE_MASK))) {
      io_write_string (f, "\n     ");
      switch (a) {
      case IDENTIFIER:
	{
	  sprintf (output_line, "Identifier %s ", SYMBOL (NODE (s)));
	  io_write_string (f, output_line);
	  brief_moid_flat (f, MOID (s));
	  break;
	}
      case INDICANT:
	{
	  sprintf (output_line, "Indicant %s ", SYMBOL (NODE (s)));
	  io_write_string (f, output_line);
	  brief_moid_flat (f, MOID (s));
	  break;
	}
      case PRIO_SYMBOL:
	{
	  sprintf (output_line, "Priority %s %d", SYMBOL (NODE (s)), PRIO (s));
	  io_write_string (f, output_line);
	  break;
	}
      case OP_SYMBOL:
	{
	  sprintf (output_line, "Operator %s ", SYMBOL (NODE (s)));
	  io_write_string (f, output_line);
	  brief_moid_flat (f, MOID (s));
	  break;
	}
      case LABEL:
	{
	  sprintf (output_line, "Label %s", SYMBOL (NODE (s)));
	  io_write_string (f, output_line);
	  break;
	}
      case ANONYMOUS:
	{
	  switch (PRIO (s)) {
	  case ROUTINE_TEXT:
	    {
	      sprintf (output_line, "Routine text ");
	      break;
	    }
	  case FORMAT_TEXT:
	    {
	      sprintf (output_line, "Format text ");
	      break;
	    }
	  case FORMAT_IDENTIFIER:
	    {
	      sprintf (output_line, "Format item ");
	      break;
	    }
	  case COLLATERAL_CLAUSE:
	    {
	      sprintf (output_line, "Display ");
	      break;
	    }
	  case GENERATOR:
	    {
	      sprintf (output_line, "Generator ");
	      break;
	    }
	  case PROTECT_FROM_SWEEP:
	    {
	      sprintf (output_line, "Sweep protect %p ", NODE (s));
	      break;
	    }
	  }
	  io_write_string (f, output_line);
	  brief_moid_flat (f, MOID (s));
	  break;
	}
      default:
	{
	  sprintf (output_line, "Internal %d ", a);
	  io_write_string (f, output_line);
	  brief_moid_flat (f, MOID (s));
	  break;
	}
      }
      if (where != NULL && where->info != NULL && where->info->line != NULL) {
	sprintf (output_line, " line %d", where->info->line->number);
	io_write_string (f, output_line);
      }
    }
  }
}

/*!
\brief xref_decs
\param f
\param next
**/

static void xref_decs (FILE_T f, SYMBOL_TABLE_T * next)
{
  if (next->indicants != NULL) {
    xref_tags (f, next->indicants, INDICANT);
  }
  if (next->operators != NULL) {
    xref_tags (f, next->operators, OP_SYMBOL);
  }
  if (PRIO (next) != NULL) {
    xref_tags (f, PRIO (next), PRIO_SYMBOL);
  }
  if (next->identifiers != NULL) {
    xref_tags (f, next->identifiers, IDENTIFIER);
  }
  if (next->labels != NULL) {
    xref_tags (f, next->labels, LABEL);
  }
  if (next->anonymous != NULL) {
    xref_tags (f, next->anonymous, ANONYMOUS);
  }
}

/*!
\brief xref1_moid
\param f
\param p
**/

static void xref1_moid (FILE_T f, MOID_T * p)
{
  if (EQUIVALENT (p) == NULL || SHOW_EQ) {
    sprintf (output_line, "\n     %s ", brief_mode_string (p));
    io_write_string (f, output_line);
    print_mode_flat (f, p);
  }
}

/*!
\brief xref_moids
\param f
\param p
**/

static void xref_moids (FILE_T f, MOID_T * p)
{
  for (; p != NULL; FORWARD (p)) {
    xref1_moid (f, p);
  }
}

/*!
\brief moid_listing
\param f
\param m
**/

static void moid_listing (FILE_T f, MOID_LIST_T * m)
{
  for (; m != NULL; FORWARD (m)) {
    xref1_moid (f, MOID (m));
  }
}

/*!
\brief cross_reference
\param f
\param p
\param l
**/

static void cross_reference (FILE_T f, NODE_T * p, SOURCE_LINE_T * l)
{
  if (cross_reference_safe) {
    for (; p != NULL; FORWARD (p)) {
      if (whether_new_lexical_level (p) && l == LINE (p)) {
	SYMBOL_TABLE_T *c = SYMBOL_TABLE (SUB (p));
	sprintf (output_line, "\n++++ Cross reference [level %d", c->level);
	io_write_string (f, output_line);
	if (PREVIOUS (c) == stand_env) {
	  sprintf (output_line, ", in standard environ]");
	} else {
	  sprintf (output_line, ", in level %d]", PREVIOUS (c)->level);
	}
	io_write_string (f, output_line);
	if (c->moids != NULL) {
	  xref_moids (f, c->moids);
	}
	if (c != NULL) {
	  xref_decs (f, c);
	}
      }
      cross_reference (f, SUB (p), l);
    }
  }
}

/*!
\brief low_level
\param p
\param x
\param y
\param l
**/

static void low_level (NODE_T * p, int x, int *y, SOURCE_LINE_T * l)
{
  for (; p != NULL; FORWARD (p)) {
    if ((MASK (p) & TREE_MASK) && l == LINE (p)) {
      if (x < *y) {
	*y = x;
      }
    }
    low_level (SUB (p), x + 1, y, l);
  }
}

/*!
\brief tree_listing
\param f
\param p
\param x
\param y
\param l
**/

static void tree_listing (FILE_T f, NODE_T * p, int x, int *y, SOURCE_LINE_T * l)
{
  if (tree_listing_safe) {
    for (; p != NULL; FORWARD (p)) {
      if ((MASK (p) & TREE_MASK) && l == LINE (p)) {
	if (MASK (p) & TREE_MASK) {
	  sprintf (output_line, "\n     %p %-3x ", p, x);
	  io_write_string (f, output_line);
	  if (p->protect_sweep == NULL) {
	    sprintf (output_line, " ");
	  } else {
	    sprintf (output_line, "*");
	  }
	  io_write_string (f, output_line);
	  if (SYMBOL_TABLE (p) != NULL) {
	    sprintf (output_line, "%c ", digit_to_char (LEX_LEVEL (p)));
	  } else {
	    sprintf (output_line, "  ");
	  }
	  io_write_string (f, output_line);
	  sprintf (output_line, "%c %c %c ", digit_to_char (p->info->PROCEDURE_NUMBER), digit_to_char (p->info->PROCEDURE_LEVEL), digit_to_char (PAR_LEVEL (p)));
	  io_write_string (f, output_line);
/* Print grammatical stuff. */
	  sprintf (output_line, "%-24s ", non_terminal_string (edit_line, ATTRIBUTE (p)));
	  io_write_string (f, output_line);
	  if (SUB (p) == NULL) {
	    sprintf (output_line, "%-24s ", SYMBOL (p));
	  } else {
	    sprintf (output_line, "%-24s ", " ");
	  }
	  io_write_string (f, output_line);
	  if (MOID (p) != NULL) {
	    sprintf (output_line, "#%03X %s ", MOID (p)->number, moid_to_string (MOID (p), 32));
	    io_write_string (f, output_line);
	  }
	}
      }
      tree_listing (f, SUB (p), x + 1, y, l);
    }
  }
}

/*!
\brief leaves_to_print
\param p
\param l
\return
**/

static int leaves_to_print (NODE_T * p, SOURCE_LINE_T * l)
{
  int z = 0;
  for (; p != NULL && z == 0; FORWARD (p)) {
    if (l == LINE (p) && (MASK (p) & TREE_MASK)) {
      z++;
    } else {
      z += leaves_to_print (SUB (p), l);
    }
  }
  return (z);
}

/*!
\brief source_listing
\param module
**/

void source_listing (MODULE_T * module)
{
  SOURCE_LINE_T *line = module->top_line;
  FILE_T f = module->files.listing.fd;
  int listed = 0;
  if (module->files.listing.opened == 0) {
    diagnostic_node (A_ERROR, NULL, ERROR_CANNOT_WRITE_LISTING, NULL);
    return;
  }
  for (; line != NULL; line = NEXT (line)) {
    int k = strlen (line->string) - 1;
    if (line->number <= 0) {
/* Mask the prelude and postlude. */
      continue;
    }
    if ((line->string)[k] == '\n') {
      (line->string)[k] = '\0';
    }
/*
X "ABCD.A68" 0001 01 01 # Source line #
*/
    if (line->list) {
      listed++;
/* Print source line. */
      write_source_line (f, line);
/* Cross reference for lexical levels starting at this line. */
      if (module->options.cross_reference) {
	cross_reference (f, line->top_node, line);
      }
/* Syntax tree listing connected with this line. */
      if (module->options.tree_listing && leaves_to_print (module->top_node, line)) {
	int y = INT_MAX;
	sprintf (output_line, "\n++++ Syntax tree for line %d", line->number);
	io_write_string (f, output_line);
	low_level (module->top_node, 1, &y, line);
	tree_listing (f, module->top_node, 1, &y, line);
      }
    }
  }
/* Warn if there was no source at all. */
  if (listed == 0) {
    sprintf (output_line, "\n     No lines to list");
    io_write_string (f, output_line);
  }
}

/*!
\brief write_listing
\param module
**/

void write_listing (MODULE_T * module)
{
  SOURCE_LINE_T *z;
  FILE_T f = module->files.listing.fd;
  if (module->options.moid_listing && top_moid_list != NULL) {
    sprintf (output_line, "\n++++ Moid listing");
    io_write_string (f, output_line);
    moid_listing (f, top_moid_list);
  }
  if (module->options.standard_prelude_listing && stand_env != NULL) {
    sprintf (output_line, "\n++++ Standard prelude listing");
    io_write_string (f, output_line);
    xref_decs (f, stand_env);
  }
  if (module->top_refinement != NULL) {
    REFINEMENT_T *x = module->top_refinement;
    sprintf (output_line, "\n++++ Refinements");
    io_write_string (f, output_line);
    while (x != NULL) {
      sprintf (output_line, "\n  \"%s\"", x->name);
      io_write_string (f, output_line);
      if (x->line_defined != NULL) {
	sprintf (output_line, ", defined in line %d", x->line_defined->number);
	io_write_string (f, output_line);
      }
      if (x->line_applied != NULL) {
	sprintf (output_line, ", applied in line %d", x->line_applied->number);
	io_write_string (f, output_line);
      }
      switch (x->applications) {
      case 0:{
	  sprintf (output_line, ", not applied");
	  io_write_string (f, output_line);
	  break;
	}
      case 1:{
	  break;
	}
      default:{
	  sprintf (output_line, ", applied more than once");
	  io_write_string (f, output_line);
	  break;
	}
      }
      x = NEXT (x);
    }
  }
  if (module->options.list != NULL) {
    OPTION_LIST_T *i;
    int k = 1;
    sprintf (output_line, "\n++++ Options and pragmat items");
    io_write_string (f, output_line);
    for (i = module->options.list; i != NULL; i = NEXT (i)) {
      sprintf (output_line, "\n     %-4d %s", k++, i->str);
      io_write_string (f, output_line);
    }
  }
  if (module->options.statistics_listing) {
    if (error_count + warning_count > 0) {
      sprintf (output_line, "\n++++ Diagnostics: %d error(s), %d warning(s)", error_count, warning_count);
      io_write_string (f, output_line);
      for (z = module->top_line; z != NULL; z = NEXT (z)) {
	if (z->diagnostics != NULL) {
	  write_source_line (f, z);
	}
      }
    }
    sprintf (output_line, "\n++++ Garbage collections: %d", garbage_collects);
    io_write_string (f, output_line);
  }
  io_write_string (f, "\n");
}
