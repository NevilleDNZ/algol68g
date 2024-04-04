#
# Algol 68 Genie 3.4.4, configure.ac
#
# To configure a68g for the widest range of platforms, issue: 
#   ./configure --enable-generic
#
# Following optional libraries are suggested for a full build on debian or derived linux distributions (Ubuntu, Raspberry Pi):
#   sudo apt install libncurses-dev libreadline-dev libmpfr libgmp libgsl-dev r-mathlib libplot-dev libghc-postgresql-libpq-dev
#

AC_INIT([algol68g], [3.4.4], [Marcel van der Veer <algol68g@xs4all.nl>])
AC_PREREQ([2.60])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([AC_PACKAGE_NAME-AC_PACKAGE_VERSION])
AC_MSG_NOTICE([])

# Check whether compiler supports $1 as a command-line option. 
# If it does, add the string to CFLAGS.

AC_DEFUN([A68G_AC_PROG_CC_CFLAGS], 
  [AC_MSG_CHECKING([whether $CC accepts $1])
  a68g_ac_save_CFLAGS=$CFLAGS
  CFLAGS="$a68g_ac_save_CFLAGS $1"
  _AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], 
    AC_MSG_RESULT(yes), 
    [CFLAGS="$a68g_ac_save_CFLAGS"
    AC_MSG_RESULT(no)])
])

#
# Platform ids.
#

AC_MSG_NOTICE([select host system...])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

a68g_silicon_mac=no
bsd_include_dirs=no

AC_MSG_CHECKING([platform])
case "$host" in
#
# Linux.
#
# aarch64*-*-linux* is for RaspberryPi-4 on ARM-64 - otherwise http/tcp isnt found
# Generic *-*-linux-gnu catches for instance s390x-ibm-linux-gnu
#

*86-*-gnu | *86_64-*-gnu | *86-*-linux* | *86_64-*-linux* | arm*-*-linux* | aarch*-*-linux* | *-*-linux-gnu)
   AC_DEFINE(BUILD_LINUX, 1, [Define this if LINUX was detected]) 
   AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant])
   AC_MSG_RESULT([linux])
  ;;

#
# Cygwin.
#

*86-*-cygwin* | *86_64-*-cygwin*) 
  AC_DEFINE(BUILD_CYGWIN, 1, [Define this if CYGWIN was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant])
  AC_MSG_RESULT([cygwin])
  ;;
  
#
# MacOS.
#

*86-*-*darwin* | *86_64-*-*darwin* | aarch64-*-*darwin*)
  AC_DEFINE(BUILD_BSD, 1, [Define this if DARWIN was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant]) 
  AC_MSG_RESULT([macOS])
  ;;

#
# FreeBSD.
#

*86-*-freebsd* | *86_64-*-freebsd*) 
  AC_DEFINE(BUILD_BSD, 1, [Define this if FreeBSD was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant]) 
  AC_MSG_RESULT([freebsd])
  ;;

#
# NetBSD.
#

*86-*-netbsd* | *86_64-*-netbsd*) 
  AC_DEFINE(BUILD_BSD, 1, [Define this if NetBSD was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant]) 
  AC_MSG_RESULT([netbsd])
  ;;

#
# OpenBSD.
#

*86-*-openbsd* | *86_64-*-openbsd*) 
  AC_DEFINE(BUILD_BSD, 1, [Define this if OpenBSD was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant]) 
  bsd_include_dirs=yes
  AC_MSG_RESULT([openbsd])
  ;;
  
#
# Haiku.
#

*86-*-haiku* | *86_64-*-haiku*) 
  AC_DEFINE(HAVE_UNTESTED, 1, [Define this if untested OS was detected]) 
  AC_MSG_WARN([untested platform])
  AC_DEFINE(BUILD_HAIKU, 1, [Define this if HAIKU was detected]) 
  AC_DEFINE(HAVE_IEEE_754, 1, [Define this if IEEE_754 compliant]) 
  enable_stable=yes
  AC_MSG_RESULT([haiku])
  ;;
  
#
# Others, untested.
#

*)
  AC_DEFINE(HAVE_UNTESTED, 1, [Define this if untested OS was detected]) 
  AC_MSG_WARN([untested platform])
  AC_MSG_RESULT([interpreter-only])
  enable_stable=yes
  ;;
esac

#
# Extra options.
#

