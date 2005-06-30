/*!
\file extensions.c
\brief extensions to A68 except partial parametrization
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

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
This code implements some UNIX/Linux related routines.
Partly contributions by Sian Leitch <sian@sleitch.nildram.co.uk>. 
*/

#ifdef WIN32_VERSION
#define PATTERN A68_PATTERN
#endif

#include "algol68g.h"
#include "genie.h"
#include "transput.h"

#ifdef HAVE_UNIX

#ifndef WIN32_VERSION
#include <sys/wait.h>
#endif

#define VECTOR_SIZE 512
#define FD_READ 0
#define FD_WRITE 1

extern A68_REF tmp_to_a68_string (NODE_T *, char *);

extern A68_CHANNEL stand_in_channel, stand_out_channel, stand_draw_channel, stand_back_channel, stand_error_channel;

/*!
\brief PROC INT argc
\param p position in syntax tree, should not be NULL
**/

void genie_argc (NODE_T * p)
{
  RESET_ERRNO;
  PUSH_INT (p, global_argc);
}

/*!
\brief PROC (INT) STRING argv
\param p position in syntax tree, should not be NULL
**/

void genie_argv (NODE_T * p)
{
  A68_INT index;
  RESET_ERRNO;
  POP_INT (p, &index);
  if (index.value >= 1 && index.value <= global_argc) {
    PUSH_REF (p, c_to_a_string (p, global_argv[index.value - 1]));
  } else {
    PUSH_REF (p, empty_string (p));
  }
}

/*!
\brief convert [] STRING row to char *vec[]
\param p position in syntax tree, should not be NULL
\param row
**/

static void convert_string_vector (NODE_T * p, char *vec[], A68_REF row)
{
  BYTE_T *z = ADDRESS (&row);
  A68_ARRAY *arr = (A68_ARRAY *) & z[0];
  A68_TUPLE *tup = (A68_TUPLE *) & z[SIZE_OF (A68_ARRAY)];
  int k = 0;
  if (get_row_size (tup, arr->dimensions) != 0) {
    BYTE_T *base_addr = ADDRESS (&arr->array);
    BOOL_T done = A_FALSE;
    initialise_internal_index (tup, arr->dimensions);
    while (!done) {
      ADDR_T index = calculate_internal_index (tup, arr->dimensions);
      ADDR_T elem_addr = (index + arr->slice_offset) * arr->elem_size + arr->field_offset;
      BYTE_T *elem = &base_addr[elem_addr];
      int size = a68_string_size (p, *(A68_REF *) elem);
      TEST_INIT (p, *(A68_REF *) elem, MODE (STRING));
      vec[k] = (char *) get_heap_space (1 + size);
      a_to_c_string (p, vec[k], *(A68_REF *) elem);
      if (k == VECTOR_SIZE - 1) {
	diagnostic (A_RUNTIME_ERROR, p, "too many arguments");
	exit_genie (p, A_RUNTIME_ERROR);
      }
      if (strlen (vec[k]) > 0) {
	k++;
      }
      done = increment_internal_index (tup, arr->dimensions);
    }
  }
  vec[k] = NULL;
}

/*!
\brief free char *vec[]
**/

static void free_vector (char *vec[])
{
  int k = 0;
  while (vec[k] != NULL) {
    free (vec[k]);
    k++;
  }
}

/*!
\brief reset error number
\param p position in syntax tree, should not be NULL
**/

void genie_reset_errno (NODE_T * p)
{
  (void) *p;
  RESET_ERRNO;
}

/*!
\brief error number
\param p position in syntax tree, should not be NULL
**/

void genie_errno (NODE_T * p)
{
  PUSH_INT (p, errno);
}

/*!
\brief PROC strerror = (INT) STRING
\param p position in syntax tree, should not be NULL
**/

void genie_strerror (NODE_T * p)
{
  A68_INT i;
  POP_INT (p, &i);
  PUSH_REF (p, c_to_a_string (p, strerror (i.value)));
}

/*!
\brief set up file for usage in pipe
\param p position in syntax tree, should not be NULL
\param z
\param fd
\param chan
\param r_mood
\param w_mood
\param p position in syntax tree, should not be NULLid
**/

static void set_up_file (NODE_T * p, A68_REF * z, int fd, A68_CHANNEL chan, BOOL_T r_mood, BOOL_T w_mood, int pid)
{
  A68_FILE *f;
  *z = heap_generator (p, MODE (REF_FILE), SIZE_OF (A68_FILE));
  f = (A68_FILE *) ADDRESS (z);
  f->status = (pid < 0) ? 0 : INITIALISED_MASK;
  f->identification = nil_ref;
  f->terminator = nil_ref;
  f->channel = chan;
  f->fd = fd;
  f->device.stream = NULL;
  f->opened = A_TRUE;
  f->open_exclusive = A_FALSE;
  f->read_mood = r_mood;
  f->write_mood = w_mood;
  f->char_mood = A_TRUE;
  f->draw_mood = A_FALSE;
  f->format = nil_format;
  f->transput_buffer = get_unblocked_transput_buffer (p);
  reset_transput_buffer (f->transput_buffer);
  set_default_mended_procedures (f);
}

