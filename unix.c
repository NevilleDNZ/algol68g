/*!
\file unix.c
\brief extensions to A68G
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2011 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* Low-level UNIX routines */

#if defined HAVE_CONFIG_H
#include "a68g-config.h"
#endif

#include "a68g.h"

#define MAX_RESTART 256

BOOL_T halt_typing;
static int chars_in_tty_line;

char output_line[BUFFER_SIZE], edit_line[BUFFER_SIZE], input_line[BUFFER_SIZE];

/*!
\brief initialise output to STDOUT
**/

void init_tty (void)
{
  chars_in_tty_line = 0;
  halt_typing = A68_FALSE;
  change_masks (program.top_node, BREAKPOINT_INTERRUPT_MASK, A68_FALSE);
}

/*!
\brief terminate current line on STDOUT
**/

void io_close_tty_line (void)
{
  if (chars_in_tty_line > 0) {
    io_write_string (STDOUT_FILENO, NEWLINE_STRING);
  }
}

/*!
\brief get a char from STDIN
\return same
**/

char get_stdin_char (void)
{
  ssize_t j;
  char ch[4];
  RESET_ERRNO;
  j = io_read_conv (STDIN_FILENO, &(ch[0]), 1);
  ABEND (j < 0, "cannot read char from stdin", NULL);
  return ((char) (j == 1 ? ch[0] : EOF_CHAR));
}

/*!
\brief read string from STDIN, until NEWLINE_STRING
\param prompt prompt string
\return input line buffer
**/

char *read_string_from_tty (char *prompt)
{
  int ch, k = 0, n;
  if (prompt != NULL) {
    io_close_tty_line ();
    io_write_string (STDOUT_FILENO, prompt);
  }
  ch = get_stdin_char ();
  while (ch != NEWLINE_CHAR && k < BUFFER_SIZE - 1) {
    if (ch == EOF_CHAR) {
      input_line[0] = EOF_CHAR;
      input_line[1] = NULL_CHAR;
      chars_in_tty_line = 1;
      return (input_line);
    } else {
      input_line[k++] = (char) ch;
      ch = get_stdin_char ();
    }
  }
  input_line[k] = NULL_CHAR;
  n = (int) strlen (input_line);
  chars_in_tty_line = (ch == NEWLINE_CHAR ? 0 : (n > 0 ? n : 1));
  return (input_line);
}

/*!
\brief write string to file
\param f file number
\param z string to write
**/

void io_write_string (FILE_T f, const char *z)
{
  ssize_t j;
  RESET_ERRNO;
  if (f != STDOUT_FILENO && f != STDERR_FILENO) {
/* Writing to file */
    j = io_write_conv (f, z, strlen (z));
    ABEND (j < 0, "cannot write", NULL);
  } else {
/* Writing to TTY */
    int first, k;
/* Write parts until end-of-string */
    first = 0;
    do {
      k = first;
/* How far can we get? */
      while (z[k] != NULL_CHAR && z[k] != NEWLINE_CHAR) {
        k++;
      }
      if (k > first) {
/* Write these characters */
        int n = k - first;
        j = io_write_conv (f, &(z[first]), (size_t) n);
        ABEND (j < 0, "cannot write", NULL);
        chars_in_tty_line += n;
      }
      if (z[k] == NEWLINE_CHAR) {
/* Pretty-print newline */
        k++;
        first = k;
        j = io_write_conv (f, NEWLINE_STRING, 1);
        ABEND (j < 0, "cannot write", NULL);
        chars_in_tty_line = 0;
      }
    } while (z[k] != NULL_CHAR);
  }
}

/*!
\brief read bytes from file into buffer
\param fd file descriptor, must be open
\param buf character buffer, size must be >= n
\param n maximum number of bytes to read
\return number of bytes read or -1 in case of error
**/

ssize_t io_read (FILE_T fd, void *buf, size_t n)
{
  size_t to_do = n;
  int restarts = 0;
  char *z = (char *) buf;
  while (to_do > 0) {
    ssize_t bytes_read;
    RESET_ERRNO;
    bytes_read = read (fd, z, to_do);
    if (bytes_read < 0) {
      if (errno == EINTR) {
/* interrupt, retry */
        bytes_read = 0;
        if (restarts++ > MAX_RESTART) {
          return (-1);
        }
      } else {
/* read error */
        return (-1);
      }
    } else if (bytes_read == 0) {
      break;                    /* EOF_CHAR */
    }
    to_do -= (size_t) bytes_read;
    z += bytes_read;
  }
  return ((ssize_t) n - (ssize_t) to_do);       /* return >= 0 */
}

/*!
\brief writes n bytes from buffer to file
\param fd file descriptor, must be open
\param buf character buffer, size must be >= n
\param n maximum number of bytes to write
\return n or -1 in case of error
**/

ssize_t io_write (FILE_T fd, const void *buf, size_t n)
{
  size_t to_do = n;
  int restarts = 0;
  char *z = (char *) buf;
  while (to_do > 0) {
    ssize_t bytes_written;
    RESET_ERRNO;
    bytes_written = write (fd, z, to_do);
    if (bytes_written <= 0) {
      if (errno == EINTR) {
/* interrupt, retry */
        bytes_written = 0;
        if (restarts++ > MAX_RESTART) {
          return (-1);
        }
      } else {
/* write error */
        return (-1);
      }
    }
    to_do -= (size_t) bytes_written;
    z += bytes_written;
  }
  return ((ssize_t) n);
}

/*!
\brief read bytes from file into buffer
\param fd file descriptor, must be open
\param buf character buffer, size must be >= n
\param n maximum number of bytes to read
\return number of bytes read or -1 in case of error
**/

ssize_t io_read_conv (FILE_T fd, void *buf, size_t n)
{
  size_t to_do = n;
  int restarts = 0;
  char *z = (char *) buf;
  while (to_do > 0) {
    ssize_t bytes_read;
    RESET_ERRNO;
    bytes_read = read (fd, z, to_do);
    if (bytes_read < 0) {
      if (errno == EINTR) {
/* interrupt, retry */
        bytes_read = 0;
        if (restarts++ > MAX_RESTART) {
          return (-1);
        }
      } else {
/* read error */
        return (-1);
      }
    } else if (bytes_read == 0) {
      break;                    /* EOF_CHAR */
    }
    to_do -= (size_t) bytes_read;
    z += bytes_read;
  }
  return ((ssize_t) n - (ssize_t) to_do);
}