AC_ARG_ENABLE(arch, [AS_HELP_STRING([--enable-arch=cpu], [if using gcc, enable emitting architecture-tuned assembly code (default is "no")])], , enable_arch=no) 
AC_ARG_ENABLE(clang, [AS_HELP_STRING([--enable-clang], [prefer clang over gcc if both are installed (default is "no")])], , enable_clang=no) 
AC_ARG_ENABLE(compiler, [AS_HELP_STRING([--enable-compiler], [enable plugin compiler (default is "yes")])], , enable_compiler=yes) 
AC_ARG_ENABLE(core, [AS_HELP_STRING([--enable-core], [build without optional libraries or plugin compiler (default is "no")])], , enable_core=no) 
AC_ARG_ENABLE(curl, [AS_HELP_STRING([--enable-curl], [enable fetching web page contents (default is "yes")])], , enable_curl="yes") 
AC_ARG_ENABLE(curses, [AS_HELP_STRING([--enable-curses], [enable curses library (default is "yes")])], , enable_curses=yes) 
AC_ARG_ENABLE(generic, [AS_HELP_STRING([--enable-generic], [build for non-GNU systems (default is "no")])], , enable_generic=no) 
AC_ARG_ENABLE(gsl, [AS_HELP_STRING([--enable-gsl], [enable GNU scientific library (default is "yes")])], , enable_gsl=yes) 
AC_ARG_ENABLE(long-types, [AS_HELP_STRING([--enable-long-types], [enable int*8 and real*10, real*16 modes (default is "yes")])], , enable_long_types=yes) 
AC_ARG_ENABLE(mathlib, [AS_HELP_STRING([--enable-mathlib], [enable R mathlib library (default is "yes")])], , enable_mathlib=yes) 
AC_ARG_ENABLE(mpfr, [AS_HELP_STRING([--enable-mpfr], [enable GNU MPFR (default is "yes")])], , enable_mpfr=yes) 
AC_ARG_ENABLE(parallel, [AS_HELP_STRING([--enable-parallel], [enable Algol 68 parallel-clause (default is "yes")])], , enable_parallel=yes) 
AC_ARG_ENABLE(pic, [AS_HELP_STRING([--enable-pic=option], [if using gcc, enable option to generate PIC (default is "-fPIC")])], , enable_pic="-fPIC") 
AC_ARG_ENABLE(plotutils, [AS_HELP_STRING([--enable-plotutils], [enable GNU plotting utilities (default is "yes")])], , enable_plotutils=yes) 
AC_ARG_ENABLE(postgresql, [AS_HELP_STRING([--enable-postgresql], [enable PostgreSQL (default is "yes")])], , enable_postgresql=yes) 
AC_ARG_ENABLE(quadmath, [AS_HELP_STRING([--enable-quadmath], [enable GCC quadmath library (default is "yes")])], , enable_quadmath=yes)
AC_ARG_ENABLE(readline, [AS_HELP_STRING([--enable-readline], [enable readline library (default is "yes")])], , enable_readline=yes) 
AC_ARG_ENABLE(stable, [AS_HELP_STRING([--enable-stable], [conservative build without optional libraries (default is "no")])], , enable_stable=no) 
AC_ARG_ENABLE(standard-types, [AS_HELP_STRING([--enable-standard-types], [enable int*4 and real*8 modes (default is "yes")])], , enable_standard_types=yes) 
AC_ARG_ENABLE(www, [AS_HELP_STRING([--enable-www], [enable fetching web page contents (default is "yes")])], , enable_www="yes") 

AC_CHECK_PROG(sed_exists, sed, "yes", "no")
AC_CHECK_PROG(tar_exists, tar, "yes", "no")

if test "x$sed_exists" = "xno"; then
  AC_MSG_NOTICE([plugin compiler needs sed])
  enable_compiler=no
fi

if test "x$tar_exists" = "xno"; then
  AC_MSG_NOTICE([plugin compiler needs tar])
  enable_compiler=no
fi

#
# Initialisation.
#

AM_INIT_AUTOMAKE([subdir-objects serial-tests])
AC_PREFIX_DEFAULT(/usr/local)
AC_CONFIG_SRCDIR([src/include/a68g.h])
AC_CONFIG_HEADERS([a68g-config.h])
AC_PROG_INSTALL

#
# C compiler.
#

AC_MSG_NOTICE([select C compiler...])
AC_LANG(C)

if test "x$enable_clang" = "xyes"; then
  a68g_compilers="clang gcc"
else
  a68g_compilers="gcc clang"
fi

AC_PROG_CC([$a68g_compilers])
if test "x$GCC" != "xyes"; then
  AC_MSG_ERROR([no gcc compatible C compiler])
else
# On rhel8 "rpmbuild" sneaks in -pie and -fPIE, so a68g also needs to pass through.
# On Suse15 the header files for pgsql and R have their own directories.
  COPTFLAGS="-g -O2 -Wall -I /usr/include/pgsql -I /usr/lib64/R/include"
  LDOPTFLAG="-L /usr/lib64/R/lib"
