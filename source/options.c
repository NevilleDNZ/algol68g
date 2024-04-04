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
This code handles options to Algol68G.
 
Option syntax does not follow GNU standards; it looks like old mainframe syntax.

Options come from:
  [1] A .rc file (normally a68g.rc).
  [2] The command line. Those options overrule options from [1].
  [3] Pragmat items. Those options overrule options from [1] and [2].         */

#include "algol68g.h"
#include "genie.h"
#include "mp.h"

OPTIONS_T *options;
BOOL_T gnu_diags;

/*-------1---------2---------3---------4---------5---------6---------7---------+
strip_minus: strip minus signs preceeding a string. We accept any number.     */

static char *
strip_minus (char *p)
{
  while (p[0] == '-' || p[0] == '/')
    {
      p++;
    }
  return (new_string (p));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
online_help: give brief help if someone types 'a68g -help'.                   */

static void
online_help ()
{
  printf ("\nAlgol68G %s, Copyright (C) 2001-2004 J. Marcel van der Veer", REVISION);
  printf ("\nAlgol68G comes with ABSOLUTELY NO WARRANTY;");
  printf ("\nSee the GNU General Public License for more details.");
  printf ("\n");
  printf ("\nusage: %s [options | filename]", A68G_NAME);
  printf ("\n");
  printf ("\nOptions that execute Algol 68 code from the command line:");
  printf ("\n");
  printf ("\n   print unit                Print value yielded by Algol 68 unit `unit'");
  printf ("\n   execute unit              Execute Algol 68 unit `unit'");
  printf ("\n");
  printf ("\nOptions to control the listing file:");
  printf ("\n");
  printf ("\n   extensive                 Make extensive listing");
  printf ("\n   listing                   Make concise listing");
  printf ("\n   moids                     Make overview of moids in listing file");
  printf ("\n   preludelisting            Make a listing of preludes");
  printf ("\n   source, nosource          Switch on/off listing of source lines in listing file");
  printf ("\n   statistics                Print statistics in listing file");
  printf ("\n   tree, notree              Switch on/off syntax tree listing in listing file");
  printf ("\n   unused                    Make an overview of unused tags in the listing file");
  printf ("\n   xref, noxref              Switch on/off cross reference in the listing file");
  printf ("\n");
  printf ("\nInterpreter options:");
  printf ("\n");
  printf ("\n   assertions, noassertions  Switch on/off elaboration of assertions");
  printf ("\n   precision number          Sets precision for LONG LONG modes to `number' significant digits");
  printf ("\n   timelimit number          Interrupt the interpreter after `number' seconds");
  printf ("\n   trace, notrace            Switch on/off tracing of a running program");
  printf ("\n");
  printf ("\nOptions to control the stropping regime:");
  printf ("\n");
  printf ("\n   boldstropping             Set stropping mode to bold stropping (default)");
  printf ("\n   quotestropping            Set stropping mode to quote stropping");
  printf ("\n");
  printf ("\nOptions to control memory usage:");
  printf ("\n");
  printf ("\n   heap number               Set heap size to `number'");
  printf ("\n   handles number            Set handle space size to `number'");
  printf ("\n   frame number              Set frame stack size to `number'");
  printf ("\n   stack number              Set expression stack size to `number'");
  printf ("\n");
  printf ("\nMiscellaneous options:");
  printf ("\n");
  printf ("\n   brackets                  Consider [ .. ] and { .. } as equivalent to ( .. )");
  printf ("\n   check, norun              Check syntax only, interpreter does not start");
  printf ("\n   echo string               Echo `string' to standard output");
  printf ("\n   exit, --                  Ignore next options");
  printf ("\n   file string               Accept string as generic filename");
  printf ("\n   gnudiagnostics            Give GNU style diagnostics");
  printf ("\n   nowarnings                Suppress warning messages");
  printf ("\n   pragmats, nopragmats      Switch on/off elaboration of pragmat items");
  printf ("\n   reductions                Print parser reductions");
  printf ("\n   verbose                   Informs you on program actions");
  printf ("\n   version                   State the version of the running copy");
  printf ("\n");
  fflush (stdout);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
add_option_list: add an option to the list, to be processed later.            */

void
add_option_list (OPTION_LIST_T ** l, char *str)
{
  if (*l == NULL)
    {
      *l = (OPTION_LIST_T *) get_heap_space (SIZE_OF (OPTION_LIST_T));
      (*l)->scan = source_scan;
      (*l)->str = new_string (str);
      (*l)->processed = A_FALSE;
      NEXT (*l) = NULL;
    }
  else
    {
      add_option_list (&(NEXT (*l)), str);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
init_options: initialise option handler.                                      */

void
init_options (MODULE_T * module)
{
  options = (OPTIONS_T *) malloc ((size_t) SIZE_OF (OPTIONS_T));
  module->options.list = NULL;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
eq: whether 'p' and 'q'. 
Upper case letters in 'q' are mandatory, lower case (if present) must whether.  */

static BOOL_T
eq (MODULE_T * module, char *p, char *q)
{
  if (module->options.pragmat_sema)
    {
      return (match_string (p, q, '='));
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
prune_echoes: process echoes gathered in the option list.                     */

void
prune_echoes (MODULE_T * module, OPTION_LIST_T * i)
{
  while (i != NULL)
    {
      if (i->scan == source_scan)
	{
	  char *p = strip_minus (i->str);
/* ECHO echoes a string */
	  if (eq (module, p, "ECHO"))
	    {
	      {
		char *car = strchr (p, '=');
		if (car != NULL)
		  {
		    io_close_tty_line ();
		    sprintf (output_line, "%s", &car[1]);
		    io_write_string (STDOUT_FILENO, output_line);
		  }
		else
		  {
		    i = NEXT (i);
		    if (i != NULL)
		      {
			if (strcmp (i->str, "=") == 0)
			  {
			    i = NEXT (i);
			  }
			if (i != NULL)
			  {
			    io_close_tty_line ();
			    sprintf (output_line, "%s", i->str);
			    io_write_string (STDOUT_FILENO, output_line);
			  }
		      }
		  }
	      }
	    }
	}
      i = NEXT (i);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_options: process options gathered in the option list.                     */

static int
fetch_integral (char *p, OPTION_LIST_T ** i, BOOL_T * error)
{
  char *car = NULL, *num = NULL;
  int k, mult = 1;
  *error = A_FALSE;
/* Fetch argument. */
  car = strchr (p, '=');
  if (car == NULL)
    {
      *i = NEXT (*i);
      *error = (*i == NULL);
      if (!error && strcmp ((*i)->str, "=") == 0)
	{
	  *i = NEXT (*i);
	  *error = ((*i) == NULL);
	}
      if (!*error)
	{
	  num = (*i)->str;
	}
    }
  else
    {
      num = &car[1];
    }
/* Translate argument into integer. */
  if (*error)
    {
      diagnostic (A_ERROR, NULL, "invalid argument in option Z", p, NULL);
      return (0);
    }
  else
    {
      char *postfix;
      errno = 0;
      k = strtol (num, &postfix, 0);	/* Accept also octal and hex. */
      *error = (postfix == num);
      if (errno != 0 || *error)
	{
	  diagnostic (A_ERROR, NULL, "invalid argument in option Z", p, NULL);
	  *error = A_TRUE;
	}
      else
	{
/* Accept postfix multipliers: 32k, 64M, 1G. */
	  if (postfix != NULL)
	    {
	      switch (postfix[0])
		{
		case '\0':
		  break;
		case 'k':
		case 'K':
		  mult = KILOBYTE;
		  break;
		case 'm':
		case 'M':
		  mult = MEGABYTE;
		  break;
		case 'g':
		case 'G':
		  mult = GIGABYTE;
		  break;
		default:
		  diagnostic (A_ERROR, NULL, "invalid argument in option Z", p, NULL);
		  *error = A_TRUE;
		  break;
		}
	      if (postfix[0] != '\0' && postfix[1] != '\0')
		{
		  diagnostic (A_ERROR, NULL, "trailing characters in option Z", p, NULL);
		  *error = A_TRUE;
		}
	    }
	}
      return (k * mult);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
set_options: process options gathered in the option list.                     */

BOOL_T
set_options (MODULE_T * module, OPTION_LIST_T * i, BOOL_T cmd_line)
{
  BOOL_T no_err = A_TRUE, go_on = A_TRUE, name_set = A_FALSE;
  OPTION_LIST_T *j = i;
  errno = 0;
  while (i != NULL && no_err && go_on)
    {
      if (!(i->processed))
	{
/* Accept both UNIX '-option [=] value' as well as VMS '/OPTION [=] VALUE' */
	  BOOL_T minus_sign = ((i->str)[0] == '-') || ((i->str)[0] == '/');
	  char *p = strip_minus (i->str);
	  if (!minus_sign && cmd_line)
	    {
/* Item without '-'s is generic filename */
	      if (!name_set)
		{
		  module->files.generic_name = new_string (p);
		  name_set = A_TRUE;
		}
	      else
		{
		  diagnostic (A_ERROR, NULL, "in option Z: source filename already set to Z", p, module->files.generic_name, NULL);
		  no_err = A_FALSE;
		}
	    }
	  else if (eq (module, p, "EXIT"))
	    {
	      go_on = A_FALSE;
	    }
	  else if (eq (module, p, ""))
	    {
/* Empty item (from specifying '-' or '--') stops option processing */
	      go_on = A_FALSE;
	    }
	  else if (eq (module, p, "File") && cmd_line)
	    {
/* FILE accepts its argument as generic filename */
	      i = NEXT (i);
	      if (i != NULL && strcmp (i->str, "=") == 0)
		{
		  i = NEXT (i);
		}
	      if (i != NULL)
		{
		  if (!name_set)
		    {
		      module->files.generic_name = new_string (i->str);
		      name_set = A_TRUE;
		    }
		  else
		    {
		      diagnostic (A_ERROR, NULL, "in option Z: source filename already set to Z", p, module->files.generic_name, NULL);
		      no_err = A_FALSE;
		    }
		}
	      else
		{
		  diagnostic (A_ERROR, NULL, "in option Z: filename expected", p, NULL);
		  no_err = A_FALSE;
		}
	    }
	  else if (eq (module, p, "Help") && cmd_line == A_TRUE)
	    {
/* HELP gives online help */
	      online_help ();
	      a68g_exit (EXIT_SUCCESS);
	    }
	  else if (eq (module, p, "ECHO"))
	    {
/* ECHO is treated later. */
	      if (strchr (p, '=') == NULL)
		{
		  i = NEXT (i);
		  if (i != NULL)
		    {
		      if (strcmp (i->str, "=") == 0)
			{
			  i = NEXT (i);
			}
		    }
		}
	    }
	  else if (eq (module, p, "Execute") || eq (module, p, "Print"))
	    {
/* EXECUTE and PRINT execute their argument as Algol 68 text */
	      if (cmd_line == A_FALSE)
		{
		  diagnostic (A_ERROR, NULL, "option Z only valid from command line", p, NULL);
		  no_err = A_FALSE;
		}
	      else if ((i = NEXT (i)) != NULL)
		{
		  BOOL_T error = A_FALSE;
		  if (strcmp (i->str, "=") == 0)
		    {
		      error = (i = NEXT (i)) == NULL;
		    }
		  if (!error)
		    {
		      char name[BUFFER_SIZE];
		      FILE *f;
		      strcpy (name, ".");
		      strcat (name, A68G_NAME);
		      strcat (name, ".x");
		      f = fopen (name, "w");
		      abend (f == NULL, "cannot open temp file", NULL);
		      if (eq (module, p, "Execute"))
			{
			  fprintf (f, "%s\n", i->str);
			}
		      else
			{
			  fprintf (f, "(print ((%s)))\n", i->str);
			}
		      fclose (f);
		      module->files.generic_name = new_string (name);
		    }
		  else
		    {
		      diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
		      no_err = A_FALSE;
		    }
		}
	      else
		{
		  diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
		  no_err = A_FALSE;
		}
	    }
	  else if (eq (module, p, "HEAP") || eq (module, p, "HANDLES") || eq (module, p, "STACK") || eq (module, p, "Frame"))
	    {
/* EXPAND, HEAP, HANDLES, STACK and FRAME set core allocation */
	      BOOL_T error = A_FALSE;
	      int k = fetch_integral (p, &i, &error);
/* Adjust size. */
	      if (error || errno > 0)
		{
		  diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
		  no_err = A_FALSE;
		}
	      else if (k > 0)
		{
		  if (k < MIN_MEM_SIZE)
		    {
		      diagnostic (A_WARNING, NULL, "in option Z: D set to D", p, k, MIN_MEM_SIZE, NULL);
		      k = MIN_MEM_SIZE;
		    }
		  if (eq (module, p, "HEAP"))
		    {
		      heap_size = k;
		    }
		  else if (eq (module, p, "HANDLE"))
		    {
		      handle_pool_size = k;
		    }
		  else if (eq (module, p, "STACK"))
		    {
		      expr_stack_size = k;
		    }
		  else if (eq (module, p, "Frame"))
		    {
		      frame_stack_size = k;
		    }
		}
	    }
	  else if (eq (module, p, "BRackets"))
	    {
/* BRACKETS extends Algol 68 syntax for brackets. */
	      module->options.brackets = A_TRUE;
	    }
	  else if (eq (module, p, "REDuctions"))
	    {
/* REDUCTIONS gives parser reductions.*/
	      module->options.reductions = A_TRUE;
	    }
	  else if (eq (module, p, "GNUDiagnostics"))
	    {
/* GNUDIAGNOSTIC gives GNU style diagnostics iso VMS style */
	      gnu_diags = A_TRUE;
	    }
	  else if (eq (module, p, "QUOTEStropping"))
	    {
/* QUOTESTROPPING set stropping to quote stropping */
	      module->options.stropping = QUOTE_STROPPING;
	    }
	  else if (eq (module, p, "BOLDStropping"))
	    {
/* BOLDSTROPPING set stropping to bold stropping, which is nowadays the expected default */
	      module->options.stropping = BOLD_STROPPING;
	    }
	  else if (eq (module, p, "Check") || eq (module, p, "NORun"))
	    {
/* CHECK and NORUN just check for syntax */
	      module->options.check_only = A_TRUE;
	    }
	  else if (eq (module, p, "REGRESSION"))
	    {
/* REGRESSION is an option that set preferences when running the Algol68G test suite */
	      module->options.regression_test = A_TRUE;
	      gnu_diags = A_FALSE;
	      module->options.time_limit = 120;
	    }
	  else if (eq (module, p, "NOWarnings"))
	    {
/* NOWARNINGS switches warnings off */
	      module->options.no_warnings = A_TRUE;
	    }
	  else if (eq (module, p, "PRagmats"))
	    {
/* PRAGMATS and NOPRAGMATS switch on/off pragmat processing */
	      module->options.pragmat_sema = A_TRUE;
	    }
	  else if (eq (module, p, "NOPRagmats"))
	    {
	      module->options.pragmat_sema = A_FALSE;
	    }
	  else if (eq (module, p, "VERBose"))
	    {
/* VERBOSE in case you want to know what Algol68G is doing */
	      module->options.verbose = A_TRUE;
	    }
	  else if (eq (module, p, "Version"))
	    {
/* VERSION lists the current version at an appropriate time in the future */
	      module->options.version = A_TRUE;
	    }
	  else if (eq (module, p, "Xref"))
	    {
/* XREF and NOXREF switch on/off a cross reference */
	      module->options.source_listing = A_TRUE;
	      module->options.cross_reference = A_TRUE;
	      module->options.nodemask |= (CROSS_REFERENCE_MASK | SOURCE_MASK);
	    }
	  else if (eq (module, p, "NOXref"))
	    {
	      module->options.nodemask &= ~(CROSS_REFERENCE_MASK | SOURCE_MASK);
	    }
	  else if (eq (module, p, "PRELUDElisting"))
	    {
/* PRELUDELISTING cross references preludes, if they ever get implemented ... */
	      module->options.standard_prelude_listing = A_TRUE;
	    }
	  else if (eq (module, p, "STatistics"))
	    {
/* STATISTICS prints process statistics */
	      module->options.statistics_listing = A_TRUE;
	    }
	  else if (eq (module, p, "TREE"))
	    {
/* TREE and NOTREE switch on/off printing of the syntax tree. This gets bulky! */
	      module->options.source_listing = A_TRUE;
	      module->options.tree_listing = A_TRUE;
	      module->options.nodemask |= (TREE_MASK | SOURCE_MASK);
	    }
	  else if (eq (module, p, "NOTREE"))
	    {
	      module->options.nodemask ^= (TREE_MASK | SOURCE_MASK);
	    }
	  else if (eq (module, p, "UNUSED"))
	    {
/* UNUSED indicates unused tags */
	      module->options.unused = A_TRUE;
	    }
	  else if (eq (module, p, "EXTensive"))
	    {
/* EXTENSIVE set of options for an extensive listing */
	      module->options.source_listing = A_TRUE;
	      module->options.tree_listing = A_TRUE;
	      module->options.cross_reference = A_TRUE;
	      module->options.moid_listing = A_TRUE;
	      module->options.standard_prelude_listing = A_TRUE;
	      module->options.statistics_listing = A_TRUE;
	      module->options.unused = A_TRUE;
	      module->options.nodemask |= (CROSS_REFERENCE_MASK | TREE_MASK | CODE_MASK | SOURCE_MASK);
	    }
	  else if (eq (module, p, "LISTing"))
	    {
/* LISTING set of options for a default listing */
	      module->options.source_listing = A_TRUE;
	      module->options.cross_reference = A_TRUE;
	      module->options.statistics_listing = A_TRUE;
	      module->options.nodemask |= (SOURCE_MASK | CROSS_REFERENCE_MASK);
	    }
	  else if (eq (module, p, "TTY"))
	    {
/* TTY send listing to standout. Remnant from my mainframe past */
	      module->options.cross_reference = A_TRUE;
	      module->options.statistics_listing = A_TRUE;
	      module->options.nodemask |= (SOURCE_MASK | CROSS_REFERENCE_MASK);
	    }
	  else if (eq (module, p, "SOURCE"))
	    {
/* SOURCE and NOSOURCE print source lines */
	      module->options.source_listing = A_TRUE;
	      module->options.nodemask |= SOURCE_MASK;
	    }
	  else if (eq (module, p, "NOSOURCE"))
	    {
	      module->options.nodemask &= ~SOURCE_MASK;
	    }
	  else if (eq (module, p, "MOIDS"))
	    {
/* MOIDS prints an overview of moids used in the program */
	      module->options.moid_listing = A_TRUE;
	    }
	  else if (eq (module, p, "Assertions"))
	    {
/* ASSERTIONS and NOASSERTIONS switch on/off the processing of assertions */
	      module->options.nodemask |= ASSERT_MASK;
	    }
	  else if (eq (module, p, "NOAssertions"))
	    {
	      module->options.nodemask &= ~ASSERT_MASK;
	    }
	  else if (eq (module, p, "PRECision"))
	    {
/* PRECISION sets the precision */
	      BOOL_T error = A_FALSE;
	      int k = fetch_integral (p, &i, &error);
	      if (error || errno > 0)
		{
		  diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
		  no_err = A_FALSE;
		}
	      else if (k > 1)
		{
		  if (k > MAX_MP_PRECISION)
		    {
		      diagnostic (A_WARNING, NULL, "in option Z: very high precision impedes performance", p, NULL);
		    }
		  if (int_to_mp_digits (k) > long_mp_digits ())
		    {
		      set_longlong_mp_digits (int_to_mp_digits (k));
		    }
		  else
		    {
		      k = 1;
		      while (int_to_mp_digits (k) <= long_mp_digits ())
			{
			  k++;
			}
		      diagnostic (A_ERROR, NULL, "in option Z: precision must exceed D", p, k - 1, NULL);
		      no_err = A_FALSE;
		    }
		}
	      else
		{
		  diagnostic (A_ERROR, NULL, "in option Z: invalid precision D", p, k, NULL);
		  no_err = A_FALSE;
		}
	    }
	  else if (eq (module, p, "TRace"))
	    {
/* TRACE and NOTRACE switch on/off tracing of the running program */
	      module->options.trace = A_TRUE;
	      module->options.nodemask |= TRACE_MASK;
	    }
	  else if (eq (module, p, "NOTRace"))
	    {
	      module->options.nodemask &= ~TRACE_MASK;
	    }
	  else if (eq (module, p, "TImelimit"))
	    {
/* TIMELIMIT lets the interpreter stop after so-many seconds */
	      BOOL_T error = A_FALSE;
	      int k = fetch_integral (p, &i, &error);
	      if (error || errno > 0)
		{
		  diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
		  no_err = A_FALSE;
		}
	      else if (k < 1)
		{
		  diagnostic (A_WARNING, NULL, "in option Z: invalid time limit", p, NULL);
		  no_err = A_FALSE;
		}
	      else
		{
		  module->options.time_limit = k;
		}
	    }
	  else
	    {
/* Unrecognised. */
	      diagnostic (A_ERROR, NULL, "in option Z", p, NULL);
	      no_err = A_FALSE;
	    }
	}
/* Go processing next item, if present */
      if (i != NULL)
	{
	  i = NEXT (i);
	}
    }
/* Mark options as processed. */
  for (; j != NULL; j = NEXT (j))
    {
      j->processed = A_TRUE;
    }
  return (no_err && errno == 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
default_mem_sizes: set default core size.                                     */

void
default_mem_sizes (void)
{
#ifdef PRE_MACOS_X
/* 4 MB */
  frame_stack_size = 256 * KILOBYTE;
  expr_stack_size = 256 * KILOBYTE;
  heap_size = 3 * MEGABYTE;
  handle_pool_size = 512 * KILOBYTE;
#else
/* 12 MB */
  frame_stack_size = MEGABYTE;
  expr_stack_size = MEGABYTE;
  heap_size = 8 * MEGABYTE;
  handle_pool_size = 2 * MEGABYTE;
#endif
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
default_options: set default values for options.                              */

void
default_options (MODULE_T * module)
{
  module->options.check_only = A_FALSE;
  module->options.moid_listing = A_FALSE;
  module->options.tree_listing = A_FALSE;
  module->options.source_listing = A_FALSE;
  module->options.statistics_listing = A_FALSE;
  module->options.standard_prelude_listing = A_FALSE;
  module->options.verbose = A_FALSE;
  module->options.version = A_FALSE;
  module->options.cross_reference = A_FALSE;
  module->options.no_warnings = A_FALSE;
  module->options.unused = A_FALSE;
  module->options.pragmat_sema = A_TRUE;
  module->options.trace = A_FALSE;
  module->options.regression_test = A_FALSE;
  module->options.nodemask = ASSERT_MASK;
  module->options.time_limit = 0;
  module->options.stropping = BOLD_STROPPING;
  module->options.brackets = A_FALSE;
  module->options.reductions = A_FALSE;
  gnu_diags = A_FALSE;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
read_rc_options: read options from the .rc file.                              */

void
read_rc_options (MODULE_T * module)
{
  FILE *f;
  char *name = (char *) get_heap_space (1 + strlen (A68G_NAME) + strlen (".rc"));
  strcpy (name, ".");
  strcat (name, A68G_NAME);
  strcat (name, "rc");
  f = fopen (name, "r");
  if (f != NULL)
    {
      while (!feof (f))
	{
	  if (fgets (input_line, CMS_LINE_SIZE, f) != NULL)
	    {
	      if (input_line[strlen (input_line) - 1] == '\n')
		{
		  input_line[strlen (input_line) - 1] = '\0';
		}
	      isolate_options (module, input_line);
	    }
	}
      fclose (f);
      set_options (module, module->options.list, A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
isolate_options: tokenise string 'p' that holds options.                      */

void
isolate_options (MODULE_T * module, char *p)
{
  char *q;
/* 'q' points at first significant char in item .*/
  while (p[0] != '\0')
    {
/* Skip white space ... */
      while (p[0] == ' ' && p[0] != '\0')
	{
	  p++;
	}
/* ... then tokenise an item */
      if (p[0] != '\0')
	{
/* Item can be "string" */
	  if (p[0] == '"')
	    {
	      p++;
	      q = p;
	      /* Skip the string */
	      while (p[0] != '"' && p[0] != '\0')
		{
		  p++;
		}
	      if (p[0] != '\0')
		{
		  p[0] = '\0';	/* p[0] was '\"' */
		  p++;
		}
	    }
/* Item can be 'string' */
	  else if (p[0] == '\'')
	    {
	      p++;
	      q = p;
	      /* Skip the string */
	      while (p[0] != '\'' && p[0] != '\0')
		{
		  p++;
		}
	      if (p[0] != '\0')
		{
		  p[0] = '\0';	/* p[0] was '\'' */
		  p++;
		}
	    }
	  else
/* Item is not a delimited string */
	    {
	      q = p;
/* Tokenise symbol and gather it in the option list for later processing.
   Skip '='s, we accept if someone writes -prec=60 -heap=8192. */
	      if (*q == '=')
		{
		  p++;
		}
	      else
		{
		  /* Skip item */
		  while (p[0] != ' ' && p[0] != '\0' && p[0] != '=')
		    {
		      p++;
		    }
		}
	      if (p[0] != '\0')
		{
		  p[0] = '\0';
		  p++;
		}
	    }
/* 'q' points to first significant char in item, and 'p' points after item */
	  add_option_list (&(module->options.list), q);
	}
    }
}
