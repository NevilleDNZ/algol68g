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
This file defines system dependencies that cannot be detected (reliably) in an 
automatic way. These dependencies are

   DO_SYSTEM_STACK_CHECK
   HAVE_MODIFIABLE_X_TITLE
   HAVE_UNIX_CLOCK
   WIN32_VERSION
   OS2_VERSION
   PRE_MACOS_X
 
Define or undefine the directives below depending on your system. 
Refer to the file INSTALL.                                                    */

#ifndef A68G_CONFIG_H
#define A68G_CONFIG_H

/*-------1---------2---------3---------4---------5---------6---------7---------+
If you want the interpreter to check the system stack then define 
DO_SYSTEM_STACK_CHECK. This check assumes that the stack grows linearly (either 
upwards or downwards) and that the difference between (1) the address of a local 
variable in the current stack frame and (2) the address of a local variable in 
a stack frame at the start of execution of the Algol 68 program, is a measure 
of system stack size.                                                         */

#define DO_SYSTEM_STACK_CHECK

/*-------1---------2---------3---------4---------5---------6---------7---------+
Did you edit GNU libplot so you can modify X Window titles? If yes then define 
HAVE_MODIFIABLE_X_TITLE else undefine it.                                     */

#define HAVE_MODIFIABLE_X_TITLE

/*-------1---------2---------3---------4---------5---------6---------7---------+
HAVE_UNIX_CLOCK attempts getting usec resolution to the clock on UNIX systems.
Undefining it means that clock () will be called, which is portable.          */

#define HAVE_UNIX_CLOCK

/*-------1---------2---------3---------4---------5---------6---------7---------+
Theo Vosse made a Win32 executable of Algol68G. To select his modifications, 
define WIN32_VERSION. When defining WIN32_VERSION you may want to change
directives following the definition below to reflect your particular system. 
HAVE_IEEE_754 is ok for Pentiums.                                             */

#undef WIN32_VERSION

#ifdef WIN32_VERSION
#define HAVE_CURSES 1
#define HAVE_PLOTUTILS 1
#undef HAVE_MODIFIABLE_X_TITLE
#define HAVE_IEEE_754
#define HAVE_UNIX
#undef HAVE_UNIX_CLOCK
#endif

/*-------1---------2---------3---------4---------5---------6---------7---------+
Jim Heifetz ported Algol68G to OS/2. 
To select his modifications, define OS2_VERSION.                              */

#undef OS2_VERSION

/*-------1---------2---------3---------4---------5---------6---------7---------+
What was to become Algol68G started as an Algol 68 subset implementation for 
desktop computers at the time (XT and Mac SE).

As to have a test on non-UNIX platforms, versions up to Mark 3.2 were tested on 
a Performa 630 (68LC040, System 7.5.3) and Mark 4 was still tested on a PowerMac 
(G3, MacOS 8.0).

To compile on pre-MacOS X Macs, define PRE_MACOS_X. When defining PRE_MACOS_X 
you may want to change MAC_STACK_SIZE to reflect your particular system. 
HAVE_IEEE_754 is ok.                                                          */

#undef PRE_MACOS_X

#ifdef PRE_MACOS_X
#define MAC_STACK_SIZE 128L
#undef HAVE_CURSES
#undef HAVE_GSL
#undef HAVE_PLOTUTILS
#undef HAVE_MODIFIABLE_X_TITLE
#undef HAVE_UNIX_CLOCK
#define HAVE_IEEE_754
#endif

#endif