# from /usr/lib/rpm/redhat/macros
  CFLAGS="${CFLAGS:-$COPTFLAGS}" ; export CFLAGS
  LDFLAGS="${LDFLAGS:-$LDOPTFLAGS}" ; export LDFLAGS
  A68G_AC_PROG_CC_CFLAGS([--std=c17])
  A68G_AC_PROG_CC_CFLAGS([-Wall])
  A68G_AC_PROG_CC_CFLAGS([-Wshadow])
  A68G_AC_PROG_CC_CFLAGS([-Wunused-variable])
  A68G_AC_PROG_CC_CFLAGS([-Wunused-parameter])
  A68G_AC_PROG_CC_CFLAGS([-Wno-long-long])
# A68G_AC_PROG_CC_CFLAGS([-Wimplicit-function-declaration])
# A68G_AC_PROG_CC_CFLAGS([-fno-diagnostics-color])

#
# Test on gcc capabilities.
#

  AC_MSG_CHECKING([__attribute__((aligned())) supported])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [typedef int aint __attribute__((aligned(8)));])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   AC_MSG_FAILURE([stop -- C compiler does not support __attribute__aligned directive])],
  [])
  AC_C_INLINE()

#
# Check -Wl,--export-dynamic or -export_dynamic, needed for creating shared objects.
# If either is supported, add to LDFLAGS
#
# GNU ld and LLVM lld accept --export-dynamic, OS X ld accepts -export_dynamic.
#
# Test for -export_dynamic on Darwin platforms only.
# Platforms like OpenBSD interpret this as -e xport_dynamic which is not intended.
#
# Check whether we can link to a particular function and run.
#

# Generic case first.

  a68g_ac_arg="-Wl,--export-dynamic"
  a68g_ac_save_LDFLAGS=$LDFLAGS
  AC_MSG_CHECKING([whether $CC accepts $a68g_ac_arg])
  LDFLAGS="$a68g_ac_save_LDFLAGS $a68g_ac_arg"
  AC_RUN_IFELSE([AC_LANG_PROGRAM([extern void exit (); void (*fptr) () = exit;],[])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_EXPORT_DYNAMIC, 1, [Define this if EXPORT_DYNAMIC is recognised])
     AC_DEFINE(EXPORT_DYNAMIC_FLAGS, "--export-dynamic -shared", [Define link loader flags])
    ],
    [AC_MSG_RESULT(no)
     LDFLAGS=$a68g_ac_save_LDFLAGS
    ],
    [AC_MSG_RESULT(assuming no)
     LDFLAGS=$a68g_ac_save_LDFLAGS
    ]
  )

# Extra check, targetting (Silicon) Macs.

  case "$host" in

    *86-*-*darwin* | *86_64-*-*darwin* | aarch64-*-*darwin*)

      a68g_ac_arg="-Wl,-export_dynamic"
      AC_MSG_CHECKING([whether $CC accepts $a68g_ac_arg])
      a68g_ac_save_LDFLAGS=$LDFLAGS
      LDFLAGS="$a68g_ac_save_LDFLAGS $a68g_ac_arg"
      AC_RUN_IFELSE([AC_LANG_PROGRAM([extern void exit (); void (*fptr) () = exit;],[])],
        [AC_MSG_RESULT(yes)
         AC_DEFINE(HAVE_EXPORT_DYNAMIC, 1, [Define this if EXPORT_DYNAMIC is recognised])
  
         # On x86 Darwin we need to have Xcode tools installed and libSystem.dylib available for the plugin compiler
         case "$host" in
           *86-*-*darwin* | *86_64-*-*darwin*)
             AC_DEFINE(EXPORT_DYNAMIC_FLAGS, "-export_dynamic -undefined dynamic_lookup -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem", [Define link loader flags])
             ;;
           aarch64-*-*darwin*)  
             AC_DEFINE(EXPORT_DYNAMIC_FLAGS, "-export_dynamic -undefined dynamic_lookup", [Define link loader flags])
             ;;
         esac
        ],
        [AC_MSG_RESULT(no)
         LDFLAGS=$a68g_ac_save_LDFLAGS
        ],
        [AC_MSG_RESULT(assuming no)
         LDFLAGS=$a68g_ac_save_LDFLAGS
        ]
      )
      ;;

  esac
	
  # AC_MSG_NOTICE($LDFLAGS)

fi

#
# Optionally, tune for a specific processor.
#