/*!
\brief writes n bytes from buffer to file
\param fd file descriptor, must be open
\param buf character buffer, size must be >= n
\param n maximum number of bytes to write
\return n or -1 in case of error
**/

ssize_t io_write_conv (FILE_T fd, const void *buf, size_t n)
{
  size_t to_do = n;
  int restarts = 0;
  char *z = (char *) buf;
  while (to_do > 0) {
    ssize_t bytes_written;
    RESET_ERRNO;
    bytes_written = write (fd, z, to_do);
    if (bytes_written <= 0) {
      if (errno == EINTR) {
/* interrupt, retry */
        bytes_written = 0;
        if (restarts++ > MAX_RESTART) {
          return (-1);
        }
      } else {
/* write error */
        return (-1);
      }
    }
    to_do -= (size_t) bytes_written;
    z += bytes_written;
  }
  return ((ssize_t) n);
}

/*
This code implements some UNIX/Linux/BSD related routines.
In part contributed by Sian Leitch. 
*/

#define VECTOR_SIZE 512
#define FD_READ 0
#define FD_WRITE 1

extern A68_REF tmp_to_a68_string (NODE_T *, char *);

extern A68_CHANNEL stand_in_channel, stand_out_channel, stand_draw_channel, stand_back_channel, stand_error_channel;

#if defined HAVE_DIRENT_H

#include <dirent.h>

/*!
\brief PROC (STRING) [] STRING directory
\param p position in tree
**/

void genie_directory (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
    PUSH_PRIMITIVE (p, A68_MAX_INT, A68_INT);
  } else {
    char *dir_name = a_to_c_string (p, buffer, name);
    A68_REF z, row;
    A68_ARRAY arr;
    A68_TUPLE tup;
    int k, n = 0;
    A68_REF *base;
    DIR *dir;
    struct dirent *entry;
    dir = opendir (dir_name);
    if (dir == NULL) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    do {
      entry = readdir (dir);
      if (errno != 0) {
        diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
        exit_genie (p, A68_RUNTIME_ERROR);
      }
      if (entry != NULL) {
        n++;
      }
    } while (entry != NULL);
    rewinddir (dir);
    if (errno != 0) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    z = heap_generator (p, MODE (ROW_STRING), ALIGNED_SIZE_OF (A68_ARRAY) + ALIGNED_SIZE_OF (A68_TUPLE));
    BLOCK_GC_HANDLE (&z);
    row = heap_generator (p, MODE (ROW_STRING), n * MOID_SIZE (MODE (STRING)));
    DIM (&arr) = 1;
    MOID (&arr) = MODE (STRING);
    arr.elem_size = MOID_SIZE (MODE (STRING));
    arr.slice_offset = 0;
    arr.field_offset = 0;
    ARRAY (&arr) = row;
    LWB (&tup) = 1;
    UPB (&tup) = n;
    tup.shift = LWB (&tup);
    tup.span = 1;
    tup.k = 0;
    PUT_DESCRIPTOR (arr, tup, &z);
    base = (A68_REF *) ADDRESS (&row);
    for (k = 0; k < n; k++) {
      entry = readdir (dir);
      if (errno != 0) {
        diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
        exit_genie (p, A68_RUNTIME_ERROR);
      }
      base[k] = c_to_a_string (p, entry->d_name);
    }
    if (closedir (dir) != 0) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    PUSH_REF (p, z);
    UNBLOCK_GC_HANDLE (&z);
    free (buffer);
  }
}

#endif

/*!
\brief PROC [] INT utc time
\param p position in tree
**/

void genie_utctime (NODE_T * p)
{
  time_t dt;
  if (time (&dt) == (time_t) - 1) {
    (void) empty_row (p, MODE (ROW_INT));
  } else {
    A68_REF row;
    ADDR_T sp = stack_pointer;
    struct tm *tod = gmtime (&dt);
    PUSH_PRIMITIVE (p, tod->tm_year + 1900, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_mon + 1, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_mday, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_hour, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_min, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_sec, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_wday + 1, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_isdst, A68_INT);
    row = genie_make_row (p, MODE (INT), 8, sp);
    stack_pointer = sp;
    PUSH_REF (p, row);
  }
}

/*!
\brief PROC [] INT local time
\param p position in tree
**/

void genie_localtime (NODE_T * p)
{
  time_t dt;
  if (time (&dt) == (time_t) - 1) {
    (void) empty_row (p, MODE (ROW_INT));
  } else {
    A68_REF row;
    ADDR_T sp = stack_pointer;
    struct tm *tod = localtime (&dt);
    PUSH_PRIMITIVE (p, tod->tm_year + 1900, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_mon + 1, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_mday, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_hour, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_min, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_sec, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_wday + 1, A68_INT);
    PUSH_PRIMITIVE (p, tod->tm_isdst, A68_INT);
    row = genie_make_row (p, MODE (INT), 8, sp);
    stack_pointer = sp;
    PUSH_REF (p, row);
  }
}

/*!
\brief PROC INT argc
\param p position in tree
**/

void genie_argc (NODE_T * p)
{
  RESET_ERRNO;
  PUSH_PRIMITIVE (p, global_argc, A68_INT);
}

/*!
\brief PROC (INT) STRING argv
\param p position in tree
**/

void genie_argv (NODE_T * p)
{
  A68_INT a68g_index;
  RESET_ERRNO;
  POP_OBJECT (p, &a68g_index, A68_INT);
  if (VALUE (&a68g_index) >= 1 && VALUE (&a68g_index) <= global_argc) {
    char *q = global_argv[VALUE (&a68g_index) - 1];
    int n = (int) strlen (q);
/* Allow for spaces ending in # to have A68 comment syntax with '#!' */
    while (n > 0 && (IS_SPACE(q[n - 1]) || q[n - 1] == '#')) {
      q[--n] = NULL_CHAR;
    }
    PUSH_REF (p, c_to_a_string (p, q));
  } else {
    PUSH_REF (p, empty_string (p));
  }
}

/*!
\brief PROC STRING pwd
\param p position in tree
**/

