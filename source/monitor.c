/*!
\file monitor.c
\brief low-level monitor for the interpreter
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

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


/*
This is a basic monitor for Algol68G. It activates when the interpreter
receives SIGINT (CTRL-C, for instance) or when PROC VOID break is called.

The monitor allows single stepping (unit-wise through serial/enquiry
clauses) and has basic means for inspecting call-frame stack and heap. 
*/

#include "algol68g.h"
#include "genie.h"
#include "mp.h"
#include "transput.h"

#define WRITE(s) io_write_string (f, s)

#define MAX_ROW_ELEMS 10

BOOL_T in_monitor = A_FALSE;
static int tabs = 0;

/*!
\brief whether item at "w" of mode "q" is initialised
\param p
\param w
\param q
\param result
\return TRUE or FALSE
**/

static BOOL_T check_initialisation (NODE_T * p, BYTE_T * w, MOID_T * q, BOOL_T * result)
{
  BOOL_T initialised = A_TRUE, recognised = A_FALSE;
  switch (q->short_id) {
  case MODE_NO_CHECK:
  case UNION_SYMBOL:
    {
      initialised = A_TRUE;
      recognised = A_TRUE;
      break;
    }
  case REF_SYMBOL:
    {
      A68_REF *z = (A68_REF *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case PROC_SYMBOL:
    {
      A68_PROCEDURE *z = (A68_PROCEDURE *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_INT:
    {
      A68_INT *z = (A68_INT *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_REAL:
    {
      A68_REAL *z = (A68_REAL *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_COMPLEX:
    {
      A68_REAL *r = (A68_REAL *) w;
      A68_REAL *i = (A68_REAL *) (w + SIZE_OF (A68_REAL));
      initialised = (r->status & INITIALISED_MASK) && (i->status & INITIALISED_MASK);
      recognised = A_TRUE;
      break;
    }
  case MODE_LONG_INT:
  case MODE_LONG_REAL:
  case MODE_LONG_BITS:
    {
      MP_DIGIT_T *z = (MP_DIGIT_T *) w;
      initialised = (int) z[0] & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_LONG_COMPLEX:
    {
      MP_DIGIT_T *r = (MP_DIGIT_T *) w;
      MP_DIGIT_T *i = (MP_DIGIT_T *) (w + size_long_mp ());
      initialised = ((int) r[0] & INITIALISED_MASK) && ((int) i[0] & INITIALISED_MASK);
      recognised = A_TRUE;
      break;
    }
  case MODE_BOOL:
    {
      A68_BOOL *z = (A68_BOOL *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_CHAR:
    {
      A68_CHAR *z = (A68_CHAR *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_BITS:
    {
      A68_BITS *z = (A68_BITS *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_BYTES:
    {
      A68_BYTES *z = (A68_BYTES *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_LONG_BYTES:
    {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_FILE:
    {
      A68_FILE *z = (A68_FILE *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_FORMAT:
    {
      A68_FORMAT *z = (A68_FORMAT *) w;
      initialised = z->status & INITIALISED_MASK;
      recognised = A_TRUE;
      break;
    }
  case MODE_PIPE:
    {
      A68_REF *read = (A68_REF *) w;
      A68_REF *write = (A68_REF *) (w + SIZE_OF (A68_REF));
      A68_INT *pid = (A68_INT *) (w + 2 * SIZE_OF (A68_REF));
      initialised = (read->status & INITIALISED_MASK) && (write->status & INITIALISED_MASK) && (pid->status & INITIALISED_MASK);
      recognised = A_TRUE;
      break;
    }
  }
  if (result == NULL) {
    if (recognised == A_TRUE && initialised == A_FALSE) {
      diagnostic (A_RUNTIME_ERROR, p, EMPTY_VALUE_ERROR_FROM, q);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  } else {
    *result = initialised;
  }
  return (recognised);
}

/*!
\brief show value of object
\param p
\param f
\param item
\param mode
**/

static void print_item (NODE_T * p, FILE_T f, BYTE_T * item, MOID_T * mode)
{
  A68_REF nil_file = nil_ref;
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, mode, item, nil_file);
  if (get_transput_buffer_index (UNFORMATTED_BUFFER) > 0) {
    if (mode == MODE (ROW_CHAR) || mode == MODE (STRING)) {
      sprintf (output_line, " value is `%s'", get_transput_buffer (UNFORMATTED_BUFFER));
    } else {
      char *str = get_transput_buffer (UNFORMATTED_BUFFER);
      while (IS_SPACE (str[0])) {
	str++;
      }
      sprintf (output_line, " value is %s", str);

    }
    io_write_string (f, output_line);
  } else {
    io_write_string (f, " cannot show value");
  }
}

/*!
\brief indented newline and MORE function
\param f
**/

static void newline (FILE_T f)
{
  int k;
  io_write_string (f, "\n");
  for (k = 0; k < tabs; k++) {
    io_write_string (f, "\t");
  }
}

/*!
\brief show value of object
\param p
\param f
\param item
\param mode
**/

static void show_item (NODE_T * p, FILE_T f, BYTE_T * item, MOID_T * mode)
{
  if (WHETHER (mode, REF_SYMBOL)) {
    A68_REF *z = (A68_REF *) item;
    if (IS_NIL (*z)) {
      io_write_string (STDOUT_FILENO, " is NIL");
    } else {
      ADDR_T addr = z->offset;
      io_write_string (STDOUT_FILENO, " refers to");
      if (z->segment == heap_segment) {
	addr += z->handle->offset;
	io_write_string (STDOUT_FILENO, " heap");
      } else if (z->segment == frame_segment) {
	io_write_string (STDOUT_FILENO, " frame");
      } else if (z->segment == stack_segment) {
	io_write_string (STDOUT_FILENO, " stack");
      } else if (z->segment == handle_segment) {
	io_write_string (STDOUT_FILENO, " handle");
      }
      sprintf (output_line, "[%d]", addr);
      io_write_string (STDOUT_FILENO, output_line);
      if (z->segment != frame_segment) {
	tabs++;
	show_item (p, f, ADDRESS (z), SUB (mode));
	tabs--;
      }
    }
  } else if ((WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL)) && mode != MODE (ROW_CHAR) && mode != MODE (STRING)) {
    MOID_T *deflexed = DEFLEX (mode);
    A68_ARRAY *arr;
    A68_TUPLE *tup;
    tabs++;
    if (!(((A68_REF *) item)->status & INITIALISED_MASK)) {
      io_write_string (STDOUT_FILENO, " uninitialised");
    } else {
      int count = 0;
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      sprintf (output_line, " has %d elements", ROW_SIZE (tup));
      io_write_string (f, output_line);
      if (get_row_size (tup, arr->dimensions) != 0) {
	BYTE_T *base_addr = ADDRESS (&arr->array);
	BOOL_T done = A_FALSE;
	initialise_internal_index (tup, arr->dimensions);
	while (!done && ++count <= MAX_ROW_ELEMS) {
	  if (count == MAX_ROW_ELEMS) {
	    newline (f);
	    sprintf (output_line, "element [%d] ...", count);
	    io_write_string (f, output_line);
	  } else {
	    ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	    ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	    BYTE_T *elem = &base_addr[elem_addr];
	    newline (f);
	    sprintf (output_line, "element [%d]", count);
	    io_write_string (STDOUT_FILENO, output_line);
	    show_item (p, f, elem, SUB (deflexed));
	    done = increment_internal_index (tup, arr->dimensions);
	  }
	}
      }
    }
    tabs--;
  } else if (WHETHER (mode, STRUCT_SYMBOL)) {
    PACK_T *q = PACK (mode);
    tabs++;
    for (; q != NULL; q = NEXT (q)) {
      BYTE_T *elem = &item[q->offset];
      newline (f);
      sprintf (output_line, "field %s `%s'", moid_to_string (MOID (q), 32), TEXT (q));
      io_write_string (STDOUT_FILENO, output_line);
      show_item (p, f, elem, MOID (q));
    }
    tabs--;
  } else if (WHETHER (mode, UNION_SYMBOL)) {
    A68_POINTER *z = (A68_POINTER *) item;
    tabs++;
    newline (f);
    sprintf (output_line, "component %s", moid_to_string ((MOID_T *) (z->value), 32));
    io_write_string (STDOUT_FILENO, output_line);
    show_item (p, f, &item[SIZE_OF (A68_POINTER)], (MOID_T *) (z->value));
    tabs--;
  } else {
    BOOL_T init;
    if (check_initialisation (p, item, mode, &init)) {
      if (init) {
	if (WHETHER (mode, PROC_SYMBOL)) {
	  A68_PROCEDURE *z = (A68_PROCEDURE *) item;
	  if (z != NULL && z->status & STANDENV_PROCEDURE_MASK) {
	    io_write_string (STDOUT_FILENO, " standenv procedure");
	  } else if (z != NULL && z->status & SKIP_PROCEDURE_MASK) {
	    io_write_string (STDOUT_FILENO, " skip procedure");
	  } else if (z != NULL && z->body != NULL) {
	    sprintf (output_line, " source line %d, environ at frame[%d]", LINE ((NODE_T *) z->body)->number, z->environ);
	    io_write_string (STDOUT_FILENO, output_line);
	  } else {
	    io_write_string (STDOUT_FILENO, " cannot show value");
	  }
	} else if (mode == MODE (FORMAT)) {
	  A68_FORMAT *z = (A68_FORMAT *) item;
	  if (z != NULL && z->body != NULL) {
	    sprintf (output_line, " source line %d, environ at frame[%d]", LINE (z->body)->number, z->environ);
	    io_write_string (STDOUT_FILENO, output_line);
	  } else {
	    io_write_string (STDOUT_FILENO, " cannot show value");
	  }
	} else {
	  print_item (p, f, item, mode);
	}
      } else {
	io_write_string (STDOUT_FILENO, " uninitialised");
      }
    } else {
      io_write_string (STDOUT_FILENO, " cannot show value");
    }
  }
}

/*!
\brief overview of frame items
\param f
\param p
\param link
\param q
\param modif
**/

static void show_frame_items (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * q, int modif)
{
  (void) p;
  for (; q != NULL; q = NEXT (q)) {
    ADDR_T pos_in_frame_stack = link + FRAME_INFO_SIZE + q->offset;
    newline (STDOUT_FILENO);
    if (modif != ANONYMOUS) {
      sprintf (output_line, "frame[%d] %s `%s'", pos_in_frame_stack, moid_to_string (MOID (q), 32), SYMBOL (NODE (q)));
      io_write_string (STDOUT_FILENO, output_line);
    } else {
      switch (PRIO (q)) {
      case GENERATOR:
	{
	  sprintf (output_line, "frame[%d] LOC %s ", pos_in_frame_stack, moid_to_string (MOID (q), 32));
	  io_write_string (STDOUT_FILENO, output_line);
	  break;
	}
      default:
	{
	  sprintf (output_line, "frame[%d] internal %s", pos_in_frame_stack, moid_to_string (MOID (q), 32));
	  io_write_string (STDOUT_FILENO, output_line);
	  break;
	}
      }
    }
    show_item (p, f, FRAME_ADDRESS (pos_in_frame_stack), MOID (q));
  }
}

/*!
\brief view contents of stack frame
\param f
\param p
\param link
**/

void show_stack_frame (FILE_T f, NODE_T * p, ADDR_T link)
{
/* show the frame starting at frame pointer 'link', using symbol table from p as a map. */
  if (p != NULL) {
    SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
    sprintf (output_line, "\nstack frame level %d", q->level);
    io_write_string (STDOUT_FILENO, output_line);
    where (STDOUT_FILENO, p);
    show_frame_items (f, p, link, q->identifiers, IDENTIFIER);
    show_frame_items (f, p, link, q->operators, OPERATOR);
    show_frame_items (f, p, link, q->anonymous, ANONYMOUS);
  }
}

/*!
\brief shows line where 'p' is at and draws a '-' beneath the position
\param f
\param p
**/

void where (FILE_T f, NODE_T * p)
{
  SOURCE_LINE_T *l = p->info->line;
  int k, pos_in_line = strlen (l->string) - strlen (p->info->char_in_line);
  char z[BUFFER_SIZE];
  strncpy (z, l->string, BUFFER_SIZE);
  z[BUFFER_SIZE - 1] = '\0';
  for (k = 0; k < (int) strlen (z); k++) {
    if (z[k] == '\t') {
      z[k] = ' ';
    } else if (z[k] == '\n') {
      z[k] = '\0';
    }
  }
  sprintf (output_line, "\n%-4d %s\n     ", l->number, z);
  io_write_string (f, output_line);
  for (k = 0; k < pos_in_line; k++) {
    sprintf (output_line, " ");
    io_write_string (f, output_line);
  }
  sprintf (output_line, "-\n");
  io_write_string (f, output_line);
}

/*!
\brief shows lines around the line where 'p' is at
\param f
\param p
\param depth
**/

static void list (FILE_T f, NODE_T * p, int depth)
{
  if (p != NULL && p->info->module != NULL) {
    SOURCE_LINE_T *r = p->info->line;
    SOURCE_LINE_T *l = p->info->module->top_line;
    for (; l != NULL; l = NEXT (l)) {
      if (abs (r->number - l->number) <= depth) {
	int k, pos_in_line = strlen (l->string) - strlen (p->info->char_in_line);
	char z[BUFFER_SIZE];
	strncpy (z, l->string, BUFFER_SIZE);
	z[BUFFER_SIZE - 1] = '\0';
	for (k = 0; k < (int) strlen (z); k++) {
	  if (z[k] == '\t') {
	    z[k] = ' ';
	  } else if (z[k] == '\n') {
	    z[k] = '\0';
	  }
	}
	sprintf (output_line, "\n%-4d %s", l->number, z);
	io_write_string (f, output_line);
	if (l == r) {
	  sprintf (output_line, "\n     ");
	  io_write_string (f, output_line);
	  for (k = 0; k < pos_in_line; k++) {
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

/*!
\brief overview of the heap
\param f
\param p
\param z
**/

void show_heap (FILE_T f, NODE_T * p, A68_HANDLE * z)
{
  (void) p;
  sprintf (output_line, "\nsize=%d available=%d garbage collections=%d", heap_size, heap_available (), garbage_collects);
  io_write_string (f, output_line);
  for (; z != NULL; z = NEXT (z)) {
    newline (f);
    sprintf (output_line, "heap[%d-%d] %s", z->offset, z->offset + z->size, moid_to_string (MOID (z), 32));
    io_write_string (f, output_line);
  }
}

/*!
\brief overview of the stack
\param f
\param link
\param depth
**/

void stack_dump (FILE_T f, ADDR_T link, int depth)
{
  if (depth > 0 && link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    NODE_T *p = FRAME_TREE (link);
    show_stack_frame (f, p, link);
    stack_dump (f, dynamic_link, depth - 1);
  }
}

/*!
\brief tag_search
\param f
\param p
\param link
\param i
\param sym
**/

void tag_search (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * i, char *sym)
{
  for (; i != NULL; i = NEXT (i)) {
    ADDR_T pos_in_frame_stack = link + FRAME_INFO_SIZE + i->offset;
    if (NODE (i) != NULL && strcmp (SYMBOL (NODE (i)), sym) == 0) {
      where (f, NODE (i));
      sprintf (output_line, "frame[%d] %s `%s'", pos_in_frame_stack, moid_to_string (MOID (i), 32), SYMBOL (NODE (i)));
      io_write_string (STDOUT_FILENO, output_line);
      show_item (p, f, FRAME_ADDRESS (pos_in_frame_stack), MOID (i));
    }
  }
}

/*!
\brief search symbol in stack
\param f
\param link
\param sym
**/

void stack_search (FILE_T f, ADDR_T link, char *sym)
{
  if (link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    NODE_T *p = FRAME_TREE (link);
    if (p != NULL) {
      SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
      tag_search (f, p, link, q->identifiers, sym);
      tag_search (f, p, link, q->operators, sym);
    }
    stack_search (f, dynamic_link, sym);
  }
}

/*!
\brief set or reset breakpoints
\param p
\param set
\param num
**/

static void breakpoints (NODE_T * p, BOOL_T set, int num)
{
  for (; p != NULL; p = NEXT (p)) {
    breakpoints (SUB (p), set, num);
    if (set) {
      if (LINE (p)->number == num) {
	MASK (p) |= BREAKPOINT_MASK;
      }
    } else {
      MASK (p) &= ~BREAKPOINT_MASK;
    }
  }
}

/*!
\brief monitor command overview to tty
\param f
**/

static void genie_help (FILE_T f)
{
  WRITE ("(Supply sufficient characters for unique recognition)");
  WRITE ("\nbreakpoint n   set breakpoints on units in line `n'");
  WRITE ("\nbreakpoint     clear all breakpoints");
  WRITE ("\ncontinue       continue execution");
  WRITE ("\nexamine n      write value of symbols named `n' in the call stack");
  WRITE ("\nframe          write the current call stack frame");
  WRITE ("\nheap           write contents of the heap");
  WRITE ("\nhelp           print brief help text");
  WRITE ("\nht             suppress program output to tty");
  WRITE ("\nhx             terminates the program");
  WRITE ("\nlist [n]       show `n' lines around the interrupted line (default n=10)");
  WRITE ("\nnext           single step");
  WRITE ("\nstack [n]      write `n' frames in the call stack frames (default n=3)");
  WRITE ("\nstep           single step");
  WRITE ("\nquit           terminates the program");
  WRITE ("\nresume         continues execution");
  WRITE ("\nwhere          show the interrupted line");
}

/*!
\brief execute monitor command
\param p
\param cmd
\return 
**/

static BOOL_T single_stepper (NODE_T * p, char *cmd)
{
  if (match_string (cmd, "Continue", '\0')) {
    return (A_TRUE);
  } else if (match_string (cmd, "Examine", ' ')) {
    char *sym = cmd;
    while (!IS_SPACE (sym[0]) && sym[0] != '\0') {
      sym++;
    }
    while (IS_SPACE (sym[0]) && sym[0] != '\0') {
      sym++;
    }
    if (sym[0] != '\0') {
      stack_search (STDOUT_FILENO, frame_pointer, sym);
    }
    return (A_FALSE);
  } else if (match_string (cmd, "Frame", '\0')) {
    stack_dump (STDOUT_FILENO, frame_pointer, 1);
    return (A_FALSE);
  } else if (match_string (cmd, "HEAp", '\0')) {
    show_heap (STDOUT_FILENO, p, busy_handles);
    return (A_FALSE);
  } else if (match_string (cmd, "HELp", '\0')) {
    genie_help (STDOUT_FILENO);
    return (A_FALSE);
  } else if (match_string (cmd, "HT", '\0')) {
    halt_typing = A_TRUE;
    return (A_TRUE);
  } else if (match_string (cmd, "HX", '\0')) {
    exit_genie (p, A_RUNTIME_ERROR);
    return (A_TRUE);
  } else if (match_string (cmd, "Breakpoint", ' ')) {
    char *num = cmd, *end;
    unsigned k;
    while (!IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    while (IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    if (num[0] != '\0') {
      RESET_ERRNO;
      k = a68g_strtoul (num, &end, 10);
      if (end[0] == '\0' && errno == 0) {
	breakpoints (p->info->module->top_node, A_TRUE, k);
      } else {
	sprintf (output_line, "error: argument: %s", strerror (errno));
	io_write_string (STDOUT_FILENO, output_line);
      }
    } else {
      breakpoints (p->info->module->top_node, A_FALSE, 0);
    }
    return (A_FALSE);
  } else if (match_string (cmd, "List", ' ')) {
    char *num = cmd, *end;
    unsigned k;
    while (!IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    while (IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    if (num[0] != '\0') {
      RESET_ERRNO;
      k = a68g_strtoul (num, &end, 10);
      if (end[0] == '\0' && errno == 0) {
	list (STDOUT_FILENO, p, k);
      } else {
	sprintf (output_line, "error: argument: %s", strerror (errno));
	io_write_string (STDOUT_FILENO, output_line);
      }
    } else {
      list (STDOUT_FILENO, p, 10);
    }
    return (A_FALSE);
  } else if (match_string (cmd, "Resume", '\0')) {
    return (A_TRUE);
  } else if (match_string (cmd, "STAck", ' ')) {
    char *num = cmd, *end;
    unsigned k;
    while (!IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    while (IS_SPACE (num[0]) && num[0] != '\0') {
      num++;
    }
    if (num[0] != '\0') {
      RESET_ERRNO;
      k = a68g_strtoul (num, &end, 10);
      if (end[0] == '\0' && errno == 0) {
	stack_dump (STDOUT_FILENO, frame_pointer, k);
      } else {
	sprintf (output_line, "error: argument: %s", strerror (errno));
	io_write_string (STDOUT_FILENO, output_line);
      }
    } else {
      stack_dump (STDOUT_FILENO, frame_pointer, 3);
    }
    return (A_FALSE);
  } else if (match_string (cmd, "STEp", '\0')) {
    sys_request_flag = A_TRUE;
    return (A_TRUE);
  } else if (match_string (cmd, "Next", '\0')) {
    sys_request_flag = A_TRUE;
    return (A_TRUE);
  } else if (match_string (cmd, "Quit", '\0')) {
    exit_genie (p, A_RUNTIME_ERROR);
    return (A_TRUE);
  } else if (match_string (cmd, "Where", '\0')) {
    where (STDOUT_FILENO, p);
    return (A_FALSE);
  } else if (strcmp (cmd, "?") == 0) {
    genie_help (STDOUT_FILENO);
    return (A_FALSE);
  } else if (strlen (cmd) == 0) {
    return (A_FALSE);
  } else {
    io_write_string (STDOUT_FILENO, "? (try `help')");
    return (A_FALSE);
  }
}

/*!
\brief execute monitor
\param p
**/

void single_step (NODE_T * p)
{
  SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
  BOOL_T do_cmd = A_TRUE;
  in_monitor = A_TRUE;
  sys_request_flag = A_FALSE;
  where (STDOUT_FILENO, p);
  sprintf (output_line, "\nInterrupt at lexical level=%d", q->level);
  io_write_string (STDOUT_FILENO, output_line);
  sprintf (output_line, "\nCTRL-C will now terminate the program");
  io_write_string (STDOUT_FILENO, output_line);
  sprintf (output_line, "\nframe stack pointer=%d; expression stack pointer=%d", frame_pointer, stack_pointer);
  io_write_string (STDOUT_FILENO, output_line);
  while (do_cmd) {
    char *cmd;
    int empty = 0;
/* A little loop to prevent ugly things in batch. */
    while (strlen (cmd = read_string_from_tty ("monitor")) == 0 && empty++ < 9) {;
    }
    if (strlen (cmd) == 0) {
      exit_genie (p, A_RUNTIME_ERROR);
    }
    do_cmd = !single_stepper (p, cmd);
  }
  in_monitor = A_FALSE;
}
