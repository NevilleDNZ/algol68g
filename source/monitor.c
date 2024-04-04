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
#include "transput.h"

BOOL_T in_monitor = A_FALSE;

void heap_dump (FILE_T, NODE_T *, A68_HANDLE *);
void show_data_item (FILE_T, NODE_T *, MOID_T *, BYTE_T *);

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static void
show_frame_items (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * q, int modif)
{
  (void) p;
  for (; q != NULL; q = NEXT (q))
    {
      ADDR_T pos_in_frame_stack = link + FRAME_INFO_SIZE + q->offset;
      if (modif != ANONYMOUS)
	{
	  sprintf (output_line, "\n%s%08x %s \"%s\" ", BARS, pos_in_frame_stack, moid_to_string (MOID (q), 32), SYMBOL (NODE (q)));
	  io_write_string (f, output_line);
	}
      else
	{
	  switch (PRIO (q))
	    {
	    case ROUTINE_TEXT:
	    case COLLATERAL_CLAUSE:
	    case PROTECT_FROM_SWEEP:
	      {
		break;
	      }
	    case GENERATOR:
	      {
		sprintf (output_line, "\n%s%08x LOC generator %s ", BARS, pos_in_frame_stack, moid_to_string (MOID (q), 32));
		io_write_string (f, output_line);
		break;
	      }
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
show_stack_frame (FILE_T f, NODE_T * p, ADDR_T link)
{
/* show the frame starting at frame pointer 'link', using symbol table from p as a map */ if (p != NULL)
    {
      SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
      sprintf (output_line, "\n%sstack frame level %d", BARS, q->level);
      io_write_string (f, output_line);
      where (f, p);
      show_frame_items (f, p, link, q->identifiers, IDENTIFIER);
      show_frame_items (f, p, link, q->operators, OPERATOR);
      show_frame_items (f, p, link, q->anonymous, ANONYMOUS);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
where: shows line where 'p' is at and draws a '-' beneath the position.       */

void
where (FILE_T f, NODE_T * p)
{
  SOURCE_LINE_T *l = p->info->line;
  int k, pos_in_line = strlen (l->string) - strlen (p->info->char_in_line);
  char z[BUFFER_SIZE];
  strncpy (z, l->string, BUFFER_SIZE);
  z[BUFFER_SIZE - 1] = '\0';
  for (k = 0; k < (int) strlen (z); k++)
    {
      if (z[k] == '\t')
	{
	  z[k] = ' ';
	}
      else if (z[k] == '\n')
	{
	  z[k] = '\0';
	}
    }
  sprintf (output_line, "\n%-4d %s\n     ", l->number, z);
  io_write_string (f, output_line);
  for (k = 0; k < pos_in_line; k++)
    {
      sprintf (output_line, " ");
      io_write_string (f, output_line);
    }
  sprintf (output_line, "-\n");
  io_write_string (f, output_line);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
list: shows line where 'p' is at and draws a '-' beneath the position.        */

static void
list (FILE_T f, NODE_T * p, int depth)
{
  if (p != NULL && p->info->module != NULL)
    {
      SOURCE_LINE_T *r = p->info->line;
      SOURCE_LINE_T *l = p->info->module->top_line;
      for (; l != NULL; l = NEXT (l))
	{
	  if (abs (r->number - l->number) <= depth)
	    {
	      int k, pos_in_line = strlen (l->string) - strlen (p->info->char_in_line);
	      char z[BUFFER_SIZE];
	      strncpy (z, l->string, BUFFER_SIZE);
	      z[BUFFER_SIZE - 1] = '\0';
	      for (k = 0; k < (int) strlen (z); k++)
		{
		  if (z[k] == '\t')
		    {
		      z[k] = ' ';
		    }
		  else if (z[k] == '\n')
		    {
		      z[k] = '\0';
		    }
		}
	      sprintf (output_line, "\n%-4d %s", l->number, z);
	      io_write_string (f, output_line);
	      if (l == r)
		{
		  sprintf (output_line, "\n     ");
		  io_write_string (f, output_line);
		  for (k = 0; k < pos_in_line; k++)
		    {
		      sprintf (output_line, " ");
		      io_write_string (f, output_line);
		    }
		  sprintf (output_line, "-");
		  io_write_string (f, output_line);
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
heap_dump (FILE_T f, NODE_T * p, A68_HANDLE * handle)
{
  (void) p;
  sprintf (output_line, "\nsize=%d available=%d defragmentations=%d", heap_size, heap_available (), garbage_collects);
  io_write_string (f, output_line);
  for (; handle != NULL; handle = NEXT (handle))
    {
      sprintf (output_line, "\n%s%08x %s %d bytes", BARS, handle->offset, moid_to_string (MOID (handle), 32), handle->size);
      io_write_string (f, output_line);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
stack_dump (FILE_T f, ADDR_T link, int depth)
{
  if (depth > 0 && link > 0)
    {
      int dynamic_link = FRAME_DYNAMIC_LINK (link);
      NODE_T *p = FRAME_TREE (link);
      show_stack_frame (f, p, link);
      stack_dump (f, dynamic_link, depth - 1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static void
genie_help (FILE_T f)
{
  sprintf (output_line, "(Upper case letters are mandatory for recognition)");
  io_write_string (f, output_line);
  sprintf (output_line, "\nWHere      Show where the program was interrupted");
  io_write_string (f, output_line);
  sprintf (output_line, "\nLIst       Show lines where the program was interrupted");
  io_write_string (f, output_line);
  sprintf (output_line, "\nHEap       Write contents of the heap");
  io_write_string (f, output_line);
  sprintf (output_line, "\nSTack      Write the chain of call frames");
  io_write_string (f, output_line);
  sprintf (output_line, "\nFRame      Write the current call frame");
  io_write_string (f, output_line);
  sprintf (output_line, "\nHT         Halt Typing - suppress output to tty");
  io_write_string (f, output_line);
  sprintf (output_line, "\nHX         Halt Execution - terminates the program");
  io_write_string (f, output_line);
  sprintf (output_line, "\nQUIT       Terminates the program");
  io_write_string (f, output_line);
  sprintf (output_line, "\nContinue   Continues execution");
  io_write_string (f, output_line);
  sprintf (output_line, "\nREsume     Continues execution");
  io_write_string (f, output_line);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static BOOL_T
single_stepper (NODE_T * p, char *cmd)
{
  if (match_string (cmd, "HEap", '\0'))
    {
      heap_dump (STDOUT_FILENO, p, unfree_handles);
      return (A_FALSE);
    }
  else if (match_string (cmd, "FRame", '\0'))
    {
      stack_dump (STDOUT_FILENO, frame_pointer, 1);
      return (A_FALSE);
    }
  else if (match_string (cmd, "STack", '\0'))
    {
      stack_dump (STDOUT_FILENO, frame_pointer, 128);
      return (A_FALSE);
    }
  else if (match_string (cmd, "HT", '\0'))
    {
      halt_typing = A_TRUE;
      return (A_TRUE);
    }
  else if (match_string (cmd, "HX", '\0') || match_string (cmd, "QUIT", '\0'))
    {
      exit_genie (p, A_RUNTIME_ERROR);
      return (A_TRUE);
    }
  else if (match_string (cmd, "Continue", '\0'))
    {
      return (A_TRUE);
    }
  else if (match_string (cmd, "REsume", '\0'))
    {
      return (A_TRUE);
    }
  else if (match_string (cmd, "WHere", '\0'))
    {
      where (STDOUT_FILENO, p);
      return (A_FALSE);
    }
  else if (match_string (cmd, "LIst", '\0'))
    {
      list (STDOUT_FILENO, p, 10);
      return (A_FALSE);
    }
  else if (match_string (cmd, "HElp", '\0'))
    {
      genie_help (STDOUT_FILENO);
      return (A_FALSE);
    }
  else if (strlen (cmd) == 0)
    {
      return (A_FALSE);
    }
  else
    {
      sprintf (output_line, "Unknown command, try `help'");
      io_write_string (STDOUT_FILENO, output_line);
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
single_step (NODE_T * p)
{
  SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
  in_monitor = A_TRUE;
  sprintf (output_line, "\n%sstack frame level=%d", BARS, q->level);
  io_write_string (STDOUT_FILENO, output_line);
  where (STDOUT_FILENO, p);
  sprintf (output_line, "\n%sframe stack=%08x; expression stack=%08x", BARS, frame_pointer, stack_pointer);
  io_write_string (STDOUT_FILENO, output_line);
  while (!single_stepper (p, read_string_from_tty (A68G_NAME)))
    {;
    }
  in_monitor = A_FALSE;
}