void genie_pwd (NODE_T * p)
{
  size_t size = BUFFER_SIZE;
  char *buffer = NULL;
  BOOL_T cont = A68_TRUE;
  RESET_ERRNO;
  while (cont) {
    buffer = (char *) malloc (size);
    if (buffer == NULL) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    if (getcwd (buffer, size) == buffer) {
      cont = A68_FALSE;
    } else {
      free (buffer);
      cont = (BOOL_T) (errno == 0);
      size *= 2;
    }
  }
  if (buffer != NULL && errno == 0) {
    PUSH_REF (p, c_to_a_string (p, buffer));
    free (buffer);
  } else {
    PUSH_REF (p, empty_string (p));
  }
}

/*!
\brief PROC (STRING) INT cd
\param p position in tree
**/

void genie_cd (NODE_T * p)
{
  A68_REF dir;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &dir);
  CHECK_INIT (p, INITIALISED (&dir), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, dir)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    int rc = chdir (a_to_c_string (p, buffer, dir));
    if (rc == 0) {
      PUSH_PRIMITIVE (p, chdir (a_to_c_string (p, buffer, dir)), A68_INT);
    } else {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_FILE_ACCESS);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    free (buffer);
  }
}

/*!
\brief PROC (STRING) BITS
\param p position in tree
**/

void genie_file_mode (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (unsigned) (status.st_mode), A68_BITS);
    } else {
      PUSH_PRIMITIVE (p, 0x0, A68_BITS);
    }
    free (buffer);
  }
}

/*!
\brief PROC (STRING) BOOL file is block device
\param p position in tree
**/

