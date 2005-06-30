/*!
\file options.c
\brief handles options
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
This code handles options to Algol68G.

Option syntax does not follow GNU standards.

Options come from:
  [1] A .rc file (normally a68g.rc).
  [2] The command line. Those options overrule options from [1].
  [3] Pragmat items. Those options overrule options from [1] and [2]. 
*/

#include "algol68g.h"
#include "genie.h"
#include "mp.h"

OPTIONS_T *options;
BOOL_T gnu_diags;

/*!
\brief strip minus signs preceeding a string
\param p
\return
**/

static char *strip_minus (char *p)
{
  while (p[0] == '-') {
    p++;
  }
  return (new_string (p));
}

/*!
\brief give brief help if someone types 'a68g -help'
\return
**/

static void online_help ()
{
  printf ("\nAlgol68G %s, Copyright (C) 2001-2005 J. Marcel van der Veer", REVISION);
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
  printf ("\n   run                       Override the check/norun option");
  printf ("\n   echo string               Echo `string' to standard output");
  printf ("\n   exit, --                  Ignore next options");
  printf ("\n   file string               Accept string as generic filename");
  printf ("\n   gnudiagnostics            Give GNU style diagnostics");
  printf ("\n   nowarnings                Suppress warning messages");
  printf ("\n   pragmats, nopragmats      Switch on/off elaboration of pragmat items");
  printf ("\n   reductions                Print parser reductions");
  printf ("\n   verbose                   Inform on program actions");
  printf ("\n   version                   State the version of the running copy");
  printf ("\n   warnings                  Enable warning messages");
  printf ("\n");
  fflush (stdout);
}

/*!
\brief add an option to the list, to be processed later
\param l
\param str
\param line
**/

void add_option_list (OPTION_LIST_T ** l, char *str, SOURCE_LINE_T * line)
{
  if (*l == NULL) {
    *l = (OPTION_LIST_T *) get_heap_space (SIZE_OF (OPTION_LIST_T));
    (*l)->scan = source_scan;
    (*l)->str = new_string (str);
    (*l)->processed = A_FALSE;
    (*l)->line = line;
    NEXT (*l) = NULL;
  } else {
    add_option_list (&(NEXT (*l)), str, line);
  }
}

/*!
\brief initialise option handler
\param module
**/

void init_options (MODULE_T * module)
{
  options = (OPTIONS_T *) malloc ((size_t) SIZE_OF (OPTIONS_T));
  module->options.list = NULL;
}

/*!
\brief eq
\param module
\param p
\param q
\return
**/

static BOOL_T eq (MODULE_T * module, char *p, char *q)
{
/* Upper case letters in 'q' are mandatory, lower case must match. */
  if (module->options.pragmat_sema) {
    return (match_string (p, q, '='));
  } else {
    return (A_FALSE);
  }
}

/*!
\brief process echoes gathered in the option list
\param module
\param i
**/

void prune_echoes (MODULE_T * module, OPTION_LIST_T * i)
{
  while (i != NULL) {
    if (i->scan == source_scan) {
      char *p = strip_minus (i->str);
/* ECHO echoes a string. */
      if (eq (module, p, "ECHO")) {
	{
	  char *car = strchr (p, '=');
	  if (car != NULL) {
	    io_close_tty_line ();
	    sprintf (output_line, "%s", &car[1]);
	    io_write_string (STDOUT_FILENO, output_line);
	  } else {
	    FORWARD (i);
	    if (i != NULL) {
	      if (strcmp (i->str, "=") == 0) {
		FORWARD (i);
	      }
	      if (i != NULL) {
		io_close_tty_line ();
		sprintf (output_line, "%s", i->str);
		io_write_string (STDOUT_FILENO, output_line);
	      }
	    }
	  }
	}
      }
    }
    FORWARD (i);
  }
}

/*!
\brief process options gathered in the option list
\param p
\param i
\param error
\return
**/

