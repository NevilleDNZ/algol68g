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

BOOL_T halt_typing, sys_request_flag;
int chars_in_tty_line;

char output_line[CMS_LINE_SIZE], edit_line[CMS_LINE_SIZE], input_line[CMS_LINE_SIZE];

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_tty: initialise output to STDOUT.                                        */

void
init_tty (void)
{
  chars_in_tty_line = 0;
  halt_typing = A_FALSE;
  sys_request_flag = A_FALSE;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
io_close_tty_line: terminate current line on STDOUT.                          */

void
io_close_tty_line (void)
{
  if (chars_in_tty_line > 0)
    {
      io_write_string (STDOUT_FILENO, "\n");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
get_stdin_char: get a char from STDIN.                                        */

char
get_stdin_char (void)
{
  ssize_t j;
  char ch;
  j = io_read (STDIN_FILENO, &ch, 1);
  return (j == 1 ? ch : EOF);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_string_from_tty: read string from STDIN, until '\n'.                     */

char *
read_string_from_tty (char *prompt)
{
  int ch, k = 0;
  io_write_string (STDOUT_FILENO, "\n");
  io_write_string (STDOUT_FILENO, prompt);
  io_write_string (STDOUT_FILENO, ">");
  while (IS_CNTRL (ch = get_stdin_char ()))
    {;
    }
  while (ch != '\n' && ch != EOF && k < CMS_LINE_SIZE - 1)
    {
      input_line[k++] = ch;
      ch = get_stdin_char ();
    }
  if (ch == '\n')
    {
      chars_in_tty_line++;
    }
  input_line[k] = '\0';
  return (input_line);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
io_read_string: read z from f including '\n', max characters at most.         */

size_t
io_read_string (FILE_T f, char *z, size_t max)
{
  int j = 1, k = 0;
  char ch = '\0';
#ifdef PRE_MACOS_X
  char nl = '\r';
#else
  char nl = '\n';
#endif
  abend (max < 2, "no buffer", NULL);
  while ((j == 1) && (ch != nl) && (k < (int) (max - 1)))
    {
      errno = 0;
      j = io_read (f, &ch, 1);
      abend (errno != 0, "cannot read string", NULL);
      if (j == 1)
	{
	  z[k++] = ch;
	}
    }
  z[k] = '\0';
  return (k);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
io_write_string: write z to f.                                                */

void
io_write_string (FILE_T f, const char *z)
{
  errno = 0;
  if (f != STDOUT_FILENO)
    {
/* Writing to file */
      errno = 0;
      io_write (f, z, strlen (z));
      abend (errno != 0, "cannot write", NULL);
    }
  else
    {
/* Writing to TTY */
      int first, k;
/* Write parts until end-of-string */
      first = 0;
      do
	{
	  k = first;
/* How far can we get? */
	  while (z[k] != '\0' && z[k] != '\n')
	    {
	      k++;
	    }
	  if (k > first)
	    {
/* Write these characters */
	      io_write (STDOUT_FILENO, &(z[first]), k - first);
	      abend (errno != 0, "cannot write", NULL);
	      chars_in_tty_line += (k - first);
	    }
	  if (z[k] == '\n')
	    {
/* Pretty-print newline */
	      k++;
	      first = k;
	      io_write (STDOUT_FILENO, "\n", 1);
	      abend (errno != 0, "cannot write", NULL);
	      chars_in_tty_line = 0;
	    }
	}
      while (z[k] != '\0');
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
io_read: read n bytes to buf.                                                 */

ssize_t
io_read (FILE_T fd, void *buf, size_t n)
{
  size_t to_do = n;
  char *z = (char *) buf;
  while (to_do > 0)
    {
#ifdef WIN32_VERSION
      int bytes_read = read (fd, z, to_do);
#else
      ssize_t bytes_read = read (fd, z, to_do);
#endif
      if (bytes_read < 0)
	{
#ifndef PRE_MACOS_X
	  if (errno == EINTR)
	    {
/* interrupt, retry */
	      bytes_read = 0;
	    }
	  else
	    {
/* read error */
	      return (-1);
	    }
#else
	  return (-1);
#endif
	}
      else if (bytes_read == 0)
	{
	  break;		/* EOF */
	}
      to_do -= bytes_read;
      z += bytes_read;
    }
#ifdef PRE_MACOS_X
  {
/* Translate '\r' into '\n' */
    unsigned k, lim = n - to_do;
    z = buf;
    for (k = 0; k < lim; k++)
      {
	if (z[k] == '\r')
	  {
	    z[k] = '\n';
	  }
      }
  }
#endif
  return (n - to_do);		/* return >= 0 */
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
io_write: write n bytes from buf (unbuffered).                                */

ssize_t
io_write (FILE_T fd, const void *buf, size_t n)
{
  size_t to_do = n;
  char *z = (char *) buf;
#ifdef PRE_MACOS_X
  {
/* Translate '\n' into '\r' */
    unsigned k;
    for (k = 0; k < n; k++)
      {
	if (z[k] == '\n')
	  {
	    z[k] = '\r';
	  }
      }
  }
#endif
  while (to_do > 0)
    {
      ssize_t bytes_written = write (fd, z, to_do);
      if (bytes_written <= 0)
	{
#ifndef PRE_MACOS_X
	  if (errno == EINTR)
	    {
/* interrupt, retry */
	      bytes_written = 0;
	    }
	  else
	    {
/* write error */
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