if test "x$enable_arch" != "xno"; then
  AC_MSG_CHECKING([whether $CC accepts -march=$enable_arch])
  a68g_ac_save_CFLAGS=$CFLAGS
  a68g_ac_march="-march=$enable_arch"
  CFLAGS="$a68g_ac_save_CFLAGS $a68g_ac_march"
  _AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], 
    [AC_MSG_RESULT(yes)
     AC_DEFINE_UNQUOTED(HAVE_TUNING, "$a68g_ac_march", [Define this if user wants to tune for a specific CPU]) 
    ], 
    [AC_MSG_RESULT(no)
     AC_MSG_WARN([your CPU name is not accepted; resetting to default])
     CFLAGS="$a68g_ac_save_CFLAGS"
    ]
  )
fi

#
# Some platforms want another or no PIC option.
#

if test "x$enable_compiler" = "xyes"; then
  if test "x$enable_pic" != "xno"; then
    AC_MSG_CHECKING([whether $CC accepts $enable_pic])
    a68g_ac_save_CFLAGS=$CFLAGS
    CFLAGS="$a68g_ac_save_CFLAGS $enable_pic"
    _AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], 
      [AC_MSG_RESULT(yes)
       AC_DEFINE_UNQUOTED(HAVE_PIC, "$enable_pic", [Define this as PIC option]) 
      ], 
      [AC_MSG_RESULT(no)
       AC_MSG_WARN([your PIC option is not accepted; configuring interpreter-only])
      ]
    )
    CFLAGS="$a68g_ac_save_CFLAGS"
  fi
fi

AM_PROG_CC_C_O
AC_PROG_CPP
AC_MSG_NOTICE([check types...])
AC_C_CHAR_UNSIGNED
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T

AC_MSG_CHECKING([__off_t or off_t])
AC_COMPILE_IFELSE(
[AC_LANG_SOURCE([
#include <unistd.h>
#include <sys/types.h>
__off_t dummy;
])],
  [AC_MSG_RESULT([__off_t])],
  [AC_MSG_RESULT([off_t])
   AC_DEFINE(__off_t, off_t, [Define this if we have no __off_t])]
)

AC_MSG_CHECKING([__pid_t or pid_t])
AC_COMPILE_IFELSE(
[AC_LANG_SOURCE[
#include <unistd.h>
#include <sys/types.h>
__pid_t dummy;
])],
  [AC_MSG_RESULT([__pid_t])
   AC_DEFINE(a68_pid_t, __pid_t, [Define this if we have __pid_t])],
  [AC_MSG_RESULT([pid_t])
   AC_DEFINE(a68_pid_t, pid_t, [Define this if we have no __pid_t])]
)

AC_MSG_CHECKING([__mode_t or mode_t])
AC_COMPILE_IFELSE(
[AC_LANG_SOURCE([
#include <unistd.h>
#include <sys/types.h>
__mode_t dummy;
])],
  [AC_MSG_RESULT([__mode_t])],
  [AC_MSG_RESULT([mode_t])
   AC_DEFINE(__mode_t, mode_t, [Define this if we have no __mode_t])]
)

#
# Test on glibc
#

AC_MSG_CHECKING([GNU C library])
AC_RUN_IFELSE([AC_LANG_PROGRAM([], [(void) gnu_get_libc_version();])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_GNU_C_LIB, 1, [Define this if GNU C library is detected.])
  ],
  [AC_MSG_RESULT(no)
   enable_generic=yes
  ],
  [AC_MSG_RESULT(assuming no)
   enable_generic=yes
  ]
)

#
# We know about the platform now.
#

if test "x$enable_stable" = "xyes"; then
  enable_generic=yes
  enable_core=yes
fi

if test "x$enable_generic" = "xyes"; then
  enable_standard_types=yes
  enable_long_types=no
fi

if test "x$enable_core" = "xyes"; then
  enable_compiler=no
  enable_curl=no
  enable_curses=no
  enable_gsl=no
  enable_mathlib=no
  enable_mpfr=no
  enable_parallel=no
  enable_plotutils=no
  enable_postgresql=no
  enable_readline=no
  enable_www=no
fi

#
# Extra include directories.
#

AC_MSG_NOTICE([check extra include directories...])

EXTRA_INCLUDES=""

# Set -I/usr/local/include for *BSD

if test "x$bsd_include_dirs" = "xyes"; then
  AC_MSG_NOTICE([... /usr/local/include])
  EXTRA_INCLUDES="-I/usr/local/include"
fi

# Homebrew on macOS