static int fetch_integral (char *p, OPTION_LIST_T ** i, BOOL_T * error)
{
  char *car = NULL, *num = NULL;
  int k, mult = 1;
  *error = A_FALSE;
/* Fetch argument. */
  car = strchr (p, '=');
  if (car == NULL) {
    FORWARD (*i);
    *error = (*i == NULL);
    if (!error && strcmp ((*i)->str, "=") == 0) {
      FORWARD (*i);
      *error = ((*i) == NULL);
    }
    if (!*error) {
      num = (*i)->str;
    }
  } else {
    num = &car[1];
  }
/* Translate argument into integer. */
  if (*error) {
    sprintf (edit_line, "syntax error in option `%s'", (*i)->str);
    scan_error ((*i)->line, edit_line);
    return (0);
  } else {
    char *postfix;
    RESET_ERRNO;
    k = strtol (num, &postfix, 0);	/* Accept also octal and hex. */
    *error = (postfix == num);
    if (errno != 0 || *error) {
      sprintf (edit_line, "syntax error in option `%s'", (*i)->str);
      scan_error ((*i)->line, edit_line);
      *error = A_TRUE;
    } else if (k < 0) {
      sprintf (edit_line, "negative value in option `%s'", (*i)->str);
      scan_error ((*i)->line, edit_line);
      *error = A_TRUE;
    } else {
/* Accept postfix multipliers: 32k, 64M, 1G. */
      if (postfix != NULL) {
	switch (postfix[0]) {
	case '\0':
	  {
	    mult = 1;
	    break;
	  }
	case 'k':
	case 'K':
	  {
	    mult = KILOBYTE;
	    break;
	  }
	case 'm':
	case 'M':
	  {
	    mult = MEGABYTE;
	    break;
	  }
	case 'g':
	case 'G':
	  {
	    mult = GIGABYTE;
	    break;
	  }
	default:
	  {
	    sprintf (edit_line, "syntax error in option `%s'", (*i)->str);
	    scan_error ((*i)->line, edit_line);
	    *error = A_TRUE;
	    break;
	  }
	}
	if (postfix[0] != '\0' && postfix[1] != '\0') {
	  sprintf (edit_line, "syntax error in option `%s'", (*i)->str);
	  scan_error ((*i)->line, edit_line);
	  *error = A_TRUE;
	}
      }
    }
    return (k * mult);
  }
}

/*!
\brief process options gathered in the option list
\param module
\param i
\param cmd_line
\return
**/

