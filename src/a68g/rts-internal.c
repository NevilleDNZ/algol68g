//! @file rts-internal.c
//! @author J. Marcel van der Veer
//!
//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].
//!
//! @section License
//!
//! This program is free software; you can redistribute it and/or modify it 
//! under the terms of the GNU General Public License as published by the 
//! Free Software Foundation; either version 3 of the License, or 
//! (at your option) any later version.
//!
//! This program is distributed in the hope that it will be useful, but 
//! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//! or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
//! more details. You should have received a copy of the GNU General Public 
//! License along with this program. If not, see [http://www.gnu.org/licenses/].

//! @section Synopsis 
//!
//! Transput routines.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"
#include "a68g-transput.h"

// These routines use A68G's RR transput routines,
// essentially mimicking code of the form
//
//    PROC puts = (REF STRING s, [] SIMPLOUT items) VOID:
//         BEGIN FILE f;
//               associate (f, s);
//               put (f, items);
//               close (f)
//         END 
//
// which is not the most efficient, though practical.

static void associate (A68_FILE *f, A68_REF s)
{
  STATUS (f) = INIT_MASK;
  FILE_ENTRY (f) = -1;
  CHANNEL (f) = A68 (associate_channel);
  OPENED (f) = A68_TRUE;
  OPEN_EXCLUSIVE (f) = A68_FALSE;
  READ_MOOD (f) = A68_FALSE;
  WRITE_MOOD (f) = A68_FALSE;
  CHAR_MOOD (f) = A68_FALSE;
  DRAW_MOOD (f) = A68_FALSE;
  TMP_FILE (f) = A68_FALSE;
  IDENTIFICATION (f) = nil_ref;
  TERMINATOR (f) = nil_ref;
  FORMAT (f) = nil_format;
  FD (f) = A68_NO_FILE;
  STRING (f) = s;
  STRPOS (f) = 0;
  DEVICE_MADE (&DEVICE (f)) = A68_FALSE;
  STREAM (&DEVICE (f)) = NO_STREAM;
  set_default_event_procedures (f);
}

//! @brief PROC (REF STRING, [] SIMPLIN) VOID gets

void genie_get_text (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLIN.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLIN);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_reading (p, ref_file);
// Read.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_read_standard (p, mode, item, ref_file);
      elem_index += SIZE (M_SIMPLIN);
    }
  }
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}

//! @brief PROC (REF STRING, [] SIMPLOUT) VOID puts

void genie_put_text (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLOUT.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLOUT);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_writing (p, ref_file);
// Write.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    reset_transput_buffer (UNFORMATTED_BUFFER);
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_write_standard (p, mode, item, ref_file);
      elem_index += SIZE (M_SIMPLOUT);
    }
    * DEREF (A68_REF, &ref_string) = c_to_a_string (p, get_transput_buffer (UNFORMATTED_BUFFER), DEFAULT_WIDTH);
  }
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}

//! @brief PROC (REF STRING, [] SIMPLIN) VOID getsf

void genie_getf_text (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLIN.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLIN);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_reading (p, ref_file);
// Save stack state since formats have frames.
  ADDR_T pop_fp = FRAME_POINTER (file);
  ADDR_T pop_sp = STACK_POINTER (file);
  FRAME_POINTER (file) = A68_FP;
  STACK_POINTER (file) = A68_SP;
// Process [] SIMPLIN.
  if (BODY (&FORMAT (file)) != NO_NODE) {
    open_format_frame (p, ref_file, &FORMAT (file), NOT_EMBEDDED_FORMAT, A68_FALSE);
  }
  int formats = 0;
// Read.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_read_standard_format (p, mode, item, ref_file, &formats);
      elem_index += SIZE (M_SIMPLIN);
    }
  }
// Empty the format to purge insertions.
  purge_format_read (p, ref_file);
  BODY (&FORMAT (file)) = NO_NODE;
// Forget about active formats.
  A68_FP = FRAME_POINTER (file);
  A68_SP = STACK_POINTER (file);
  FRAME_POINTER (file) = pop_fp;
  STACK_POINTER (file) = pop_sp;
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}

//! @brief PROC (REF STRING, [] SIMPLOUT) VOID putsf

void genie_putf_text (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLOUT.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLOUT);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_writing (p, ref_file);
// Save stack state since formats have frames.
  ADDR_T pop_fp = FRAME_POINTER (file);
  ADDR_T pop_sp = STACK_POINTER (file);
  FRAME_POINTER (file) = A68_FP;
  STACK_POINTER (file) = A68_SP;