if test -d /opt/homebrew; then
  AC_MSG_NOTICE([... /opt/homebrew])
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/opt/homebrew/include"
  CFLAGS="$CFLAGS -I/opt/homebrew/include"
  CPPFLAGS="$CPPFLAGS -I/opt/homebrew/include"
  CXXFLAGS="$CXXFLAGS -I/opt/homebrew/include"
  LDFLAGS="$LDFLAGS -L/opt/homebrew/lib"
fi

# MacPorts on macOS

if test -d /opt/local; then
  AC_MSG_NOTICE([... /opt/local])
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/opt/local/include"
  CFLAGS="$CFLAGS -I/opt/local/include"
  CPPFLAGS="$CPPFLAGS -I/opt/local/include"
  CXXFLAGS="$CXXFLAGS -I/opt/local/include"
  LDFLAGS="$LDFLAGS -L/opt/local/lib"
fi

# GSL on OpenBSD

if test -d /usr/local/include/gsl; then
  AC_MSG_NOTICE([... /usr/local/include/gsl])
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/usr/local/include"
  CFLAGS="$CFLAGS -I/usr/local/include"
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
  CXXFLAGS="$CXXFLAGS -I/usr/local/include"
  LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi

# R on Debian
# 
# if test -d /usr/share/R/include; then
#   AC_MSG_NOTICE([... /usr/share/R/include])
#   AC_DEFINE(HAVE_USR_SHARE_R_INCLUDE, 1, [Define this if /usr/share/R/include was detected]) 
#   EXTRA_INCLUDES="$EXTRA_INCLUDES -I/usr/share/R/include"
#   CFLAGS="$CFLAGS -I/usr/share/R/include"
#   CPPFLAGS="$CPPFLAGS -I/usr/share/R/include"
#   CXXFLAGS="$CXXFLAGS -I/usr/share/R/include"
#   LDFLAGS="$LDFLAGS -L/usr/share/R/lib"
# fi

# Postgresql

if test -d /usr/local/pgsql/include; then
  AC_MSG_NOTICE([... /usr/local/pgsql/include])
  AC_DEFINE(HAVE_USR_LOCAL_PGSQL_INCLUDE, 1, [Define this if /usr/local/pgsql/include was detected]) 
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/usr/local/pgsql/include"
  CFLAGS="$CFLAGS -I/usr/local/pgsql/include"
  CPPFLAGS="$CPPFLAGS -I/usr/local/pgsql/include"
  CXXFLAGS="$CXXFLAGS -I/usr/local/pgsql/include"
  LDFLAGS="$LDFLAGS -L/usr/local/pgsql/lib"
fi

if test -d /usr/pkg/pgsql/include; then
  AC_MSG_NOTICE([... /usr/pkg/pgsql/include])
  AC_DEFINE(HAVE_USR_PKG_PGSQL_INCLUDE, 1, [Define this if /usr/pkg/pgsql/include was detected]) 
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/usr/pkg/pgsql/include"
  CFLAGS="$CFLAGS -I/usr/pkg/pgsql/include"
  CPPFLAGS="$CPPFLAGS -I/usr/pkg/pgsql/include"
  CXXFLAGS="$CXXFLAGS -I/usr/pkg/pgsql/include"
  LDFLAGS="$LDFLAGS -L/usr/pkg/pgsql/lib"
fi

if test -d /opt/local/pgsql/include; then
  AC_MSG_NOTICE([... /opt/local/pgsql/include])
  AC_DEFINE(HAVE_OPT_LOCAL_PGSQL_INCLUDE, 1, [Define this if /opt/local/pgsql/include was detected]) 
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/opt/local/pgsql/include"
  CFLAGS="$CFLAGS -I/opt/local/pgsql/include"
  CPPFLAGS="$CPPFLAGS -I/opt/local/pgsql/include"
  CXXFLAGS="$CXXFLAGS -I/opt/local/pgsql/include"
  LDFLAGS="$LDFLAGS -L/opt/local/pgsql/lib"
fi

if test -d /usr/include/postgresql; then
  AC_MSG_NOTICE([... /usr/include/postgresql])
  AC_DEFINE(HAVE_USR_INCLUDE_POSTGRESQL, 1, [Define this if /usr/include/postgresql was detected]) 
  EXTRA_INCLUDES="$EXTRA_INCLUDES -I/usr/include/postgresql"
  CFLAGS="$CFLAGS -I/usr/include/postgresql"
  CPPFLAGS="$CPPFLAGS -I/usr/include/postgresql"
  CXXFLAGS="$CXXFLAGS -I/usr/include/postgresql"
  LDFLAGS="$LDFLAGS -L/usr/lib"
fi

AC_DEFINE_UNQUOTED(INCLUDE_DIR, "$EXTRA_INCLUDES", [Platform dependent])

#
# Checks for header files.
#