/*!
\brief create and push a pipe
\param p position in syntax tree, should not be NULL
\param fd_r
\param fd_w
\param p position in syntax tree, should not be NULLid
**/

static void genie_mkpipe (NODE_T * p, int fd_r, int fd_w, int pid)
{
  A68_REF r, w;
  RESET_ERRNO;
/* Set up pipe. */
  set_up_file (p, &r, fd_r, stand_in_channel, A_TRUE, A_FALSE, pid);
  set_up_file (p, &w, fd_w, stand_out_channel, A_FALSE, A_TRUE, pid);
/* push pipe. */
  PUSH_REF_FILE (p, r);
  PUSH_REF_FILE (p, w);
  PUSH_INT (p, pid);
}

/*!
\brief push an environment string
\param p position in syntax tree, should not be NULL
**/

void genie_getenv (NODE_T * p)
{
  A68_REF a_env;
  char *val, *z, *z_env;
  RESET_ERRNO;
  POP (p, &a_env, SIZE_OF (A68_REF));
  TEST_INIT (p, a_env, MODE (STRING));
  z_env = (char *) get_heap_space (1 + a68_string_size (p, a_env));
  z = a_to_c_string (p, z_env, a_env);
  val = getenv (z);
  if (val == NULL) {
    a_env = empty_string (p);
  } else {
    a_env = tmp_to_a68_string (p, val);
  }
  PUSH (p, &a_env, SIZE_OF (A68_REF));
}

/*!
\brief PROC fork = INT
\param p position in syntax tree, should not be NULL
**/

void genie_fork (NODE_T * p)
{
  RESET_ERRNO;
#if defined WIN32_VERSION && defined __DEVCPP__
  PUSH_INT (p, -1);
#else
  PUSH_INT (p, fork ());
#endif
}

/*!
\brief PROC execve = (STRING, [] STRING, [] STRING) INT 
\param p position in syntax tree, should not be NULL
\return
**/

void genie_execve (NODE_T * p)
{
  int ret;
  A68_REF a_prog, a_args, a_env;
  char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
  RESET_ERRNO;
/* Pop parameters. */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Convert strings and hasta el infinito. */
  prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
  a_to_c_string (p, prog, a_prog);
  convert_string_vector (p, argv, a_args);
  convert_string_vector (p, envp, a_env);
  if (argv[0] == NULL) {
    diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
    exit_genie (p, A_RUNTIME_ERROR);
  }
  ret = execve (prog, argv, envp);
/* execve only returns if it fails. */
  free_vector (argv);
  free_vector (envp);
  free (prog);
  PUSH_INT (p, ret);
}

/*!
\brief PROC execve child = (STRING, [] STRING, [] STRING) INT
\param p position in syntax tree, should not be NULL
**/

