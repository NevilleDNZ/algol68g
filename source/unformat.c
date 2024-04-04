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

/*-------1---------2---------3---------4---------5---------6---------7---------+
Transput library - General routines and unformatted transput.

But Eeyore wasn't listening. He was taking the balloon out, and putting
it back again, as happy as could be ... Winnie the Pooh, A.A. Milne.
- Revised Report on the Algorithmic Language Algol 68.                        */

A68_CHANNEL stand_in_channel, stand_out_channel, stand_draw_channel, stand_back_channel, stand_error_channel;
A68_REF stand_in, stand_out, stand_back, stand_error;

/*-------1---------2---------3---------4---------5---------6---------7---------+
Strings in transput are of arbitrary size. For this, we have transput buffers.
A transput buffer is a REF STRUCT (INT size, index, STRING buffer).
It is in the heap, but cannot be sweeped. If it is too small, we give up on
it and make a larger one.                                                     */

#define TRANSPUT_BUFFER_SIZE 	1024
static A68_REF ref_transput_buffer[MAX_TRANSPUT_BUFFER];

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_transput_buffer_size: set max number of chars in a transput buffer.       */

void
set_transput_buffer_size (int n, int size)
{
  A68_INT *k = (A68_INT *) (ADDRESS (&ref_transput_buffer[n]));
  k->status = INITIALISED_MASK;
  k->value = size;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_transput_buffer_index: set char index for transput buffer.                */

void
set_transput_buffer_index (int n, int index)
{
  A68_INT *k = (A68_INT *) (ADDRESS (&ref_transput_buffer[n]) + SIZE_OF (A68_INT));
  k->status = INITIALISED_MASK;
  k->value = index;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_transput_buffer_size: get max number of chars in a transput buffer.       */

int
get_transput_buffer_size (int n)
{
  A68_INT *k = (A68_INT *) (ADDRESS (&ref_transput_buffer[n]));
  return (k->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_transput_buffer_index: get char index for transput buffer.                */

int
get_transput_buffer_index (int n)
{
  A68_INT *k = (A68_INT *) (ADDRESS (&ref_transput_buffer[n]) + SIZE_OF (A68_INT));
  return (k->value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_transput_buffer: get char[] from transput buffer.                         */

char *
get_transput_buffer (int n)
{
  return ((char *) (ADDRESS (&ref_transput_buffer[n]) + 2 * SIZE_OF (A68_INT)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unblock_transput_buffer: mark that it is no longer in use.                    */

void
unblock_transput_buffer (int n)
{
  set_transput_buffer_index (n, -1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_unblocked_transput_buffer: find first unused one (for opening a file).    */

int
get_unblocked_transput_buffer (NODE_T * p)
{
  int k;
  for (k = 0; k < MAX_TRANSPUT_BUFFER; k++)
    {
      if (get_transput_buffer_index (k) == -1)
	{
	  return (k);
	}
    }
/* Oops! */
  diagnostic (A_RUNTIME_ERROR, p, "too many open files");
  exit_genie (p, A_RUNTIME_ERROR);
  return (-1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reset_transput_buffer: empty contents of transput buffer.                     */

void
reset_transput_buffer (int n)
{
  set_transput_buffer_index (n, 0);
  (get_transput_buffer (n))[0] = '\0';
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_transput_buffers: initialise transput buffers before use.                */

void
init_transput_buffers (NODE_T * p)
{
  int k;
  for (k = 0; k < MAX_TRANSPUT_BUFFER; k++)
    {
      ref_transput_buffer[k] = heap_generator (p, MODE (ROWS), 2 * SIZE_OF (A68_INT) + TRANSPUT_BUFFER_SIZE);
      protect_sweep_handle (&ref_transput_buffer[k]);
      set_transput_buffer_size (k, TRANSPUT_BUFFER_SIZE);
      reset_transput_buffer (k);
    }
/* Last buffers are available for FILE values */
  for (k = FIXED_TRANSPUT_BUFFERS; k < MAX_TRANSPUT_BUFFER; k++)
    {
      unblock_transput_buffer (k);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
enlarge_transput_buffer: make a transput buffer larger.                       */

void
enlarge_transput_buffer (NODE_T * p, int k, int size)
{
  int index = get_transput_buffer_index (k);
  char *sb_1 = get_transput_buffer (k), *sb_2;
  up_garbage_sema ();
  unprotect_sweep_handle (&ref_transput_buffer[k]);
  ref_transput_buffer[k] = heap_generator (p, MODE (ROWS), 2 * SIZE_OF (A68_INT) + size);
  protect_sweep_handle (&ref_transput_buffer[k]);
  set_transput_buffer_size (k, size);
  set_transput_buffer_index (k, index);
  sb_2 = get_transput_buffer (k);
  strcpy (sb_2, sb_1);
  down_garbage_sema ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_char_transput_buffer: add ch to transput buffer.

If the buffer is full, make it larger.                                        */

void
add_char_transput_buffer (NODE_T * p, int k, char ch)
{
  char *sb = get_transput_buffer (k);
  int size = get_transput_buffer_size (k);
  int index = get_transput_buffer_index (k);
  if (index == size - 2)
    {
      enlarge_transput_buffer (p, k, size + TRANSPUT_BUFFER_SIZE);
      add_char_transput_buffer (p, k, ch);
    }
  else
    {
      sb[index] = ch;
      sb[index + 1] = '\0';
      set_transput_buffer_index (k, index + 1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_string_transput_buffer: add char[] to transput buffer.                    */

void
add_string_transput_buffer (NODE_T * p, int k, char *ch)
{
  while (ch[0] != '\0')
    {
      add_char_transput_buffer (p, k, ch[0]);
      ch++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_a_string_transput_buffer: add A68 string to transput buffer.              */

void
add_a_string_transput_buffer (NODE_T * p, int k, BYTE_T * ref)
{
  A68_REF row = *(A68_REF *) ref;
  TEST_INIT (p, row, MODE (ROWS));
  if (row.status & INITIALISED_MASK)
    {
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      int size;
      GET_DESCRIPTOR (arr, tup, &row);
      size = ROW_SIZE (tup);
      if (size > 0)
	{
	  int i;
	  BYTE_T *base_address = ADDRESS (&arr->array);
	  for (i = tup->lower_bound; i <= tup->upper_bound; i++)
	    {
	      int addr = INDEX_1_DIM (arr, tup, i);
	      A68_CHAR *ch = (A68_CHAR *) & (base_address[addr]);
	      TEST_INIT (p, *ch, MODE (CHAR));
	      add_char_transput_buffer (p, k, ch->value);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_string_from_stack_transput_buffer: pop A68 string and add to buffer.      */

void
add_string_from_stack_transput_buffer (NODE_T * p, int k)
{
  DECREMENT_STACK_POINTER (p, SIZE_OF (A68_REF));
  add_a_string_transput_buffer (p, k, STACK_TOP);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pop_char_transput_buffer: pop first character from transput buffer.           */

char
pop_char_transput_buffer (int k)
{
  char *sb = get_transput_buffer (k);
  int index = get_transput_buffer_index (k);
  if (index <= 0)
    {
      return ('\0');
    }
  else
    {
      char ch = sb[0];
      MOVE (&sb[0], &sb[1], index);
      set_transput_buffer_index (k, index - 1);
      return (ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Routines that involve the A68 expression stack.                               */

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_string_from_stack: print A68 string on the stack to file          */

void
genie_write_string_from_stack (NODE_T * p, A68_REF ref_file)
{
  A68_REF row;
  int size;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROWS));
  size = a68_string_size (p, row);
  if (size > 0)
    {
      FILE_T f = FILE_DEREF (&ref_file)->fd;
      set_transput_buffer_index (OUTPUT_BUFFER, 0);	/* discard anything in there */
      if (get_transput_buffer_size (OUTPUT_BUFFER) < 1 + size)
	{
	  enlarge_transput_buffer (p, OUTPUT_BUFFER, 1 + size);
	}
      io_write_string (f, a_to_c_string (p, get_transput_buffer (OUTPUT_BUFFER), row));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
stack_string: allocate a temporary string on the stack.                       */

char *
stack_string (NODE_T * p, int size)
{
  char *new_str = (char *) STACK_TOP;
  int rem = size % SIZE_OF (int);
  if (rem)
    {
      size += SIZE_OF (int) - rem;
    }
  INCREMENT_STACK_POINTER (p, size);
  memset (new_str, '\0', size);
  return (new_str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Transput basic RTS routines.                                                  */


/*-------1---------2---------3---------4---------5---------6---------7---------+
REF FILE standin                                                              */

void
genie_stand_in (NODE_T * p)
{
  PUSH (p, &stand_in, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
REF FILE standout                                                             */

void
genie_stand_out (NODE_T * p)
{
  PUSH (p, &stand_out, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
REF FILE standback                                                            */

void
genie_stand_back (NODE_T * p)
{
  PUSH (p, &stand_back, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
REF FILE standerror                                                           */

void
genie_stand_error (NODE_T * p)
{
  PUSH (p, &stand_error, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR error char                                                               */

void
genie_error_char (NODE_T * p)
{
  PUSH_CHAR (p, ERROR_CHAR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR exp char                                                                 */

void
genie_exp_char (NODE_T * p)
{
  PUSH_CHAR (p, EXPONENT_CHAR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR flip char                                                                */

void
genie_flip_char (NODE_T * p)
{
  PUSH_CHAR (p, FLIP_CHAR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR flop char                                                                */

void
genie_flop_char (NODE_T * p)
{
  PUSH_CHAR (p, FLOP_CHAR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR null char                                                                */

void
genie_null_char (NODE_T * p)
{
  PUSH_CHAR (p, '\0');
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHAR blank                                                                    */

void
genie_blank_char (NODE_T * p)
{
  PUSH_CHAR (p, BLANK_CHAR);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHANNEL standin channel                                                       */

void
genie_stand_in_channel (NODE_T * p)
{
  PUSH_CHANNEL (p, stand_in_channel);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHANNEL standout channel                                                      */

void
genie_stand_out_channel (NODE_T * p)
{
  PUSH_CHANNEL (p, stand_out_channel);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHANNEL stand draw channel                                                    */

void
genie_stand_draw_channel (NODE_T * p)
{
  PUSH_CHANNEL (p, stand_draw_channel);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHANNEL standback channel                                                     */

void
genie_stand_back_channel (NODE_T * p)
{
  PUSH_CHANNEL (p, stand_back_channel);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
CHANNEL standerror channel                                                    */

void
genie_stand_error_channel (NODE_T * p)
{
  PUSH_CHANNEL (p, stand_error_channel);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC STRING program idf                                                       */

void
genie_program_idf (NODE_T * p)
{
  PUSH_REF (p, c_to_a_string (p, a68_prog.files.generic_name));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
FILE and CHANNEL initialisations.                                             */

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_default_mended_procedure*/

void
set_default_mended_procedure (A68_PROCEDURE * z)
{
  z->body = nil_pointer;
  z->environ = nil_ref;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_channel: ibid.                                                           */

static void
init_channel (A68_CHANNEL * chan, BOOL_T r, BOOL_T s, BOOL_T g, BOOL_T p, BOOL_T b, BOOL_T d)
{
  chan->status = INITIALISED_MASK;
  chan->reset = r;
  chan->set = s;
  chan->get = g;
  chan->put = p;
  chan->bin = b;
  chan->draw = d;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_default_mended_procedures: default event handlers.                        */

void
set_default_mended_procedures (A68_FILE * f)
{
  set_default_mended_procedure (&(f->file_end_mended));
  set_default_mended_procedure (&(f->page_end_mended));
  set_default_mended_procedure (&(f->line_end_mended));
  set_default_mended_procedure (&(f->value_error_mended));
  set_default_mended_procedure (&(f->open_error_mended));
  set_default_mended_procedure (&(f->transput_error_mended));
  set_default_mended_procedure (&(f->format_end_mended));
  set_default_mended_procedure (&(f->format_error_mended));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_file: set up a REF FILE object.                                          */

static void
init_file (NODE_T * p, A68_REF * ref_file, A68_CHANNEL c, FILE_T s, BOOL_T rm, BOOL_T wm, BOOL_T cm)
{
  A68_FILE *f;
  *ref_file = heap_generator (p, MODE (REF_FILE), SIZE_OF (A68_FILE));
  protect_sweep_handle (ref_file);
  f = (A68_FILE *) ADDRESS (ref_file);
  f->status = INITIALISED_MASK;
  f->identification = nil_ref;
  f->terminator = nil_ref;
  f->channel = c;
  f->fd = s;
  f->transput_buffer = get_unblocked_transput_buffer (p);
  reset_transput_buffer (f->transput_buffer);
  f->eof = A_FALSE;
  f->tmp_file = A_FALSE;
  f->opened = A_TRUE;
  f->open_exclusive = A_FALSE;
  f->read_mood = rm;
  f->write_mood = wm;
  f->char_mood = cm;
  f->draw_mood = A_FALSE;
  f->format = nil_format;
  set_default_mended_procedures (f);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_init_transput: initialise the transput RTL.

Algol68G has four "standard files":

       stand in
       stand out
       stand back
       stand error                                                           */

void
genie_init_transput (NODE_T * p)
{
  init_transput_buffers (p);
/* Channels */
  init_channel (&stand_in_channel, A_FALSE, A_FALSE, A_TRUE, A_FALSE, A_FALSE, A_FALSE);
  init_channel (&stand_out_channel, A_FALSE, A_FALSE, A_FALSE, A_TRUE, A_FALSE, A_FALSE);
  init_channel (&stand_back_channel, A_TRUE, A_TRUE, A_TRUE, A_TRUE, A_TRUE, A_FALSE);
  init_channel (&stand_error_channel, A_FALSE, A_FALSE, A_FALSE, A_TRUE, A_FALSE, A_FALSE);
#ifdef HAVE_PLOTUTILS
  init_channel (&stand_draw_channel, A_FALSE, A_FALSE, A_FALSE, A_FALSE, A_FALSE, A_TRUE);
#else
  init_channel (&stand_draw_channel, A_FALSE, A_FALSE, A_FALSE, A_FALSE, A_FALSE, A_TRUE);
#endif
/* Files */
  init_file (p, &stand_in, stand_in_channel, STDIN_FILENO, A_TRUE, A_FALSE, A_TRUE);
  init_file (p, &stand_out, stand_out_channel, STDOUT_FILENO, A_FALSE, A_TRUE, A_TRUE);
  init_file (p, &stand_back, stand_back_channel, -1, A_FALSE, A_FALSE, A_FALSE);
  init_file (p, &stand_error, stand_error_channel, STDERR_FILENO, A_FALSE, A_TRUE, A_TRUE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) STRING idf                                                    */

void
genie_idf (NODE_T * p)
{
  A68_REF ref_file, ref_filename;
  char *filename;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  ref_file = *(A68_REF *) STACK_TOP;
  ref_filename = FILE_DEREF (&ref_file)->identification;
  TEST_INIT (p, ref_filename, MODE (ROWS));
  TEST_NIL (p, ref_filename, MODE (ROWS));
  filename = (char *) ADDRESS (&ref_filename);
  PUSH_REF (p, c_to_a_string (p, filename));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) STRING term                                                   */

void
genie_term (NODE_T * p)
{
  A68_REF ref_file, ref_term;
  char *term;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  ref_file = *(A68_REF *) STACK_TOP;
  ref_term = FILE_DEREF (&ref_file)->terminator;
  TEST_INIT (p, ref_term, MODE (ROWS));
  TEST_NIL (p, ref_term, MODE (ROWS));
  term = (char *) ADDRESS (&ref_term);
  PUSH_REF (p, c_to_a_string (p, term));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, STRING) VOID make term                                        */

void
genie_make_term (NODE_T * p)
{
  int size;
  A68_FILE *file;
  A68_REF ref_file, ref_str;
  POP_REF (p, &ref_str);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  ref_file = *(A68_REF *) STACK_TOP;
  file = FILE_DEREF (&ref_file);
/* Don't check initialisation so we can "make term" before opening.
   That is ok.                                                                */
  size = a68_string_size (p, ref_str);
  if (((file->terminator.status & INITIALISED_MASK) != 0) && !IS_NIL (file->terminator))
    {
      unprotect_sweep_handle (&(file->terminator));
    }
  file->terminator = heap_generator (p, MODE (C_STRING), 1 + size);
  protect_sweep_handle (&(file->terminator));
  a_to_c_string (p, (char *) ADDRESS (&(file->terminator)), ref_str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL put possible                                             */

void
genie_put_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.put);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL get possible                                             */

void
genie_get_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.get);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL bin possible                                             */

void
genie_bin_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.bin);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL set possible                                             */

void
genie_set_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.set);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL reset possible                                           */

void
genie_reset_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.reset);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) BOOL draw possible                                            */

void
genie_draw_possible (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  PUSH_BOOL (p, file->channel.draw);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, STRING, CHANNEL) INT open                                     */

void
genie_open (NODE_T * p)
{
  A68_CHANNEL channel;
  A68_REF ref_iden, ref_file;
  A68_FILE *file;
  int size;
  POP (p, &channel, SIZE_OF (A68_CHANNEL));
  POP_REF (p, &ref_iden);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  file->status = INITIALISED_MASK;
  file->channel = channel;
  file->opened = A_TRUE;
  file->open_exclusive = A_FALSE;
  file->read_mood = A_FALSE;
  file->write_mood = A_FALSE;
  file->char_mood = A_FALSE;
  file->draw_mood = A_FALSE;
  size = a68_string_size (p, ref_iden);
  if (((file->identification.status & INITIALISED_MASK) != 0) && !IS_NIL (file->identification))
    {
      unprotect_sweep_handle (&(file->identification));
    }
  file->identification = heap_generator (p, MODE (C_STRING), 1 + size);
  protect_sweep_handle (&(file->identification));
  a_to_c_string (p, (char *) ADDRESS (&(file->identification)), ref_iden);
  file->terminator = nil_ref;
  file->format = nil_format;
  file->fd = -1;
  file->device.stream = NULL;
  set_default_mended_procedures (file);
  PUSH_INT (p, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, STRING, CHANNEL) INT establish                                */

void
genie_establish (NODE_T * p)
{
  A68_CHANNEL channel;
  A68_REF ref_iden, ref_file;
  A68_FILE *file;
  int size;
  POP (p, &channel, SIZE_OF (A68_CHANNEL));
  POP_REF (p, &ref_iden);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  file->status = INITIALISED_MASK;
  file->channel = channel;
  file->opened = A_TRUE;
  file->open_exclusive = A_TRUE;
  file->read_mood = A_FALSE;
  file->write_mood = A_FALSE;
  file->char_mood = A_FALSE;
  file->draw_mood = A_FALSE;
  if (!file->channel.put)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "putting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  size = a68_string_size (p, ref_iden);
  if (((file->identification.status & INITIALISED_MASK) != 0) && !IS_NIL (file->identification))
    {
      unprotect_sweep_handle (&(file->identification));
    }
  file->identification = heap_generator (p, MODE (C_STRING), 1 + size);
  protect_sweep_handle (&(file->identification));
  a_to_c_string (p, (char *) ADDRESS (&(file->identification)), ref_iden);
  file->terminator = nil_ref;
  file->format = nil_format;
  file->fd = -1;
  file->device.stream = NULL;
  set_default_mended_procedures (file);
  PUSH_INT (p, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, CHANNEL) INT create                                           */

void
genie_create (NODE_T * p)
{
  A68_CHANNEL channel;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &channel, SIZE_OF (A68_CHANNEL));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  file->status = INITIALISED_MASK;
  file->channel = channel;
  file->opened = A_TRUE;
  file->open_exclusive = A_FALSE;
  file->read_mood = A_FALSE;
  file->write_mood = A_FALSE;
  file->char_mood = A_FALSE;
  file->draw_mood = A_FALSE;
  if (((file->identification.status & INITIALISED_MASK) != 0) && !IS_NIL (file->identification))
    {
      unprotect_sweep_handle (&(file->identification));
    }
  file->identification = nil_ref;
  file->terminator = nil_ref;
  file->format = nil_format;
  file->fd = -1;
  file->device.stream = NULL;
  set_default_mended_procedures (file);
  PUSH_INT (p, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID close                                                    */

void
genie_close (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened || (!file->read_mood && !file->write_mood && !file->draw_mood))
    {
      return;
    }
  file->device.device_made = A_FALSE;
#ifdef HAVE_PLOTUTILS
  if (file->device.device_opened)
    {
      close_device (p, file);
      file->device.stream = NULL;
      return;
    }
#endif
  if (file->fd != -1 && close (file->fd) == -1)
    {
      diagnostic (A_RUNTIME_ERROR, p, "error while closing file");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else
    {
      file->fd = -1;
      file->opened = A_FALSE;
      unblock_transput_buffer (file->transput_buffer);
      set_default_mended_procedures (file);
    }
  if (file->tmp_file)
    {
/* Remove the file if it is temporary */
      if (!IS_NIL (file->identification))
	{
	  char *filename;
	  TEST_INIT (p, file->identification, MODE (ROWS));
	  filename = (char *) ADDRESS (&(file->identification));
	  if (remove (filename) != 0)
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "error while scratching file");
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  unprotect_sweep_handle (&(file->identification));
	  file->identification = nil_ref;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID lock                                                     */

void
genie_lock (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened || (!file->read_mood && !file->write_mood && !file->draw_mood))
    {
      return;
    }
  file->device.device_made = A_FALSE;
#ifdef HAVE_PLOTUTILS
  if (file->device.device_opened)
    {
      close_device (p, file);
      file->device.stream = NULL;
      return;
    }
#endif
#if defined HAVE_UNIX && !defined __DEVCPP__
  errno = 0;
  fchmod (file->fd, (mode_t) 0x0);
  abend (errno != 0, "cannot lock file", NULL);
#endif
  if (file->fd != -1 && close (file->fd) == -1)
    {
      diagnostic (A_RUNTIME_ERROR, p, "error while locking file");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else
    {
      file->fd = -1;
      file->opened = A_FALSE;
      unblock_transput_buffer (file->transput_buffer);
      set_default_mended_procedures (file);
    }
  if (file->tmp_file)
    {
/* Remove the file if it is temporary */
      if (!IS_NIL (file->identification))
	{
	  char *filename;
	  TEST_INIT (p, file->identification, MODE (ROWS));
	  filename = (char *) ADDRESS (&(file->identification));
	  if (remove (filename) != 0)
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "error while scratching file");
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  unprotect_sweep_handle (&(file->identification));
	  file->identification = nil_ref;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID erase                                                    */

void
genie_erase (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened || (!file->read_mood && !file->write_mood && !file->draw_mood))
    {
      return;
    }
  file->device.device_made = A_FALSE;
#ifdef HAVE_PLOTUTILS
  if (file->device.device_opened)
    {
      close_device (p, file);
      file->device.stream = NULL;
      return;
    }
#endif
  if (file->fd != -1 && close (file->fd) == -1)
    {
      diagnostic (A_RUNTIME_ERROR, p, "error while scratching file");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else
    {
      file->fd = -1;
      file->opened = A_FALSE;
      unblock_transput_buffer (file->transput_buffer);
      set_default_mended_procedures (file);
    }
/* Remove the file */
  if (!IS_NIL (file->identification))
    {
      char *filename;
      TEST_INIT (p, file->identification, MODE (ROWS));
      filename = (char *) ADDRESS (&(file->identification));
      if (remove (filename) != 0)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "error while scratching file");
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      unprotect_sweep_handle (&(file->identification));
      file->identification = nil_ref;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID reset                                                    */

void
genie_reset (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->fd != -1 && close (file->fd) == -1)
    {
      diagnostic (A_RUNTIME_ERROR, p, "error while resetting file");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  else
    {
      file->read_mood = A_FALSE;
      file->write_mood = A_FALSE;
      file->char_mood = A_FALSE;
      file->draw_mood = A_FALSE;
      file->fd = -1;
      set_default_mended_procedures (file);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on file end                        */

void
genie_on_file_end (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->file_end_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on page end                        */

void
genie_on_page_end (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->page_end_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on line end                        */

void
genie_on_line_end (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->line_end_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on format end                      */

void
genie_on_format_end (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->format_end_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on format error                    */

void
genie_on_format_error (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->format_error_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on value error                     */

void
genie_on_value_error (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->value_error_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on open error                      */

void
genie_on_open_error (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->open_error_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, PROC (REF FILE) BOOL) VOID on transput error                  */

void
genie_on_transput_error (NODE_T * p)
{
  A68_PROCEDURE z;
  A68_REF ref_file;
  A68_FILE *file;
  POP (p, &z, SIZE_OF (A68_PROCEDURE));
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->transput_error_mended = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
on_event_handler: invoke event routine.                                       */

void
on_event_handler (NODE_T * p, A68_PROCEDURE z, A68_REF ref_file)
{
  if (z.body.value == NULL)
    {				/* Default action */
      PUSH_BOOL (p, A_FALSE);
    }
  else
    {
      ADDR_T sp0 = stack_pointer;
      PUSH_REF_FILE (p, ref_file);
      if (z.body.status & STANDENV_PROCEDURE_MASK)
	{
/* RTS has no interpretable body */
	  GENIE_PROCEDURE *proc = (GENIE_PROCEDURE *) (z.body.value);
	  if (proc != NULL)
	    {
	      (void) proc (p);
	    }
	}
      else
	{
	  NODE_T *body = ((NODE_T *) z.body.value);
	  if (WHETHER (body, ROUTINE_TEXT))
	    {
	      NODE_T *entry = SUB (body);
	      PACK_T *args = PACK (MODE (PROC_REF_FILE_BOOL));
	      ADDR_T fp0;
	      if (args == NULL)
		{
		  diagnostic (A_RUNTIME_ERROR, p, INTERNAL_ERROR, "genie_call");
		  exit_genie (p, A_RUNTIME_ERROR);
		}
	      open_frame (entry, IS_PROCEDURE_PARM, z.environ.offset);
/* Copy arguments from stack to frame */
	      stack_pointer = sp0;
	      fp0 = 0;
	      for (; args != NULL; args = NEXT (args))
		{
		  int size = MOID_SIZE (MOID (args));
		  MOVE ((BYTE_T *) FRAME_LOCAL (frame_pointer, fp0), (BYTE_T *) STACK_ADDRESS (sp0), (unsigned) size);
		  sp0 += size;
		  fp0 += size;
		}
/* Execute routine text */
	      EXECUTE_UNIT (NEXT (NEXT (NEXT (entry))));
	      CLOSE_FRAME;
	    }
	  else
	    {
	      EXECUTE_UNIT (body);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
end_of_file_error: handle end-of-file event.                                  */

void
end_of_file_error (NODE_T * p, A68_REF ref_file)
{
  A68_BOOL z;
  on_event_handler (p, FILE_DEREF (&ref_file)->file_end_mended, ref_file);
  POP_BOOL (p, &z);
  if (z.value == A_FALSE)
    {
      diagnostic (A_RUNTIME_ERROR, p, "attempt to read past end of file");
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
open_error: handle file-open-error event.                                     */

void
open_error (NODE_T * p, A68_REF ref_file, char *mode)
{
  A68_BOOL z;
  on_event_handler (p, FILE_DEREF (&ref_file)->open_error_mended, ref_file);
  POP_BOOL (p, &z);
  if (z.value == A_FALSE)
    {
      A68_FILE *file;
      char *filename;
      TEST_NIL (p, ref_file, MODE (REF_FILE));
      file = FILE_DEREF (&ref_file);
      TEST_INIT (p, *file, MODE (FILE));
      if (!IS_NIL (file->identification))
	{
	  filename = (char *) ADDRESS (&(FILE_DEREF (&ref_file)->identification));
	}
      else
	{
	  filename = "(NIL filename)";
	}
      diagnostic (A_RUNTIME_ERROR, p, "cannot open Z for Y", filename, mode);
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
value_error: handle value_error event.                                        */

void
value_error (NODE_T * p, MOID_T * m, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  if (f->eof)
    {
      end_of_file_error (p, ref_file);
    }
  else
    {
      A68_BOOL z;
      on_event_handler (p, f->value_error_mended, ref_file);
      POP_BOOL (p, &z);
      if (z.value == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "error transputting M value", m);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
transput_error: handle transput-error event.                                  */

void
transput_error (NODE_T * p, A68_REF ref_file, MOID_T * m)
{
  A68_BOOL z;
  on_event_handler (p, FILE_DEREF (&ref_file)->transput_error_mended, ref_file);
  POP_BOOL (p, &z);
  if (z.value == A_FALSE)
    {
      diagnostic (A_RUNTIME_ERROR, p, "cannot transput M", m);
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Implementation of put and get.                                                */

/*-------1---------2---------3---------4---------5---------6---------7---------+
char_scanner: get next char from file.                                        */

int
char_scanner (A68_FILE * f)
{
  if (get_transput_buffer_index (f->transput_buffer) > 0)
    {
      f->eof = A_FALSE;
      return (pop_char_transput_buffer (f->transput_buffer));
    }
  else
    {
      ssize_t chars_read;
      char ch;
      chars_read = io_read (f->fd, &ch, 1);
      if (chars_read == 1)
	{
	  f->eof = A_FALSE;
	  return (ch);
	}
      else
	{
	  f->eof = A_TRUE;
	  return (EOF);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
unchar_scanner: push back look-ahead character to file.                       */

void
unchar_scanner (NODE_T * p, A68_FILE * f, char ch)
{
  f->eof = A_FALSE;
  add_char_transput_buffer (p, f->transput_buffer, ch);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID new line */

void
genie_new_line (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      io_write_string (file->fd, "\n");
    }
  else if (file->read_mood)
    {
      BOOL_T go_on = !file->eof;
      while (go_on)
	{
	  int ch = char_scanner (file);
	  go_on = (ch != '\n') && (ch != EOF) && !file->eof;
	}
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, "file has undetermined mood");
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID new page                                                 */

void
genie_new_page (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      io_write_string (file->fd, "\f");
    }
  else if (file->read_mood)
    {
      BOOL_T go_on = !file->eof;
      while (go_on)
	{
	  int ch = char_scanner (file);
	  go_on = (ch != '\f') && (ch != EOF) && !file->eof;
	}
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, "file has undetermined mood");
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE) VOID space                                                    */

void
genie_space (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      io_write_string (file->fd, " ");
    }
  else if (file->read_mood)
    {
      if (!file->eof)
	{
	  (void) char_scanner (file);
	}
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, "file has undetermined mood");
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
skip_nl_ff.                                                                   */

#define IS_NL_FF(ch) ((ch) == '\n' || (ch) == '\f')

void
skip_nl_ff (NODE_T * p, int *ch, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  while ((*ch) != EOF && IS_NL_FF (*ch))
    {
      A68_BOOL z;
      if (*ch == '\n')
	{
	  on_event_handler (p, f->line_end_mended, ref_file);
	}
      else
	{
	  on_event_handler (p, f->page_end_mended, ref_file);
	}
      POP_BOOL (p, &z);
      if (z.value == A_FALSE)
	{
	  (*ch) = char_scanner (f);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_integer: scan an int from file.                                          */

void
scan_integer (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  int ch;
  reset_transput_buffer (INPUT_BUFFER);
  ch = char_scanner (f);
  while (ch != EOF && (IS_SPACE (ch) || IS_NL_FF (ch)))
    {
      if (IS_NL_FF (ch))
	{
	  skip_nl_ff (p, &ch, ref_file);
	}
      else
	{
	  ch = char_scanner (f);
	}
    }
  if (ch != EOF && (ch == '+' || ch == '-'))
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
    }
  while (ch != EOF && IS_DIGIT (ch))
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
    }
  if (ch != EOF)
    {
      unchar_scanner (p, f, ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_real: scan a real from file.                                             */

void
scan_real (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  char x_e = EXPONENT_CHAR;
  int ch;
  reset_transput_buffer (INPUT_BUFFER);
  ch = char_scanner (f);
  while (ch != EOF && (IS_SPACE (ch) || IS_NL_FF (ch)))
    {
      if (IS_NL_FF (ch))
	{
	  skip_nl_ff (p, &ch, ref_file);
	}
      else
	{
	  ch = char_scanner (f);
	}
    }
  if (ch != EOF && (ch == '+' || ch == '-'))
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
    }
  while (ch != EOF && IS_DIGIT (ch))
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
    }
  if (ch == EOF || !(ch == '.' || TO_UPPER (ch) == x_e))
    {
      goto salida;
    }
  if (ch == '.')
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
      while (ch != EOF && IS_DIGIT (ch))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, ch);
	  ch = char_scanner (f);
	}
    }
  if (ch == EOF || TO_UPPER (ch) != x_e)
    {
      goto salida;
    }
  if (TO_UPPER (ch) == x_e)
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
      while (ch != EOF && ch == ' ')
	{
	  ch = char_scanner (f);
	}
      if (ch != EOF && (ch == '+' || ch == '-'))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, ch);
	  ch = char_scanner (f);
	}
      while (ch != EOF && IS_DIGIT (ch))
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, ch);
	  ch = char_scanner (f);
	}
    }
salida:
  if (ch != EOF)
    {
      unchar_scanner (p, f, ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_bits: scan a bits from file.                                             */

void
scan_bits (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  int ch, flip = FLIP_CHAR, flop = FLOP_CHAR;
  reset_transput_buffer (INPUT_BUFFER);
  ch = char_scanner (f);
  while (ch != EOF && (IS_SPACE (ch) || IS_NL_FF (ch)))
    {
      if (IS_NL_FF (ch))
	{
	  skip_nl_ff (p, &ch, ref_file);
	}
      else
	{
	  ch = char_scanner (f);
	}
    }
  while (ch != EOF && (ch == flip || ch == flop))
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
      ch = char_scanner (f);
    }
  if (ch != EOF)
    {
      unchar_scanner (p, f, ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_char: scan a char from file.                                             */

void
scan_char (NODE_T * p, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  int ch;
  reset_transput_buffer (INPUT_BUFFER);
  ch = char_scanner (f);
  skip_nl_ff (p, &ch, ref_file);
  if (ch != EOF)
    {
      add_char_transput_buffer (p, INPUT_BUFFER, ch);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
scan_string: scan a string from file.                                         */

void
scan_string (NODE_T * p, char *term, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  if (f->eof)
    {
      end_of_file_error (p, ref_file);
    }
  else
    {
      int ch;
      reset_transput_buffer (INPUT_BUFFER);
      ch = char_scanner (f);
      skip_nl_ff (p, &ch, ref_file);
      while (ch != EOF && (term != NULL ? strchr (term, ch) == NULL : A_TRUE) && !(IS_NL_FF (ch)) && ch != '\0')
	{
	  add_char_transput_buffer (p, INPUT_BUFFER, ch);
	  ch = char_scanner (f);
	}
      if (ch != EOF)
	{
	  unchar_scanner (p, f, ch);
	}
      else if (get_transput_buffer_index (INPUT_BUFFER) == 0)
	{
	  end_of_file_error (p, ref_file);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
open_physical_file: open a file, or establish it.
mode: required access.                                                        */

FILE_T open_physical_file (NODE_T * p, A68_REF ref_file, int mode, mode_t acc)
{
  A68_FILE *file;
  A68_REF ref_filename;
  char *filename;
  (void) acc;
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  file->tmp_file = A_FALSE;
  if (IS_NIL (file->identification))
    {
/* No identification, so generate a unique identification.
  "tmpnam" is not safe, "mkstemp" is Unix, so a68g brings its own tmpnam.    */
#ifdef PRE_MACOS_X
#define TMP_SIZE 8
#define TRIALS 32
#else
#define TMP_SIZE 16
#define TRIALS 512
#endif
      char filename[TMP_SIZE + 16];
      char *letters = "0123456789abcdefghijklmnopqrstuvwxyz";
      int k, len = strlen (letters);
      BOOL_T good_file = A_FALSE;
      for (k = 0; k < TRIALS && good_file == A_FALSE; k++)
	{
	  int j, index;
	  strcpy (filename, A68G_NAME);
	  strcat (filename, ".");
	  for (j = 0; j < TMP_SIZE; j++)
	    {
	      char chars[2];
	      do
		{
		  index = (int) (rng_53_bit () * len);
		}
	      while (index < 0 || index >= len);
	      chars[0] = letters[index];
	      chars[1] = '\0';
	      strcat (filename, chars);
	    }
	  strcat (filename, ".tmp");
	  errno = 0;
#ifdef PRE_MACOS_X
	  file->fd = open (filename, mode | O_EXCL);
#else
	  file->fd = open (filename, mode | O_EXCL, acc);
#endif
	  good_file = (file->fd != -1 && errno == 0);
	}
#undef TMP_SIZE
#undef TRIALS
      if (good_file == A_FALSE)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "cannot create unique temporary file name");
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      file->identification = heap_generator (p, MODE (C_STRING), 1 + strlen (filename));
      protect_sweep_handle (&(file->identification));
      strcpy ((char *) ADDRESS (&(file->identification)), filename);
      file->transput_buffer = get_unblocked_transput_buffer (p);
      reset_transput_buffer (file->transput_buffer);
      file->eof = A_FALSE;
      file->tmp_file = A_TRUE;
      return (file->fd);
    }
  else
    {
/* Opening an identified file. */
      ref_filename = file->identification;
      TEST_INIT (p, ref_filename, MODE (ROWS));
      TEST_NIL (p, ref_filename, MODE (ROWS));
      filename = (char *) ADDRESS (&ref_filename);
      if (file->open_exclusive)
	{
/* Establishing requires that the file does not exist. */
	  if (mode == (A_WRITE_ACCESS))
	    {
	      mode |= O_EXCL;
	    }
	  file->open_exclusive = A_FALSE;
	}
#ifdef PRE_MACOS_X
      file->fd = open (filename, mode);
#else
      file->fd = open (filename, mode, acc);
#endif
      file->transput_buffer = get_unblocked_transput_buffer (p);
      reset_transput_buffer (file->transput_buffer);
      file->eof = A_FALSE;
      return (file->fd);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_call_proc_ref_file_void: call PROC (REF FILE) VOID during transput.     */

void
genie_call_proc_ref_file_void (NODE_T * p, A68_REF ref_file, A68_PROCEDURE u)
{
  ADDR_T save_sp = stack_pointer;
  if (u.body.status & STANDENV_PROCEDURE_MASK)
    {
/* RTS has no interpretable body */
      GENIE_PROCEDURE *pr = (GENIE_PROCEDURE *) (u.body.value);
      PUSH_REF_FILE (p, ref_file);
      if (pr != NULL)
	{
	  (void) pr (p);
	}
    }
  else
    {
      NODE_T *body = ((NODE_T *) u.body.value);
      if (WHETHER (body, ROUTINE_TEXT))
	{
	  NODE_T *entry = SUB (body);
	  open_frame (entry, IS_PROCEDURE_PARM, u.environ.offset);
	  MOVE (FRAME_OFFSET (FRAME_INFO_SIZE), &ref_file, MOID_SIZE (MODE (REF_FILE)));
	  if (WHETHER (entry, PARAMETER_PACK))
	    {
	      entry = NEXT (entry);
	    }
	  EXECUTE_UNIT (NEXT (NEXT (entry)));
	  CLOSE_FRAME;
	}
      else
	{
	  EXECUTE_UNIT (body);
	}
    }
  stack_pointer = save_sp;
}

/* Unformatted transput */

/*-------1---------2---------3---------4---------5---------6---------7---------+
char_value                                                                    */

static int
char_value (int ch)
{
  switch (ch)
    {
    case '0':
      return (0);
    case '1':
      return (1);
    case '2':
      return (2);
    case '3':
      return (3);
    case '4':
      return (4);
    case '5':
      return (5);
    case '6':
      return (6);
    case '7':
      return (7);
    case '8':
      return (8);
    case '9':
      return (9);
    case 'A':
    case 'a':
      return (10);
    case 'B':
    case 'b':
      return (11);
    case 'C':
    case 'c':
      return (12);
    case 'D':
    case 'd':
      return (13);
    case 'E':
    case 'e':
      return (14);
    case 'F':
    case 'f':
      return (15);
    default:
      return (-1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
a68g_strtoul: special case for a68g since some systems have no strtoul.
Assume that str is either NULL, empty or contains a valid unsigned int.       */

unsigned long
a68g_strtoul (char *str, char **end, int base)
{
  if (str == NULL || str[0] == '\0')
    {
      (*end) = NULL;
      errno = EDOM;
      return (0);
    }
  else
    {
      int j, k = 0, start;
      char *q = str;
      unsigned long mul = 1, sum = 0;
      while (IS_SPACE (q[k]))
	{
	  k++;
	}
      if (q[k] == '+')
	{
	  k++;
	}
      start = k;
      while (IS_XDIGIT (q[k]))
	{
	  k++;
	}
      (*end) = &q[k];
      for (j = k - 1; j >= start; j--)
	{
	  unsigned add = char_value (q[j]) * mul;
	  if (MAX_UNT - sum >= add)
	    {
	      sum += add;
	      mul *= base;
	    }
	  else
	    {
	      errno = ERANGE;
	      return (0);
	    }
	}
      return (sum);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bits_to_int: return BITS value of BITS denoter.                               */

static unsigned
bits_to_int (NODE_T * p, char *a)
{
  int base = 0;
  unsigned bits = 0;
  char *radix = NULL, *end = NULL;
  errno = 0;
  base = a68g_strtoul (a, &radix, 10);
  if (radix != NULL && TO_UPPER (radix[0]) == RADIX_CHAR && errno == 0)
    {
      if (base < 2 || base > 16)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "radix D must be 2 upto 16", base);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      bits = a68g_strtoul (&(radix[1]), &end, base);
      if (end != NULL && end[0] == '\0' && errno == 0)
	{
	  return (bits);
	}
    }
  diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, MODE (BITS));
  exit_genie (p, A_RUNTIME_ERROR);
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_bits_to_long_int: return LONG BITS value of LONG BITS denoter.           */

static void
long_bits_to_long_int (NODE_T * p, mp_digit * z, char *a, MOID_T * m)
{
  int base = 0;
  char *radix = NULL;
  errno = 0;
  base = a68g_strtoul (a, &radix, 10);
  if (radix != NULL && TO_UPPER (radix[0]) == RADIX_CHAR && errno == 0)
    {
      int digits = get_mp_digits (m);
      ADDR_T save_sp = stack_pointer;
      mp_digit *v = stack_mp (p, digits);
      mp_digit *w = stack_mp (p, digits);
      char *q = radix;
      while (q[0] != '\0')
	{
	  q++;
	}
      SET_MP_ZERO (z, digits);
      set_mp_short (w, (mp_digit) 1, 0, digits);
      if (base < 2 || base > 16)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "radix D must be 2 upto 16", base);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      while ((--q) != radix)
	{
	  int digit = char_value (q[0]);
	  if (digit >= 0 && digit < base)
	    {
	      mul_mp_digit (p, v, w, (mp_digit) digit, digits);
	      add_mp (p, z, z, v, digits);
	    }
	  else
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "digit D is not in [0, D>", digit, base);
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  mul_mp_digit (p, w, w, (mp_digit) base, digits);
	}
      check_long_bits_value (p, z, m);
      stack_pointer = save_sp;
    }
  else
    {
      diagnostic (A_RUNTIME_ERROR, p, ERROR_IN_DENOTER, m);
      exit_genie (p, A_RUNTIME_ERROR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_string_to_value_internal: convert string to mode and store.             */

BOOL_T genie_string_to_value_internal (NODE_T * p, MOID_T * m, char *a, BYTE_T * item)
{
  errno = 0;
/* strto.. does not mind empty strings */
  if (strlen (a) == 0)
    {
      return (A_FALSE);
    }
  if (m == MODE (INT))
    {
      A68_INT *z = (A68_INT *) item;
      char *end;
      z->value = strtol (a, &end, 10);
      if (end[0] == '\0' && errno == 0)
	{
	  z->status = INITIALISED_MASK;
	  return (A_TRUE);
	}
      else
	{
	  return (A_FALSE);
	}
    }
  else if (m == MODE (REAL))
    {
      A68_REAL *z = (A68_REAL *) item;
      char *end;
      z->value = strtod (a, &end);
      if (end[0] == '\0' && errno == 0)
	{
	  z->status = INITIALISED_MASK;
	  return (A_TRUE);
	}
      else
	{
	  return (A_FALSE);
	}
    }
  else if (m == MODE (LONG_INT) || m == MODE (LONGLONG_INT))
    {
      int digits = get_mp_digits (m);
      mp_digit *z = (mp_digit *) item;
      if (string_to_mp (p, z, a, digits) == NULL)
	{
	  return (A_FALSE);
	}
      if (!check_mp_int (z, m))
	{
	  errno = ERANGE;
	  return (A_FALSE);
	}
      MP_STATUS (z) = INITIALISED_MASK;
      return (A_TRUE);
    }
  else if (m == MODE (LONG_REAL) || m == MODE (LONGLONG_REAL))
    {
      int digits = get_mp_digits (m);
      mp_digit *z = (mp_digit *) item;
      if (string_to_mp (p, z, a, digits) == NULL)
	{
	  return (A_FALSE);
	}
      MP_STATUS (z) = INITIALISED_MASK;
      return (A_TRUE);
    }
  else if (m == MODE (BOOL))
    {
      A68_BOOL *z = (A68_BOOL *) item;
      char q = a[0], flip = FLIP_CHAR, flop = FLOP_CHAR;
      if (q == flip || q == flop)
	{
	  z->value = (q == flip);
	  z->status = INITIALISED_MASK;
	  return (A_TRUE);
	}
      else
	{
	  return (A_FALSE);
	}
    }
  else if (m == MODE (BITS))
    {
      A68_BITS *z = (A68_BITS *) item;
      int status = A_TRUE;
      if (a[0] == FLIP_CHAR || a[0] == FLOP_CHAR)
	{
/* [] BOOL denotation is "TTFFFFTFT ..." */
	  if (strlen (a) > (size_t) BITS_WIDTH)
	    {
	      errno = ERANGE;
	      status = A_FALSE;
	    }
	  else
	    {
	      int j = strlen (a) - 1;
	      unsigned k = 0x1;
	      z->value = 0;
	      for (; j >= 0; j--)
		{
		  if (a[j] == FLIP_CHAR)
		    {
		      z->value += k;
		    }
		  else if (a[j] != FLOP_CHAR)
		    {
		      status = A_FALSE;
		    }
		  k <<= 1;
		}
	    }
	}
      else
	{
/* BITS denotation is also allowed */
	  z->value = bits_to_int (p, a);
	}
      if (errno != 0 || status == A_FALSE)
	{
	  return (A_FALSE);
	}
      z->status = INITIALISED_MASK;
      return (A_TRUE);
    }
  else if (m == MODE (LONG_BITS) || m == MODE (LONGLONG_BITS))
    {
      int digits = get_mp_digits (m);
      int status = A_TRUE;
      ADDR_T save_sp = stack_pointer;
      mp_digit *z = (mp_digit *) item;
      if (a[0] == FLIP_CHAR || a[0] == FLOP_CHAR)
	{
/* [] BOOL denotation is "TTFFFFTFT ..." */
	  if (strlen (a) > (size_t) BITS_WIDTH)
	    {
	      errno = ERANGE;
	      status = A_FALSE;
	    }
	  else
	    {
	      int j;
	      mp_digit *w = stack_mp (p, digits);
	      SET_MP_ZERO (z, digits);
	      set_mp_short (w, (mp_digit) 1, 0, digits);
	      for (j = strlen (a) - 1; j >= 0; j--)
		{
		  if (a[j] == FLIP_CHAR)
		    {
		      add_mp (p, z, z, w, digits);
		    }
		  else if (a[j] != FLOP_CHAR)
		    {
		      status = A_FALSE;
		    }
		  mul_mp_digit (p, w, w, (mp_digit) 2, digits);
		}
	    }
	}
      else
	{
/* BITS denotation is also allowed */
	  long_bits_to_long_int (p, z, a, m);
	}
      stack_pointer = save_sp;
      if (errno != 0 || status == A_FALSE)
	{
	  return (A_FALSE);
	}
      MP_STATUS (z) = INITIALISED_MASK;
      return (A_TRUE);
    }
  return (A_FALSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_string_to_value: convert string in input buffer.                        */

void
genie_string_to_value (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  char *str = get_transput_buffer (INPUT_BUFFER);
  errno = 0;
  add_char_transput_buffer (p, INPUT_BUFFER, '\0');	/* end string, just in case */
  if (mode == MODE (INT))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (REAL))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (BOOL))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (BITS))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      if (genie_string_to_value_internal (p, mode, str, item) == A_FALSE)
	{
	  value_error (p, mode, ref_file);
	}
    }
  else if (mode == MODE (CHAR))
    {
      A68_CHAR *z = (A68_CHAR *) item;
      if (str[0] == '\0')
	{
	  value_error (p, mode, ref_file);
	}
      else
	{
	  int len = strlen (str);
	  if (len == 0 || len > 1)
	    {
	      value_error (p, mode, ref_file);
	    }
	  z->value = str[0];
	  z->status = INITIALISED_MASK;
	}
    }
  else if (mode == MODE (BYTES))
    {
      A68_BYTES *z = (A68_BYTES *) item;
      if (strlen (str) > BYTES_WIDTH)
	{
	  value_error (p, mode, ref_file);
	}
      strcpy (z->value, str);
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (LONG_BYTES))
    {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) item;
      if (strlen (str) > LONG_BYTES_WIDTH)
	{
	  value_error (p, mode, ref_file);
	}
      strcpy (z->value, str);
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
      A68_REF z;
      z = c_to_a_string (p, str);
      if (mode == MODE (ROW_CHAR))
	{
	  genie_revise_lower_bound (p, *(A68_REF *) item, z);
	  genie_assign_stowed (z, (A68_REF *) item, p, MODE (ROW_CHAR));
	}
      else
	{
	  *(A68_REF *) item = z;
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_read_standard: read object from file.                                   */

void
genie_read_standard (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  errno = 0;
  if (mode == MODE (INT) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      scan_integer (p, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (mode == MODE (REAL) || mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      scan_real (p, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (mode == MODE (BOOL))
    {
      scan_char (p, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (mode == MODE (CHAR))
    {
      scan_char (p, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (mode == MODE (BITS) || mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      scan_bits (p, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (mode == MODE (BYTES) || mode == MODE (LONG_BYTES) || mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
      char *term = (char *) ADDRESS (&f->terminator);
      scan_string (p, term, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  genie_read_standard (p, MOID (q), &item[q->offset], ref_file);
	}
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      if (!(z->status | INITIALISED_MASK) || z->value == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, EMPTY_VALUE_ERROR, mode);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      genie_read_standard (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      genie_read_standard (p, SUB (deflexed), &base_addr[elem_addr], ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC ([] SIMPLIN) VOID read                                                   */

void
genie_read (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  genie_stand_in (p);
  PUSH_REF (p, row);
  genie_read_file (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, [] SIMPLIN) VOID get                                          */

void
genie_read_file (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLIN));
  TEST_NIL (p, row, MODE (ROW_SIMPLIN));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "write");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.get)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "getting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_READ_ACCESS, 0)) == -1)
	{
	  open_error (p, ref_file, "getting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_TRUE;
	  file->write_mood = A_FALSE;
	  file->char_mood = A_TRUE;
	}
    }
  if (!file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "binary");
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Read */
  base_address = ADDRESS (&arr->array);
  elem_index = 0;
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + SIZE_OF (A68_POINTER)];
      if (mode == MODE (PROC_REF_FILE_VOID))
	{
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
	}
      else if (mode == MODE (FORMAT))
	{
	  /* ignore */ ;
	}
      else
	{
	  if (file->eof)
	    {
	      end_of_file_error (p, ref_file);
	    }
	  TEST_NIL (p, *(A68_REF *) item, SUB (mode));
	  genie_read_standard (p, SUB (mode), ADDRESS ((A68_REF *) item), ref_file);
	}
      elem_index += MOID_SIZE (MODE (SIMPLIN));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_value_to_string                                                         */

void
genie_value_to_string (NODE_T * p, MOID_T * moid, BYTE_T * item)
{
  if (moid == MODE (INT))
    {
      A68_INT *z = (A68_INT *) item;
      PUSH_POINTER (p, MODE (INT));
      PUSH_INT (p, z->value);
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_INT)));
      PUSH_INT (p, INT_WIDTH + 1);
      genie_whole (p);
    }
  else if (moid == MODE (LONG_INT))
    {
      mp_digit *z = (mp_digit *) item;
      PUSH_POINTER (p, MODE (LONG_INT));
      PUSH (p, z, get_mp_size (MODE (LONG_INT)));
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + get_mp_size (MODE (LONG_INT))));
      PUSH_INT (p, LONG_WIDTH + 1);
      genie_whole (p);
    }
  else if (moid == MODE (LONGLONG_INT))
    {
      mp_digit *z = (mp_digit *) item;
      PUSH_POINTER (p, MODE (LONGLONG_INT));
      PUSH (p, z, get_mp_size (MODE (LONGLONG_INT)));
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + get_mp_size (MODE (LONGLONG_INT))));
      PUSH_INT (p, LONGLONG_WIDTH + 1);
      genie_whole (p);
    }
  else if (moid == MODE (REAL))
    {
      A68_REAL *z = (A68_REAL *) item;
      PUSH_POINTER (p, MODE (REAL));
      PUSH_REAL (p, z->value);
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_REAL)));
      PUSH_INT (p, REAL_WIDTH + EXP_WIDTH + 4);
      PUSH_INT (p, REAL_WIDTH - 1);
      PUSH_INT (p, EXP_WIDTH + 1);
      genie_float (p);
    }
  else if (moid == MODE (LONG_REAL))
    {
      mp_digit *z = (mp_digit *) item;
      PUSH_POINTER (p, MODE (LONG_REAL));
      PUSH (p, z, get_mp_size (MODE (LONG_REAL)));
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + get_mp_size (MODE (LONG_REAL))));
      PUSH_INT (p, LONG_REAL_WIDTH + LONG_EXP_WIDTH + 4);
      PUSH_INT (p, LONG_REAL_WIDTH - 1);
      PUSH_INT (p, LONG_EXP_WIDTH + 1);
      genie_float (p);
    }
  else if (moid == MODE (LONGLONG_REAL))
    {
      mp_digit *z = (mp_digit *) item;
      PUSH_POINTER (p, MODE (LONGLONG_REAL));
      PUSH (p, z, get_mp_size (MODE (LONGLONG_REAL)));
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + get_mp_size (MODE (LONGLONG_REAL))));
      PUSH_INT (p, LONGLONG_REAL_WIDTH + LONGLONG_EXP_WIDTH + 4);
      PUSH_INT (p, LONGLONG_REAL_WIDTH - 1);
      PUSH_INT (p, LONGLONG_EXP_WIDTH + 1);
      genie_float (p);
    }
  else if (moid == MODE (BITS))
    {
      A68_BITS *z = (A68_BITS *) item;
      char *str = stack_string (p, 8 + BITS_WIDTH);
      unsigned bit = 0x1;
      int j;
      for (j = 1; j < BITS_WIDTH; j++)
	{
	  bit <<= 1;
	}
      for (j = 0; j < BITS_WIDTH; j++)
	{
	  str[j] = (z->value & bit) ? FLIP_CHAR : FLOP_CHAR;
	  bit >>= 1;
	}
      str[j] = '\0';
    }
  else if (moid == MODE (LONG_BITS) || moid == MODE (LONGLONG_BITS))
    {
      int bits = get_mp_bits_width (moid), word = get_mp_bits_words (moid);
      int cher = bits;
      char *str = stack_string (p, 8 + bits);
      ADDR_T save_sp = stack_pointer;
      unsigned *row = stack_mp_bits (p, (mp_digit *) item, moid);
      str[cher--] = '\0';
      while (cher >= 0)
	{
	  unsigned bit = 0x1;
	  int j;
	  for (j = 0; j < MP_BITS_BITS && cher >= 0; j++)
	    {
	      str[cher--] = (row[word - 1] & bit) ? FLIP_CHAR : FLOP_CHAR;
	      bit <<= 1;
	    }
	  word--;
	}
      stack_pointer = save_sp;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_standard: print object to file.                                   */

void
genie_write_standard (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  errno = 0;
  if (mode == MODE (INT) || mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      genie_value_to_string (p, mode, item);
      add_string_from_stack_transput_buffer (p, UNFORMATTED_BUFFER);
    }
  else if (mode == MODE (REAL) || mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      genie_value_to_string (p, mode, item);
      add_string_from_stack_transput_buffer (p, UNFORMATTED_BUFFER);
    }
  else if (mode == MODE (BOOL))
    {
      A68_BOOL *z = (A68_BOOL *) item;
      char flipflop = z->value == A_TRUE ? FLIP_CHAR : FLOP_CHAR;
      add_char_transput_buffer (p, UNFORMATTED_BUFFER, flipflop);
    }
  else if (mode == MODE (CHAR))
    {
      A68_CHAR *ch = (A68_CHAR *) item;
      add_char_transput_buffer (p, UNFORMATTED_BUFFER, ch->value);
    }
  else if (mode == MODE (BITS) || mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      char *str = (char *) STACK_TOP;
      genie_value_to_string (p, mode, item);
      add_string_transput_buffer (p, UNFORMATTED_BUFFER, str);
    }
  else if (mode == MODE (BYTES))
    {
      A68_BYTES *z = (A68_BYTES *) item;
      add_string_transput_buffer (p, UNFORMATTED_BUFFER, z->value);
    }
  else if (mode == MODE (LONG_BYTES))
    {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) item;
      add_string_transput_buffer (p, UNFORMATTED_BUFFER, z->value);
    }
  else if (mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
/* Handle these separately since this is faster than straightening */
      add_a_string_transput_buffer (p, UNFORMATTED_BUFFER, item);
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      genie_write_standard (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  BYTE_T *elem = &item[q->offset];
	  genie_check_initialisation (p, elem, MOID (q), NULL);
	  genie_write_standard (p, MOID (q), elem, ref_file);
	}
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      BYTE_T *elem = &base_addr[elem_addr];
	      genie_check_initialisation (p, elem, SUB (deflexed), NULL);
	      genie_write_standard (p, SUB (deflexed), elem, ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      abend (IS_NIL (ref_file), "conversion error: ", strerror (errno));
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
write_purge_buffer: purge buffer.                                             */

static void
write_purge_buffer (A68_REF ref_file, int b)
{
  A68_FILE *file = FILE_DEREF (&ref_file);
  if (!(file->fd == STDOUT_FILENO && halt_typing))
    {
      io_write_string (file->fd, get_transput_buffer (b));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC ([] SIMPLOUT) VOID print, write                                          */

void
genie_write (NODE_T * p)
{
  A68_REF row;
  POP_REF (p, &row);
  genie_stand_out (p);
  PUSH_REF (p, row);
  genie_write_file (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, [] SIMPLOUT) VOID put                                         */

void
genie_write_file (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLOUT));
  TEST_NIL (p, row, MODE (ROW_SIMPLOUT));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->read_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "read");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.put)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "putting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_WRITE_ACCESS, A68_PROTECTION)) == -1)
	{
	  open_error (p, ref_file, "putting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_FALSE;
	  file->write_mood = A_TRUE;
	  file->char_mood = A_TRUE;
	}
    }
  if (!file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "binary");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  base_address = ADDRESS (&(arr->array));
  elem_index = 0;
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & (base_address[elem_index]);
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + SIZE_OF (A68_POINTER)];
      if (mode == MODE (PROC_REF_FILE_VOID))
	{
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
	}
      else if (mode == MODE (FORMAT))
	{
	  /* ignore */ ;
	}
      else
	{
	  reset_transput_buffer (UNFORMATTED_BUFFER);
	  genie_write_standard (p, mode, item, ref_file);
	  write_purge_buffer (ref_file, UNFORMATTED_BUFFER);
	}
      elem_index += MOID_SIZE (MODE (SIMPLOUT));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_read_bin_standard: read object binary from file.                        */

static void
genie_read_bin_standard (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  errno = 0;
  if (mode == MODE (INT))
    {
      A68_INT *z = (A68_INT *) item;
      io_read (f->fd, &(z->value), SIZE_OF (z->value));
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      mp_digit *z = (mp_digit *) item;
      io_read (f->fd, z, get_mp_size (mode));
      MP_STATUS (z) = INITIALISED_MASK;
    }
  else if (mode == MODE (REAL))
    {
      A68_REAL *z = (A68_REAL *) item;
      io_read (f->fd, &(z->value), SIZE_OF (z->value));
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      mp_digit *z = (mp_digit *) item;
      io_read (f->fd, z, get_mp_size (mode));
      MP_STATUS (z) = INITIALISED_MASK;
    }
  else if (mode == MODE (BOOL))
    {
      A68_BOOL *z = (A68_BOOL *) item;
      io_read (f->fd, &(z->value), SIZE_OF (z->value));
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (CHAR))
    {
      A68_CHAR *z = (A68_CHAR *) item;
      io_read (f->fd, &(z->value), SIZE_OF (z->value));
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (BITS))
    {
      A68_BITS *z = (A68_BITS *) item;
      io_read (f->fd, &(z->value), SIZE_OF (z->value));
      z->status = INITIALISED_MASK;
    }
  else if (mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      mp_digit *z = (mp_digit *) item;
      io_read (f->fd, z, get_mp_size (mode));
      MP_STATUS (z) = INITIALISED_MASK;
    }
  else if (mode == MODE (BYTES) || mode == MODE (LONG_BYTES) || mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
      char *term = (char *) ADDRESS (&f->terminator);
      scan_string (p, term, ref_file);
      genie_string_to_value (p, mode, item, ref_file);
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      if (!(z->status | INITIALISED_MASK) || z->value == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, EMPTY_VALUE_ERROR, mode);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      genie_read_bin_standard (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  genie_read_bin_standard (p, MOID (q), &item[q->offset], ref_file);
	}
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      genie_read_bin_standard (p, SUB (deflexed), &base_addr[elem_addr], ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, [] SIMPLIN) VOID get bin                                      */

void
genie_read_bin_file (NODE_T * p)
{
  A68_REF ref_file;
  A68_FILE *file;
  A68_REF row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLIN));
  TEST_NIL (p, row, MODE (ROW_SIMPLIN));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  ref_file = *(A68_REF *) STACK_TOP;
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->write_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "write");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.get)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "getting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.bin)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "binary getting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_READ_ACCESS | O_BINARY, 0)) == -1)
	{
	  open_error (p, ref_file, "binary getting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_TRUE;
	  file->write_mood = A_FALSE;
	  file->char_mood = A_FALSE;
	}
    }
  if (file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "text");
      exit_genie (p, A_RUNTIME_ERROR);
    }
/* Read */
  elem_index = 0;
  base_address = ADDRESS (&(arr->array));
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + SIZE_OF (A68_POINTER)];
      if (mode == MODE (PROC_REF_FILE_VOID))
	{
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
	}
      else if (mode == MODE (FORMAT))
	{
	  /* ignore */ ;
	}
      else
	{
	  if (file->eof)
	    {
	      end_of_file_error (p, ref_file);
	    }
	  TEST_NIL (p, *(A68_REF *) item, SUB (mode));
	  genie_read_bin_standard (p, SUB (mode), ADDRESS ((A68_REF *) item), ref_file);
	}
      elem_index += MOID_SIZE (MODE (SIMPLIN));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_write_bin_standard: write object binary to file                         */

static void
genie_write_bin_standard (NODE_T * p, MOID_T * mode, BYTE_T * item, A68_REF ref_file)
{
  A68_FILE *f = FILE_DEREF (&ref_file);
  errno = 0;
  if (mode == MODE (INT))
    {
      io_write (f->fd, &(((A68_INT *) item)->value), SIZE_OF (((A68_INT *) item)->value));
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      io_write (f->fd, (mp_digit *) item, get_mp_size (mode));
    }
  else if (mode == MODE (REAL))
    {
      io_write (f->fd, &(((A68_REAL *) item)->value), SIZE_OF (((A68_REAL *) item)->value));
    }
  else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      io_write (f->fd, (mp_digit *) item, get_mp_size (mode));
    }
  else if (mode == MODE (BOOL))
    {
      io_write (f->fd, &(((A68_BOOL *) item)->value), SIZE_OF (((A68_BOOL *) item)->value));
    }
  else if (mode == MODE (CHAR))
    {
      io_write (f->fd, &(((A68_CHAR *) item)->value), SIZE_OF (((A68_CHAR *) item)->value));
    }
  else if (mode == MODE (BITS))
    {
      io_write (f->fd, &(((A68_BITS *) item)->value), SIZE_OF (((A68_BITS *) item)->value));
    }
  else if (mode == MODE (LONG_BITS) || mode == MODE (LONGLONG_BITS))
    {
      io_write (f->fd, (mp_digit *) item, get_mp_size (mode));
    }
  else if (mode == MODE (BYTES))
    {
      io_write (f->fd, &(((A68_BYTES *) item)->value), SIZE_OF (((A68_BYTES *) item)->value));
    }
  else if (mode == MODE (LONG_BYTES))
    {
      io_write (f->fd, &(((A68_LONG_BYTES *) item)->value), SIZE_OF (((A68_LONG_BYTES *) item)->value));
    }
  else if (mode == MODE (ROW_CHAR) || mode == MODE (STRING))
    {
      reset_transput_buffer (UNFORMATTED_BUFFER);
      add_a_string_transput_buffer (p, UNFORMATTED_BUFFER, item);
      io_write_string (f->fd, get_transput_buffer (UNFORMATTED_BUFFER));
    }
  else if (WHETHER (mode, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      genie_write_bin_standard (p, (MOID_T *) (z->value), &item[SIZE_OF (A68_POINTER)], ref_file);
    }
  else if (WHETHER (mode, STRUCT_SYMBOL))
    {
      PACK_T *q = PACK (mode);
      for (; q != NULL; q = NEXT (q))
	{
	  BYTE_T *elem = &item[q->offset];
	  genie_check_initialisation (p, elem, MOID (q), NULL);
	  genie_write_bin_standard (p, MOID (q), elem, ref_file);
	}
    }
  else if (WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL))
    {
      MOID_T *deflexed = DEFLEX (mode);
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      TEST_INIT (p, *(A68_REF *) item, MODE (ROWS));
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      if (get_row_size (tup, arr->dimensions) != 0)
	{
	  BYTE_T *base_addr = ADDRESS (&arr->array);
	  BOOL_T done = A_FALSE;
	  initialise_internal_index (tup, arr->dimensions);
	  while (!done)
	    {
	      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	      ADDR_T elem_addr = ROW_ELEMENT (arr, index);
	      BYTE_T *elem = &base_addr[elem_addr];
	      genie_check_initialisation (p, elem, SUB (deflexed), NULL);
	      genie_write_bin_standard (p, SUB (deflexed), elem, ref_file);
	      done = increment_internal_index (tup, arr->dimensions);
	    }
	}
    }
  if (errno != 0)
    {
      transput_error (p, ref_file, mode);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REF FILE, [] SIMPLOUT) VOID put bin                                     */

void
genie_write_bin_file (NODE_T * p)
{
  A68_REF ref_file, row;
  A68_FILE *file;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  BYTE_T *base_address;
  int elems, k, elem_index;
  POP_REF (p, &row);
  TEST_INIT (p, row, MODE (ROW_SIMPLOUT));
  TEST_NIL (p, row, MODE (ROW_SIMPLOUT));
  GET_DESCRIPTOR (arr, tup, &row);
  elems = ROW_SIZE (tup);
  POP_REF (p, &ref_file);
  ref_file = *(A68_REF *) STACK_TOP;
  TEST_NIL (p, ref_file, MODE (REF_FILE));
  file = FILE_DEREF (&ref_file);
  TEST_INIT (p, *file, MODE (FILE));
  if (!file->opened)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_NOT_OPEN);
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->draw_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "draw");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (file->read_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "read");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.put)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "putting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->channel.bin)
    {
      diagnostic (A_RUNTIME_ERROR, p, CHANNEL_DOES_NOT, "binary putting");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  if (!file->read_mood && !file->write_mood)
    {
      if ((file->fd = open_physical_file (p, ref_file, A_WRITE_ACCESS | O_BINARY, A68_PROTECTION)) == -1)
	{
	  open_error (p, ref_file, "binary putting");
	}
      else
	{
	  file->draw_mood = A_FALSE;
	  file->read_mood = A_FALSE;
	  file->write_mood = A_TRUE;
	  file->char_mood = A_FALSE;
	}
    }
  if (file->char_mood)
    {
      diagnostic (A_RUNTIME_ERROR, p, FILE_HAS_MOOD, "text");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  base_address = ADDRESS (&arr->array);
  elem_index = 0;
  for (k = 0; k < elems; k++)
    {
      A68_POINTER *z = (A68_POINTER *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (z->value);
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + SIZE_OF (A68_POINTER)];
      if (mode == MODE (PROC_REF_FILE_VOID))
	{
	  genie_call_proc_ref_file_void (p, ref_file, *(A68_PROCEDURE *) item);
	}
      else if (mode == MODE (FORMAT))
	{
	  /* ignore */ ;
	}
      else
	{
	  genie_write_bin_standard (p, mode, item, ref_file);
	}
      elem_index += MOID_SIZE (MODE (SIMPLOUT));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Next are formatting routines "whole", "fixed" and "float" for mode
INT, LONG INT and LONG LONG INT, and REAL, LONG REAL and LONG LONG REAL.
They are direct implementations of the routines described in the
Revised Report, although those were only meant as a specification.

The rest of Algol68G should only reference "genie_whole", "genie_fixed"
or "genie_float" since internal routines like "sub_fixed" may leav the
stack corrupted when called directly.                                         */

/*-------1---------2---------3---------4---------5---------6---------7---------+
error_chars: generate a string of error chars.                                */

char *
error_chars (char *s, int n)
{
  int k = (n != 0 ? ABS (n) : 1);
  s[k] = '\0';
  while (--k >= 0)
    {
      s[k] = ERROR_CHAR;
    }
  return (s);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tmp_to_a68_string: convert C string to A68 string.                            */

A68_REF tmp_to_a68_string (NODE_T * p, char *temp_string)
{
  A68_REF z;
/* no compaction allowed since temp_string might be up for sweeping ... */
  up_garbage_sema ();
  z = c_to_a_string (p, temp_string);
  down_garbage_sema ();
  return (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
plusto: add c to str, assuming that "str" is large enough.                    */

static char *
plusto (char c, char *str)
{
  MOVE (&str[1], &str[0], (unsigned) (strlen (str) + 1));
  str[0] = c;
  return (str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
string_plusab_char: add c to str, assuming that "str" is large enough.        */

char *
string_plusab_char (char *str, char c)
{
  char z[2];
  z[0] = c;
  z[1] = '\0';
  return (strcat (str, z));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
leading_spaces: add leading spaces to str until length is width .             */

static char *
leading_spaces (char *str, int width)
{
  int j = width - strlen (str);
  while (--j >= 0)
    {
      plusto (' ', str);
    }
  return (str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
digchar: convert int to char using a table.                                   */

static char
digchar (int k)
{
  char *s = "0123456789abcdef";
  if (k >= 0 && k < (int) strlen (s))
    {
      return (s[k]);
    }
  else
    {
      return (ERROR_CHAR);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_sub_whole: standard string for LONG INT.                                 */

char *
long_sub_whole (NODE_T * p, mp_digit * n, int digits, int width)
{
  char *s = stack_string (p, 8 + width);
  int len = 0;
  s[0] = '\0';
  do
    {
      if (len < width)
	{
/* Sic transit gloria mundi */
	  int n_mod_10 = (int) MP_DIGIT (n, (int) (1 + MP_EXPONENT (n))) % 10;
	  plusto (digchar (n_mod_10), s);
	}
      len++;
      over_mp_digit (p, n, n, (mp_digit) 10, digits);
    }
  while (MP_DIGIT (n, 1) > 0);
  if (len > width)
    {
      error_chars (s, width);
    }
  return (s);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sub_whole: standard string for INT.                                           */

char *
sub_whole (NODE_T * p, int n, int width)
{
  char *s = stack_string (p, 8 + width);
  int len = 0;
  s[0] = '\0';
  do
    {
      if (len < width)
	{
	  plusto (digchar (n % 10), s);
	}
      len++;
      n /= 10;
    }
  while (n != 0);
  if (len > width)
    {
      error_chars (s, width);
    }
  return (s);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whole: formatted string for NUMBER.                                           */

char *
whole (NODE_T * p)
{
  int old_sp, arg_sp;
  A68_INT width;
  MOID_T *mode;
  POP_INT (p, &width);
  arg_sp = stack_pointer;
  DECREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
  old_sp = stack_pointer;
  mode = (MOID_T *) (((A68_POINTER *) STACK_TOP)->value);
  if (mode == MODE (INT))
    {
      int x = ((A68_INT *) (STACK_OFFSET (SIZE_OF (A68_POINTER))))->value;
      int length = ABS (width.value) - (x < 0 || width.value > 0 ? 1 : 0);
      int n = ABS (x);
      int size = (x < 0 ? 1 : (width.value > 0 ? 1 : 0));
      char *s;
      if (width.value == 0)
	{
	  int m = n;
	  length = 0;
	  while ((m /= 10, length++, m != 0))
	    {
	      ;
	    }
	}
      size += length;
      size = 8 + (size > width.value ? size : width.value);
      s = stack_string (p, size);
      strcpy (s, sub_whole (p, n, length));
      if (length == 0 || strchr (s, ERROR_CHAR) != NULL)
	{
	  error_chars (s, width.value);
	}
      else
	{
	  if (x < 0)
	    {
	      plusto ('-', s);
	    }
	  else if (width.value > 0)
	    {
	      plusto ('+', s);
	    }
	  if (width.value != 0)
	    {
	      leading_spaces (s, ABS (width.value));
	    }
	}
      return (s);
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      int digits = get_mp_digits (mode);
      int length, size;
      char *s;
      mp_digit *n = (mp_digit *) (STACK_OFFSET (SIZE_OF (A68_POINTER)));
      BOOL_T ltz;
      stack_pointer = arg_sp;	/* We keep the mp where it's at */
      if (MP_EXPONENT (n) >= digits)
	{
	  int max_length = (mode == MODE (LONG_INT) ? LONG_INT_WIDTH : LONGLONG_INT_WIDTH);
	  int length = (width.value == 0 ? max_length : width.value);
	  s = stack_string (p, 1 + length);
	  error_chars (s, length);
	  return (s);
	}
      ltz = MP_DIGIT (n, 1) < 0;
      length = ABS (width.value) - (ltz || width.value > 0 ? 1 : 0);
      size = (ltz ? 1 : (width.value > 0 ? 1 : 0));
      MP_DIGIT (n, 1) = fabs (MP_DIGIT (n, 1));
      if (width.value == 0)
	{
	  mp_digit *m = stack_mp (p, digits);
	  MOVE_MP (m, n, digits);
	  length = 0;
	  while ((over_mp_digit (p, m, m, (mp_digit) 10, digits), length++, MP_DIGIT (m, 1) != 0))
	    {
	      ;
	    }
	}
      size += length;
      size = 8 + (size > width.value ? size : width.value);
      s = stack_string (p, size);
      strcpy (s, long_sub_whole (p, n, digits, length));
      if (length == 0 || strchr (s, ERROR_CHAR) != NULL)
	{
	  error_chars (s, width.value);
	}
      else
	{
	  if (ltz)
	    {
	      plusto ('-', s);
	    }
	  else if (width.value > 0)
	    {
	      plusto ('+', s);
	    }
	  if (width.value != 0)
	    {
	      leading_spaces (s, ABS (width.value));
	    }
	}
      return (s);
    }
  else if (mode == MODE (REAL) || mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
      PUSH_INT (p, width.value);
      PUSH_INT (p, 0);
      return (fixed (p));
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_choose_dig: next digit from LONG.                                        */

static char
long_choose_dig (NODE_T * p, mp_digit * y, int digits)
{
/* Assuming positive "y" */
  int old_sp = stack_pointer, c;
  mp_digit *t = stack_mp (p, digits);
  mul_mp_digit (p, y, y, (mp_digit) 10, digits);
  c = MP_EXPONENT (y) == 0 ? (int) MP_DIGIT (y, 1) : 0;
  if (c > 9)
    {
      c = 9;
    }
  set_mp_short (t, (mp_digit) c, 0, digits);
  sub_mp (p, y, y, t, digits);
/* Reset the stack to prevent overflow, there may be many digits */
  stack_pointer = old_sp;
  return (digchar (c));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_sub_fixed: standard string for LONG.                                     */

char *
long_sub_fixed (NODE_T * p, mp_digit * x, int digits, int width, int after)
{
  char *str = stack_string (p, 8 + width);
  int before = 0, j, len, old_sp = stack_pointer;
  BOOL_T overflow;
  mp_digit *y = stack_mp (p, digits);
  mp_digit *s = stack_mp (p, digits);
  mp_digit *t = stack_mp (p, digits);
  set_mp_short (t, (mp_digit) (MP_RADIX / 10), -1, digits);
  pow_mp_int (p, t, t, after, digits);
  div_mp_digit (p, t, t, (mp_digit) 2, digits);
  add_mp (p, y, x, t, digits);
  set_mp_short (s, (mp_digit) 1, 0, digits);
  while ((sub_mp (p, t, y, s, digits), MP_DIGIT (t, 1) >= 0))
    {
      before++;
      mul_mp_digit (p, s, s, (mp_digit) 10, digits);
    }
  div_mp (p, y, y, s, digits);
  str[0] = '\0';
  len = 0;
  overflow = A_FALSE;
  for (j = 0; j < before && !overflow; j++)
    {
      if (!(overflow = len >= width))
	{
	  string_plusab_char (str, long_choose_dig (p, y, digits));
	  len++;
	}
    }
  if (after > 0 && !(overflow = len >= width))
    {
      string_plusab_char (str, '.');
    }
  for (j = 0; j < after && !overflow; j++)
    {
      if (!(overflow = len >= width))
	{
	  string_plusab_char (str, long_choose_dig (p, y, digits));
	  len++;
	}
    }
  if (overflow || (int) strlen (str) > width)
    {
      error_chars (str, width);
    }
  stack_pointer = old_sp;
  return (str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
choose_dig: next digit for REAL.                                              */

static char
choose_dig (double *y)
{
/* Assuming positive "y" */
  int c = (int) (*y *= 10);
  if (c > 9)
    {
      c = 9;
    }
  *y -= c;
  return (digchar (c));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sub_fixed: standard string for REAL.                                          */

char *
sub_fixed (NODE_T * p, double x, int width, int after)
{
  char *str = stack_string (p, 8 + width);
  int before = 0, j, len, expo;
  BOOL_T overflow;
  double y, z;
/* Round and scale. */
  z = y = x + 0.5 * ten_to_the_power (-after);
  expo = 0;
  while (z >= 1)
    {
      expo++;
      z /= 10;
    }
  before += expo;
/* Trick to avoid overflow. */
  if (expo > 30)
    {
      expo -= 30;
      y /= ten_to_the_power (30);
    }
/* Scale number. */
  y /= ten_to_the_power (expo);
  len = 0;
/* Put digits, prevent garbage from overstretching precision. */
  overflow = A_FALSE;
  for (j = 0; j < before && !overflow; j++)
    {
      if (!(overflow = len >= width))
	{
	  char ch = (len < REAL_WIDTH ? choose_dig (&y) : '0');
	  string_plusab_char (str, ch);
	  len++;
	}
    }
  if (after > 0 && !(overflow = len >= width))
    {
      string_plusab_char (str, '.');
    }
  for (j = 0; j < after && !overflow; j++)
    {
      if (!(overflow = len >= width))
	{
	  char ch = (len < REAL_WIDTH ? choose_dig (&y) : '0');
	  string_plusab_char (str, ch);
	  len++;
	}
    }
  if (overflow || (int) strlen (str) > width)
    {
      error_chars (str, width);
    }
  return (str);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
fixed: formatted string for NUMBER.                                           */

char *
fixed (NODE_T * p)
{
  A68_INT width, after;
  MOID_T *mode;
  int old_sp, arg_sp;
  POP_INT (p, &after);
  POP_INT (p, &width);
  arg_sp = stack_pointer;
  DECREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
  mode = (MOID_T *) (((A68_POINTER *) STACK_TOP)->value);
  old_sp = stack_pointer;
  if (mode == MODE (REAL))
    {
      double x = ((A68_REAL *) (STACK_OFFSET (SIZE_OF (A68_POINTER))))->value;
      int length = ABS (width.value) - (x < 0 || width.value > 0 ? 1 : 0);
      char *s;
      stack_pointer = arg_sp;
      if (after.value >= 0 && (length > after.value || width.value == 0))
	{
	  double y = fabs (x), z0, z1;
	  if (width.value == 0)
	    {
	      length = (after.value == 0 ? 1 : 0);
	      z0 = ten_to_the_power (-after.value);
	      z1 = ten_to_the_power (length);
/*
	pow_real_int (p, &z0, 0.1, after.value);
	pow_real_int (p, &z1, 10.0, length);
*/
	      while (y + 0.5 * z0 > z1)
		{
		  length++;
		  z1 *= 10.0;
		}
	      length += (after.value == 0 ? 0 : after.value + 1);
	    }
	  s = stack_string (p, 8 + length);
	  s = sub_fixed (p, y, length, after.value);
	  if (strchr (s, ERROR_CHAR) == NULL)
	    {
	      if (length > (int) strlen (s) && (s[0] != '\0' ? s[0] == '.' : A_TRUE) && y < 1.0)
		{
		  plusto ('0', s);
		}
	      if (x < 0)
		{
		  plusto ('-', s);
		}
	      else if (width.value > 0)
		{
		  plusto ('+', s);
		}
	      if (width.value != 0)
		{
		  leading_spaces (s, ABS (width.value));
		}
	      return (s);
	    }
	  else if (after.value > 0)
	    {
	      stack_pointer = arg_sp;
	      PUSH_INT (p, width.value);
	      PUSH_INT (p, after.value - 1);
	      return (fixed (p));
	    }
	  else
	    {
	      return (error_chars (s, width.value));
	    }
	}
      else
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  return (error_chars (s, width.value));
	}
    }
  else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      int digits = get_mp_digits (mode);
      int length;
      BOOL_T ltz;
      char *s;
      mp_digit *x = (mp_digit *) (STACK_OFFSET (SIZE_OF (A68_POINTER)));
      stack_pointer = arg_sp;
      ltz = MP_DIGIT (x, 1) < 0;
      MP_DIGIT (x, 1) = fabs (MP_DIGIT (x, 1));
      length = ABS (width.value) - (ltz || width.value > 0 ? 1 : 0);
      if (after.value >= 0 && (length > after.value || width.value == 0))
	{
	  mp_digit *z0 = stack_mp (p, digits);
	  mp_digit *z1 = stack_mp (p, digits);
	  mp_digit *t = stack_mp (p, digits);
	  if (width.value == 0)
	    {
	      length = (after.value == 0 ? 1 : 0);
	      set_mp_short (z0, (mp_digit) (MP_RADIX / 10), -1, digits);
	      set_mp_short (z1, (mp_digit) 10, 0, digits);
	      pow_mp_int (p, z0, z0, after.value, digits);
	      pow_mp_int (p, z1, z1, length, digits);
	      while ((div_mp_digit (p, t, z0, (mp_digit) 2, digits), add_mp (p, t, x, t, digits), sub_mp (p, t, t, z1, digits), MP_DIGIT (t, 1) > 0))
		{
		  length++;
		  mul_mp_digit (p, z1, z1, (mp_digit) 10, digits);
		}
	      length += (after.value == 0 ? 0 : after.value + 1);
	    }
	  s = stack_string (p, 8 + length);
	  s = long_sub_fixed (p, x, digits, length, after.value);
	  if (strchr (s, ERROR_CHAR) == NULL)
	    {
	      if (length > (int) strlen (s) && (s[0] != '\0' ? s[0] == '.' : A_TRUE) && (MP_EXPONENT (x) < 0 || MP_DIGIT (x, 1) == 0))
		{
		  plusto ('0', s);
		}
	      if (ltz)
		{
		  plusto ('-', s);
		}
	      else if (width.value > 0)
		{
		  plusto ('+', s);
		}
	      if (width.value != 0)
		{
		  leading_spaces (s, ABS (width.value));
		}
	      return (s);
	    }
	  else if (after.value > 0)
	    {
	      stack_pointer = arg_sp;
	      MP_DIGIT (x, 1) = ltz ? -fabs (MP_DIGIT (x, 1)) : fabs (MP_DIGIT (x, 1));
	      PUSH_INT (p, width.value);
	      PUSH_INT (p, after.value - 1);
	      return (fixed (p));
	    }
	  else
	    {
	      return (error_chars (s, width.value));
	    }
	}
      else
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  return (error_chars (s, width.value));
	}
    }
  else if (mode == MODE (INT))
    {
      int x = ((A68_INT *) (STACK_OFFSET (SIZE_OF (A68_POINTER))))->value;
      PUSH_POINTER (p, MODE (REAL));
      PUSH_REAL (p, (double) x);
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_REAL)));
      PUSH_INT (p, width.value);
      PUSH_INT (p, after.value);
      return (fixed (p));
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      stack_pointer = old_sp;
      if (mode == MODE (LONG_INT))
	{
	  ((A68_POINTER *) STACK_TOP)->value = (void *) MODE (LONG_REAL);
	}
      else
	{
	  ((A68_POINTER *) STACK_TOP)->value = (void *) MODE (LONGLONG_REAL);
	}
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
      PUSH_INT (p, width.value);
      PUSH_INT (p, after.value);
      return (fixed (p));
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
long_standardise: scale LONG for formatting.                                  */

void
long_standardise (NODE_T * p, mp_digit * y, int digits, int before, int after, int *q)
{
  int j, old_sp = stack_pointer;
  mp_digit *f = stack_mp (p, digits);
  mp_digit *g = stack_mp (p, digits);
  mp_digit *h = stack_mp (p, digits);
  mp_digit *t = stack_mp (p, digits);
  set_mp_short (g, (mp_digit) 1, 0, digits);
  for (j = 0; j < before; j++)
    {
      mul_mp_digit (p, g, g, (mp_digit) 10, digits);
    }
  div_mp_digit (p, h, g, (mp_digit) 10, digits);
/* Speed huge exponents */
  if ((MP_EXPONENT (y) - MP_EXPONENT (g)) > 1)
    {
      (*q) += LOG_MP_BASE * ((int) MP_EXPONENT (y) - (int) MP_EXPONENT (g) - 1);
      MP_EXPONENT (y) = MP_EXPONENT (g) + 1;
    }
  while ((sub_mp (p, t, y, g, digits), MP_DIGIT (t, 1) >= 0))
    {
      div_mp_digit (p, y, y, (mp_digit) 10, digits);
      (*q)++;
    }
  if (MP_DIGIT (y, 1) != 0)
    {
/* Speed huge exponents */
      if ((MP_EXPONENT (y) - MP_EXPONENT (h)) < -1)
	{
	  (*q) -= LOG_MP_BASE * ((int) MP_EXPONENT (h) - (int) MP_EXPONENT (y) - 1);
	  MP_EXPONENT (y) = MP_EXPONENT (h) - 1;
	}
      while ((sub_mp (p, t, y, h, digits), MP_DIGIT (t, 1) < 0))
	{
	  mul_mp_digit (p, y, y, (mp_digit) 10, digits);
	  (*q)--;
	}
    }
  set_mp_short (f, (mp_digit) 1, 0, digits);
  for (j = 0; j < after; j++)
    {
      div_mp_digit (p, f, f, (mp_digit) 10, digits);
    }
  div_mp_digit (p, t, f, (mp_digit) 2, digits);
  add_mp (p, t, y, t, digits);
  sub_mp (p, t, t, g, digits);
  if (MP_DIGIT (t, 1) >= 0)
    {
      MOVE_MP (y, h, digits);
      (*q)++;
    }
  stack_pointer = old_sp;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
standardise: scale REAL for formatting.                                       */

void
standardise (double *y, int before, int after, int *p)
{
  int j;
  double f, g = 1.0, h;
  for (j = 0; j < before; j++)
    {
      g *= 10.0;
    }
  h = g / 10.0;
  while (*y >= g)
    {
      *y *= 0.1;
      (*p)++;
    }
  if (*y != 0.0)
    {
      while (*y < h)
	{
	  *y *= 10.0;
	  (*p)--;
	}
    }
  f = 1.0;
  for (j = 0; j < after; j++)
    {
      f *= 0.1;
    }
  if (*y + 0.5 * f >= g)
    {
      *y = h;
      (*p)++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
fleet: formatted string for NUMBER.                                           */

char *
fleet (NODE_T * p)
{
  int old_sp, arg_sp;
  A68_INT width, after, expo;
  MOID_T *mode;
/* POP arguments */
  POP_INT (p, &expo);
  POP_INT (p, &after);
  POP_INT (p, &width);
  arg_sp = stack_pointer;
  DECREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
  mode = (MOID_T *) (((A68_POINTER *) STACK_TOP)->value);
  old_sp = stack_pointer;
  if (mode == MODE (REAL))
    {
      double x = ((A68_REAL *) (STACK_OFFSET (SIZE_OF (A68_POINTER))))->value;
      int before = ABS (width.value) - ABS (expo.value) - (after.value != 0 ? after.value + 1 : 0) - 2;
      stack_pointer = arg_sp;
#ifdef HAVE_IEEE_754
      if (isnan (x))
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  if (width.value >= (int) strlen (NAN_STRING))
	    {
	      memset (s, ' ', width.value);
	      strncpy (s, NAN_STRING, strlen (NAN_STRING));
	      return (s);
	    }
	  else
	    {
	      return (error_chars (s, width.value));
	    }
	}
      else if (isinf (x))
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  if (width.value >= (int) strlen (INF_STRING))
	    {
	      memset (s, ' ', width.value);
	      strncpy (s, INF_STRING, strlen (INF_STRING));
	      return (s);
	    }
	  else
	    {
	      return (error_chars (s, width.value));
	    }
	}
#endif
      if (SIGN (before) + SIGN (after.value) > 0)
	{
	  char *s, *t1, *t2;
	  double y = fabs (x);
	  int q = 0;
	  standardise (&y, before, after.value, &q);
	  PUSH_POINTER (p, MODE (REAL));
	  PUSH_REAL (p, SIGN (x) * y);
	  INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_REAL)));
	  PUSH_INT (p, SIGN (width.value) * (ABS (width.value) - ABS (expo.value) - 1));
	  PUSH_INT (p, after.value);
	  t1 = fixed (p);
	  PUSH_POINTER (p, MODE (INT));
	  PUSH_INT (p, q);
	  INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_INT)));
	  PUSH_INT (p, expo.value);
	  t2 = whole (p);
	  s = stack_string (p, 8 + strlen (t1) + 1 + strlen (t2));
	  strcpy (s, t1);
	  string_plusab_char (s, EXPONENT_CHAR);
	  strcat (s, t2);
	  if (expo.value == 0 || strchr (s, ERROR_CHAR) != NULL)
	    {
	      stack_pointer = arg_sp;
	      PUSH_INT (p, width.value);
	      PUSH_INT (p, after.value != 0 ? after.value - 1 : 0);
	      PUSH_INT (p, expo.value > 0 ? expo.value + 1 : expo.value - 1);
	      return (fleet (p));
	    }
	  else
	    {
	      return (s);
	    }
	}
      else
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  return (error_chars (s, width.value));
	}
    }
  else if (mode == MODE (LONG_REAL) || mode == MODE (LONGLONG_REAL))
    {
      int digits = get_mp_digits (mode);
      int before;
      BOOL_T ltz;
      mp_digit *x = (mp_digit *) (STACK_OFFSET (SIZE_OF (A68_POINTER)));
      stack_pointer = arg_sp;
      ltz = MP_DIGIT (x, 1) < 0;
      MP_DIGIT (x, 1) = fabs (MP_DIGIT (x, 1));
      before = ABS (width.value) - ABS (expo.value) - (after.value != 0 ? after.value + 1 : 0) - 2;
      if (SIGN (before) + SIGN (after.value) > 0)
	{
	  char *s, *t1, *t2;
	  mp_digit *z = stack_mp (p, digits);
	  int q = 0;
	  MOVE_MP (z, x, digits);
	  long_standardise (p, z, digits, before, after.value, &q);
	  PUSH_POINTER (p, mode);
	  MP_DIGIT (z, 1) = ltz ? -MP_DIGIT (z, 1) : MP_DIGIT (z, 1);
	  PUSH (p, z, SIZE_MP (digits));
	  INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_MP (digits)));
	  PUSH_INT (p, SIGN (width.value) * (ABS (width.value) - ABS (expo.value) - 1));
	  PUSH_INT (p, after.value);
	  t1 = fixed (p);
	  PUSH_POINTER (p, MODE (INT));
	  PUSH_INT (p, q);
	  INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_INT)));
	  PUSH_INT (p, expo.value);
	  t2 = whole (p);
	  s = stack_string (p, (int) (8 + strlen (t1) + 1 + strlen (t2)));
	  strcpy (s, t1);
	  string_plusab_char (s, EXPONENT_CHAR);
	  strcat (s, t2);
	  if (expo.value == 0 || strchr (s, ERROR_CHAR) != NULL)
	    {
	      stack_pointer = arg_sp;
	      PUSH_INT (p, width.value);
	      PUSH_INT (p, after.value != 0 ? after.value - 1 : 0);
	      PUSH_INT (p, expo.value > 0 ? expo.value + 1 : expo.value - 1);
	      return (fleet (p));
	    }
	  else
	    {
	      return (s);
	    }
	}
      else
	{
	  char *s = stack_string (p, 8 + ABS (width.value));
	  return (error_chars (s, width.value));
	}
    }
  else if (mode == MODE (INT))
    {
      int x = ((A68_INT *) (STACK_OFFSET (SIZE_OF (A68_POINTER))))->value;
      PUSH_POINTER (p, MODE (REAL));
      PUSH_REAL (p, (double) x);
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)) - (SIZE_OF (A68_POINTER) + SIZE_OF (A68_REAL)));
      PUSH_INT (p, width.value);
      PUSH_INT (p, after.value);
      PUSH_INT (p, expo.value);
      return (fleet (p));
    }
  else if (mode == MODE (LONG_INT) || mode == MODE (LONGLONG_INT))
    {
      stack_pointer = old_sp;
      if (mode == MODE (LONG_INT))
	{
	  ((A68_POINTER *) STACK_TOP)->value = (void *) MODE (LONG_REAL);
	}
      else
	{
	  ((A68_POINTER *) STACK_TOP)->value = (void *) MODE (LONGLONG_REAL);
	}
      INCREMENT_STACK_POINTER (p, MOID_SIZE (MODE (NUMBER)));
      PUSH_INT (p, width.value);
      PUSH_INT (p, after.value);
      PUSH_INT (p, expo.value);
      return (fleet (p));
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (NUMBER, INT) STRING whole                                               */

void
genie_whole (NODE_T * p)
{
  int old_sp = stack_pointer;
  A68_REF ref;
  char *str = whole (p);
  stack_pointer = old_sp - SIZE_OF (A68_INT) - MOID_SIZE (MODE (NUMBER));
  ref = tmp_to_a68_string (p, str);
  PUSH (p, &ref, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (NUMBER, INT, INT) STRING fixed                                          */

void
genie_fixed (NODE_T * p)
{
  int old_sp = stack_pointer;
  A68_REF ref;
  char *str = fixed (p);
  stack_pointer = old_sp - 2 * SIZE_OF (A68_INT) - MOID_SIZE (MODE (NUMBER));
  ref = tmp_to_a68_string (p, str);
  PUSH (p, &ref, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (NUMBER, INT, INT, INT) STRING float                                     */

void
genie_float (NODE_T * p)
{
  int old_sp = stack_pointer;
  A68_REF ref;
  char *str = fleet (p);
  stack_pointer = old_sp - 3 * SIZE_OF (A68_INT) - MOID_SIZE (MODE (NUMBER));
  ref = tmp_to_a68_string (p, str);
  PUSH (p, &ref, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
ALGOL68C routines.                                                            */

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC INT read int                                                             */

void
genie_read_int (NODE_T * p)
{
  genie_read_standard (p, MODE (INT), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_INT));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG INT read long int                                                   */

void
genie_read_long_int (NODE_T * p)
{
  genie_read_standard (p, MODE (LONG_INT), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, get_mp_size (MODE (LONG_INT)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG LONG INT read long long int                                         */

void
genie_read_longlong_int (NODE_T * p)
{
  genie_read_standard (p, MODE (LONGLONG_INT), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, get_mp_size (MODE (LONGLONG_INT)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC REAL read real                                                           */

void
genie_read_real (NODE_T * p)
{
  genie_read_standard (p, MODE (REAL), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_REAL));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG REAL read long real                                                 */

void
genie_read_long_real (NODE_T * p)
{
  genie_read_standard (p, MODE (LONG_REAL), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, get_mp_size (MODE (LONG_REAL)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG LONG REAL read long long real                                       */

void
genie_read_longlong_real (NODE_T * p)
{
  genie_read_standard (p, MODE (LONGLONG_REAL), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, get_mp_size (MODE (LONGLONG_REAL)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC COMPLEX read complex                                                     */

void
genie_read_complex (NODE_T * p)
{
  genie_read_real (p);
  genie_read_real (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG COMPLEX read long complex                                           */

void
genie_read_long_complex (NODE_T * p)
{
  genie_read_long_real (p);
  genie_read_long_real (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG LONG COMPLEX read long long complex                                 */

void
genie_read_longlong_complex (NODE_T * p)
{
  genie_read_longlong_real (p);
  genie_read_longlong_real (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC BOOL read bool                                                           */

void
genie_read_bool (NODE_T * p)
{
  genie_read_standard (p, MODE (BOOL), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_BOOL));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC BITS read bits                                                           */

void
genie_read_bits (NODE_T * p)
{
  genie_read_standard (p, MODE (BITS), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_BITS));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG BITS read long bits                                                 */

void
genie_read_long_bits (NODE_T * p)
{
  mp_digit *z = stack_mp (p, get_mp_digits (MODE (LONG_BITS)));
  genie_read_standard (p, MODE (LONG_BITS), (BYTE_T *) z, stand_in);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG LONG BITS read long long bits                                       */

void
genie_read_longlong_bits (NODE_T * p)
{
  mp_digit *z = stack_mp (p, get_mp_digits (MODE (LONGLONG_BITS)));
  genie_read_standard (p, MODE (LONGLONG_BITS), (BYTE_T *) z, stand_in);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC CHAR read char                                                           */

void
genie_read_char (NODE_T * p)
{
  genie_read_standard (p, MODE (CHAR), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_CHAR));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC BYTES read bytes                                                         */

void
genie_read_bytes (NODE_T * p)
{
  genie_read_standard (p, MODE (BYTES), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_BYTES));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC LONG BYTES read long bytes                                               */

void
genie_read_long_bytes (NODE_T * p)
{
  genie_read_standard (p, MODE (LONG_BYTES), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_LONG_BYTES));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC STRING read string                                                       */

void
genie_read_string (NODE_T * p)
{
  genie_read_standard (p, MODE (STRING), STACK_TOP, stand_in);
  INCREMENT_STACK_POINTER (p, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (INT) VOID print int                                                     */

void
genie_print_int (NODE_T * p)
{
  int size = MOID_SIZE (MODE (INT));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (INT), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG INT) VOID print long int                                           */

void
genie_print_long_int (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONG_INT));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONG_INT), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG LONG INT) VOID print long long int                                 */

void
genie_print_longlong_int (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONGLONG_INT));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONGLONG_INT), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (REAL) VOID print real                                                   */

void
genie_print_real (NODE_T * p)
{
  int size = MOID_SIZE (MODE (REAL));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (REAL), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG REAL) VOID print long real                                         */

void
genie_print_long_real (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONG_REAL));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONG_REAL), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG LONG REAL) VOID print long long real                               */

void
genie_print_longlong_real (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONGLONG_REAL));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONGLONG_REAL), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (COMPLEX) VOID print complex                                             */

void
genie_print_complex (NODE_T * p)
{
  int size = MOID_SIZE (MODE (COMPLEX));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (COMPLEX), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG COMPLEX) VOID print long complex                                   */

void
genie_print_long_complex (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONG_COMPLEX));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONG_COMPLEX), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG LONG COMPLEX) VOID print long long complex                         */

void
genie_print_longlong_complex (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONGLONG_COMPLEX));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONGLONG_COMPLEX), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (CHAR) VOID print char                                                   */

void
genie_print_char (NODE_T * p)
{
  int size = MOID_SIZE (MODE (CHAR));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (CHAR), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (BITS) VOID print bits                                                   */

void
genie_print_bits (NODE_T * p)
{
  int size = MOID_SIZE (MODE (BITS));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (BITS), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG BITS) VOID print long bits                                         */

void
genie_print_long_bits (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONG_BITS));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONG_BITS), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG LONG BITS) VOID print long long bits                               */

void
genie_print_longlong_bits (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONGLONG_BITS));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONGLONG_BITS), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (BOOL) VOID print bool                                                   */

void
genie_print_bool (NODE_T * p)
{
  int size = MOID_SIZE (MODE (BOOL));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (BOOL), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (BYTES) VOID print bytes                                                 */

void
genie_print_bytes (NODE_T * p)
{
  int size = MOID_SIZE (MODE (BYTES));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (BYTES), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (LONG BYTES) VOID print long bytes                                       */

void
genie_print_long_bytes (NODE_T * p)
{
  int size = MOID_SIZE (MODE (LONG_BYTES));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, MODE (LONG_BYTES), STACK_OFFSET (-size), stand_out);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);
  DECREMENT_STACK_POINTER (p, size);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
PROC (STRING) VOID print string                                               */

void
genie_print_string (NODE_T * p)
{
  reset_transput_buffer (UNFORMATTED_BUFFER);
  add_string_from_stack_transput_buffer (p, UNFORMATTED_BUFFER);
  write_purge_buffer (stand_out, UNFORMATTED_BUFFER);

}