AC_MSG_NOTICE([check standard header files...])

#
# <math.h> test is GSL proof.
#

AC_CHECK_HEADERS([math.h])
AC_CHECK_LIB([m], [cos])

AC_MSG_CHECKING([whether math.h has signam])
AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <math.h>], [return signgam;])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_MATH_H_SIGNGAM, 1, [Define this if math.h includes POSIX's signgam.])
  ],
  [AC_MSG_RESULT(no)
  ],
  [AC_MSG_RESULT(assuming no)
  ]
)

AC_HEADER_ASSERT
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ

AC_CHECK_HEADERS([assert.h complex.h ctype.h endian.h errno.h execinfo.h fcntl.h fenv.h float.h libgen.h limits.h regex.h setjmp.h signal.h stdarg.h stddef.h stdio.h stdlib.h sys/ioctl.h sys/resource.h sys/time.h termios.h time.h unistd.h])

#
# Functions we expect.
#

AC_MSG_NOTICE([check standard functions...])
AC_CHECK_FUNCS(aligned_alloc)
AC_CHECK_FUNCS(basename)
AC_CHECK_FUNCS(csqrt)
AC_CHECK_FUNCS(exit)
AC_CHECK_FUNCS(fprintf)
AC_CHECK_FUNCS(free)
AC_CHECK_FUNCS(longjmp)
AC_CHECK_FUNCS(malloc)
AC_CHECK_FUNCS(memcpy)
AC_CHECK_FUNCS(memmove)
AC_CHECK_FUNCS(memset)
AC_CHECK_FUNCS(posix_memalign)
AC_CHECK_FUNCS(printf)
AC_CHECK_FUNCS(setjmp)
AC_CHECK_FUNCS(signal)
AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(sqrt)
AC_CHECK_FUNCS(strcmp)
AC_CHECK_FUNCS(strncmp)
AC_CHECK_FUNCS(strncpy)

#
# Checks for functions / macros.
#

AC_MSG_CHECKING([for isfinite])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include <math.h>], [int a = isfinite(0.0)])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_ISFINITE, 1, [Define if isfinite() is available])], 
    [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for finite])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include <math.h>], [int a = finite(0.0)])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_FINITE, 1, [Define if finite() is available])], 
    [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for isinf])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include <math.h>], [int a = isinf(0.0)])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_ISINF, 1, [Define if isinf() is available])], 
    [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for isnan])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include <math.h>], [int a = isnan(0.0)])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_ISNAN, 1, [Define if isnan() is available])], 
    [AC_MSG_RESULT(no)]
)

AC_MSG_NOTICE([select optional headers and libraries...])

if test "x$enable_standard_types" = "xyes"; then
  AC_MSG_CHECKING([int is 32 bit])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (int) != 4;])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   enable_long_types=no],
  [])
  AC_MSG_CHECKING([unsigned is 32 bit])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (unsigned) != 4;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_long_types=no],
  [])
  AC_MSG_CHECKING([uint64_t is 64 bit])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdint.h>], [return sizeof (uint64_t) != 8;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_standard_types=no
     enable_long_types=no],
  [])
  AC_MSG_CHECKING([double is 64 bit])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (double) != 8;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_long_types=no],
  [])
  AC_MSG_CHECKING([long double has 64-bit mantissa])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <float.h>], [return LDBL_MANT_DIG != 64;])],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_FLT80, 1, [Define this if long double has a 64-bit mantissa])],
    [AC_MSG_RESULT(no)],
  [])
fi

if test "x$enable_standard_types" = "xno"; then
  AC_MSG_FAILURE([stop -- unexpected lengths for int and/or double and/or uint64_t])
fi

if test "x$enable_long_types" = "xyes"; then
  AC_MSG_CHECKING([64-bit long long int is available])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (long long) != 8;])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   enable_long_types=no],
  [])
  AC_MSG_CHECKING([64-bit long long unsigned is available])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (long long unsigned) != 8;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_long_types=no],
  [])
  AC_MSG_CHECKING([__float128 is available])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([], [return sizeof (__float128) != 16;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_long_types=no],
  [])
  AC_MSG_CHECKING([__complex128 is available])
  AC_RUN_IFELSE([AC_LANG_PROGRAM([typedef _Complex float __attribute__((mode(TC))) __complex128;], [return sizeof (__complex128) != 32;])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     enable_long_types=no],
  [])
fi

