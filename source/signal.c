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
sigint_handler: raise SYSREQUEST so you get to a monitor.                     */

static void
sigint_handler (int i)
{
  (void) i;
  abend (signal (SIGINT, sigint_handler) == SIG_ERR, "cannot install SIGINT handler", NULL);
  if (!sys_request_flag && !in_monitor)
    {
      sys_request_flag = A_TRUE;
#ifdef HAVE_CURSES
      genie_curses_end (NULL);
#endif
    }
  else
    {
      a68g_exit (EXIT_SUCCESS);	/* or FAILURE :-) */
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
install_signal_handlers.                                                      */

void
install_signal_handlers (void)
{
  abend (signal (SIGINT, sigint_handler) == SIG_ERR, "cannot install SIGINT handler", NULL);
}
