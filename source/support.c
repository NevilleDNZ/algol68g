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

#ifdef HAVE_UNIX
#ifndef WIN32_VERSION
#include <sys/resource.h>
#endif
#include <sys/time.h>
#endif

ADDR_T fixed_heap_pointer, temp_heap_pointer;

POSTULATE_T *top_postulate, *old_postulate;

KEYWORD_T *top_keyword;
TOKEN_T *top_token;

/*-------1---------2---------3---------4---------5---------6---------7---------+
low_core_alert: give an error upon getting low on core.                       */

void
low_core_alert (void)
{
  abend (A_TRUE, OUT_OF_CORE, NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

BYTE_T *
get_fixed_heap_space (size_t s)
{
  BYTE_T *z = HEAP_ADDRESS (fixed_heap_pointer);
  s = ALIGN (s);
  fixed_heap_pointer += s;
  if (fixed_heap_pointer >= temp_heap_pointer)
    {
      low_core_alert ();
    }
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

BYTE_T *
get_temp_heap_space (size_t s)
{
  s = ALIGN (s);
  temp_heap_pointer -= s;
  if (temp_heap_pointer <= fixed_heap_pointer)
    {
      low_core_alert ();
    }
  return (HEAP_ADDRESS (temp_heap_pointer));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_stack_size: get size of stack segment.                                    */

void
get_stack_size (void)
{
#if defined HAVE_UNIX && !defined WIN32_VERSION
  struct rlimit limits;
  errno = 0;
  abend (!(getrlimit (RLIMIT_STACK, &limits) == 0 && errno == 0), "getrlimit fails", NULL);
  stack_size = (int) (limits.rlim_cur < limits.rlim_max ? limits.rlim_cur : limits.rlim_max);
/* A heuristic in case getrlimit yields extreme numbers: the frame stack is 
   assumed to fill at a rate comparable to the C stack, so the C stack needs 
   not be larger than the frame stack. This may not be true.                  */
  if (stack_size < KILOBYTE || (stack_size > 96 * MEGABYTE && stack_size > frame_stack_size))
    {
      stack_size = frame_stack_size;
    }
#elif defined PRE_MACOS_X
  stack_size = StackSpace ();
#elif defined WIN32_VERSION
  stack_size = MEGABYTE;
#else
  stack_size = 0;		/* No stack check. */
#endif
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
digit_to_char: get int value from digit.                                      */

char
digit_to_char (int i)
{
  char *z = "0123456789abcdefghijklmnopqrstuvwxyz";
  if (i >= 0 && i < (int) strlen (z))
    {
      return (z[i]);
    }
  else
    {
      return ('*');
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_node_info: ibid.                                                          */

NODE_INFO_T *
new_node_info (void)
{
  NODE_INFO_T *z = (NODE_INFO_T *) get_fixed_heap_space (SIZE_OF (NODE_INFO_T));
  z->module = NULL;
  z->mask = 0;
  z->PROCEDURE_LEVEL = 0;
  z->PROCEDURE_NUMBER = 0;
  z->char_in_line = NULL;
  z->symbol = NULL;
  z->line = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_node: ibid.                                                               */

NODE_T *
new_node (void)
{
  NODE_T *z = (NODE_T *) get_fixed_heap_space (SIZE_OF (NODE_T));
  z->info = new_node_info ();
  z->attribute = 0;
  z->annotation = 0;
  z->error = A_FALSE;
  z->genie.propagator.unit = NULL;
  z->genie.propagator.source = NULL;
  z->genie.whether_coercion = A_FALSE;
  z->genie.whether_new_lexical_level = A_FALSE;
  z->genie.seq = NULL;
  z->genie.seq_set = A_FALSE;
  z->genie.parent = NULL;
  z->genie.function_name = NULL;
  z->genie.constant = NULL;
  SYMBOL_TABLE (z) = NULL;
  MOID (z) = NULL;
  NEXT (z) = NULL;
  PREVIOUS (z) = NULL;
  SUB (z) = NULL;
  z->inits = NULL;
  PACK (z) = NULL;
  z->msg = NULL;
  z->tag = NULL;
  z->protect_sweep = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_symbol_table: ibid.                                                       */

SYMBOL_TABLE_T *
new_symbol_table (SYMBOL_TABLE_T * p)
{
  SYMBOL_TABLE_T *z = (SYMBOL_TABLE_T *) get_fixed_heap_space (SIZE_OF (SYMBOL_TABLE_T));
  z->level = symbol_table_count++;
  z->nest = symbol_table_count;
  z->attribute = 0;
  z->environ = NULL;
  z->ap_increment = 0;
  z->empty_table = A_FALSE;
  z->initialise_frame = A_TRUE;
  z->proc_ops = A_TRUE;
  z->initialise_anon = A_TRUE;
  PREVIOUS (z) = p;
  z->identifiers = NULL;
  z->operators = NULL;
  PRIO (z) = NULL;
  z->indicants = NULL;
  z->labels = NULL;
  z->local_identifiers = NULL;
  z->local_operators = NULL;
  z->anonymous = NULL;
  z->moids = NULL;
  z->jump_to = NULL;
  z->inits = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_moid: ibid.                                                               */

MOID_T *
new_moid ()
{
  MOID_T *z = (MOID_T *) get_fixed_heap_space (SIZE_OF (MOID_T));
  z->attribute = 0;
  z->number = 0;
  z->dimensions = 0;
  z->well_formed = A_FALSE;
  z->use = A_FALSE;
  z->has_ref = A_FALSE;
  z->has_flex = A_FALSE;
  z->has_rows = A_FALSE;
  z->in_standard_environ = A_FALSE;
  z->size = 0;
  NODE (z) = NULL;
  PACK (z) = NULL;
  SUB (z) = NULL;
  z->equivalent_mode = 0;
  z->slice = NULL;
  z->deflexed_mode = NULL;
  z->name = NULL;
  z->multiple_mode = NULL;
  z->trim = NULL;
  NEXT (z) = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_pack: ibid.                                                               */

PACK_T *
new_pack ()
{
  PACK_T *z = (PACK_T *) get_fixed_heap_space (SIZE_OF (PACK_T));
  MOID (z) = NULL;
  z->text = NULL;
  NODE (z) = NULL;
  NEXT (z) = NULL;
  PREVIOUS (z) = NULL;
  z->size = 0;
  z->offset = 0;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_tag: ibid.                                                                */

TAG_T *
new_tag ()
{
  TAG_T *z = (TAG_T *) get_fixed_heap_space (SIZE_OF (TAG_T));
  SYMBOL_TABLE (z) = NULL;
  MOID (z) = NULL;
  NODE (z) = NULL;
  z->unit = NULL;
  z->value = NULL;
  z->stand_env_proc = 0;
  z->procedure = NULL;
  z->scope = PRIMAL_SCOPE;
  z->scope_assigned = A_FALSE;
  PRIO (z) = 0;
  z->use = A_FALSE;
  z->in_proc = A_FALSE;
  HEAP (z) = A_FALSE;
  z->access = 0;
  z->size = 0;
  z->offset = 0;
  z->youngest_environ = 0;
  z->loc_assigned = A_FALSE;
  z->loc_procedure = A_FALSE;
  NEXT (z) = NULL;
  z->body = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_source_line: ibid.                                                        */

SOURCE_LINE_T *
new_source_line ()
{
  SOURCE_LINE_T *z = (SOURCE_LINE_T *) get_fixed_heap_space (SIZE_OF (SOURCE_LINE_T));
  z->string = NULL;
  z->messages = NULL;
  z->number = 0;
  z->print_status = 0;
  z->min_level = 0;
  z->max_level = 0;
  z->min_proc_level = 0;
  z->max_proc_level = 0;
  z->list = A_FALSE;
  z->top_node = NULL;
  NEXT (z) = NULL;
  PREVIOUS (z) = NULL;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_special_mode: introduce a special, internal mode.                        */

void
make_special_mode (MOID_T ** n, int m)
{
  (*n) = new_moid ();
  (*n)->attribute = 0;
  (*n)->number = m;
  PACK (*n) = NULL;
  SUB (*n) = NULL;
  (*n)->deflexed_mode = NULL;
  (*n)->name = NULL;
  (*n)->slice = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_module: future expansion

void
add_module (MODULE_CHAIN ** top, MODULE_T this)
{
  MODULE_CHAIN *new_one = (MODULE_CHAIN *) get_heap_space (SIZE_OF (MODULE_CHAIN));
  new_one->module = this;
  NEXT(new_one) = *top;
  *top = new_one;
}

*/

/*-------1---------2---------3---------4---------5---------6---------7---------+
match_string: whether x matches c; case insensitive. 
x: string.
c: string to match, leading '-' or caps in c are mandatory.                   */

BOOL_T
match_string (char *x, char *c, char alt)
{
  BOOL_T match = A_TRUE;
  while ((IS_UPPER (c[0]) || IS_DIGIT (c[0]) || c[0] == '-') && match)
    {
      match &= (TO_LOWER (x[0]) == TO_LOWER ((c++)[0]));
      if (x[0] != '\0')
	{
	  x++;
	}
    }
  while (x[0] != '\0' && x[0] != alt && c[0] != '\0' && match)
    {
      match &= (TO_LOWER ((x++)[0]) == TO_LOWER ((c++)[0]));
    }
  return (match ? (x[0] == '\0' || x[0] == alt) : A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether: whether attributes whether in subsequent nodes.                          */

BOOL_T
whether (NODE_T * p, ...)
{
  va_list vl;
  int a;
  va_start (vl, p);
  while ((a = va_arg (vl, int)) != 0)
    {
      if (p != NULL && (a == WILDCARD || (a >= 0 ? a == ATTRIBUTE (p) : -a != ATTRIBUTE (p))))
	{
	  p = NEXT (p);
	}
      else
	{
	  va_end (vl);
	  return (A_FALSE);
	}
    }
  va_end (vl);
  return (A_TRUE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_sub: isolate nodes p-q making p a branch to p-q.                         */

void
make_sub (NODE_T * p, NODE_T * q, int t)
{
  NODE_T *z = new_node ();
  abend (p == NULL || q == NULL, INTERNAL_ERROR, "make_sub");
  MOVE (z, p, SIZE_OF (NODE_T));
  PREVIOUS (z) = NULL;
  if (p == q)
    {
      NEXT (z) = NULL;
    }
  else
    {
      if (NEXT (p) != NULL)
	{
	  PREVIOUS (NEXT (p)) = z;
	}
      NEXT (p) = NEXT (q);
      if (NEXT (p) != NULL)
	{
	  PREVIOUS (NEXT (p)) = p;
	}
      NEXT (q) = NULL;
    }
  SUB (p) = z;
  ATTRIBUTE (p) = t;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_level: find symbol table at level 'i'.                                   */

SYMBOL_TABLE_T *
find_level (NODE_T * n, int i)
{
  if (n == NULL)
    {
      return (NULL);
    }
  else
    {
      SYMBOL_TABLE_T *s = SYMBOL_TABLE (n);
      if (s != NULL && s->level == i)
	{
	  return (s);
	}
      else if ((s = find_level (SUB (n), i)) != NULL)
	{
	  return (s);
	}
      else if ((s = find_level (NEXT (n), i)) != NULL)
	{
	  return (s);
	}
      else
	{
	  return (NULL);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
seconds: time versus arbitrary origin.                                        */

double
seconds ()
{
#ifdef HAVE_UNIX_CLOCK
  struct rusage rus;
  getrusage (RUSAGE_SELF, &rus);
  return ((double) (rus.ru_utime.tv_sec + rus.ru_utime.tv_usec * 1e-6));
#else
  return (clock () / (double) CLOCKS_PER_SEC);
#endif
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_new_lexical_level: whether 'p' is top of lexical level.                    */

BOOL_T
whether_new_lexical_level (NODE_T * p)
{
  switch (ATTRIBUTE (p))
    {
      {
    case ALT_DO_PART:
    case BRIEF_ELIF_IF_PART:
    case BRIEF_INTEGER_OUSE_PART:
    case BRIEF_UNITED_OUSE_PART:
    case CHOICE:
    case CLOSED_CLAUSE:
    case CONDITIONAL_CLAUSE:
    case DO_PART:
    case ELIF_PART:
    case ELSE_PART:
    case EXPORT_CLAUSE:
    case FORMAT_TEXT:
    case INTEGER_CASE_CLAUSE:
    case INTEGER_CHOICE_CLAUSE:
    case INTEGER_IN_PART:
    case INTEGER_OUT_PART:
    case OUT_PART:
    case ROUTINE_TEXT:
    case SPECIFIED_UNIT:
    case THEN_PART:
    case UNITED_CASE_CLAUSE:
    case UNITED_CHOICE:
    case UNITED_IN_PART:
    case UNITED_OUSE_PART:
    case WHILE_PART:
	return (A_TRUE);
      }
    default:
      {
	return (A_FALSE);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
some_node: return some node.                                                  */

NODE_T *
some_node (char *t)
{
  NODE_T *z = new_node ();
  SYMBOL (z) = t;
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_postulates: initialise use of elems-lists.                               */

void
init_postulates ()
{
  top_postulate = NULL;
  old_postulate = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_postulates: make old elem-list available for new use.                   */

void
reset_postulates ()
{
  old_postulate = top_postulate;
  top_postulate = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
make_postulate: add elements to elem-list.                                          */

void
make_postulate (POSTULATE_T ** p, MOID_T * a, MOID_T * b)
{
  POSTULATE_T *new_one;
  if (old_postulate != NULL)
    {
      new_one = old_postulate;
      old_postulate = NEXT (old_postulate);
    }
  else
    {
      new_one = (POSTULATE_T *) get_temp_heap_space (SIZE_OF (POSTULATE_T));
    }
  new_one->a = a;
  new_one->b = b;
  NEXT (new_one) = *p;
  *p = new_one;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_postulated_pair: where elements are in the list.                                    */

POSTULATE_T *
whether_postulated_pair (POSTULATE_T * p, MOID_T * a, MOID_T * b)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (p->a == a && p->b == b)
	{
	  return (p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_postulated: where element is in the list.                                     */

POSTULATE_T *
whether_postulated (POSTULATE_T * p, MOID_T * a)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (p->a == a)
	{
	  return (p);
	}
    }
  return (NULL);
}


/*------------------+
| Control of C heap |
+------------------*/

/*-------1---------2---------3---------4---------5---------6---------7---------+
discard_heap: ibid.                                                           */

void
discard_heap (void)
{
  if (heap_segment != NULL)
    {
      free (heap_segment);
    }
  fixed_heap_pointer = 0;
  temp_heap_pointer = 0;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_heap: initialise C and A68 heap management.                              */

void
init_heap (void)
{
  int heap_a_size = ALIGN (heap_size);
  int handle_a_size = ALIGN (handle_pool_size);
  int frame_a_size = ALIGN (frame_stack_size);
  int expr_a_size = ALIGN (expr_stack_size);
  int total_size = heap_a_size + handle_a_size + frame_a_size + expr_a_size;
  BYTE_T *core = (BYTE_T *) malloc ((size_t) total_size);
  if (core == NULL)
    {
      low_core_alert ();
    }
  heap_segment = &core[0];
  handle_segment = &heap_segment[heap_a_size];
  frame_segment = &handle_segment[handle_a_size];
  stack_segment = &frame_segment[frame_a_size];
  fixed_heap_pointer = sizeof (ADDR_T);
  temp_heap_pointer = total_size;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
free_heap: actions when closing the heap.                                     */

void
free_heap (void)
{
  return;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_heap_space: return pointer to block of "s" bytes.                         */

void *
get_heap_space (size_t s)
{
  char *z = (char *) malloc (ALIGN (s));
  if (z == NULL)
    {
      low_core_alert ();
    }
  return ((void *) z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_string: make a new copy of "t".                                           */

char *
new_string (char *t)
{
  char *z = (char *) get_heap_space ((size_t) (strlen (t) + 1));
  return (strcpy (z, t));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
new_fixed_string: make a new copy of "t".                                     */

char *
new_fixed_string (char *t)
{
  char *z = (char *) get_fixed_heap_space ((size_t) (strlen (t) + 1));
  return (strcpy (z, t));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_token: add token to the token tree, iterative version.                    */

TOKEN_T *
add_token (TOKEN_T ** p, char *t)
{
  char *z = new_fixed_string (t);
  while (*p != NULL)
    {
      int k = strcmp (z, (*p)->text);
      if (k < 0)
	{
	  p = &(*p)->less;
	}
      else if (k > 0)
	{
	  p = &(*p)->more;
	}
      else
	{
	  return (*p);
	}
    }
  *p = (TOKEN_T *) get_fixed_heap_space (SIZE_OF (TOKEN_T));
  (*p)->text = z;
  (*p)->less = (*p)->more = NULL;
  return (*p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_token: find token in the token tree, iterative version.                  */

TOKEN_T *
find_token (TOKEN_T ** p, char *t)
{
  while (*p != NULL)
    {
      int k = strcmp (t, (*p)->text);
      if (k < 0)
	{
	  p = &(*p)->less;
	}
      else if (k > 0)
	{
	  p = &(*p)->more;
	}
      else
	{
	  return (*p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_keyword: add keyword to the tree, iterative version.                      */

static void
add_keyword (KEYWORD_T ** p, int a, char *t)
{
  while (*p != NULL)
    {
      int k = strcmp (t, (*p)->text);
      if (k < 0)
	{
	  p = &(*p)->less;
	}
      else
	{
	  p = &(*p)->more;
	}
    }
  *p = (KEYWORD_T *) get_fixed_heap_space (SIZE_OF (KEYWORD_T));
  (*p)->attribute = a;
  (*p)->text = t;
  (*p)->less = (*p)->more = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_keyword: find keyword in the tree, iterative version.                    */

KEYWORD_T *
find_keyword (KEYWORD_T * p, char *t)
{
  while (p != NULL)
    {
      int k = strcmp (t, p->text);
      if (k < 0)
	{
	  p = p->less;
	}
      else if (k > 0)
	{
	  p = p->more;
	}
      else
	{
	  return (p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_keyword_from_attribute: find keyword in the tree, iterative version.     */

KEYWORD_T *
find_keyword_from_attribute (KEYWORD_T * p, int a)
{
  if (p == NULL)
    {
      return (NULL);
    }
  else if (a == ATTRIBUTE (p))
    {
      return (p);
    }
  else
    {
      KEYWORD_T *z;
      if ((z = find_keyword_from_attribute (p->less, a)) != NULL)
	{
	  return (z);
	}
      else if ((z = find_keyword_from_attribute (p->more, a)) != NULL)
	{
	  return (z);
	}
      else
	{
	  return (NULL);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_up_tables: make tables of keywords and non-terminals.                     */

void
set_up_tables ()
{
/* Entries are randomised to balance the tree */
  add_keyword (&top_keyword, POINT_SYMBOL, ".");
  add_keyword (&top_keyword, ACCO_SYMBOL, "{");
  add_keyword (&top_keyword, OCCA_SYMBOL, "}");
  add_keyword (&top_keyword, PUBLIC_SYMBOL, "PUBLIC");
  add_keyword (&top_keyword, DEF_SYMBOL, "DEF");
  add_keyword (&top_keyword, FED_SYMBOL, "FED");
  add_keyword (&top_keyword, CODE_SYMBOL, "CODE");
  add_keyword (&top_keyword, EDOC_SYMBOL, "EDOC");
  add_keyword (&top_keyword, ENVIRON_SYMBOL, "ENVIRON");
  add_keyword (&top_keyword, COLON_SYMBOL, ":");
  add_keyword (&top_keyword, THEN_BAR_SYMBOL, "|");
  add_keyword (&top_keyword, SUB_SYMBOL, "[");
  add_keyword (&top_keyword, BY_SYMBOL, "BY");
  add_keyword (&top_keyword, OP_SYMBOL, "OP");
  add_keyword (&top_keyword, COMMA_SYMBOL, ",");
  add_keyword (&top_keyword, AT_SYMBOL, "AT");
  add_keyword (&top_keyword, PRIO_SYMBOL, "PRIO");
  add_keyword (&top_keyword, STYLE_I_COMMENT_SYMBOL, "CO");
  add_keyword (&top_keyword, END_SYMBOL, "END");
  add_keyword (&top_keyword, GO_SYMBOL, "GO");
  add_keyword (&top_keyword, PRIVATE_SYMBOL, "PRIVATE");
  add_keyword (&top_keyword, TO_SYMBOL, "TO");
  add_keyword (&top_keyword, ELSE_BAR_SYMBOL, "|:");
  add_keyword (&top_keyword, THEN_SYMBOL, "THEN");
  add_keyword (&top_keyword, TRUE_SYMBOL, "TRUE");
  add_keyword (&top_keyword, PROC_SYMBOL, "PROC");
  add_keyword (&top_keyword, FOR_SYMBOL, "FOR");
  add_keyword (&top_keyword, GOTO_SYMBOL, "GOTO");
  add_keyword (&top_keyword, WHILE_SYMBOL, "WHILE");
  add_keyword (&top_keyword, IS_SYMBOL, ":=:");
  add_keyword (&top_keyword, ASSIGN_TO_SYMBOL, "=:");
  add_keyword (&top_keyword, COMPLEX_SYMBOL, "COMPLEX");
  add_keyword (&top_keyword, COMPL_SYMBOL, "COMPL");
  add_keyword (&top_keyword, FROM_SYMBOL, "FROM");
  add_keyword (&top_keyword, BOLD_PRAGMAT_SYMBOL, "PRAGMAT");
  add_keyword (&top_keyword, POSTLUDE_SYMBOL, "POSTLUDE");
  add_keyword (&top_keyword, BOLD_COMMENT_SYMBOL, "COMMENT");
  add_keyword (&top_keyword, DO_SYMBOL, "DO");
  add_keyword (&top_keyword, STYLE_II_COMMENT_SYMBOL, "#");
  add_keyword (&top_keyword, CASE_SYMBOL, "CASE");
  add_keyword (&top_keyword, LOC_SYMBOL, "LOC");
  add_keyword (&top_keyword, CHAR_SYMBOL, "CHAR");
  add_keyword (&top_keyword, ISNT_SYMBOL, ":/=:");
  add_keyword (&top_keyword, REF_SYMBOL, "REF");
  add_keyword (&top_keyword, PRELUDE_SYMBOL, "PRELUDE");
  add_keyword (&top_keyword, NIL_SYMBOL, "NIL");
  add_keyword (&top_keyword, ASSIGN_SYMBOL, ":=");
  add_keyword (&top_keyword, FI_SYMBOL, "FI");
  add_keyword (&top_keyword, FILE_SYMBOL, "FILE");
  add_keyword (&top_keyword, PAR_SYMBOL, "PAR");
  add_keyword (&top_keyword, ASSERT_SYMBOL, "ASSERT");
  add_keyword (&top_keyword, OUSE_SYMBOL, "OUSE");
  add_keyword (&top_keyword, IN_SYMBOL, "IN");
  add_keyword (&top_keyword, LONG_SYMBOL, "LONG");
  add_keyword (&top_keyword, SEMI_SYMBOL, ";");
  add_keyword (&top_keyword, EMPTY_SYMBOL, "EMPTY");
  add_keyword (&top_keyword, MODE_SYMBOL, "MODE");
  add_keyword (&top_keyword, IF_SYMBOL, "IF");
  add_keyword (&top_keyword, OD_SYMBOL, "OD");
  add_keyword (&top_keyword, OF_SYMBOL, "OF");
  add_keyword (&top_keyword, STRUCT_SYMBOL, "STRUCT");
  add_keyword (&top_keyword, STYLE_I_PRAGMAT_SYMBOL, "PR");
  add_keyword (&top_keyword, BUS_SYMBOL, "]");
  add_keyword (&top_keyword, SKIP_SYMBOL, "SKIP");
  add_keyword (&top_keyword, SHORT_SYMBOL, "SHORT");
  add_keyword (&top_keyword, IS_SYMBOL, "IS");
  add_keyword (&top_keyword, ESAC_SYMBOL, "ESAC");
  add_keyword (&top_keyword, CHANNEL_SYMBOL, "CHANNEL");
  add_keyword (&top_keyword, ANDF_SYMBOL, "ANDF");
  add_keyword (&top_keyword, ORF_SYMBOL, "ORF");
  add_keyword (&top_keyword, REAL_SYMBOL, "REAL");
  add_keyword (&top_keyword, STRING_SYMBOL, "STRING");
  add_keyword (&top_keyword, BOOL_SYMBOL, "BOOL");
  add_keyword (&top_keyword, ISNT_SYMBOL, "ISNT");
  add_keyword (&top_keyword, FALSE_SYMBOL, "FALSE");
  add_keyword (&top_keyword, UNION_SYMBOL, "UNION");
  add_keyword (&top_keyword, OUT_SYMBOL, "OUT");
  add_keyword (&top_keyword, OPEN_SYMBOL, "(");
  add_keyword (&top_keyword, BEGIN_SYMBOL, "BEGIN");
  add_keyword (&top_keyword, FLEX_SYMBOL, "FLEX");
  add_keyword (&top_keyword, VOID_SYMBOL, "VOID");
  add_keyword (&top_keyword, BITS_SYMBOL, "BITS");
  add_keyword (&top_keyword, ELSE_SYMBOL, "ELSE");
  add_keyword (&top_keyword, EXIT_SYMBOL, "EXIT");
  add_keyword (&top_keyword, HEAP_SYMBOL, "HEAP");
  add_keyword (&top_keyword, INT_SYMBOL, "INT");
  add_keyword (&top_keyword, BYTES_SYMBOL, "BYTES");
  add_keyword (&top_keyword, PIPE_SYMBOL, "PIPE");
  add_keyword (&top_keyword, FORMAT_SYMBOL, "FORMAT");
  add_keyword (&top_keyword, SEMA_SYMBOL, "SEMA");
  add_keyword (&top_keyword, CLOSE_SYMBOL, ")");
  add_keyword (&top_keyword, AT_SYMBOL, "@");
  add_keyword (&top_keyword, ELIF_SYMBOL, "ELIF");
  add_keyword (&top_keyword, FORMAT_DELIMITER_SYMBOL, "$");
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
A list of 10 ^ 2 ^ n for conversion purposes on IEEE 754 platforms.           */

#define MAX_DOUBLE_EXPO 511

static double pow_10[] = {
  10.0, 100.0, 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256
};

/*-------1---------2---------3---------4---------5---------6---------7---------+
ten_to_the_power: return 10 ** expo.
This way appears sufficiently accurate.                                       */

double
ten_to_the_power (int expo)
{
  double dbl_expo, *dep;
  BOOL_T neg_expo;
  dbl_expo = 1;
  if ((neg_expo = expo < 0) == A_TRUE)
    {
      expo = -expo;
    }
  abend (expo > MAX_DOUBLE_EXPO, "exponent too large", "in multiprecision library");
  for (dep = pow_10; expo != 0; expo >>= 1, dep++)
    {
      if (expo & 0x1)
	{
	  dbl_expo *= *dep;
	}
    }
  return (neg_expo ? 1 / dbl_expo : dbl_expo);
}