if test "x$enable_long_types" = "xyes"; then
  if test "x$enable_quadmath" = "xyes"; then
    AC_MSG_NOTICE([quadmath...])
    AC_CHECK_HEADERS([quadmath.h], [], [enable_quadmath=no], [])
    AC_CHECK_LIB([quadmath], [lgammaq], [], [enable_quadmath=no])
    if test "x$enable_quadmath" = "xyes"; then
      AC_DEFINE(HAVE_QUADMATH, 1, [Define this if a good quadmath installation was detected])
    else
      enable_long_types=no
    fi
  fi
else
  AC_MSG_NOTICE([quadmath ignored for this system])
  enable_quadmath=no
  enable_long_types=no
fi

if test "x$enable_long_types" = "xyes"; then
  if test "x$enable_mpfr" = "xyes"; then
    AC_MSG_NOTICE([GNU MPFR...])
    AC_CHECK_LIB([gmp], [__gmpz_init], [], [enable_mpfr=no])
    if test "x$enable_mpfr" = "xyes"; then
      if test "x$CC" = "xclang"; then
        AC_MSG_NOTICE([GNU MPFR ignored for $CC])
        enable_mpfr=no
      else
        AC_CHECK_HEADERS([mpfr.h])
        if test "x$enable_mpfr" = "xyes"; then
          AC_CHECK_LIB([mpfr], [mpfr_gamma], [], [enable_mpfr=no])
          AC_CHECK_LIB([mpfr], [mpfr_gamma_inc], [], [enable_mpfr=no])
          AC_CHECK_LIB([mpfr], [mpfr_get_float128], [], [enable_mpfr=no])
          AC_CHECK_LIB([mpfr], [mpfr_set_float128], [], [enable_mpfr=no])
          if test "x$enable_mpfr" = "xyes"; then
            AC_DEFINE(HAVE_GNU_MPFR, 1, [Define this if a recent GNU MPFR installation was detected])
          fi
        fi
      fi
    fi
  fi
else
  AC_MSG_NOTICE([GNU MPFR ignored])
  enable_mpfr=no
fi

if test "x$enable_long_types" = "xyes"; then
  if test "x$enable_long_types" = "xyes"; then
    AC_DEFINE(HAVE_LONG_TYPES, 1, [Define this if a good INT*8/REAL*10/REAL*16 installation was detected]) 
  fi
fi

if test "x$enable_parallel" = "xyes"; then
  AC_MSG_NOTICE([check POSIX pthreads...])
  AC_CHECK_HEADERS([pthread.h], [], [enable_parallel=no], [])
  AC_CHECK_LIB([pthread], [pthread_attr_getstacksize], [], [enable_parallel=no])
  if test "x$enable_parallel" = "xyes"; then
    AC_DEFINE(BUILD_PARALLEL_CLAUSE, 1, [Define this if a good pthread installation was detected]) 
  fi
fi

if test "x$enable_mathlib" = "xyes"; then
  AC_MSG_NOTICE([check mathlib...])
  AC_CHECK_HEADERS([Rmath.h], [], [enable_mathlib=no], 
    [#define MATHLIB_STANDALONE
     #include <Rmath.h>
    ])
  AC_CHECK_LIB([Rmath], [ptukey], [], [enable_mathlib=no])
  if test "x$enable_mathlib" = "xyes"; then
    AC_DEFINE(HAVE_MATHLIB, 1, [Define this if a good mathlib installation was detected]) 
  fi
fi

if test "x$enable_gsl" = "xyes"; then
  AC_MSG_NOTICE([check GNU Scientific Library...])
  AX_PATH_GSL(2.5, [enable_gsl=yes], [enable_gsl=no])
  if test "x$enable_gsl" = "xyes"; then
    AC_CHECK_LIB([gslcblas], [cblas_dgemm], [], [enable_gsl=no])
    AC_CHECK_LIB([gsl], [gsl_blas_ddot], [], [enable_gsl=no])
    AC_CHECK_HEADERS([gsl/gsl_blas.h gsl/gsl_complex.h gsl/gsl_complex_math.h gsl/gsl_eigen.h gsl/gsl_errno.h gsl/gsl_fft_complex.h gsl/gsl_integration.h gsl/gsl_linalg.h gsl/gsl_math.h gsl/gsl_matrix.h gsl/gsl_permutation.h gsl/gsl_sf.h gsl/gsl_sf_ellint.h gsl/gsl_sf_hermite.h gsl/gsl_statistics.h gsl/gsl_vector.h gsl/gsl_version.h], [], [enable_gsl=no], [])
  fi
  if test "x$enable_gsl" = "xyes"; then
    AC_DEFINE(HAVE_GSL, 1, [Define this if a good GNU GSL installation was detected])
  fi
fi