void genie_file_is_block_device (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISBLK (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

/*!
\brief PROC (STRING) BOOL file is char device
\param p position in tree
**/

void genie_file_is_char_device (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISCHR (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

/*!
\brief PROC (STRING) BOOL file is directory
\param p position in tree
**/

void genie_file_is_directory (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISDIR (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

/*!
\brief PROC (STRING) BOOL file is regular
\param p position in tree
**/

void genie_file_is_regular (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISREG (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

#if defined __S_IFIFO

/*!
\brief PROC (STRING) BOOL file is fifo
\param p position in tree
**/

void genie_file_is_fifo (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISFIFO (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

#endif

#if defined S_ISLNK

/*!
\brief PROC (STRING) BOOL file is link
\param p position in tree
**/

void genie_file_is_link (NODE_T * p)
{
  A68_REF name;
  char *buffer;
  RESET_ERRNO;
  POP_REF (p, &name);
  CHECK_INIT (p, INITIALISED (&name), MODE (STRING));
  buffer = (char *) malloc ((size_t) (1 + a68_string_size (p, name)));
  if (buffer == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_OUT_OF_CORE);
    exit_genie (p, A68_RUNTIME_ERROR);
  } else {
    struct stat status;
    if (stat (a_to_c_string (p, buffer, name), &status) == 0) {
      PUSH_PRIMITIVE (p, (BOOL_T) (S_ISLNK (status.st_mode) != 0 ? A68_TRUE : A68_FALSE), A68_BOOL);
    } else {
      PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    }
    free (buffer);
  }
}

#endif

/*!
\brief convert [] STRING row to char *vec[]
\param p position in tree
\param vec string vector
\param row [] STRING
**/

static void convert_string_vector (NODE_T * p, char *vec[], A68_REF row)
{
  BYTE_T *z = ADDRESS (&row);
  A68_ARRAY *arr = (A68_ARRAY *) & z[0];
  A68_TUPLE *tup = (A68_TUPLE *) & z[ALIGNED_SIZE_OF (A68_ARRAY)];
  int k = 0;
  if (get_row_size (tup, DIM (arr)) != 0) {
    BYTE_T *base_addr = ADDRESS (&ARRAY (arr));
    BOOL_T done = A68_FALSE;
    initialise_internal_index (tup, DIM (arr));
    while (!done) {
      ADDR_T a68g_index = calculate_internal_index (tup, DIM (arr));
      ADDR_T elem_addr = (a68g_index + arr->slice_offset) * arr->elem_size + arr->field_offset;
      BYTE_T *elem = &base_addr[elem_addr];
      int size = a68_string_size (p, *(A68_REF *) elem);
      CHECK_INIT (p, INITIALISED ((A68_REF *) elem), MODE (STRING));
      vec[k] = (char *) get_heap_space ((size_t) (1 + size));
      ASSERT (a_to_c_string (p, vec[k], *(A68_REF *) elem) != NULL);
      if (k == VECTOR_SIZE - 1) {
        diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_TOO_MANY_ARGUMENTS);
        exit_genie (p, A68_RUNTIME_ERROR);
      }
      if (strlen (vec[k]) > 0) {
        k++;
      }
      done = increment_internal_index (tup, DIM (arr));
    }
  }
  vec[k] = NULL;
}

/*!
\brief free char *vec[]
\param vec string vector
**/

static void free_vector (char *vec[])
{
  int k = 0;
  while (vec[k] != NULL) {
    free (vec[k]);
    k++;
  }
}

/*!
\brief reset error number
\param p position in tree
**/

void genie_reset_errno (NODE_T * p)
{
  (void) *p;
  RESET_ERRNO;
}

/*!
\brief error number
\param p position in tree
**/

void genie_errno (NODE_T * p)
{
  PUSH_PRIMITIVE (p, errno, A68_INT);
}

/*!
\brief PROC strerror = (INT) STRING
\param p position in tree
**/

void genie_strerror (NODE_T * p)
{
  A68_INT i;
  POP_OBJECT (p, &i, A68_INT);
  PUSH_REF (p, c_to_a_string (p, strerror (VALUE (&i))));
}

/*!
\brief set up file for usage in pipe
\param p position in tree
\param z pointer to file
\param fd file number
\param chan channel
\param r_mood read mood
\param w_mood write mood
\param pid pid
**/

static void set_up_file (NODE_T * p, A68_REF * z, int fd, A68_CHANNEL chan, BOOL_T r_mood, BOOL_T w_mood, int pid)
{
  A68_FILE *f;
  *z = heap_generator (p, MODE (REF_FILE), ALIGNED_SIZE_OF (A68_FILE));
  f = (A68_FILE *) ADDRESS (z);
  STATUS (f) = (STATUS_MASK) ((pid < 0) ? 0 : INITIALISED_MASK);
  f->identification = nil_ref;
  f->terminator = nil_ref;
  f->channel = chan;
  f->fd = fd;
  f->device.stream = NULL;
  f->opened = A68_TRUE;
  f->open_exclusive = A68_FALSE;
  f->read_mood = r_mood;
  f->write_mood = w_mood;
  f->char_mood = A68_TRUE;
  f->draw_mood = A68_FALSE;
  FORMAT (f) = nil_format;
  f->transput_buffer = get_unblocked_transput_buffer (p);
  f->string = nil_ref;
  reset_transput_buffer (f->transput_buffer);
  set_default_mended_procedures (f);
}

/*!
\brief create and push a pipe
\param p position in tree
\param fd_r read file number
\param fd_w write file number
\param pid pid
**/

static void genie_mkpipe (NODE_T * p, int fd_r, int fd_w, int pid)
{
  A68_REF r, w;
  RESET_ERRNO;
/* Set up pipe */
  set_up_file (p, &r, fd_r, stand_in_channel, A68_TRUE, A68_FALSE, pid);
  set_up_file (p, &w, fd_w, stand_out_channel, A68_FALSE, A68_TRUE, pid);
/* push pipe */
  PUSH_REF (p, r);
  PUSH_REF (p, w);
  PUSH_PRIMITIVE (p, pid, A68_INT);
}

/*!
\brief push an environment string
\param p position in tree
**/

void genie_getenv (NODE_T * p)
{
  A68_REF a_env;
  char *val, *z, *z_env;
  RESET_ERRNO;
  POP_REF (p, &a_env);
  CHECK_INIT (p, INITIALISED (&a_env), MODE (STRING));
  z_env = (char *) get_heap_space ((size_t) (1 + a68_string_size (p, a_env)));
  z = a_to_c_string (p, z_env, a_env);
  val = getenv (z);
  if (val == NULL) {
    a_env = empty_string (p);
  } else {
    a_env = tmp_to_a68_string (p, val);
  }
  PUSH_REF (p, a_env);
}

/*!
\brief PROC fork = INT
\param p position in tree
**/

void genie_fork (NODE_T * p)
{
  int pid;
  RESET_ERRNO;
  pid = (int) fork ();
  PUSH_PRIMITIVE (p, pid, A68_INT);
}

/*!
\brief PROC execve = (STRING, [] STRING, [] STRING) INT 
\param p position in tree
**/

void genie_execve (NODE_T * p)
{
  int ret;
  A68_REF a_prog, a_args, a_env;
  char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
  RESET_ERRNO;
/* Pop parameters */
  POP_REF (p, &a_env);
  POP_REF (p, &a_args);
  POP_REF (p, &a_prog);
/* Convert strings and hasta el infinito */
  prog = (char *) get_heap_space ((size_t) (1 + a68_string_size (p, a_prog)));
  ASSERT (a_to_c_string (p, prog, a_prog) != NULL);
  convert_string_vector (p, argv, a_args);
  convert_string_vector (p, envp, a_env);
  if (argv[0] == NULL) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_EMPTY_ARGUMENT);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  ret = execve (prog, argv, envp);
/* execve only returns if it fails */
  free_vector (argv);
  free_vector (envp);
  free (prog);
  PUSH_PRIMITIVE (p, ret, A68_INT);
}

/*!
\brief PROC execve child = (STRING, [] STRING, [] STRING) INT
\param p position in tree
**/

void genie_execve_child (NODE_T * p)
{
  int pid;
  A68_REF a_prog, a_args, a_env;
  RESET_ERRNO;
/* Pop parameters */
  POP_REF (p, &a_env);
  POP_REF (p, &a_args);
  POP_REF (p, &a_prog);
/* Now create the pipes and fork */
  pid = (int) fork ();
  if (pid == -1) {
    PUSH_PRIMITIVE (p, -1, A68_INT);
  } else if (pid == 0) {
/* Child process */
    char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings */
    prog = (char *) get_heap_space ((size_t) (1 + a68_string_size (p, a_prog)));
    ASSERT (a_to_c_string (p, prog, a_prog) != NULL);
    convert_string_vector (p, argv, a_args);
    convert_string_vector (p, envp, a_env);
    if (argv[0] == NULL) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_EMPTY_ARGUMENT);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process */
    a68g_exit (EXIT_FAILURE);
    PUSH_PRIMITIVE (p, 0, A68_INT);
  } else {
/* parent process */
    PUSH_PRIMITIVE (p, pid, A68_INT);
  }
}

/*!
\brief PROC execve child pipe = (STRING, [] STRING, [] STRING) PIPE
\param p position in tree
**/

void genie_execve_child_pipe (NODE_T * p)
{
/*
Child redirects STDIN and STDOUT.
Return a PIPE that contains the descriptors for the parent.

       pipe ptoc
       ->W...R->
 PARENT         CHILD
       <-R...W<-
       pipe ctop
*/
  int pid, ptoc_fd[2], ctop_fd[2];
  A68_REF a_prog, a_args, a_env;
  RESET_ERRNO;
/* Pop parameters */
  POP_REF (p, &a_env);
  POP_REF (p, &a_args);
  POP_REF (p, &a_prog);
/* Now create the pipes and fork */
  if ((pipe (ptoc_fd) == -1) || (pipe (ctop_fd) == -1)) {
    genie_mkpipe (p, -1, -1, -1);
    return;
  }
  pid = (int) fork ();
  if (pid == -1) {
/* Fork failure */
    genie_mkpipe (p, -1, -1, -1);
    return;
  }
  if (pid == 0) {
/* Child process */
    char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings */
    prog = (char *) get_heap_space ((size_t) (1 + a68_string_size (p, a_prog)));
    ASSERT (a_to_c_string (p, prog, a_prog) != NULL);
    convert_string_vector (p, argv, a_args);
    convert_string_vector (p, envp, a_env);
/* Set up redirection */
    ASSERT (close (ctop_fd[FD_READ]) == 0);
    ASSERT (close (ptoc_fd[FD_WRITE]) == 0);
    ASSERT (close (STDIN_FILENO) == 0);
    ASSERT (close (STDOUT_FILENO) == 0);
    ASSERT (dup2 (ptoc_fd[FD_READ], STDIN_FILENO) != -1);
    ASSERT (dup2 (ctop_fd[FD_WRITE], STDOUT_FILENO) != -1);
    if (argv[0] == NULL) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_EMPTY_ARGUMENT);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process */
    a68g_exit (EXIT_FAILURE);
    genie_mkpipe (p, -1, -1, -1);
  } else {
/* Parent process */
    ASSERT (close (ptoc_fd[FD_READ]) == 0);
    ASSERT (close (ctop_fd[FD_WRITE]) == 0);
    genie_mkpipe (p, ctop_fd[FD_READ], ptoc_fd[FD_WRITE], pid);
  }
}

/*!
\brief PROC execve_output = (STRING, [] STRING, [] STRING, REF_STRING) INT
\param p position in tree
**/

void genie_execve_output (NODE_T * p)
{
/*
Child redirects STDIN and STDOUT.

       pipe ptoc
       ->W...R->
 PARENT         CHILD
       <-R...W<-
       pipe ctop
*/
  int pid, ptoc_fd[2], ctop_fd[2];
  A68_REF a_prog, a_args, a_env, dest;
  RESET_ERRNO;
/* Pop parameters */
  POP_REF (p, &dest);
  POP_REF (p, &a_env);
  POP_REF (p, &a_args);
  POP_REF (p, &a_prog);
/* Now create the pipes and fork */
  if ((pipe (ptoc_fd) == -1) || (pipe (ctop_fd) == -1)) {
    PUSH_PRIMITIVE (p, -1, A68_INT);
    return;
  }
  pid = (int) fork ();
  if (pid == -1) {
/* Fork failure */
    PUSH_PRIMITIVE (p, -1, A68_INT);
    return;
  }
  if (pid == 0) {
/* Child process */
    char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings */
    prog = (char *) get_heap_space ((size_t) (1 + a68_string_size (p, a_prog)));
    ASSERT (a_to_c_string (p, prog, a_prog) != NULL);
    convert_string_vector (p, argv, a_args);
    convert_string_vector (p, envp, a_env);
/* Set up redirection */
    ASSERT (close (ctop_fd[FD_READ]) == 0);
    ASSERT (close (ptoc_fd[FD_WRITE]) == 0);
    ASSERT (close (STDIN_FILENO) == 0);
    ASSERT (close (STDOUT_FILENO) == 0);
    ASSERT (dup2 (ptoc_fd[FD_READ], STDIN_FILENO) != -1);
    ASSERT (dup2 (ctop_fd[FD_WRITE], STDOUT_FILENO) != -1);
    if (argv[0] == NULL) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_EMPTY_ARGUMENT);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process */
    a68g_exit (EXIT_FAILURE);
    PUSH_PRIMITIVE (p, -1, A68_INT);
  } else {
/* Parent process */
    char ch;
    int pipe_read, ret, status;
    ASSERT (close (ptoc_fd[FD_READ]) == 0);
    ASSERT (close (ctop_fd[FD_WRITE]) == 0);
    reset_transput_buffer (INPUT_BUFFER);
    do {
      pipe_read = (int) io_read_conv (ctop_fd[FD_READ], &ch, 1);
      if (pipe_read > 0) {
        add_char_transput_buffer (p, INPUT_BUFFER, ch);
      }
    } while (pipe_read > 0);
    do {
      ret = (int) waitpid ((__pid_t) pid, &status, 0);
    } while (ret == -1 && errno == EINTR);
    if (ret != pid) {
      status = -1;
    }
    if (!IS_NIL (dest)) {
      *(A68_REF *) ADDRESS (&dest) = c_to_a_string (p, get_transput_buffer (INPUT_BUFFER));
    }
    PUSH_PRIMITIVE (p, ret, A68_INT);
  }
}

/*!
\brief PROC create pipe = PIPE
\param p position in tree
**/

void genie_create_pipe (NODE_T * p)
{
  RESET_ERRNO;
  genie_stand_in (p);
  genie_stand_out (p);
  PUSH_PRIMITIVE (p, -1, A68_INT);
}

/*!
\brief PROC wait pid = (INT) VOID
\param p position in tree
**/

void genie_waitpid (NODE_T * p)
{
  A68_INT k;
  RESET_ERRNO;
  POP_OBJECT (p, &k, A68_INT);
  ASSERT (waitpid ((__pid_t) VALUE (&k), NULL, 0) != -1);
}

/*
Next part contains some routines that interface Algol68G and the curses library.
Be sure to know what you are doing when you use this, but on the other hand,
"reset" will always restore your terminal. 
*/

#if (defined HAVE_CURSES_H && defined HAVE_LIBNCURSES)

#define CHECK_CURSES_RETVAL(f) {\
  if (!(f)) {\
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_CURSES, NULL);\
    exit_genie (p, A68_RUNTIME_ERROR);\
  }}

BOOL_T a68g_curses_mode = A68_FALSE;

/*!
\brief clean_curses
**/

void clean_curses (void)
{
  if (a68g_curses_mode == A68_TRUE) {
    (void) attrset (A_NORMAL);
    (void) endwin ();
    a68g_curses_mode = A68_FALSE;
  }
}

/*!
\brief init_curses
**/

void init_curses (void)
{
  (void) initscr ();
  (void) cbreak (); /* raw () would cut off ctrl-c */
  (void) noecho ();
  (void) nonl ();
  (void) curs_set (0);
}

/*!
\brief watch stdin for input, do not wait very long
\return character read
**/

int rgetchar (void)
{
  int retval;
  struct timeval tv;
  fd_set rfds;
  tv.tv_sec = 0;
  tv.tv_usec = 100;
  FD_ZERO (&rfds);
  FD_SET (0, &rfds);
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (retval) {
    /* FD_ISSET(0, &rfds) will be true.  */
    return (getch ());
  } else {
    return (NULL_CHAR);
  }
}

/*!
\brief PROC curses start = VOID
\param p position in tree
**/

void genie_curses_start (NODE_T * p)
{
  errno = 0;
  init_curses ();
  if (errno != 0) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_CURSES, NULL);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  a68g_curses_mode = A68_TRUE;
}

/*!
\brief PROC curses end = VOID
\param p position in tree
**/

void genie_curses_end (NODE_T * p)
{
  (void) p;
  clean_curses ();
}

/*!
\brief PROC curses clear = VOID
\param p position in tree
**/

void genie_curses_clear (NODE_T * p)
{
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  CHECK_CURSES_RETVAL (clear () != ERR);
}

/*!
\brief PROC curses refresh = VOID
\param p position in tree
**/

void genie_curses_refresh (NODE_T * p)
{
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  CHECK_CURSES_RETVAL (refresh () != ERR);
}

/*!
\brief PROC curses lines = INT
\param p position in tree
**/

void genie_curses_lines (NODE_T * p)
{
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  PUSH_PRIMITIVE (p, LINES, A68_INT);
}

/*!
\brief PROC curses columns = INT
\param p position in tree
**/

void genie_curses_columns (NODE_T * p)
{
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  PUSH_PRIMITIVE (p, COLS, A68_INT);
}

/*!
\brief PROC curses getchar = CHAR
\param p position in tree
**/

void genie_curses_getchar (NODE_T * p)
{
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  PUSH_PRIMITIVE (p, (char) rgetchar (), A68_CHAR);
}

/*!
\brief PROC curses putchar = (CHAR) VOID
\param p position in tree
**/

void genie_curses_putchar (NODE_T * p)
{
  A68_CHAR ch;
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  POP_OBJECT (p, &ch, A68_CHAR);
  if (addch ((chtype) (VALUE (&ch))) == ERR) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_CURSES_OFF_SCREEN, NULL);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
}