BOOL_T set_options (MODULE_T * module, OPTION_LIST_T * i, BOOL_T cmd_line)
{
  BOOL_T go_on = A_TRUE, name_set = A_FALSE;
  OPTION_LIST_T *j = i;
  RESET_ERRNO;
  while (i != NULL && go_on) {
    if (!(i->processed)) {
/* Accept UNIX '-option [=] value' */
      BOOL_T minus_sign = ((i->str)[0] == '-');
      char *p = strip_minus (i->str);
      if (!minus_sign && cmd_line) {
/* Item without '-'s is generic filename. */
	if (!name_set) {
	  module->files.generic_name = new_string (p);
	  name_set = A_TRUE;
	} else {
	  sprintf (edit_line, "option `%s' attempts to reset filename `%s'", i->str, module->files.generic_name);
	  scan_error (NULL, edit_line);
	}
      }
/* Preprocessor items stop option processing. */
      else if (eq (module, p, "INCLUDE")) {
	go_on = A_FALSE;
      } else if (eq (module, p, "READ")) {
	go_on = A_FALSE;
      } else if (eq (module, p, "PREPROCESSOR")) {
	go_on = A_FALSE;
      } else if (eq (module, p, "NOPREPROCESSOR")) {
	go_on = A_FALSE;
      }
/* EXIT stops option processing. */
      else if (eq (module, p, "EXIT")) {
	go_on = A_FALSE;
      }
/* Empty item (from specifying '-' or '--') stops option processing. */
      else if (eq (module, p, "")) {
	go_on = A_FALSE;
      }
/* FILE accepts its argument as generic filename. */
      else if (eq (module, p, "File") && cmd_line) {
	FORWARD (i);
	if (i != NULL && strcmp (i->str, "=") == 0) {
	  FORWARD (i);
	}
	if (i != NULL) {
	  if (!name_set) {
	    module->files.generic_name = new_string (i->str);
	    name_set = A_TRUE;
	  } else {
	    sprintf (edit_line, "option `%s' attempts to reset filename `%s'", i->str, module->files.generic_name);
	    scan_error (i->line, edit_line);
	  }
	} else {
	  sprintf (edit_line, "option `%s' expects a filename", i->str);
	  scan_error (i->line, edit_line);
	}
      }
/* HELP gives online help. */
      else if (eq (module, p, "Help") && cmd_line == A_TRUE) {
	online_help ();
	a68g_exit (EXIT_SUCCESS);
      }
/* ECHO is treated later. */
      else if (eq (module, p, "ECHO")) {
	if (strchr (p, '=') == NULL) {
	  FORWARD (i);
	  if (i != NULL) {
	    if (strcmp (i->str, "=") == 0) {
	      FORWARD (i);
	    }
	  }
	}
      }
/* EXECUTE and PRINT execute their argument as Algol 68 text. */
      else if (eq (module, p, "Execute") || eq (module, p, "Print")) {
	if (cmd_line == A_FALSE) {
	  sprintf (edit_line, "option `%s' only valid from command line", i->str);
	  scan_error (i->line, edit_line);
	} else if ((FORWARD (i)) != NULL) {
	  BOOL_T error = A_FALSE;
	  if (strcmp (i->str, "=") == 0) {
	    error = (FORWARD (i)) == NULL;
	  }
	  if (!error) {
	    char name[BUFFER_SIZE];
	    FILE *f;
	    strcpy (name, ".");
	    strcat (name, A68G_NAME);
	    strcat (name, ".x");
	    f = fopen (name, "w");
	    ABNORMAL_END (f == NULL, "cannot open temp file", NULL);
	    if (eq (module, p, "Execute")) {
	      fprintf (f, "%s\n", i->str);
	    } else {
	      fprintf (f, "(print ((%s)))\n", i->str);
	    }
	    fclose (f);
	    module->files.generic_name = new_string (name);
	  } else {
	    sprintf (edit_line, "syntax error in option `%s'", i->str);
	    scan_error (i->line, edit_line);
	  }
	} else {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	}
      }
/* HEAP, HANDLES, STACK, FRAME and OVERHEAD  set core allocation. */
      else if (eq (module, p, "HEAP") || eq (module, p, "HANDLES") || eq (module, p, "STACK") || eq (module, p, "FRAME") || eq (module, p, "OVERHEAD")) {
	BOOL_T error = A_FALSE;
	int k = fetch_integral (p, &i, &error);
/* Adjust size. */
	if (error || errno > 0) {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	} else if (k > 0) {
	  if (k < MIN_MEM_SIZE) {
	    sprintf (edit_line, "invalid value in option `%s'", i->str);
	    scan_error (i->line, edit_line);
	    k = MIN_MEM_SIZE;
	  }
	  if (eq (module, p, "HEAP")) {
	    heap_size = k;
	  } else if (eq (module, p, "HANDLE")) {
	    handle_pool_size = k;
	  } else if (eq (module, p, "STACK")) {
	    expr_stack_size = k;
	  } else if (eq (module, p, "FRAME")) {
	    frame_stack_size = k;
	  } else if (eq (module, p, "OVERHEAD")) {
	    storage_overhead = k;
	  }
	}
      }
/* BRACKETS extends Algol 68 syntax for brackets. */
      else if (eq (module, p, "BRackets")) {
	module->options.brackets = A_TRUE;
      }
/* REDUCTIONS gives parser reductions.*/
      else if (eq (module, p, "REDuctions")) {
	module->options.reductions = A_TRUE;
      }
/* GNUDIAGNOSTIC gives GNU style diagnostics iso VMS style. */
      else if (eq (module, p, "GNUDiagnostics")) {
	gnu_diags = A_TRUE;
      }
/* QUOTESTROPPING sets stropping to quote stropping. */
      else if (eq (module, p, "QUOTEstropping")) {
	module->options.stropping = QUOTE_STROPPING;
      }
/* UPPERSTROPPING sets stropping to upper stropping, which is nowadays the expected default. */
      else if (eq (module, p, "UPPERstropping")) {
	module->options.stropping = UPPER_STROPPING;
      }
/* CHECK and NORUN just check for syntax. */
      else if (eq (module, p, "Check") || eq (module, p, "NORun")) {
	module->options.check_only = A_TRUE;
      }
/* RUN overrides NORUN. */
      else if (eq (module, p, "RUN")) {
	module->options.run = A_TRUE;
      }
/* REGRESSION is an option that set preferences when running the Algol68G test suite. */
      else if (eq (module, p, "REGRESSION")) {
	module->options.regression_test = A_TRUE;
	gnu_diags = A_FALSE;
	module->options.time_limit = 120;
      }
/* NOWARNINGS switches warnings off. */
      else if (eq (module, p, "NOWarnings")) {
	module->options.no_warnings = A_TRUE;
      }
/* WARNINGS switches warnings on. */
      else if (eq (module, p, "Warnings")) {
	module->options.no_warnings = A_FALSE;
      }
/* PRAGMATS and NOPRAGMATS switch on/off pragmat processing. */
      else if (eq (module, p, "PRagmats")) {
	module->options.pragmat_sema = A_TRUE;
      } else if (eq (module, p, "NOPRagmats")) {
	module->options.pragmat_sema = A_FALSE;
      }
/* VERBOSE in case you want to know what Algol68G is doing. */
      else if (eq (module, p, "VERBose")) {
	module->options.verbose = A_TRUE;
      }
/* VERSION lists the current version at an appropriate time in the future. */
      else if (eq (module, p, "Version")) {
	module->options.version = A_TRUE;
      }
/* XREF and NOXREF switch on/off a cross reference. */
      else if (eq (module, p, "Xref")) {
	module->options.source_listing = A_TRUE;
	module->options.cross_reference = A_TRUE;
	module->options.nodemask |= (CROSS_REFERENCE_MASK | SOURCE_MASK);
      } else if (eq (module, p, "NOXref")) {
	module->options.nodemask &= ~(CROSS_REFERENCE_MASK | SOURCE_MASK);
      }
/* PRELUDELISTING cross references preludes, if they ever get implemented ... */
      else if (eq (module, p, "PRELUDElisting")) {
	module->options.standard_prelude_listing = A_TRUE;
      }
/* STATISTICS prints process statistics. */
      else if (eq (module, p, "STatistics")) {
	module->options.statistics_listing = A_TRUE;
      }
/* TREE and NOTREE switch on/off printing of the syntax tree. This gets bulky! */
      else if (eq (module, p, "TREE")) {
	module->options.source_listing = A_TRUE;
	module->options.tree_listing = A_TRUE;
	module->options.nodemask |= (TREE_MASK | SOURCE_MASK);
      } else if (eq (module, p, "NOTREE")) {
	module->options.nodemask ^= (TREE_MASK | SOURCE_MASK);
      }
/* UNUSED indicates unused tags. */
      else if (eq (module, p, "UNUSED")) {
	module->options.unused = A_TRUE;
      }
/* EXTENSIVE set of options for an extensive listing. */
      else if (eq (module, p, "EXTensive")) {
	module->options.source_listing = A_TRUE;
	module->options.tree_listing = A_TRUE;
	module->options.cross_reference = A_TRUE;
	module->options.moid_listing = A_TRUE;
	module->options.standard_prelude_listing = A_TRUE;
	module->options.statistics_listing = A_TRUE;
	module->options.unused = A_TRUE;
	module->options.nodemask |= (CROSS_REFERENCE_MASK | TREE_MASK | CODE_MASK | SOURCE_MASK);
      }
/* LISTING set of options for a default listing. */
      else if (eq (module, p, "LISTing")) {
	module->options.source_listing = A_TRUE;
	module->options.cross_reference = A_TRUE;
	module->options.statistics_listing = A_TRUE;
	module->options.nodemask |= (SOURCE_MASK | CROSS_REFERENCE_MASK);
      }
/* TTY send listing to standout. Remnant from my mainframe past. */
      else if (eq (module, p, "TTY")) {
	module->options.cross_reference = A_TRUE;
	module->options.statistics_listing = A_TRUE;
	module->options.nodemask |= (SOURCE_MASK | CROSS_REFERENCE_MASK);
      }
/* SOURCE and NOSOURCE print source lines. */
      else if (eq (module, p, "SOURCE")) {
	module->options.source_listing = A_TRUE;
	module->options.nodemask |= SOURCE_MASK;
      } else if (eq (module, p, "NOSOURCE")) {
	module->options.nodemask &= ~SOURCE_MASK;
      }
/* MOIDS prints an overview of moids used in the program. */
      else if (eq (module, p, "MOIDS")) {
	module->options.moid_listing = A_TRUE;
      }
/* ASSERTIONS and NOASSERTIONS switch on/off the processing of assertions. */
      else if (eq (module, p, "Assertions")) {
	module->options.nodemask |= ASSERT_MASK;
      } else if (eq (module, p, "NOAssertions")) {
	module->options.nodemask &= ~ASSERT_MASK;
      }
/* PRECISION sets the precision. */
      else if (eq (module, p, "PRECision")) {
	BOOL_T error = A_FALSE;
	int k = fetch_integral (p, &i, &error);
	if (error || errno > 0) {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	} else if (k > 1) {
	  if (int_to_mp_digits (k) > long_mp_digits ()) {
	    set_longlong_mp_digits (int_to_mp_digits (k));
	  } else {
	    k = 1;
	    while (int_to_mp_digits (k) <= long_mp_digits ()) {
	      k++;
	    }
	    sprintf (edit_line, "value in option `%s' must exceed %d", i->str, long_mp_digits ());
	    scan_error (i->line, edit_line);
	  }
	} else {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	}
      }
/* BREAK and NOBREAK switch on/off tracing of the running program. */
      else if (eq (module, p, "BReakpoint")) {
	module->options.nodemask |= BREAKPOINT_MASK;
      } else if (eq (module, p, "NOBReakpoint")) {
	module->options.nodemask &= ~BREAKPOINT_MASK;
      }
/* TRACE and NOTRACE switch on/off tracing of the running program. */
      else if (eq (module, p, "TRace")) {
	module->options.trace = A_TRUE;
	module->options.nodemask |= TRACE_MASK;
      } else if (eq (module, p, "NOTRace")) {
	module->options.nodemask &= ~TRACE_MASK;
      }
/* TIMELIMIT lets the interpreter stop after so-many seconds. */
      else if (eq (module, p, "TImelimit")) {
	BOOL_T error = A_FALSE;
	int k = fetch_integral (p, &i, &error);
	if (error || errno > 0) {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	} else if (k < 1) {
	  sprintf (edit_line, "syntax error in option `%s'", i->str);
	  scan_error (i->line, edit_line);
	} else {
	  module->options.time_limit = k;
	}
      } else {
/* Unrecognised. */
	sprintf (edit_line, "unrecognised option `%s'", i->str);
	scan_error (i->line, edit_line);
      }
    }
/* Go processing next item, if present. */
    if (i != NULL) {
      FORWARD (i);
    }
  }
/* Mark options as processed. */
  for (; j != NULL; j = NEXT (j)) {
    j->processed = A_TRUE;
  }
  return (errno == 0);
}