if test "x$enable_plotutils" = "xyes"; then
  AC_MSG_NOTICE([check GNU plotutils...])
  AC_CHECK_HEADERS([plot.h], [], [enable_plotutils=no], [])
  AC_CHECK_LIB([plot], [pl_alabel_r], [], [enable_plotutils=no])
  if test "x$enable_plotutils" = "xyes"; then
    AC_DEFINE(HAVE_GNU_PLOTUTILS, 1, [Define this if a good GNU plotutils installation was detected]) 
  fi
fi

if test "x$enable_curses" = "xyes"; then
  AC_MSG_NOTICE([check curses...])
# Next macro is in the debian autoconf-archive package.
# It inserts -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600, which we do not want.
  AX_WITH_CURSES
#
  if test "x$ax_cv_curses" != xyes || test "x$ax_cv_curses_color" != xyes; then
    enable_curses=no
  fi
fi

if test "x$enable_postgresql" = "xyes"; then
  AC_MSG_NOTICE([check PostgreSQL...])
  AC_CHECK_HEADERS([libpq-fe.h], [], [enable_postgresql=no], [])
  AC_CHECK_LIB([pq], [PQbackendPID], [], [enable_postgresql=no])
  if test "x$enable_postgresql" = "xyes"; then
    AC_DEFINE(HAVE_POSTGRESQL, 1, [Define this if a good PostgreSQL installation was detected]) 
  fi
fi

if test "x$enable_www" = "xyes"; then
  AC_MSG_NOTICE([check internet...])
  if test "x$enable_curl" = "xyes"; then
    AC_MSG_NOTICE([cURL...])
    AC_CHECK_HEADERS([curl/curl.h], [], [enable_curl=no], [])
    AC_CHECK_LIB([curl], [curl_easy_init], [], [enable_curl=no])
    if test "x$enable_curl" = "xyes"; then
      AC_DEFINE(HAVE_CURL, 1, [Define this if a good curl installation was detected]) 
    fi
  fi
fi

if test "x$enable_compiler" = "xyes"; then
  libdl_found=no
  AC_MSG_NOTICE([check dynamic loader via libdl...])
  AC_CHECK_HEADERS([dlfcn.h])
  AC_SEARCH_LIBS([dlopen], [dl], [], enable_compiler=no)
  if test "x$enable_compiler" = "xyes"; then
    AC_DEFINE(HAVE_DL, 1, [Define this if a good DL installation was detected]) 
    libdl_found=yes
  fi
fi

# Are we on Haiku?
if test "x$enable_compiler" = "xyes"; then
  if test "x$libdl_found" = "xno"; then
    AC_MSG_NOTICE([check dynamic loader via libroot...])
    AC_CHECK_HEADERS([dlfcn.h])
    AC_CHECK_LIB([root], [dlopen], [], enable_compiler=no)
    if test "x$enable_compiler" = "xyes"; then
      AC_DEFINE(HAVE_DL, 1, [Define this if a good DL installation was detected]) 
    fi
  fi
fi

#
# Generate files.
#

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_MSG_NOTICE([])
AC_MSG_NOTICE([AC_PACKAGE_NAME-AC_PACKAGE_VERSION by Marcel van der Veer <algol68g@xs4all.nl>])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([C compiler is $CC])
AC_MSG_NOTICE([])
if test "x$enable_long_types" = "xyes"; then
  AC_MSG_NOTICE([building with long modes])
fi
if test "x$enable_parallel" = "xyes"; then
  AC_MSG_NOTICE([building with parallel clause])
fi
if test "x$enable_mpfr" = "xyes"; then
  AC_MSG_NOTICE([building with GNU MPFR])
fi
if test "x$enable_mathlib" = "xyes"; then
  AC_MSG_NOTICE([building with R mathlib])
fi
if test "x$enable_gsl" = "xyes"; then
  AC_MSG_NOTICE([building with GNU Scientific Library])
fi
if test "x$enable_curses" = "xyes"; then
  AC_MSG_NOTICE([building with curses])
fi
if test "x$enable_plotutils" = "xyes"; then
  AC_MSG_NOTICE([building with GNU plotutils])
fi
if test "x$enable_postgresql" = "xyes"; then
  AC_MSG_NOTICE([building with PostgreSQL])
fi
if test "x$enable_curl" = "xyes"; then
  AC_MSG_NOTICE([building with curl])
fi
if test "x$enable_compiler" = "xyes"; then
  AC_MSG_NOTICE([building plugin compiler])
else
  AC_MSG_NOTICE([building interpreter-only])
fi
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Now enter 'make', optionally followed by 'make check'.])
AC_MSG_NOTICE([Enter 'make install' to install a68g for all users.])
AC_MSG_NOTICE([Consult the README file for more information.])
