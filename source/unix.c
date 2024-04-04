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
This code implements some Linux related routines.
Based in part on work by Sian Leitch <sian@sleitch.nildram.co.uk>.            */

#include "algol68g.h"

#ifdef HAVE_UNIX

#include "genie.h"
#include "transput.h"

#ifndef WIN32_VERSION
#include <sys/wait.h>
#endif

#define VECTOR_SIZE 512
#define FD_READ 0
#define FD_WRITE 1

extern A68_REF tmp_to_a68_string (NODE_T *, char *);

extern A68_CHANNEL stand_in_channel, stand_out_channel, stand_draw_channel, stand_back_channel, stand_error_channel;

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_argc: PROC INT argc                                                     */

void
genie_argc (NODE_T * p)
{
  PUSH_INT (p, global_argc);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_argv: PROC (INT) STRING argv                                            */

void
genie_argv (NODE_T * p)
{
  A68_INT index;
  POP_INT (p, &index);
  if (index.value >= 1 && index.value <= global_argc)
    {
      PUSH_REF (p, c_to_a_string (p, global_argv[index.value - 1]));
    }
  else
    {
      PUSH_REF (p, empty_string (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
convert_string_vector: convert [] STRING row to char *vec[].                  */

static void
convert_string_vector (NODE_T * p, char *vec[], A68_REF row)
{
  BYTE_T *z = ADDRESS (&row);
  A68_ARRAY *arr = (A68_ARRAY *) & z[0];
  A68_TUPLE *tup = (A68_TUPLE *) & z[SIZE_OF (A68_ARRAY)];
  int k = 0;
  if (get_row_size (tup, arr->dimensions) != 0)
    {
      BYTE_T *base_addr = ADDRESS (&arr->array);
      BOOL_T done = A_FALSE;
      initialise_internal_index (tup, arr->dimensions);
      while (!done)
	{
	  ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	  ADDR_T elem_addr = (index + arr->slice_offset) * arr->elem_size + arr->field_offset;
	  BYTE_T *elem = &base_addr[elem_addr];
	  int size = a68_string_size (p, *(A68_REF *) elem);
	  TEST_INIT (p, *(A68_REF *) elem, MODE (STRING));
	  vec[k] = (char *) get_heap_space (1 + size);
	  a_to_c_string (p, vec[k], *(A68_REF *) elem);
	  if (k == VECTOR_SIZE - 1)
	    {
	      diagnostic (A_RUNTIME_ERROR, p, "too many arguments");
	      exit_genie (p, A_RUNTIME_ERROR);
	    }
	  if (strlen (vec[k]) > 0)
	    {
	      k++;
	    }
	  done = increment_internal_index (tup, arr->dimensions);
	}
    }
  vec[k] = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
free_vector: recycle vec.                                                     */

static void
free_vector (char *vec[])
{
  int k = 0;
  while (vec[k] != NULL)
    {
      free (vec[k]);
      k++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_reset_errno: reset error number.                                        */

void
genie_reset_errno (NODE_T * p)
{
  (void) *p;
  errno = 0;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_reset_errno: error number.                                              */

void
genie_errno (NODE_T * p)
{
  PUSH_INT (p, errno);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_strerror: push explanation for current error number.                    */

void
genie_strerror (NODE_T * p)
{
  A68_INT i;
  POP_INT (p, &i);
  PUSH_REF (p, c_to_a_string (p, strerror (i.value)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_up_file: set up file for usage in pipe.                                   */

static void
set_up_file (NODE_T * p, A68_REF * z, int fd, A68_CHANNEL chan, BOOL_T r_mood, BOOL_T w_mood, int pid)
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

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_mkpipe: create and push a pipe.                                         */

static void
genie_mkpipe (NODE_T * p, int fd_r, int fd_w, int pid)
{
  A68_REF r, w;
/* Set up pipe */
  set_up_file (p, &r, fd_r, stand_in_channel, A_TRUE, A_FALSE, pid);
  set_up_file (p, &w, fd_w, stand_out_channel, A_FALSE, A_TRUE, pid);
/* push pipe */
  PUSH_REF_FILE (p, r);
  PUSH_REF_FILE (p, w);
  PUSH_INT (p, pid);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_getenv: push an environment string.                                     */

void
genie_getenv (NODE_T * p)
{
  A68_REF a_env;
  char *val, *z, *z_env;
  POP (p, &a_env, SIZE_OF (A68_REF));
  TEST_INIT (p, a_env, MODE (STRING));
  z_env = (char *) get_heap_space (1 + a68_string_size (p, a_env));
  z = a_to_c_string (p, z_env, a_env);
  val = getenv (z);
  if (val == NULL)
    {
      a_env = empty_string (p);
    }
  else
    {
      a_env = tmp_to_a68_string (p, val);
    }
  PUSH (p, &a_env, SIZE_OF (A68_REF));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_fork: fork and push the return value (self or pid).                     */

void
genie_fork (NODE_T * p)
{
#if defined WIN32_VERSION && defined __DEVCPP__
  PUSH_INT (p, -1);
#else
  PUSH_INT (p, fork ());
#endif
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_execve: replace this process with another.                              */

void
genie_execve (NODE_T * p)
{
  int ret;
  A68_REF a_prog, a_args, a_env;
  char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Pop parameters */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Convert strings and hasta el infinito */
  prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
  a_to_c_string (p, prog, a_prog);
  convert_string_vector (p, argv, a_args);
  convert_string_vector (p, envp, a_env);
  if (argv[0] == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
      exit_genie (p, A_RUNTIME_ERROR);
    }
  ret = execve (prog, argv, envp);
/* execve only returns if it fails */
  free_vector (argv);
  free_vector (envp);
  free (prog);
  PUSH_INT (p, ret);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_execve_child: execute command in a child process.                       */

void
genie_execve_child (NODE_T * p)
{
  int pid;
  A68_REF a_prog, a_args, a_env;
/* Pop parameters */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Now create the pipes and fork */
#if defined WIN32_VERSION && defined __DEVCPP__
  pid = -1;
#else
  pid = fork ();
#endif
  if (pid == -1)
    {
      PUSH_INT (p, -1);
    }
  else if (pid == 0)
    {
/* Child process */
      char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings */
      prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
      a_to_c_string (p, prog, a_prog);
      convert_string_vector (p, argv, a_args);
      convert_string_vector (p, envp, a_env);
      if (argv[0] == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process */
      a68g_exit (EXIT_FAILURE);
      PUSH_INT (p, 0);
    }
  else
    {
/* parent process */
      PUSH_INT (p, pid);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_execve_child_pipe: execute command in a child process.
child redirects STDIN and STDOUT.
return a PIPE that contains the descriptors for the parent.

       pipe ptoc
       ->W...R->  
 PARENT         CHILD
       <-R...W<-
       pipe ctop                                                              */

void
genie_execve_child_pipe (NODE_T * p)
{
  int pid, ptoc_fd[2], ctop_fd[2];
  A68_REF a_prog, a_args, a_env;
/* Pop parameters */
  POP (p, &a_env, SIZE_OF (A68_REF));
  POP (p, &a_args, SIZE_OF (A68_REF));
  POP (p, &a_prog, SIZE_OF (A68_REF));
/* Now create the pipes and fork */
#if defined WIN32_VERSION && defined __DEVCPP__
  pid = -1;
#else
  if ((pipe (ptoc_fd) == -1) || (pipe (ctop_fd) == -1))
    {
      genie_mkpipe (p, -1, -1, -1);
      return;
    }
  pid = fork ();
#endif
  if (pid == -1)
    {
/* Fork failure */
      genie_mkpipe (p, -1, -1, -1);
      return;
    }
  if (pid == 0)
    {
/* Child process */
      char *prog, *argv[VECTOR_SIZE], *envp[VECTOR_SIZE];
/* Convert  strings */
      prog = (char *) get_heap_space (1 + a68_string_size (p, a_prog));
      a_to_c_string (p, prog, a_prog);
      convert_string_vector (p, argv, a_args);
      convert_string_vector (p, envp, a_env);
/* Set up redirection */
      close (ctop_fd[FD_READ]);
      close (ptoc_fd[FD_WRITE]);
      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      dup2 (ptoc_fd[FD_READ], STDIN_FILENO);
      dup2 (ctop_fd[FD_WRITE], STDOUT_FILENO);
      if (argv[0] == NULL)
	{
	  diagnostic (A_RUNTIME_ERROR, p, "argument is empty");
	  exit_genie (p, A_RUNTIME_ERROR);
	}
      (void) execve (prog, argv, envp);
/* execve only returns if it fails - end child process */
      a68g_exit (EXIT_FAILURE);
      genie_mkpipe (p, -1, -1, -1);
    }
  else
    {
/* Parent process */
      close (ptoc_fd[FD_READ]);
      close (ctop_fd[FD_WRITE]);
      genie_mkpipe (p, ctop_fd[FD_READ], ptoc_fd[FD_WRITE], pid);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_create_pipe: return a pipe with no process.                             */

void
genie_create_pipe (NODE_T * p)
{
  genie_stand_in (p);
  genie_stand_out (p);
  PUSH_INT (p, -1);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_waitpid: stall until indicated process ends.                            */

void
genie_waitpid (NODE_T * p)
{
  A68_INT k;
  POP_INT (p, &k);
#if !defined WIN32_VERSION || !defined __DEVCPP__
  waitpid (k.value, NULL, 0);
#endif
}

#endif