/*!
\brief set default core size
**/

void default_mem_sizes (void)
{
#ifdef PRE_MACOS_X_VERSION
/* 8 MB. */
  frame_stack_size = 512 * KILOBYTE;
  expr_stack_size = 512 * KILOBYTE;
  heap_size = 6 * MEGABYTE;
  handle_pool_size = MEGABYTE;
  storage_overhead = 256 * KILOBYTE;
#else
/* 16 MB. */
  frame_stack_size = 2 * MEGABYTE;
  expr_stack_size = MEGABYTE;
  heap_size = 15 * MEGABYTE;
  handle_pool_size = 2 * MEGABYTE;
  storage_overhead = 256 * KILOBYTE;
#endif
}

/*!
\brief set default values for options
\param module
**/

void default_options (MODULE_T * module)
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
  module->options.no_warnings = A_TRUE;
  module->options.unused = A_FALSE;
  module->options.pragmat_sema = A_TRUE;
  module->options.trace = A_FALSE;
  module->options.regression_test = A_FALSE;
  module->options.nodemask = ASSERT_MASK;
  module->options.time_limit = 0;
  module->options.stropping = UPPER_STROPPING;
  module->options.brackets = A_FALSE;
  module->options.reductions = A_FALSE;
  module->options.run = A_FALSE;
  gnu_diags = A_FALSE;
}

