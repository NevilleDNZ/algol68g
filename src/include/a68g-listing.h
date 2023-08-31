//! @file a68g-listing.h
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
//! Definitions for making listing files.

#if ! defined (__A68G_LISTING_H__)
#define __A68G_LISTING_H__

void list_source_line (FILE_T, LINE_T *, BOOL_T);
void print_mode_flat (FILE_T, MOID_T *);
void tree_listing (FILE_T, NODE_T *, int, LINE_T *, int *, BOOL_T);
void write_listing_header (void);
void write_listing (void);
void write_object_listing (void);
void write_source_listing (void);
void write_tree_listing (void);

#endif