/*!
\brief PROC curses move = (INT, INT) VOID
\param p position in tree
**/

void genie_curses_move (NODE_T * p)
{
  A68_INT i, j;
  if (a68g_curses_mode == A68_FALSE) {
    genie_curses_start (p);
  }
  POP_OBJECT (p, &j, A68_INT);
  POP_OBJECT (p, &i, A68_INT);
  if (VALUE (&i) < 0 || VALUE (&i) >= LINES) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_CURSES_OFF_SCREEN, NULL);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  if (VALUE (&j) < 0 || VALUE (&j) >= COLS) {
    diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_CURSES_OFF_SCREEN, NULL);
    exit_genie (p, A68_RUNTIME_ERROR);
  }
  CHECK_CURSES_RETVAL(move (VALUE (&i), VALUE (&j)) != ERR);
}

#endif /* HAVE_CURSES_H && defined HAVE_LIBNCURSES */

#if defined HAVE_HTTP

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PROTOCOL "tcp"
#define SERVICE "http"

#define CONTENT_BUFFER_SIZE (4 * KILOBYTE)
#define TIMEOUT_INTERVAL 15

/*!
\brief send GET request to server and yield answer (TCP/HTTP only)
\param p position in tree
**/

void genie_http_content (NODE_T * p)
{
  A68_REF path_string, domain_string, content_string;
  A68_INT port_number;
  int socket_id, conn, k;
  fd_set set;
  struct timeval a68g_timeout;
  struct servent *service_address;
  struct hostent *host_address;
  struct protoent *protocol;
  struct sockaddr_in socket_address;
  char buffer[CONTENT_BUFFER_SIZE];
  RESET_ERRNO;
/* Pop arguments */
  POP_OBJECT (p, &port_number, A68_INT);
  CHECK_INIT (p, INITIALISED (&port_number), MODE (INT));
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), MODE (STRING));
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), MODE (STRING));
  POP_REF (p, &content_string);
  CHECK_REF (p, content_string, MODE (REF_STRING));
  *(A68_REF *) ADDRESS (&content_string) = empty_string (p);
