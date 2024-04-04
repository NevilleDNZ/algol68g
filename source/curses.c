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

/*-------1---------2---------3---------4---------5---------6---------7---------+
This file contains some routines that interface Algol68G and the curses library. 
It is still experimental, and not documented. Be sure to know what you are doing 
when you use this, but on the other hand, "reset" will always restore your 
terminal.                                                                     */

#ifdef WIN32_VERSION
#define PATTERN A68_PATTERN
#endif

#include "algol68g.h"
#include "genie.h"

#ifdef HAVE_CURSES

#include <sys/time.h>
#include <curses.h>

#ifdef WIN32_VERSION
#undef PATTERN
#include <winsock.h>
#endif

BOOL_T curses_active = A_FALSE;

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
clean_curses ()
{
  if (curses_active == A_TRUE)
    {
      attrset (A_NORMAL);
      endwin ();
      curses_active = A_FALSE;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
init_curses ()
{
  initscr ();
  cbreak ();			/* raw () would cut off ctrl-c */
  noecho ();
  nonl ();
  curs_set (0);
  curses_active = A_TRUE;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
rgetchar: watch stdin (fd 0) to see whether it has input, don't wait too long.*/

int
rgetchar (void)
{
  int retval;
  struct timeval tv;
  fd_set rfds;
  tv.tv_sec = 0;
  tv.tv_usec = 100;
  FD_ZERO (&rfds);
  FD_SET (0, &rfds);
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (retval)
    {
      /* FD_ISSET(0, &rfds) will be true. */
      return (getch ());
    }
  else
    {
      return ('\0');
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_start (NODE_T * p)
{
  (void) p;
  init_curses ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_end (NODE_T * p)
{
  (void) p;
  clean_curses ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_clear (NODE_T * p)
{
  (void) p;
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  clear ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_refresh (NODE_T * p)
{
  (void) p;
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  refresh ();
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_lines (NODE_T * p)
{
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  PUSH_INT (p, LINES);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_columns (NODE_T * p)
{
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  PUSH_INT (p, COLS);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_getchar (NODE_T * p)
{
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  PUSH_CHAR (p, rgetchar ());
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_putchar (NODE_T * p)
{
  A68_CHAR ch;
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  POP_CHAR (p, &ch);
  addch (ch.value);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

void
genie_curses_move (NODE_T * p)
{
  A68_INT i, j;
  if (curses_active == A_FALSE)
    {
      init_curses ();
    }
  POP_INT (p, &j);
  POP_INT (p, &i);
  move (i.value, j.value);
}

#endif
