//! @file a68g.h
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].

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
//! Machine parameters.

#if ! defined (__A68G_MACH_H__)
#define __A68G_MACH_H__

#if (A68_LEVEL >= 3)
#define A68_MAX_INT (a68g_i64mach (9))
#else
#define A68_MAX_INT (a68g_i32mach (9))
#endif

#define A68_STDIN  (a68g_i32mach (1))
#define A68_STDOUT (a68g_i32mach (2))
#define A68_STDPUN (a68g_i32mach (3))
#define A68_STDERR (a68g_i32mach (4))

#define A68_REAL_MIN (a68g_r64mach (1))
#define A68_REAL_MAX (a68g_r64mach (2))
#define A68_REAL_EPS (a68g_r64mach (4))
#define A68_REAL_MIN_EXP ((int) a68g_r64mach (6))
#define A68_REAL_MAX_EXP ((int) a68g_r64mach (7))
#define A68_REAL_DIG ((int) a68g_r64mach (8))
#define A68_REAL_MAN ((int) a68g_r64mach (9))

#define A68_DOUBLE_MIN (a68g_r128mach (1))
#define A68_DOUBLE_MAX (a68g_r128mach (2))
#define A68_DOUBLE_EPS (a68g_r128mach (4))
#define A68_DOUBLE_MIN_EXP ((int) a68g_r128mach (6))
#define A68_DOUBLE_MAX_EXP ((int) a68g_r128mach (7))
#define A68_DOUBLE_DIG ((int) a68g_r128mach (8))
#define A68_DOUBLE_MAN ((int) a68g_r128mach (9))

int a68g_i32mach (int);
GPROC genie_i32mach;

REAL_T a68g_r64mach (int);
GPROC genie_r64mach;

#if (A68_LEVEL >= 3)

INT_T a68g_i64mach (int);
GPROC genie_i64mach;

DOUBLE_T a68g_r128mach (int);
GPROC genie_r128mach;

#endif

#endif
