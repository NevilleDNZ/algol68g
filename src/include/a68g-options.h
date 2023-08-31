//! @file a68g-options.h
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
//! Definitions for option processing.

#if !defined (__A68G_OPTIONS_H__)
#define __A68G_OPTIONS_H__ 

BOOL_T set_options (OPTION_LIST_T *, BOOL_T);
char *optimisation_option (void);
void add_option_list (OPTION_LIST_T **, char *, LINE_T *);
void free_option_list (OPTION_LIST_T *);
void default_options (MODULE_T *);
void init_options (void);
void isolate_options (char *, LINE_T *);
void read_env_options (void);
void read_rc_options (void);

#endif
