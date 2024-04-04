/*!
\file io.c
\brief low-level UNIX IO routines
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


#include "algol68g.h"
#include "genie.h"

BOOL_T halt_typing, sys_request_flag;
static int chars_in_tty_line;

char output_line[BUFFER_SIZE], edit_line[BUFFER_SIZE], input_line[BUFFER_SIZE];

/*!
\brief initialise output to STDOUT
**/

void init_tty (void)
{
  chars_in_tty_line = 0;
  halt_typing = A_FALSE;
  sys_request_flag = A_FALSE;
}

/*!
\brief terminate current line on STDOUT
**/

void io_close_tty_line (void)
{
  if (chars_in_tty_line > 0) {
    io_write_string (STDOUT_FILENO, "\n");
  }
}

/*!
\brief get a char from STDIN
\return
**/

char get_stdin_char (void)
{
  ssize_t j;
  char ch;
  RESET_ERRNO;
  j = io_read_conv (STDIN_FILENO, &ch, 1);
  ABNORMAL_END (errno != 0, "cannot read char from stdin", NULL);
  return (j == 1 ? ch : EOF);
}

/*!
\brief read string from STDIN, until "\n"
\param prompt prompt string
\return
**/

char *read_string_from_tty (char *prompt)
{
  int ch, k = 0;
  io_write_string (STDOUT_FILENO, "\n");
  io_write_string (STDOUT_FILENO, prompt);
  io_write_string (STDOUT_FILENO, ">");
  while (IS_CNTRL (ch = get_stdin_char ()) && ch != NEWLINE_CHAR) {;
  }
  while (ch != NEWLINE_CHAR && ch != EOF && k < BUFFER_SIZE - 1) {
    input_line[k++] = ch;
    ch = get_stdin_char ();
  }
  if (ch == NEWLINE_CHAR) {
    chars_in_tty_line++;
  }
  input_line[k] = '\0';
  return (input_line);
}

/*!
\brief read string from file including "\n"
\param f
\param z
\param max
\return
**/

size_t io_read_string (FILE_T f, char *z, size_t max)
{
  int j = 1, k = 0;
  char ch = '\0';
#ifdef PRE_MACOS_X_VERSION
  char nl = CR_CHAR;
#else
  char nl = NEWLINE_CHAR;
#endif
  ABNORMAL_END (max < 2, "no buffer", NULL);
  while ((j == 1) && (ch != nl) && (k < (int) (max - 1))) {
    RESET_ERRNO;
    j = io_read_conv (f, &ch, 1);
    ABNORMAL_END (errno != 0, "cannot read string", NULL);
    if (j == 1) {
      z[k++] = ch;
    }
  }
  z[k] = '\0';
  return (k);
}

/*!
\brief write string to file
\param f
\param z
**/

void io_write_string (FILE_T f, const char *z)
{
  RESET_ERRNO;
  if (f != STDOUT_FILENO) {
/* Writing to file. */
    io_write_conv (f, z, strlen (z));
    ABNORMAL_END (errno != 0, "cannot write", NULL);
  } else {
/* Writing to TTY. */
    int first, k;
/* Write parts until end-of-string. */
    first = 0;
    do {
      k = first;
/* How far can we get? */
      while (z[k] != '\0' && z[k] != NEWLINE_CHAR) {
	k++;
      }
      if (k > first) {
/* Write these characters. */
	io_write_conv (STDOUT_FILENO, &(z[first]), k - first);
	ABNORMAL_END (errno != 0, "cannot write", NULL);
	chars_in_tty_line += (k - first);
      }
      if (z[k] == NEWLINE_CHAR) {
/* Pretty-print newline. */
	k++;
	first = k;
	io_write_conv (STDOUT_FILENO, "\n", 1);
	ABNORMAL_END (errno != 0, "cannot write", NULL);
	chars_in_tty_line = 0;
      }
    }
    while (z[k] != '\0');
  }
}

/*!
\brief read bytes from file into buffer and convert "\r" into "\n"
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
#ifdef WIN32_VERSION
    int bytes_read;
#else
    ssize_t bytes_read;
#endif
    RESET_ERRNO;
    bytes_read = read (fd, z, to_do);
    if (bytes_read < 0) {
#ifndef PRE_MACOS_X_VERSION
      if (errno == EINTR) {
/* interrupt, retry. */
	bytes_read = 0;
	if (restarts++ > 3) {
	  return (-1);
	}
      } else {
/* read error. */
	return (-1);
      }
#else
      return (-1);
#endif
    } else if (bytes_read == 0) {
      break;			/* EOF. */
    }
    to_do -= bytes_read;
    z += bytes_read;
  }
  return (n - to_do);		/* return >= 0 */
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
#ifndef PRE_MACOS_X_VERSION
      if (errno == EINTR) {
/* interrupt, retry. */
	bytes_written = 0;
	if (restarts++ > 3) {
	  return (-1);
	}
      } else {
/* write error. */
	return (-1);
      }
#else
      return (-1);
#endif
    }
    to_do -= bytes_written;
    z += bytes_written;
  }
  return (n);
}

/*!
\brief read bytes from file into buffer and convert "\r" into "\n"
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
#ifdef WIN32_VERSION
    int bytes_read;
#else
    ssize_t bytes_read;
#endif
    RESET_ERRNO;
    bytes_read = read (fd, z, to_do);
    if (bytes_read < 0) {
#ifndef PRE_MACOS_X_VERSION
      if (errno == EINTR) {
/* interrupt, retry. */
	bytes_read = 0;
	if (restarts++ > 3) {
	  return (-1);
	}
      } else {
/* read error. */
	return (-1);
      }
#else
      return (-1);
#endif
    } else if (bytes_read == 0) {
      break;			/* EOF. */
    }
    to_do -= bytes_read;
    z += bytes_read;
  }
#ifdef PRE_MACOS_X_VERSION
  {
/* Translate CR_CHAR into NEWLINE_CHAR. */
    unsigned k, lim = n - to_do;
    z = buf;
    for (k = 0; k < lim; k++) {
      if (z[k] == CR_CHAR) {
	z[k] = NEWLINE_CHAR;
      }
    }
  }
#endif
  return (n - to_do);		/* return >= 0 */
}

/*!
\brief writes n bytes from buffer to file and converts "\r" into "\n"
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
#ifdef PRE_MACOS_X_VERSION
  {
/* Translate NEWLINE_CHAR into CR_CHAR. */
    unsigned k;
    for (k = 0; k < n; k++) {
      if (z[k] == NEWLINE_CHAR) {
	z[k] = CR_CHAR;
      }
    }
  }
#endif
  while (to_do > 0) {
    ssize_t bytes_written;
    RESET_ERRNO;
    bytes_written = write (fd, z, to_do);
    if (bytes_written <= 0) {
#ifndef PRE_MACOS_X_VERSION
      if (errno == EINTR) {
/* interrupt, retry. */
	bytes_written = 0;
	if (restarts++ > 3) {
	  return (-1);
	}
      } else {
/* write error. */
	return (-1);
      }
#else
      return (-1);
#endif
    }
    to_do -= bytes_written;
    z += bytes_written;
  }
  return (n);
}
