//! @file plugin-driver.c
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
//! Plugin compiler driver.

#include "a68g.h"


#include "a68g-optimiser.h"
#include "a68g-options.h"
#include "a68g-parser.h"
#include "a68g-plugin.h"
#include "a68g-genie.h"

//! @brief Emit code for plugin compiler.

void plugin_driver_code (void)
{
  if (ERROR_COUNT (&A68_JOB) == 0 && OPTION_OPT_LEVEL (&A68_JOB) > NO_OPTIMISE) {
    announce_phase ("plugin code generator");
    int num = 0;
    renumber_nodes (TOP_NODE (&A68_JOB), &num);
    A68 (node_register) = (NODE_T **) get_heap_space ((size_t) num * sizeof (NODE_T));
    ABEND (A68 (node_register) == NO_VAR, ERROR_ACTION, __func__);
    register_nodes (TOP_NODE (&A68_JOB));
    FILE_OBJECT_FD (&A68_JOB) = open (FILE_OBJECT_NAME (&A68_JOB), O_WRONLY | O_CREAT | O_TRUNC, A68_PROTECTION);
    ABEND (FILE_OBJECT_FD (&A68_JOB) == -1, ERROR_ACTION, FILE_OBJECT_NAME (&A68_JOB));
    FILE_OBJECT_OPENED (&A68_JOB) = A68_TRUE;
    plugin_driver_emit (FILE_OBJECT_FD (&A68_JOB));
    ASSERT (close (FILE_OBJECT_FD (&A68_JOB)) == 0);
    FILE_OBJECT_OPENED (&A68_JOB) = A68_FALSE;
  }
}

//! @brief Compile emitted code.

void plugin_driver_compile (void)
{
#if defined (BUILD_A68_COMPILER)
//
// Compilation on Linux, BSD.
// Build shared library using gcc or clang.
// TODO: One day this should be all portable between platforms.
//
// Only compile if the A68 compiler found no errors (constant folder for instance).
//
  if (ERROR_COUNT (&A68_JOB) == 0 && OPTION_OPT_LEVEL (&A68_JOB) > 0 && !OPTION_RUN_SCRIPT (&A68_JOB)) {
    BUFFER cmd, options;
    BUFCLR (cmd);
    BUFCLR (options);
    if (OPTION_RERUN (&A68_JOB) == A68_FALSE) {
      announce_phase ("plugin compiler");
      errno = 0;
      ASSERT (snprintf (options, SNPRINTF_SIZE, "%s %s", optimisation_option (), A68_GCC_OPTIONS) >= 0);
#if defined (HAVE_PIC)
      bufcat (options, " ", BUFFER_SIZE);
      bufcat (options, HAVE_PIC, BUFFER_SIZE);
#endif
      ASSERT (snprintf (cmd, SNPRINTF_SIZE, "%s -I%s %s -c -o \"%s\" \"%s\"", C_COMPILER, INCLUDEDIR, options, FILE_BINARY_NAME (&A68_JOB), FILE_OBJECT_NAME (&A68_JOB)) >= 0);
      ABEND (system (cmd) != 0, ERROR_ACTION, cmd);
      ASSERT (snprintf (cmd, SNPRINTF_SIZE, "ld -export-dynamic -shared -o \"%s\" \"%s\"", FILE_PLUGIN_NAME (&A68_JOB), FILE_BINARY_NAME (&A68_JOB)) >= 0);
      ABEND (system (cmd) != 0, ERROR_ACTION, cmd);
      a68_rm (FILE_BINARY_NAME (&A68_JOB));
    }
  }
#endif
}

//! @brief Start interpreter with compiled plugin.