// Process [] SIMPLIN.
  if (BODY (&FORMAT (file)) != NO_NODE) {
    open_format_frame (p, ref_file, &FORMAT (file), NOT_EMBEDDED_FORMAT, A68_FALSE);
  }
  int formats = 0;
// Write.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    reset_transput_buffer (FORMATTED_BUFFER);
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_write_standard_format (p, mode, item, ref_file, &formats);
      elem_index += SIZE (M_SIMPLOUT);
    }
    * DEREF (A68_REF, &ref_string) = c_to_a_string (p, get_transput_buffer (FORMATTED_BUFFER), DEFAULT_WIDTH);
  }
// Empty the format to purge insertions.
  purge_format_write (p, ref_file);
  write_purge_buffer (p, ref_file, FORMATTED_BUFFER);
  BODY (&FORMAT (file)) = NO_NODE;
// Forget about active formats.
  A68_FP = FRAME_POINTER (file);
  A68_SP = STACK_POINTER (file);
  FRAME_POINTER (file) = pop_fp;
  STACK_POINTER (file) = pop_sp;
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}

//! @brief PROC (REF STRING, [] SIMPLOUT) REF STRING string

void genie_string (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLOUT.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLOUT);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_writing (p, ref_file);
// Write.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    reset_transput_buffer (UNFORMATTED_BUFFER);
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_write_standard (p, mode, item, ref_file);
      elem_index += SIZE (M_SIMPLOUT);
    }
    * DEREF (A68_REF, &ref_string) = c_to_a_string (p, get_transput_buffer (UNFORMATTED_BUFFER), DEFAULT_WIDTH);
  }
//
  PUSH_REF (p, ref_string);
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}

//! @brief PROC (REF STRING, [] SIMPLOUT) REF STRING stringf

void genie_stringf (NODE_T * p)
{
// Block GC momentarily.
  A68_GC (sema)++;
// Pop [] SIMPLOUT.
  A68_REF row; 
  POP_REF (p, &row);
  CHECK_REF (p, row, M_ROW_SIMPLOUT);
// Pop REF STRING.
  A68_REF ref_string;
  POP_REF (p, &ref_string);
  CHECK_REF (p, ref_string, M_REF_STRING);
// Associate a temp file with argument string.
  A68_REF ref_file = heap_generator (p, M_REF_FILE, SIZE (M_FILE));
  A68_FILE *file = FILE_DEREF (&ref_file);
  associate (file, ref_string);
  open_for_writing (p, ref_file);
// Save stack state since formats have frames.
  ADDR_T pop_fp = FRAME_POINTER (file);
  ADDR_T pop_sp = STACK_POINTER (file);
  FRAME_POINTER (file) = A68_FP;
  STACK_POINTER (file) = A68_SP;
// Process [] SIMPLIN.
  if (BODY (&FORMAT (file)) != NO_NODE) {
    open_format_frame (p, ref_file, &FORMAT (file), NOT_EMBEDDED_FORMAT, A68_FALSE);
  }
  int formats = 0;
// Write.
  A68_ARRAY *arr; A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, &row);
  int elems = ROW_SIZE (tup);
  if (elems > 0) {
    reset_transput_buffer (FORMATTED_BUFFER);
    BYTE_T *base_address = DEREF (BYTE_T, &ARRAY (arr));
    int elem_index = 0;
    for (int k = 0; k < elems; k++) {
      A68_UNION *z = (A68_UNION *) & base_address[elem_index];
      MOID_T *mode = (MOID_T *) (VALUE (z));
      BYTE_T *item = (BYTE_T *) & base_address[elem_index + A68_UNION_SIZE];
      genie_write_standard_format (p, mode, item, ref_file, &formats);
      elem_index += SIZE (M_SIMPLOUT);
    }
    * DEREF (A68_REF, &ref_string) = c_to_a_string (p, get_transput_buffer (FORMATTED_BUFFER), DEFAULT_WIDTH);
  }
// Empty the format to purge insertions.
  purge_format_write (p, ref_file);
  write_purge_buffer (p, ref_file, FORMATTED_BUFFER);
  BODY (&FORMAT (file)) = NO_NODE;
// Forget about active formats.
  A68_FP = FRAME_POINTER (file);
  A68_SP = STACK_POINTER (file);
  FRAME_POINTER (file) = pop_fp;
  STACK_POINTER (file) = pop_sp;
//
  PUSH_REF (p, ref_string);
// Discard temp file.
  unblock_transput_buffer (TRANSPUT_BUFFER (file));
  A68_GC (sema)--;
}
