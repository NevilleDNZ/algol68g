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
Since Algol 68 can pass procedures as parameters, we use static links rather 
than a display. Static link access to non-local variables is more elaborate then 
display access, but you don't have to copy the display on every call, which is 
expensive in terms of time and stack space. Early versions of Algol68G did use a
display, but speed improvement was negligible and the code was less transparent. 
So it was reverted to static links.                                           */

/*-------1---------2---------3---------4---------5---------6---------7---------+
descent: descent static link to appropriate lexical level.
lex_lvl: target lexical level
returns: pointer to stack frame at level 'lex_lvl'.                           */

ADDR_T
descent (int lex_lvl)
{
  ADDR_T static_link = frame_pointer;
  while (lex_lvl != FRAME_LEXICAL_LEVEL (static_link))
    {
      ADDR_T new_link = FRAME_STATIC_LINK (static_link);
      static_link = new_link;
    }
  return (static_link);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_scope_check.
Dynamic scope check as suggested by Dr. A.N. Walker.                          */

void
genie_scope_check (NODE_T * p, MOID_T * m)
{
/* REF. */
  if (m != NULL && WHETHER (m, REF_SYMBOL))
    {
      A68_REF *z = (A68_REF *) STACK_OFFSET (-SIZE_OF (A68_REF));
      if (z->segment == frame_segment)
	{
	  if (z->offset > frame_pointer + FRAME_INFO_SIZE + FRAME_INCREMENT (frame_pointer))
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "M A violates scope rule", m, p, NULL);
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	}
    }
/* REF embedded in a UNION. */
  else if (m != NULL && WHETHER (m, UNION_SYMBOL))
    {
      int size = MOID_SIZE (m);
      MOID_T *um = ((A68_UNION *) STACK_OFFSET (-size))->value;
      if (um != NULL && WHETHER (um, REF_SYMBOL))
	{
	  A68_REF *z = (A68_REF *) STACK_OFFSET (-size + SIZE_OF (A68_UNION));
	  if (z->segment == frame_segment)
	    {
	      if (z->offset > frame_pointer + FRAME_INFO_SIZE + FRAME_INCREMENT (frame_pointer))
		{
		  diagnostic (A_RUNTIME_ERROR, p, "M A violates scope rule", um, p, NULL);
		  exit_genie (p, A_RUNTIME_ERROR);
		}
	    }
	}
    }
}


/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_init_proc_op: initialise PROC and OP identities.
p: starting node of a declaration.
n_init: number of constants initialised.                                      */

