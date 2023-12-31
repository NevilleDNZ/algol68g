//! @file a68g-numbers.h
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2024 J. Marcel van der Veer [algol68g@xs4all.nl].

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
//! Math constants.

#if !defined (__A68G_NUMBERS_H__)
#define __A68G_NUMBERS_H__

// Compare to M. Abramowitz and I. Stegun, Handbook of Mathematical Functions. Dover publications, New York (1970).

#define CONST_LOG2_10       3.32192809488736234787031942948939017586483139302458061205475640 // log2(10)
#define CONST_M_LOG10_2     0.30102999566398119521373889472449302676818988146210854131042746 // log10(2)
#define CONST_180_OVER_PI   57.2957795130823208767981548141051703324054724665643215491602439 // 180 / pi
#define CONST_PI_2          1.57079632679489661923132169163975144209858469968755291048747230 // pi / 2
#define CONST_PI            3.14159265358979323846264338327950288419716939937510582097494459 // pi
#define CONST_PI_OVER_180   0.01745329251994329576923690768488612713442871888541725456097191 // pi / 180

#if (A68_LEVEL >= 3)
#define CONST_M_LOG10_2_Q   0.30102999566398119521373889472449302676818988146210854131042746q // log10(2)
#define CONST_180_OVER_PI_Q 57.2957795130823208767981548141051703324054724665643215491602439q // 180 / pi
#define CONST_PI_OVER_180_Q 0.01745329251994329576923690768488612713442871888541725456097191q // pi / 180
#define CONST_PI_Q          3.14159265358979323846264338327950288419716939937510582097494459q // pi
#endif

#endif
