/*!
\file config.h
\brief contains manual configuration switches
**/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2006 J. Marcel van der Veer <algol68g@xs4all.nl>.

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
This file defines system dependencies that cannot be detected (reliably) in an
automatic way. These dependencies are

   HAVE_SYSTEM_STACK_CHECK
   HAVE_MODIFIABLE_X_TITLE
   HAVE_UNIX_CLOCK
   WIN32_VERSION
   OS2_VERSION
   PRE_MACOS_X_VERSION

Define or undefine the directives below depending on your system.
You can also set them when using `make' by using the CFLAGS parameter:

   make CFLAGS=-DHAVE_SYSTEM_STACK_CHECK

Refer to the file INSTALL.
*/

#ifndef A68G_CONFIG_H
#define A68G_CONFIG_H

/*
If you want the interpreter to check the system stack then define
HAVE_SYSTEM_STACK_CHECK. This check assumes that the stack grows linearly (either
upwards or downwards) and that the difference between (1) the address of a local
variable in the current stack frame and (2) the address of a local variable in
a stack frame at the start of execution of the Algol 68 program, is a measure
of system stack size.
*/

#ifndef HAVE_SYSTEM_STACK_CHECK
#define HAVE_SYSTEM_STACK_CHECK
#endif

/*
Did you edit GNU libplot so you can modify X Window titles? If yes then define
HAVE_MODIFIABLE_X_TITLE else undefine it.
*/

#ifndef HAVE_MODIFIABLE_X_TITLE
#undef HAVE_MODIFIABLE_X_TITLE
#endif

/*
HAVE_UNIX_CLOCK attempts getting usec resolution to the clock on UNIX systems.
This calls get_rusage, which does not work on some systems.
Undefining it means that clock () will be called, which is portable.
*/

#undef HAVE_UNIX_CLOCK

/*
Theo Vosse made a Win32 executable of Algol68G. To select his modifications,
define WIN32_VERSION. When defining WIN32_VERSION you may want to change
directives following the definition below to reflect your particular system.
HAVE_IEEE_754 is ok for Pentiums.
*/

#ifndef WIN32_VERSION
#undef WIN32_VERSION
#endif

#ifdef WIN32_VERSION
#define HAVE_CURSES 1
#define HAVE_PLOTUTILS 1
#define HAVE_UNIX 1
#define HAVE_IEEE_754 1
#undef HAVE_MODIFIABLE_X_TITLE
#undef HAVE_UNIX_CLOCK
#undef HAVE_POSIX_THREADS
#undef HAVE_HTTP
#undef HAVE_REGEX
#undef HAVE_POSTGRESQL
#endif

/*
Jim Heifetz ported Algol68G to OS/2.
To select his modifications, define OS2_VERSION.			      i
*/

#ifndef OS2_VERSION
#undef OS2_VERSION
#endif

#ifdef OS2_VERSION
#define HAVE_IEEE_754
#undef HAVE_CURSES
#undef HAVE_GSL
#undef HAVE_PLOTUTILS
#undef HAVE_MODIFIABLE_X_TITLE
#undef HAVE_UNIX_CLOCK
#undef HAVE_POSIX_THREADS
#undef HAVE_HTTP
#undef HAVE_REGEX
#undef HAVE_POSTGRESQL
#endif

/*
What was to become Algol68G started as an Algol 68 subset implementation for
desktop computers at the time (PC XT and Mac SE). A first version actually ran
on a Mac SE (8 MHz 68000, 4 MB RAM). Mark 9 still compiles and runs on a 
Performa 630 (68LC040).

To compile on pre-MacOS X Macs, define PRE_MACOS_X_VERSION. When defining
PRE_MACOS_X_VERSION you may want to change MAC_STACK_SIZE to reflect your
particular system. HAVE_IEEE_754 is ok.
*/

#ifndef PRE_MACOS_X_VERSION
#undef PRE_MACOS_X_VERSION
#endif

#ifdef PRE_MACOS_X_VERSION
#define MAC_STACK_SIZE 1024L
#define HAVE_IEEE_754
#undef HAVE_CURSES
#undef HAVE_GSL
#undef HAVE_PLOTUTILS
#undef HAVE_MODIFIABLE_X_TITLE
#undef HAVE_UNIX_CLOCK
#undef HAVE_POSIX_THREADS
#undef HAVE_HTTP
#undef HAVE_REGEX
#undef HAVE_POSTGRESQL
#endif

#endif