/*!
\brief read options from the .rc file
\param module
**/

void read_rc_options (MODULE_T * module)
{
  FILE *f;
  char *name = (char *) get_heap_space (1 + strlen (A68G_NAME) + strlen (".rc"));
  strcpy (name, ".");
  strcat (name, A68G_NAME);
  strcat (name, "rc");
  f = fopen (name, "r");
  if (f != NULL) {
    while (!feof (f)) {
      if (fgets (input_line, BUFFER_SIZE, f) != NULL) {
	if (input_line[strlen (input_line) - 1] == '\n') {
	  input_line[strlen (input_line) - 1] = '\0';
	}
	isolate_options (module, input_line, NULL);
      }
    }
    fclose (f);
    set_options (module, module->options.list, A_FALSE);
  } else {
    errno = 0;
  }
}

/*!
\brief tokenise string 'p' that holds options
\param module
\param p
\param line
**/

void isolate_options (MODULE_T * module, char *p, SOURCE_LINE_T * line)
{
  char *q;
/* 'q' points at first significant char in item .*/
  while (p[0] != '\0') {
/* Skip white space ... */
    while (p[0] == ' ' && p[0] != '\0') {
      p++;
    }
/* ... then tokenise an item. */
    if (p[0] != '\0') {
/* Item can be "string". Note that these are not A68 strings. */
      if (p[0] == '"' || p[0] == '\'' || p[0] == '`') {
	char delim = p[0];
	p++;
	q = p;
	while (p[0] != delim && p[0] != '\0') {
	  p++;
	}
	if (p[0] != '\0') {
	  p[0] = '\0';		/* p[0] was delimiter. */
	  p++;
	} else {
	  scan_error (line, "unterminated string in option");
	}
      } else
/* Item is not a delimited string. */
      {
	q = p;
/* Tokenise symbol and gather it in the option list for later processing.
   Skip '='s, we accept if someone writes -prec=60 -heap=8192. */
	if (*q == '=') {
	  p++;
	} else {
	  /* Skip item. */
	  while (p[0] != ' ' && p[0] != '\0' && p[0] != '=') {
	    p++;
	  }
	}
	if (p[0] != '\0') {
	  p[0] = '\0';
	  p++;
	}
      }
/* 'q' points to first significant char in item, and 'p' points after item. */
      add_option_list (&(module->options.list), q, line);
    }
  }
}