void plugin_driver_genie (void)
{
#if defined (BUILD_A68_COMPILER)
  if (OPTION_RUN_SCRIPT (&A68_JOB)) {
    rewrite_script_source ();
  }
  void *compile_plugin = NULL;
  if (OPTION_OPT_LEVEL (&A68_JOB) > 0) {
    char plugin_name[BUFFER_SIZE];
    void *a68_plugin;
    struct stat srcstat, objstat;
    int ret;
    announce_phase ("plugin dynamic linker");
    ASSERT (snprintf (plugin_name, SNPRINTF_SIZE, "%s", FILE_PLUGIN_NAME (&A68_JOB)) >= 0);
// Correction when pwd is outside LD_PLUGIN_PATH.
// The DL cannot be loaded if it is.
    if (strcmp (plugin_name, a68_basename (plugin_name)) == 0) {
      ASSERT (snprintf (plugin_name, SNPRINTF_SIZE, "./%s", FILE_PLUGIN_NAME (&A68_JOB)) >= 0);
    }
// Check whether we are doing something rash.
    ret = stat (FILE_SOURCE_NAME (&A68_JOB), &srcstat);
    ABEND (ret != 0, ERROR_ACTION, FILE_SOURCE_NAME (&A68_JOB));
    ret = stat (plugin_name, &objstat);
    ABEND (ret != 0, ERROR_ACTION, plugin_name);
    if (OPTION_RERUN (&A68_JOB)) {
      ABEND (ST_MTIME (&srcstat) > ST_MTIME (&objstat), "plugin outdates source", "cannot RERUN");
    }
// First load a68g itself so compiler code can resolve a68g symbols.
    a68_plugin = dlopen (NULL, RTLD_NOW | RTLD_GLOBAL);
    ABEND (a68_plugin == NULL, ERROR_CANNOT_OPEN_PLUGIN, dlerror ());
// Then load compiler code.
    compile_plugin = dlopen (plugin_name, RTLD_NOW | RTLD_GLOBAL);
    ABEND (compile_plugin == NULL, ERROR_CANNOT_OPEN_PLUGIN, dlerror ());
  }
  genie (compile_plugin);
// Unload compiler plugin.
  if (compile_plugin != (void *) NULL) {
    int ret = dlclose (compile_plugin);
    ABEND (ret != 0, ERROR_ACTION, dlerror ());
  }
#endif
}

//! @brief Clean files from plugin compiler.

void plugin_driver_clean (int emitted)
{
#if defined (BUILD_A68_COMPILER)
  announce_phase ("clean up intermediate files");
  if (OPTION_OPT_LEVEL (&A68_JOB) >= OPTIMISE_0 && OPTION_REGRESSION_TEST (&A68_JOB) && !OPTION_KEEP (&A68_JOB)) {
    if (emitted) {
      a68_rm (FILE_OBJECT_NAME (&A68_JOB));
    }
    a68_rm (FILE_PLUGIN_NAME (&A68_JOB));
  }
  if (OPTION_RUN_SCRIPT (&A68_JOB) && !OPTION_KEEP (&A68_JOB)) {
    if (emitted) {
      a68_rm (FILE_OBJECT_NAME (&A68_JOB));
    }
    a68_rm (FILE_SOURCE_NAME (&A68_JOB));
    a68_rm (FILE_PLUGIN_NAME (&A68_JOB));
  } else if (OPTION_COMPILE (&A68_JOB)) {
    build_script ();
    if (!OPTION_KEEP (&A68_JOB)) {
      if (emitted) {
        a68_rm (FILE_OBJECT_NAME (&A68_JOB));
      }
      a68_rm (FILE_PLUGIN_NAME (&A68_JOB));
    }
  } else if (OPTION_OPT_LEVEL (&A68_JOB) == OPTIMISE_0 && !OPTION_KEEP (&A68_JOB)) {
    if (emitted) {
      a68_rm (FILE_OBJECT_NAME (&A68_JOB));
    }
    a68_rm (FILE_PLUGIN_NAME (&A68_JOB));
  } else if (OPTION_OPT_LEVEL (&A68_JOB) > OPTIMISE_0 && !OPTION_KEEP (&A68_JOB)) {
    if (emitted) {
      a68_rm (FILE_OBJECT_NAME (&A68_JOB));
    }
  } else if (OPTION_RERUN (&A68_JOB) && !OPTION_KEEP (&A68_JOB)) {
    if (emitted) {
      a68_rm (FILE_OBJECT_NAME (&A68_JOB));
    }
  }
#else
  (void) emitted;
#endif
}