void genie_execve_child (NODE_T * p)
{
  int pid;
  A68_REF a_prog, a_args, a_env;
  RESET_ERRNO;
/* Pop parameters. */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Now create the pipes and fork. */
#if defined WIN32_VERSION && defined __DEVCPP__
  pid = -1;
#else
  pid = fork ();
#endif
  if (pid == -1) {
    PUSH_INT (p, -1);
  } else if (pid == 0) {
/* Child process. */
    char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings. */
    prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
    a_to_c_string (p, prog, a_prog);
    convert_string_vector (p, argv, a_args);
    convert_string_vector (p, envp, a_env);
    if (argv[0] == NULL) {
      diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
      exit_genie (p, A_RUNTIME_ERROR);
    }
    (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process. */
    a68g_exit (EXIT_FAILURE);
    PUSH_INT (p, 0);
  } else {
/* parent process. */
    PUSH_INT (p, pid);
  }
}

/*!
\brief PROC execve child pipe = (STRING, [] STRING, [] STRING) PIPE
\param p position in syntax tree, should not be NULL
**/

void genie_execve_child_pipe (NODE_T * p)
{
/*
Child redirects STDIN and STDOUT.
Return a PIPE that contains the descriptors for the parent.

       pipe ptoc
       ->W...R->
 PARENT         CHILD
       <-R...W<-
       pipe ctop
*/
  int pid, ptoc_fd[2], ctop_fd[2];
  A68_REF a_prog, a_args, a_env;
  RESET_ERRNO;
/* Pop parameters. */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Now create the pipes and fork. */
#if defined WIN32_VERSION && defined __DEVCPP__
  pid = -1;
#else
  if ((pipe (ptoc_fd) == -1) || (pipe (ctop_fd) == -1)) {
    genie_mkpipe (p, -1, -1, -1);
    return;
  }
  pid = fork ();
#endif
  if (pid == -1) {
/* Fork failure. */
    genie_mkpipe (p, -1, -1, -1);
    return;
  }
  if (pid == 0) {
/* Child process. */
    char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings. */
    prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
    a_to_c_string (p, prog, a_prog);
    convert_string_vector (p, argv, a_args);
    convert_string_vector (p, envp, a_env);
/* Set up redirection. */
    close (ctop_fd[FD_READ]);
    close (ptoc_fd[FD_WRITE]);
    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    dup2 (ptoc_fd[FD_READ], STDIN_FILENO);
    dup2 (ctop_fd[FD_WRITE], STDOUT_FILENO);
    if (argv[0] == NULL) {
      diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
      exit_genie (p, A_RUNTIME_ERROR);
    }
    (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process. */
    a68g_exit (EXIT_FAILURE);
    genie_mkpipe (p, -1, -1, -1);
  } else {
/* Parent process. */
    close (ptoc_fd[FD_READ]);
    close (ctop_fd[FD_WRITE]);
    genie_mkpipe (p, ctop_fd[FD_READ], ptoc_fd[FD_WRITE], pid);
  }
}

/*!
\brief PROC create pipe = PIPE
\param p position in syntax tree, should not be NULL
**/

void genie_create_pipe (NODE_T * p)
{
  RESET_ERRNO;
  genie_stand_in (p);
  genie_stand_out (p);
  PUSH_INT (p, -1);
}

/*!
\brief PROC wait pid = (INT) VOID
\param p position in syntax tree, should not be NULL
**/

void genie_waitpid (NODE_T * p)
{
  A68_INT k;
  RESET_ERRNO;
  POP_INT (p, &k);
#if !defined WIN32_VERSION || !defined __DEVCPP__
  waitpid (k.value, NULL, 0);
#endif
}

#endif /* HAVE_UNIX. */

/*
Next part contains some routines that interface Algol68G and the curses library.
Be sure to know what you are doing when you use this, but on the other hand,
"reset" will always restore your terminal. 
*/

#ifdef HAVE_CURSES

#include <sys/time.h>
#include <curses.h>

#ifdef WIN32_VERSION
#undef PATTERN
#undef FD_READ
#undef FD_WRITE
#include <winsock.h>
#endif

BOOL_T curses_active = A_FALSE;

/*!
\brief clean_curses
**/

void clean_curses ()
{
  if (curses_active == A_TRUE) {
    attrset (A_NORMAL);
    endwin ();
    curses_active = A_FALSE;
  }
}

/*!
\brief init_curses
**/

void init_curses ()
{
  initscr ();
  cbreak ();			/* raw () would cut off ctrl-c. */
  noecho ();
  nonl ();
  curs_set (0);
  curses_active = A_TRUE;
}

/*!
\brief watch stdin for input, do not wait very long
\return
**/

int rgetchar (void)
{
#ifdef WIN32_VERSION
  return (getch ());
#else
  int retval;
  struct timeval tv;
  fd_set rfds;
  tv.tv_sec = 0;
  tv.tv_usec = 100;
  FD_ZERO (&rfds);
  FD_SET (0, &rfds);
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (retval) {
    /* FD_ISSET(0, &rfds) will be true. */
    return (getch ());
  } else {
    return ('\0');
  }
#endif
}

/*!
\brief PROC curses start = VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_start (NODE_T * p)
{
  (void) p;
  init_curses ();
}

/*!
\brief PROC curses end = VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_end (NODE_T * p)
{
  (void) p;
  clean_curses ();
}

/*!
\brief PROC curses clear = VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_clear (NODE_T * p)
{
  (void) p;
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  clear ();
}

/*!
\brief PROC curses refresh = VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_refresh (NODE_T * p)
{
  (void) p;
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  refresh ();
}

/*!
\brief PROC curses lines = INT
\param p position in syntax tree, should not be NULL
**/

void genie_curses_lines (NODE_T * p)
{
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  PUSH_INT (p, LINES);
}

/*!
\brief PROC curses columns = INT
\param p position in syntax tree, should not be NULL
**/

void genie_curses_columns (NODE_T * p)
{
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  PUSH_INT (p, COLS);
}

/*!
\brief PROC curses getchar = CHAR
\param p position in syntax tree, should not be NULL
**/

void genie_curses_getchar (NODE_T * p)
{
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  PUSH_CHAR (p, rgetchar ());
}

/*!
\brief PROC curses putchar = (CHAR) VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_putchar (NODE_T * p)
{
  A68_CHAR ch;
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  POP_CHAR (p, &ch);
  addch (ch.value);
}

/*!
\brief PROC curses move = (INT, INT) VOID
\param p position in syntax tree, should not be NULL
**/

void genie_curses_move (NODE_T * p)
{
  A68_INT i, j;
  if (curses_active == A_FALSE) {
    init_curses ();
  }
  POP_INT (p, &j);
  POP_INT (p, &i);
  move (i.value, j.value);
}

#endif /* HAVE_CURSES. */