/* Reset buffers */
  reset_transput_buffer (DOMAIN_BUFFER);
  reset_transput_buffer (PATH_BUFFER);
  reset_transput_buffer (REQUEST_BUFFER);
  reset_transput_buffer (CONTENT_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
/* Make request */
  add_string_transput_buffer (p, REQUEST_BUFFER, "GET ");
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
  add_string_transput_buffer (p, REQUEST_BUFFER, " HTTP/1.0\n\n");
/* Connect to host */
  FILL (&socket_address, 0, (int) sizeof (socket_address));
  socket_address.sin_family = AF_INET;
  service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_PRIMITIVE (p, 1, A68_INT);
    return;
  }
  if (VALUE (&port_number) == 0) {
    socket_address.sin_port = (uint16_t) (service_address->s_port);
  } else {
/*
Next line provokes inevitably:
  warning: conversion to 'short unsigned int' from 'int' may alter its value
This warning can be safely ignored.
*/
    socket_address.sin_port = (uint16_t) (htons ((uint16_t) (VALUE (&port_number))));
    if (socket_address.sin_port == 0) {
      PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    }
  }
  host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  COPY (&socket_address.sin_addr, host_address->h_addr, host_address->h_length);
  protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  socket_id = socket (PF_INET, SOCK_STREAM, protocol->p_proto);
  if (socket_id < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  conn = connect (socket_id, (const struct sockaddr *) &socket_address, (socklen_t) ALIGNED_SIZE_OF (socket_address));
  if (conn < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Read from host */
  WRITE (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_PRIMITIVE (p, errno, A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Initialise file descriptor set */
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
/* Initialise the a68g_timeout data structure */
  a68g_timeout.tv_sec = TIMEOUT_INTERVAL;
  a68g_timeout.tv_usec = 0;
/* Block until server replies or a68g_timeout blows up */
  switch (select (FD_SETSIZE, &set, NULL, NULL, &a68g_timeout)) {
  case 0:
    {
      errno = ETIMEDOUT;
      PUSH_PRIMITIVE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case -1:
    {
      PUSH_PRIMITIVE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case 1:
    {
      break;
    }
  default:
    {
      ABEND (A68_TRUE, "unexpected result from select", NULL);
    }
  }
  while ((k = (int) io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    buffer[k] = NULL_CHAR;
    add_string_transput_buffer (p, CONTENT_BUFFER, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Convert string */
  *(A68_REF *) ADDRESS (&content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER));
  ASSERT (close (socket_id) == 0);
  PUSH_PRIMITIVE (p, errno, A68_INT);
}

/*!
\brief send request to server and yield answer (TCP only)
\param p position in tree
**/

void genie_tcp_request (NODE_T * p)
{
  A68_REF path_string, domain_string, content_string;
  A68_INT port_number;
  int socket_id, conn, k;
  fd_set set;
  struct timeval a68g_timeout;
  struct servent *service_address;
  struct hostent *host_address;
  struct protoent *protocol;
  struct sockaddr_in socket_address;
  char buffer[CONTENT_BUFFER_SIZE];
  RESET_ERRNO;
/* Pop arguments */
  POP_OBJECT (p, &port_number, A68_INT);
  CHECK_INIT (p, INITIALISED (&port_number), MODE (INT));
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), MODE (STRING));
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), MODE (STRING));
  POP_REF (p, &content_string);
  CHECK_REF (p, content_string, MODE (REF_STRING));
  *(A68_REF *) ADDRESS (&content_string) = empty_string (p);
/* Reset buffers */
  reset_transput_buffer (DOMAIN_BUFFER);
  reset_transput_buffer (PATH_BUFFER);
  reset_transput_buffer (REQUEST_BUFFER);
  reset_transput_buffer (CONTENT_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
/* Make request */
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
/* Connect to host */
  FILL (&socket_address, 0, (int) sizeof (socket_address));
  socket_address.sin_family = AF_INET;
  service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_PRIMITIVE (p, 1, A68_INT);
    return;
  }
  if (VALUE (&port_number) == 0) {
    socket_address.sin_port = (uint16_t) (service_address->s_port);
  } else {
/*
Next line provokes inevitably:
  warning: conversion to 'short unsigned int' from 'int' may alter its value
This warning can be safely ignored.
*/
    socket_address.sin_port = (uint16_t) (htons ((uint16_t) (VALUE (&port_number))));
    if (socket_address.sin_port == 0) {
      PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    }
  }
  host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  COPY (&socket_address.sin_addr, host_address->h_addr, host_address->h_length);
  protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  socket_id = socket (PF_INET, SOCK_STREAM, protocol->p_proto);
  if (socket_id < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  conn = connect (socket_id, (const struct sockaddr *) &socket_address, (socklen_t) ALIGNED_SIZE_OF (socket_address));
  if (conn < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Read from host */
  WRITE (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_PRIMITIVE (p, errno, A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Initialise file descriptor set */
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
/* Initialise the a68g_timeout data structure */
  a68g_timeout.tv_sec = TIMEOUT_INTERVAL;
  a68g_timeout.tv_usec = 0;
/* Block until server replies or a68g_timeout blows up */
  switch (select (FD_SETSIZE, &set, NULL, NULL, &a68g_timeout)) {
  case 0:
    {
      errno = ETIMEDOUT;
      PUSH_PRIMITIVE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case -1:
    {
      PUSH_PRIMITIVE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case 1:
    {
      break;
    }
  default:
    {
      ABEND (A68_TRUE, "unexpected result from select", NULL);
    }
  }
  while ((k = (int) io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    buffer[k] = NULL_CHAR;
    add_string_transput_buffer (p, CONTENT_BUFFER, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
/* Convert string */
  *(A68_REF *) ADDRESS (&content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER));
  ASSERT (close (socket_id) == 0);
  PUSH_PRIMITIVE (p, errno, A68_INT);
}

#endif /* HAVE_HTTP */

#if defined HAVE_REGEX_H
/*!
\brief return code for regex interface
\param rc return code from regex routine
\return 0: match, 1: no match, 2: no core, 3: other error
**/

void push_grep_rc (NODE_T * p, int rc)
{
  switch (rc) {
  case 0:
    {
      PUSH_PRIMITIVE (p, 0, A68_INT);
      return;
    }
  case REG_NOMATCH:
    {
      PUSH_PRIMITIVE (p, 1, A68_INT);
      return;
    }
  case REG_ESPACE:
    {
      PUSH_PRIMITIVE (p, 3, A68_INT);
      return;
    }
  default:
    {
      PUSH_PRIMITIVE (p, 2, A68_INT);
      return;
    }
  }
}

/*!
\brief PROC grep in string = (STRING, STRING, REF INT, REF INT) INT
\param p position in tree
\return 0: match, 1: no match, 2: no core, 3: other error
**/

void genie_grep_in_string (NODE_T * p)
{
  A68_REF ref_pat, ref_beg, ref_end, ref_str, row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int rc, nmatch, k, max_k, widest;
  regex_t compiled;
  regmatch_t *matches;
  POP_REF (p, &ref_end);
  POP_REF (p, &ref_beg);
  POP_REF (p, &ref_str);
  POP_REF (p, &ref_pat);
  row = *(A68_REF *) & ref_str;
  CHECK_INIT (p, INITIALISED (&row), MODE (ROWS));
  GET_DESCRIPTOR (arr, tup, &row);
  reset_transput_buffer (PATTERN_BUFFER);
  reset_transput_buffer (STRING_BUFFER);
  add_a_string_transput_buffer (p, PATTERN_BUFFER, (BYTE_T *) & ref_pat);
  add_a_string_transput_buffer (p, STRING_BUFFER, (BYTE_T *) & ref_str);
  rc = regcomp (&compiled, get_transput_buffer (PATTERN_BUFFER), REG_NEWLINE | REG_EXTENDED);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
  nmatch = (int) (compiled.re_nsub);
  if (nmatch == 0) {
    nmatch = 1;
  }
  matches = malloc ((size_t) (nmatch * ALIGNED_SIZE_OF (regmatch_t)));
  if (nmatch > 0 && matches == NULL) {
    rc = 2;
    PUSH_PRIMITIVE (p, rc, A68_INT);
    regfree (&compiled);
    return;
  }
  rc = regexec (&compiled, get_transput_buffer (STRING_BUFFER), (size_t) nmatch, matches, 0);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
/* Find widest match. Do not assume it is the first one */
  widest = 0;
  max_k = 0;
  for (k = 0; k < nmatch; k++) {
    int dif = (int) (matches[k].rm_eo) - (int) (matches[k].rm_so);
    if (dif > widest) {
      widest = dif;
      max_k = k;
    }
  }
  if (!IS_NIL (ref_beg)) {
    A68_INT *i = (A68_INT *) ADDRESS (&ref_beg);
    STATUS (i) = INITIALISED_MASK;
    VALUE (i) = (int) (matches[max_k].rm_so) + (int) (tup->lower_bound);
  }
  if (!IS_NIL (ref_end)) {
    A68_INT *i = (A68_INT *) ADDRESS (&ref_end);
    STATUS (i) = INITIALISED_MASK;
    VALUE (i) = (int) (matches[max_k].rm_eo) + (int) (tup->lower_bound) - 1;
  }
  free (matches);
  push_grep_rc (p, 0);
}

/*!
\brief PROC grep in substring = (STRING, STRING, REF INT, REF INT) INT
\param p position in tree
\return 0: match, 1: no match, 2: no core, 3: other error
**/

void genie_grep_in_substring (NODE_T * p)
{
  A68_REF ref_pat, ref_beg, ref_end, ref_str, row;
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  int rc, nmatch, k, max_k, widest;
  regex_t compiled;
  regmatch_t *matches;
  POP_REF (p, &ref_end);
  POP_REF (p, &ref_beg);
  POP_REF (p, &ref_str);
  POP_REF (p, &ref_pat);
  row = *(A68_REF *) & ref_str;
  CHECK_INIT (p, INITIALISED (&row), MODE (ROWS));
  GET_DESCRIPTOR (arr, tup, &row);
  reset_transput_buffer (PATTERN_BUFFER);
  reset_transput_buffer (STRING_BUFFER);
  add_a_string_transput_buffer (p, PATTERN_BUFFER, (BYTE_T *) & ref_pat);
  add_a_string_transput_buffer (p, STRING_BUFFER, (BYTE_T *) & ref_str);
  rc = regcomp (&compiled, get_transput_buffer (PATTERN_BUFFER), REG_NEWLINE | REG_EXTENDED);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
  nmatch = (int) (compiled.re_nsub);
  if (nmatch == 0) {
    nmatch = 1;
  }
  matches = malloc ((size_t) (nmatch * ALIGNED_SIZE_OF (regmatch_t)));
  if (nmatch > 0 && matches == NULL) {
    rc = 2;
    PUSH_PRIMITIVE (p, rc, A68_INT);
    regfree (&compiled);
    return;
  }
  rc = regexec (&compiled, get_transput_buffer (STRING_BUFFER), (size_t) nmatch, matches, REG_NOTBOL);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
/* Find widest match. Do not assume it is the first one */
  widest = 0;
  max_k = 0;
  for (k = 0; k < nmatch; k++) {
    int dif = (int) (matches[k].rm_eo) - (int) (matches[k].rm_so);
    if (dif > widest) {
      widest = dif;
      max_k = k;
    }
  }
  if (!IS_NIL (ref_beg)) {
    A68_INT *i = (A68_INT *) ADDRESS (&ref_beg);
    STATUS (i) = INITIALISED_MASK;
    VALUE (i) = (int) (matches[max_k].rm_so) + (int) (tup->lower_bound);
  }
  if (!IS_NIL (ref_end)) {
    A68_INT *i = (A68_INT *) ADDRESS (&ref_end);
    STATUS (i) = INITIALISED_MASK;
    VALUE (i) = (int) (matches[max_k].rm_eo) + (int) (tup->lower_bound) - 1;
  }
  free (matches);
  push_grep_rc (p, 0);
}

/*!
\brief PROC sub in string = (STRING, STRING, REF STRING) INT
\param p position in tree
\return 0: match, 1: no match, 2: no core, 3: other error
**/

void genie_sub_in_string (NODE_T * p)
{
  A68_REF ref_pat, ref_rep, ref_str;
  int rc, nmatch, k, max_k, widest, begin, end;
  char *txt;
  regex_t compiled;
  regmatch_t *matches;
  POP_REF (p, &ref_str);
  POP_REF (p, &ref_rep);
  POP_REF (p, &ref_pat);
  if (IS_NIL (ref_str)) {
    PUSH_PRIMITIVE (p, 3, A68_INT);
    return;
  }
  reset_transput_buffer (STRING_BUFFER);
  reset_transput_buffer (REPLACE_BUFFER);
  reset_transput_buffer (PATTERN_BUFFER);
  add_a_string_transput_buffer (p, PATTERN_BUFFER, (BYTE_T *) & ref_pat);
  add_a_string_transput_buffer (p, STRING_BUFFER, (BYTE_T *) (A68_REF *) ADDRESS (&ref_str));
  rc = regcomp (&compiled, get_transput_buffer (PATTERN_BUFFER), REG_NEWLINE | REG_EXTENDED);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
  nmatch = (int) (compiled.re_nsub);
  if (nmatch == 0) {
    nmatch = 1;
  }
  matches = malloc ((size_t) (nmatch * ALIGNED_SIZE_OF (regmatch_t)));
  if (nmatch > 0 && matches == NULL) {
    PUSH_PRIMITIVE (p, rc, A68_INT);
    regfree (&compiled);
    return;
  }
  rc = regexec (&compiled, get_transput_buffer (STRING_BUFFER), (size_t) nmatch, matches, 0);
  if (rc != 0) {
    push_grep_rc (p, rc);
    regfree (&compiled);
    return;
  }
/* Find widest match. Do not assume it is the first one */
  widest = 0;
  max_k = 0;
  for (k = 0; k < nmatch; k++) {
    int dif = (int) matches[k].rm_eo - (int) matches[k].rm_so;
    if (dif > widest) {
      widest = dif;
      max_k = k;
    }
  }
  begin = (int) matches[max_k].rm_so + 1;
  end = (int) matches[max_k].rm_eo;
/* Substitute text */
  txt = get_transput_buffer (STRING_BUFFER);
  for (k = 0; k < begin - 1; k++) {
    add_char_transput_buffer (p, REPLACE_BUFFER, txt[k]);
  }
  add_a_string_transput_buffer (p, REPLACE_BUFFER, (BYTE_T *) & ref_rep);
  for (k = end; k < get_transput_buffer_size (STRING_BUFFER); k++) {
    add_char_transput_buffer (p, REPLACE_BUFFER, txt[k]);
  }
  *(A68_REF *) ADDRESS (&ref_str) = c_to_a_string (p, get_transput_buffer (REPLACE_BUFFER));
  free (matches);
  push_grep_rc (p, 0);
}
#endif /* HAVE_REGEX_H */
