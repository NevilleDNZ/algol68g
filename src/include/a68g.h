//! @file a68g.h
//! @author J. Marcel van der Veer
//!
//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].
//!
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
//! Master include file.

#if ! defined (__A68G_H__)
#define __A68G_H__

// Debugging switch, only useful during development.
#undef A68_DEBUG

// Early typedef
#define unt unsigned

// Configuration

#include "a68g-platform.h"
#include "a68g-includes.h"

// Build switches depending on platform.

#if ((defined (BUILD_LINUX) || defined (BUILD_BSD)) && defined (HAVE_DL))
#  define BUILD_A68_COMPILER
#else
#  undef BUILD_A68_COMPILER
#endif

#if defined (BUILD_LINUX)
#  define BUILD_UNIX
#elif defined (BUILD_BSD)
#  define BUILD_UNIX
#elif defined (BUILD_CYGWIN)
#  define BUILD_UNIX
#elif defined (BUILD_HAIKU)
#  define BUILD_UNIX
#endif

// REAL_T should be a REAL*8 for external libs.
typedef double REAL_T; 

// Compatibility.

#if ! defined (O_BINARY)
#  define O_BINARY 0x0000
#endif

// Forward type definitions.

typedef struct NODE_T NODE_T;
typedef unt STATUS_MASK_T, BOOL_T;

// Decide the internal representation of A68 modes.

#include "a68g-stddef.h"

#define ALIGNED __attribute__((aligned (sizeof (A68_ALIGN_T))))
#define A68_ALIGN(s) ((int) ((s) % A68_ALIGNMENT) == 0 ? (s) : ((s) - (s) % A68_ALIGNMENT + A68_ALIGNMENT))
#define A68_ALIGNMENT ((int) (sizeof (A68_ALIGN_T)))
#define SIZE_ALIGNED(p) ((int) A68_ALIGN (sizeof (p)))

#if (A68_LEVEL >= 3)
#  include "a68g-level-3.h"
#else // Vintage Algol 68 Genie (versions 1 and 2).
#  include "a68g-generic.h"
#endif

#define MP_REAL_RADIX ((MP_REAL_T) MP_RADIX)

#if defined (BUILD_WIN32)
typedef unt __off_t;
#  if defined (__MSVCRT__) && defined (_environ)
#    undef _environ
#  endif
#endif

#include "a68g-defines.h"
#include "a68g-stack.h"
#include "a68g-masks.h"
#include "a68g-enums.h"
#include "a68g-types.h"
#include "a68g-nil.h"
#include "a68g-diagnostics.h"
#include "a68g-common.h"
#include "a68g-lib.h"
#include "a68g-mach.h"

// Global declarations

BOOL_T a68_mkstemp (char *, int, mode_t);
BYTE_T *get_fixed_heap_space (size_t);
BYTE_T *get_heap_space (size_t);
BYTE_T *get_temp_heap_space (size_t);
char *a68_basename (char *);
char *a68_dirname (char *);
char *a68_relpath (char *, char *, char *);
char *ctrl_char (int);
char *moid_to_string (MOID_T *, int, NODE_T *);
char *new_fixed_string (char *);
char *new_string (char *, ...);
char *new_temp_string (char *);
char *non_terminal_string (char *, int);
char *read_string_from_tty (char *);
char *standard_environ_proc_name (GPROC);
int get_row_size (A68_TUPLE *, int);
int moid_digits (MOID_T *);
int moid_size (MOID_T *);
int (snprintf) (char *, size_t, const char *, ...); // Prevent macro substitution on Darwin.
void *a68_alloc (size_t, const char *, int);
void a68_exit (int);
void a68_free (void *);
void a68_getty (int *, int *);
void a68_rm (char *);
void abend (char *, char *, char *, int);
void announce_phase (char *);
void apropos (FILE_T, char *, char *);
void bufcat (char *, char *, int);
void bufcpy (char *, char *, int);
void default_mem_sizes (int);
void discard_heap (void);
void free_file_entries (void);
void free_syntax_tree (NODE_T *);
void get_stack_size (void);
void indenter (MODULE_T *);
void init_curses (void);
void init_file_entries (void);
void init_file_entry (int);
void init_heap (void);
void init_rng (unt);
void init_tty (void);
void install_signal_handlers (void);
void online_help (FILE_T);
void state_version (FILE_T);

// Below from R mathlib

void GetRNGstate (void);
void PutRNGstate (void);
REAL_T a68_unif_rand (void);
REAL_T a68_gauss_rand (void);
REAL_T R_unif_index (REAL_T);

#endif