static void
genie_init_proc_op (NODE_T * p, NODE_T ** inits, int *n_init)
{
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case OP_SYMBOL:
	case PROC_SYMBOL:
	case OPERATOR_PLAN:
	case DECLARER:
	  {
	    break;
	  }
	case DEFINING_IDENTIFIER:
	case DEFINING_OPERATOR:
	  {
	    NODE_T *save = *inits;
	    (*inits) = p;	/* Save position so we need not search again */
	    (*inits)->inits = save;
	    /* Execute the unit yielding PROC (..) .. */
	    EXECUTE_UNIT (NEXT (NEXT (p)));
	    TAX (p)->loc_procedure = A_TRUE;
	    /* Do a quick assignment */
	    POP (p, FRAME_OFFSET (FRAME_INFO_SIZE + TAX (p)->offset), MOID_SIZE (MOID (p)));
	    (*n_init)++;
	    return;
	  }
	default:
	  {
	    genie_init_proc_op (SUB (p), inits, n_init);
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_find_proc_op: initialise PROC and OP identity declarations.
p: node where we're looking now.
n_init: number of constants initialised.                                      */

static void
genie_find_proc_op (NODE_T * p, int *n_init)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (p->genie.whether_new_lexical_level)
	{
/* Don't enter a new lexical level - it will have its own initialisation */
	  return;
	}
      else
	{
	  switch (ATTRIBUTE (p))
	    {
/* PROC idf = ..., OP +:= = ..., OP (...) ... *=: = ... */
	    case PROCEDURE_DECLARATION:
	    case BRIEF_OPERATOR_DECLARATION:
	    case OPERATOR_DECLARATION:
	      {
		genie_init_proc_op (SUB (p), &(SYMBOL_TABLE (p)->inits), n_init);
		return;
	      }
/* PROC (...) ... idf = ... */
	    case IDENTITY_DECLARATION:
	      {
		if (MOID (SUB (p)) != NULL && WHETHER (MOID (SUB (p)), PROC_SYMBOL))
		  {
		    genie_init_proc_op (SUB (p), &(SYMBOL_TABLE (p)->inits), n_init);
		    return;
		  }
	      }
	    default:
	      {
		genie_find_proc_op (SUB (p), n_init);
		break;
	      }
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_frame_constant: driver for initialisation of PROC and OP identities.
p: node where the lexical level starts
returns: number of constants initialised.                                     */

static int
genie_frame_constant (NODE_T * p)
{
  int n_init = 0;
  if (SYMBOL_TABLE (p)->inits == NULL)
    {
      genie_find_proc_op (p, &n_init);
    }
  else
    {
      NODE_T *q = SYMBOL_TABLE (p)->inits;
      for (; q != NULL; q = q->inits)
	{
	  EXECUTE_UNIT (NEXT (NEXT (q)));
	  TAX (q)->loc_procedure = A_TRUE;
	  POP (q, FRAME_OFFSET (FRAME_INFO_SIZE + TAX (q)->offset), MOID_SIZE (MOID (q)));
	  n_init++;
	}
    }
  return (n_init);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
initialise_frame (NODE_T * p)
{
/* Initialise routine texts and format texts - link to their environment in the frame stack */
  if (SYMBOL_TABLE (p)->initialise_anon)
    {
      TAG_T *a;
      for (a = SYMBOL_TABLE (p)->anonymous; a != NULL; a = NEXT (a))
	{
	  if (PRIO (a) == FORMAT_TEXT)
	    {
	      /* Initialise format text */
	      A68_FORMAT z;
	      int youngest = TAX (NODE (a))->youngest_environ;
	      z.status = INITIALISED_MASK;
	      z.top = NODE (a);
	      z.environ.status = INITIALISED_MASK;
	      z.environ.segment = frame_segment;
	      if (youngest > 0)
		{
		  z.environ.offset = static_link_for_frame (1 + youngest);
		}
	      else
		{
		  z.environ.offset = 0;
		}
	      *(A68_FORMAT *) (FRAME_OFFSET (FRAME_INFO_SIZE + a->offset)) = z;
	      SYMBOL_TABLE (p)->initialise_anon = A_TRUE;
	    }
	  else if (PRIO (a) == ROUTINE_TEXT)
	    {
	      /* Initialise routine text */
	      A68_PROCEDURE z;
	      int youngest = TAX (NODE (a))->youngest_environ;
	      z.body.status = INITIALISED_MASK;
	      z.body.value = (void *) NODE (a);
	      z.environ.status = INITIALISED_MASK;
	      z.environ.segment = frame_segment;
	      if (youngest > 0)
		{
		  z.environ.offset = static_link_for_frame (1 + youngest);
		}
	      else
		{
		  z.environ.offset = 0;
		}
	      *(A68_PROCEDURE *) (FRAME_OFFSET (FRAME_INFO_SIZE + a->offset)) = z;
	      SYMBOL_TABLE (p)->initialise_anon = A_TRUE;
	    }
	}
    }
/* Initialise PROC and OP identities in this level, so we can have mutual recursion */
  if (SYMBOL_TABLE (p)->proc_ops)
    {
      SYMBOL_TABLE (p)->proc_ops = genie_frame_constant (p) > 0;
    }
/* Record whether we did anything */
  SYMBOL_TABLE (p)->initialise_frame = SYMBOL_TABLE (p)->initialise_anon || SYMBOL_TABLE (p)->proc_ops;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
static_link_for_frame: determine static link for stack frame.
new_lex_lvl: lexical level of new stack frame.
returns: static link for stack frame at 'new_lex_lvl'.                        */

ADDR_T
static_link_for_frame (int new_lex_lvl)
{
  ADDR_T static_link;
  int cur_lex_lvl = FRAME_LEXICAL_LEVEL (frame_pointer);
  if (cur_lex_lvl == new_lex_lvl)
    {				/* Peers */
      static_link = FRAME_STATIC_LINK (frame_pointer);
    }
  else if (cur_lex_lvl < new_lex_lvl)
    {				/* Children */
      static_link = frame_pointer;
    }
  else
    {				/* Ancestors */
      static_link = frame_pointer;
      while (FRAME_LEXICAL_LEVEL (static_link) >= new_lex_lvl)
	{
	  static_link = FRAME_STATIC_LINK (static_link);
	}
    }
  return (static_link);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
open_frame: open a stack frame.
p: node where the lexical level starts.
proc_parm: tells whether opening a call or opening a lexical level.
environ: environment in case of opening a call.                               */

void
open_frame (NODE_T * p, int proc_parm, ADDR_T environ)
{
  ADDR_T dynamic_link = frame_pointer, static_link;
  LOW_STACK_ALERT;
/* Determine the static link */
  if (proc_parm == IS_PROCEDURE_PARM)
    {
      static_link = environ > 0 ? environ : frame_pointer;
    }
  else
    {
      static_link = static_link_for_frame (LEX_LEVEL (p));
    }
/* Set up frame */
  INCREMENT_FRAME_POINTER (p, FRAME_INFO_SIZE + FRAME_INCREMENT (dynamic_link), FRAME_INFO_SIZE + SYMBOL_TABLE (p)->ap_increment);
  FRAME_DYNAMIC_LINK (frame_pointer) = dynamic_link;
  FRAME_STATIC_LINK (frame_pointer) = static_link;
  FRAME_TREE (frame_pointer) = p;
  FRAME_JUMP_STAT (frame_pointer) = NULL;
/* Initialise frame */
  memset ((BYTE_T *) (FRAME_OFFSET (FRAME_INFO_SIZE)), 0x00, (unsigned) SYMBOL_TABLE (p)->ap_increment);
  if (SYMBOL_TABLE (p)->initialise_frame)
    {
      initialise_frame (p);
    }
/* Record the pointer to the outermost level */
  if (global_pointer == 0 && LEX_LEVEL (p) == global_level)
    {
      global_pointer = frame_pointer;
    }
}
